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

#include "provider.h"

#include "hardware_abstraction_layer.h"

#include <com/lomiri/location/logging.h>
#include <com/lomiri/location/connectivity/manager.h>

#include <ubuntu/hardware/gps.h>

namespace cll = com::lomiri::location;
namespace cllg = com::lomiri::location::providers::gps;

std::string cllg::Provider::class_name()
{
    return "gps::Provider";
}

cll::Provider::Ptr cllg::Provider::create_instance(const cll::ProviderFactory::Configuration&)
{
    return cll::Provider::Ptr{new cllg::Provider{cllg::HardwareAbstractionLayer::create_default_instance()}};
}

cllg::Provider::Provider(const std::shared_ptr<HardwareAbstractionLayer>& hal)
    : cll::Provider(
          cll::Provider::Features::position | cll::Provider::Features::velocity | cll::Provider::Features::heading,
          cll::Provider::Requirements::satellites),
          hal(hal)
{

    hal->position_updates().connect([this](const location::Position& pos)
    {
        mutable_updates().position(Update<Position>(pos));
    });

    hal->heading_updates().connect([this](const location::Heading& heading)
    {
        mutable_updates().heading(Update<Heading>(heading));
    });

    hal->velocity_updates().connect([this](const location::Velocity& velocity)
    {
        mutable_updates().velocity(Update<Velocity>(velocity));
    });

    hal->space_vehicle_updates().connect([this](const std::set<location::SpaceVehicle>& svs)
    {
        mutable_updates().svs(Update<std::set<location::SpaceVehicle>>(svs));
    });
}

cllg::Provider::~Provider() noexcept
{
}

bool cllg::Provider::matches_criteria(const cll::Criteria&)
{
    return true;
}

void cllg::Provider::start_position_updates()
{
    hal->start_positioning();
}

void cllg::Provider::stop_position_updates()
{
    hal->stop_positioning();
}

void cllg::Provider::start_velocity_updates()
{   
}

void cllg::Provider::stop_velocity_updates()
{
}    

void cllg::Provider::start_heading_updates()
{
}

void cllg::Provider::stop_heading_updates()
{
}

void cllg::Provider::on_reference_location_updated(const cll::Update<cll::Position>& position)
{
    hal->inject_reference_position(position.value);
}

