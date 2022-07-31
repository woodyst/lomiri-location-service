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
#include <com/lomiri/location/proxy_provider.h>

#include <bitset>
#include <memory>

namespace cll = com::lomiri::location;

cll::ProxyProvider::ProxyProvider(const cll::ProviderSelection& selection)
    : Provider(selection.to_feature_flags()),
      providers(selection),
      connections
      {
          providers.position_updates_provider->updates().position.connect(
              [this](const cll::Update<cll::Position>& u)
              {
                  mutable_updates().position(u);
              }),
          providers.heading_updates_provider->updates().heading.connect(
              [this](const cll::Update<cll::Heading>& u)
              {
                  mutable_updates().heading(u);
              }),
          providers.velocity_updates_provider->updates().velocity.connect(
              [this](const cll::Update<cll::Velocity>& u)
              {
                  mutable_updates().velocity(u);
              })
      }
{
}

cll::ProxyProvider::~ProxyProvider() noexcept
{
}

void cll::ProxyProvider::start_position_updates()
{
    providers.position_updates_provider->state_controller()->start_position_updates();
}

void cll::ProxyProvider::stop_position_updates()
{
    providers.position_updates_provider->state_controller()->stop_position_updates();
}

void cll::ProxyProvider::start_velocity_updates()
{
    providers.velocity_updates_provider->state_controller()->start_velocity_updates();
}

void cll::ProxyProvider::stop_velocity_updates()
{
    providers.velocity_updates_provider->state_controller()->stop_velocity_updates();
}    

void cll::ProxyProvider::start_heading_updates()
{
    providers.heading_updates_provider->state_controller()->start_heading_updates();
}

void cll::ProxyProvider::stop_heading_updates()
{
    providers.heading_updates_provider->state_controller()->stop_heading_updates();
}
