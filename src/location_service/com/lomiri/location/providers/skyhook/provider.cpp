/*
 * Copyright © 2012-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */
#include <com/lomiri/location/providers/skyhook/provider.h>

#include <com/lomiri/location/logging.h>
#include <com/lomiri/location/provider_factory.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wignored-qualifiers"
#include <wpsapi.h>
#pragma GCC diagnostic pop
#include <map>
#include <thread>

namespace cll = com::lomiri::location;
namespace clls = com::lomiri::location::providers::skyhook;

namespace
{
static const std::map<int, std::string> return_code_lut =
{
    {WPS_OK, "WPS_OK"},
    {WPS_ERROR_SCANNER_NOT_FOUND, "WPS_ERROR_SCANNER_NOT_FOUND"},
    {WPS_ERROR_WIFI_NOT_AVAILABLE, "WPS_ERROR_WIFI_NOT_AVAILABLE"},
    {WPS_ERROR_NO_WIFI_IN_RANGE, "WPS_ERROR_NO_WIFI_IN_RANGE"},
    {WPS_ERROR_UNAUTHORIZED, "WPS_ERROR_UNAUTHORIZED"},
    {WPS_ERROR_SERVER_UNAVAILABLE, "WPS_ERROR_SERVER_UNAVAILABLE"},
    {WPS_ERROR_LOCATION_CANNOT_BE_DETERMINED, "WPS_ERROR_LOCATION_CANNOT_BE_DETERMINED"},
    {WPS_ERROR_PROXY_UNAUTHORIZED, "WPS_ERROR_PROXY_UNAUTHORIZED"},
    {WPS_ERROR_FILE_IO, "WPS_ERROR_FILE_IO"},
    {WPS_ERROR_INVALID_FILE_FORMAT, "WPS_ERROR_INVALID_FILE_FORMAT"},
    {WPS_ERROR_TIMEOUT, "WPS_ERROR_TIMEOUT"},
    {WPS_NOT_APPLICABLE, "WPS_NOT_APPLICABLE"},
    {WPS_GEOFENCE_ERROR, "WPS_GEOFENCE_ERROR"},
    {WPS_ERROR_NOT_TUNED, "WPS_ERROR_NOT_TUNED"},
    {WPS_NOMEM, "WPS_NOMEM"},
    {WPS_ERROR, "WPS_ERROR"}
};
}


WPS_Continuation clls::Provider::Private::periodic_callback(void* context,
                                                            WPS_ReturnCode code,
                                                            const WPS_Location* location,
                                                            const void*)
{
    if (code != WPS_OK)
    {
        LOG(WARNING) << return_code_lut.at(code);
        if (code == WPS_ERROR_WIFI_NOT_AVAILABLE)            
            return WPS_STOP;

        return WPS_CONTINUE;
    }

    auto thiz = static_cast<clls::Provider::Private*>(context);

    if (thiz->state == clls::Provider::Private::State::stop_requested)
    {
        LOG(INFO) << "Stop requested";
        thiz->state = clls::Provider::Private::State::stopped;
        return WPS_STOP;
    }

    cll::Position pos;
    pos.latitude(cll::wgs84::Latitude{location->latitude * cll::units::Degrees})
            .longitude(cll::wgs84::Longitude{location->longitude * cll::units::Degrees});
    if (location->altitude >= 0.f)
        pos.altitude(cll::wgs84::Altitude{location->altitude * cll::units::Meters});

    LOG(INFO) << pos;

    thiz->parent->deliver_position_updates(cll::Update<cll::Position>{pos, cll::Clock::now()});
        
    if (location->speed >= 0.f)
    {
        cll::Velocity v{location->speed * cll::units::MetersPerSecond};
        LOG(INFO) << v;
        thiz->parent->deliver_velocity_updates(cll::Update<cll::Velocity>{v, cll::Clock::now()});
    }

    if (location->bearing >= 0.f)
    {
        cll::Heading h{location->bearing * cll::units::Degrees};
        LOG(INFO) << h;
        thiz->parent->deliver_heading_updates(cll::Update<cll::Heading>{h, cll::Clock::now()});
    }

    return WPS_CONTINUE;
}
    
cll::Provider::Ptr clls::Provider::create_instance(const cll::ProviderFactory::Configuration& config)
{
    clls::Provider::Configuration configuration
    {
        config.get(Configuration::key_username(), ""), 
        config.get(Configuration::key_realm(), ""),
        std::chrono::milliseconds{config.get(Configuration::key_period(), 500)}
    };

    return cll::Provider::Ptr{new clls::Provider{configuration}};
}

const cll::Provider::FeatureFlags& clls::Provider::default_feature_flags()
{
    static const cll::Provider::FeatureFlags flags{"001"};
    return flags;
}

const cll::Provider::RequirementFlags& clls::Provider::default_requirement_flags()
{
    static const cll::Provider::RequirementFlags flags{"1010"};
    return flags;
}

clls::Provider::Provider(const clls::Provider::Configuration& config) 
        : com::lomiri::location::Provider(clls::Provider::default_feature_flags(), clls::Provider::default_requirement_flags()),
          config(config),
          state(State::stopped)
{
}

clls::Provider::~Provider() noexcept
{
    request_stop();
}

void clls::Provider::start()
{
    if (state != State::stopped)
        return;

    if (worker.joinable())
        worker.join();

    static const unsigned infinite_iterations = 0;

    authentication.username = config.user_name.c_str();
    authentication.realm = config.realm.c_str();

    worker = std::move(std::thread([&]()
    {
        int rc = WPS_periodic_location(&authentication, WPS_NO_STREET_ADDRESS_LOOKUP, config.period.count(),
                                       infinite_iterations, clls::Provider::Private::periodic_callback, this);

        if (rc != WPS_OK)
            LOG(ERROR) << return_code_lut.at(rc);
    }));

    state = State::started;
}

void clls::Provider::request_stop()
{
    state = State::stop_requested;
}

bool clls::Provider::matches_criteria(const cll::Criteria&)
{
    return true;
}

void clls::Provider::start_position_updates()
{
    start();
}

void clls::Provider::stop_position_updates()
{
    request_stop();
}

void clls::Provider::start_velocity_updates()
{
    start();
}

void clls::Provider::stop_velocity_updates()
{
    request_stop();
}    

void clls::Provider::start_heading_updates()
{
    start();
}

void clls::Provider::stop_heading_updates()
{
    request_stop();
}
