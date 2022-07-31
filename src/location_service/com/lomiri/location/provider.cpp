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

#include <atomic>
#include <bitset>
#include <memory>

namespace cll = com::lomiri::location;

namespace
{
static const int our_magic_disabling_value = -1;
}

void cll::Provider::Controller::enable()
{
    position_updates_counter = 0;
    heading_updates_counter = 0;
    velocity_updates_counter = 0;
}

void cll::Provider::Controller::disable()
{    
    if (position_updates_counter > 0)
        instance.stop_position_updates();
    if (heading_updates_counter > 0)
        instance.stop_heading_updates();
    if (velocity_updates_counter > 0)
        instance.stop_velocity_updates();

    position_updates_counter = our_magic_disabling_value;
    heading_updates_counter = our_magic_disabling_value;
    velocity_updates_counter = our_magic_disabling_value;
}

void cll::Provider::Controller::start_position_updates()
{
    if (position_updates_counter < 0)
        return;

    if (++position_updates_counter == 1)
    {
        instance.start_position_updates();
    }
}

void cll::Provider::Controller::stop_position_updates()
{
    if (position_updates_counter < 0)
        return;

    if (--position_updates_counter == 0)
    {
        instance.stop_position_updates();
    }
}

bool cll::Provider::Controller::are_position_updates_running() const
{
    return position_updates_counter > 0;
}

void cll::Provider::Controller::start_heading_updates()
{
    if (heading_updates_counter < 0)
        return;

    if (++heading_updates_counter == 1)
    {
        instance.start_heading_updates();
    }
}

void cll::Provider::Controller::stop_heading_updates()
{
    if (heading_updates_counter < 0)
        return;

    if (--heading_updates_counter == 0)
    {
        instance.stop_heading_updates();
    }
}

bool cll::Provider::Controller::are_heading_updates_running() const
{
    return heading_updates_counter > 0;
}

void cll::Provider::Controller::start_velocity_updates()
{
    if (velocity_updates_counter < 0)
        return;

    if (++velocity_updates_counter == 1)
    {
        instance.start_velocity_updates();
    }
}

void cll::Provider::Controller::stop_velocity_updates()
{
    if (velocity_updates_counter < 0)
        return;

    if (--velocity_updates_counter == 0)
    {
        instance.stop_velocity_updates();
    }
}

bool cll::Provider::Controller::are_velocity_updates_running() const
{
    return velocity_updates_counter > 0;
}

cll::Provider::Controller::Controller(cll::Provider& instance)
    : instance(instance),
      position_updates_counter(0),
      heading_updates_counter(0),
      velocity_updates_counter(0)
{  
}

const cll::Provider::Controller::Ptr& cll::Provider::state_controller() const
{
    return d.controller;
}

bool cll::Provider::supports(const cll::Provider::Features& f) const
{
    return (d.features & f) != Features::none;
}

bool cll::Provider::requires(const cll::Provider::Requirements& r) const
{
    return (d.requirements & r) != Requirements::none;
}

bool cll::Provider::matches_criteria(const cll::Criteria&)
{
    return false;
}

const cll::Provider::Updates& cll::Provider::updates() const
{
    return d.updates;
}

cll::Provider::Provider(
    const cll::Provider::Features& features,
    const cll::Provider::Requirements& requirements)
{
    d.features = features;
    d.requirements = requirements;
    d.controller = std::shared_ptr<Provider::Controller>(new Provider::Controller(*this));
}

cll::Provider::Updates& cll::Provider::mutable_updates()
{
    return d.updates;
}

void cll::Provider::on_wifi_and_cell_reporting_state_changed(cll::WifiAndCellIdReportingState)
{
}

void cll::Provider::on_reference_location_updated(const cll::Update<cll::Position>&)
{
}

void cll::Provider::on_reference_velocity_updated(const cll::Update<cll::Velocity>&)
{
}

void cll::Provider::on_reference_heading_updated(const cll::Update<cll::Heading>&)
{
}

void cll::Provider::start_position_updates() {}
void cll::Provider::stop_position_updates() {}
void cll::Provider::start_heading_updates() {}
void cll::Provider::stop_heading_updates() {}
void cll::Provider::start_velocity_updates() {}
void cll::Provider::stop_velocity_updates() {}

cll::Provider::Features cll::operator|(cll::Provider::Features lhs, cll::Provider::Features rhs)
{
    return static_cast<cll::Provider::Features>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
}

cll::Provider::Features cll::operator&(cll::Provider::Features lhs, cll::Provider::Features rhs)
{
    return static_cast<cll::Provider::Features>(static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs));
}

cll::Provider::Requirements cll::operator|(cll::Provider::Requirements lhs, cll::Provider::Requirements rhs)
{
    return static_cast<cll::Provider::Requirements>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
}

cll::Provider::Requirements cll::operator&(cll::Provider::Requirements lhs, cll::Provider::Requirements rhs)
{
    return static_cast<cll::Provider::Requirements>(static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs));
}
