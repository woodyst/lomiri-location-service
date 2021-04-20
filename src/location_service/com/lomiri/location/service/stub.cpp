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
#include <com/lomiri/location/service/stub.h>
#include <com/lomiri/location/service/session/stub.h>

#include <com/lomiri/location/logging.h>

#include <core/dbus/property.h>

namespace cll = com::lomiri::location;
namespace clls = com::lomiri::location::service;
namespace cllss = com::lomiri::location::service::session;

namespace dbus = core::dbus;

struct clls::Stub::Private
{
    Private(const dbus::Bus::Ptr& connection,
            const dbus::Object::Ptr& object)
        : bus(connection),
          object(object),
          state(object->get_property<clls::Interface::Properties::State>()),
          does_satellite_based_positioning(object->get_property<clls::Interface::Properties::DoesSatelliteBasedPositioning>()),
          does_report_cell_and_wifi_ids(object->get_property<clls::Interface::Properties::DoesReportCellAndWifiIds>()),
          is_online(object->get_property<clls::Interface::Properties::IsOnline>()),
          visible_space_vehicles(object->get_property<clls::Interface::Properties::VisibleSpaceVehicles>()),
          client_applications(object->get_property<clls::Interface::Properties::ClientApplications>())
    {
    }

    dbus::Bus::Ptr bus;
    dbus::Object::Ptr object;
    std::shared_ptr<dbus::Property<clls::Interface::Properties::State>> state;
    std::shared_ptr<dbus::Property<clls::Interface::Properties::DoesSatelliteBasedPositioning>> does_satellite_based_positioning;
    std::shared_ptr<dbus::Property<clls::Interface::Properties::DoesReportCellAndWifiIds>> does_report_cell_and_wifi_ids;
    std::shared_ptr<dbus::Property<clls::Interface::Properties::IsOnline>> is_online;
    std::shared_ptr<dbus::Property<clls::Interface::Properties::VisibleSpaceVehicles>> visible_space_vehicles;
    std::shared_ptr<dbus::Property<clls::Interface::Properties::ClientApplications>> client_applications;
};

clls::Stub::Stub(const dbus::Bus::Ptr& connection) : dbus::Stub<clls::Interface>(connection),
    d(new Private{connection, access_service()->object_for_path(clls::Interface::path())})
{
}

clls::Stub::~Stub() noexcept
{
}

cllss::Interface::Ptr clls::Stub::create_session_for_criteria(const cll::Criteria& criteria)
{
    auto op = d->object->transact_method<
            clls::Interface::CreateSessionForCriteria,
            clls::Interface::CreateSessionForCriteria::ResultType
            >(criteria);

    if (op.is_error())
    {
        std::stringstream ss; ss << __PRETTY_FUNCTION__ << ": " << op.error().print();
        throw std::runtime_error(ss.str());
    }

    return cllss::Interface::Ptr(new cllss::Stub{d->bus, op.value()});
}

const core::Property<clls::State>& clls::Stub::state() const
{
    return *d->state;
}

core::Property<bool>& clls::Stub::does_satellite_based_positioning()
{
    return *d->does_satellite_based_positioning;
}

core::Property<bool>& clls::Stub::does_report_cell_and_wifi_ids()
{
    return *d->does_report_cell_and_wifi_ids;
}

core::Property<bool>& clls::Stub::is_online()
{
    return *d->is_online;
}

core::Property<std::map<cll::SpaceVehicle::Key, cll::SpaceVehicle>>& clls::Stub::visible_space_vehicles()
{
    return *d->visible_space_vehicles;
}

core::Property<std::vector<std::string>>& clls::Stub::client_applications()
{
    return *d->client_applications;
}
