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
#include <com/lomiri/location/provider.h>
#include <com/lomiri/location/provider_factory.h>

#include "dummy/provider.h"
#include "dummy/delayed_provider.h"

#include <map>

namespace cll = com::lomiri::location;

namespace
{
struct FactoryInjector
{
    FactoryInjector(const std::string& name, const std::function<cll::Provider::Ptr(const cll::ProviderFactory::Configuration&)>& f)
    {
        com::lomiri::location::ProviderFactory::instance().add_factory_for_name(name, f);
    }
};
}

static FactoryInjector dummy_injector
{
    "dummy::Provider",
    com::lomiri::location::providers::dummy::Provider::create_instance
};

static FactoryInjector dummy_delayed_injector
{
    "dummy::DelayedProvider",
    com::lomiri::location::providers::dummy::DelayedProvider::create_instance
};

#include <com/lomiri/location/providers/remote/provider.h>
static FactoryInjector remote_injector
{
    "remote::Provider",
    com::lomiri::location::providers::remote::Provider::Stub::create_instance
};

#if defined(COM_LOMIRI_LOCATION_SERVICE_PROVIDERS_GEOCLUE)
#include <com/lomiri/location/providers/geoclue/provider.h>
static FactoryInjector geoclue_injector
{
    "geoclue::Provider", 
    com::lomiri::location::providers::geoclue::Provider::create_instance
};
#endif // COM_LOMIRI_LOCATION_SERVICE_PROVIDERS_GEOCLUE

#if defined(COM_LOMIRI_LOCATION_SERVICE_PROVIDERS_GPS)
#include <com/lomiri/location/providers/gps/provider.h>
static FactoryInjector gps_injector
{
    "gps::Provider", 
    com::lomiri::location::providers::gps::Provider::create_instance
};
#endif // COM_LOMIRI_LOCATION_SERVICE_PROVIDERS_GPS

#if defined(COM_LOMIRI_LOCATION_SERVICE_PROVIDERS_GPSD)
#include <com/lomiri/location/providers/gpsd/provider.h>
static FactoryInjector gpsd_injector
{
    "gpsd::Provider",
    com::lomiri::location::providers::gpsd::Provider::create_instance
};
#endif // COM_LOMIRI_LOCATION_SERVICE_PROVIDERS_GPSD


#if defined(COM_LOMIRI_LOCATION_SERVICE_PROVIDERS_SKYHOOK)
#include <com/lomiri/location/providers/skyhook/provider.h>
static FactoryInjector skyhook_injector
{
    "skyhook::Provider", 
    com::lomiri::location::providers::skyhook::Provider::create_instance
};
#endif // COM_LOMIRI_LOCATION_SERVICE_PROVIDERS_SKYHOOK
