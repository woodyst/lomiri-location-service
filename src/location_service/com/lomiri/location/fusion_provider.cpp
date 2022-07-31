/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Scott Sweeny <scott.sweeny@canonical.com
 */
 #include <com/lomiri/location/fusion_provider.h>
 #include <com/lomiri/location/logging.h>
 #include <com/lomiri/location/update.h>

#include <vector>

namespace cll = com::lomiri::location;

cll::Provider::Features all_features()
{
    return cll::Provider::Features::position |
           cll::Provider::Features::heading |
           cll::Provider::Features::velocity;
}

cll::Provider::Requirements all_requirements()
{
    return cll::Provider::Requirements::cell_network |
           cll::Provider::Requirements::data_network |
           cll::Provider::Requirements::monetary_spending |
           cll::Provider::Requirements::satellites;
}

cll::FusionProvider::FusionProvider(const std::set<location::Provider::Ptr>& providers, const UpdateSelector::Ptr& update_selector)
    : Provider{all_features(), all_requirements()},
      providers{providers}
{

    for (auto provider : providers)
    {
        connections.push_back(provider->updates().position.connect(
              [this, provider, update_selector](const cll::Update<cll::Position>& u)
              {
                  // if this is the first update, use it
                  if (!last_position) {
                      mutable_updates().position((*(last_position = WithSource<Update<Position>>{provider, u})).value);

                  // otherwise use the selector
                  } else {
                      try {
                          mutable_updates().position((*(last_position = update_selector->select(*last_position, WithSource<Update<Position>>{provider, u}))).value);
                      } catch (const std::exception& e) {
                          LOG(WARNING) << "Error while updating position";
                      }
                  }
              }));
        connections.push_back(provider->updates().heading.connect(
              [this](const cll::Update<cll::Heading>& u)
              {
                  mutable_updates().heading(u);
              }));
        connections.push_back(provider->updates().velocity.connect(
              [this](const cll::Update<cll::Velocity>& u)
              {
                  mutable_updates().velocity(u);
              }));
    }

}

// We always match :)
bool cll::FusionProvider::matches_criteria(const cll::Criteria&)
{
    return true;
}

// We forward all events to the other providers.
void cll::FusionProvider::on_wifi_and_cell_reporting_state_changed(cll::WifiAndCellIdReportingState state)
{
    for (auto provider : providers)
        provider->on_wifi_and_cell_reporting_state_changed(state);
}

void cll::FusionProvider::on_reference_location_updated(const cll::Update<cll::Position>& position)
{
    for (auto provider : providers)
        provider->on_reference_location_updated(position);
}

void cll::FusionProvider::on_reference_velocity_updated(const cll::Update<cll::Velocity>& velocity)
{
    for (auto provider : providers)
        provider->on_reference_velocity_updated(velocity);
}

void cll::FusionProvider::on_reference_heading_updated(const cll::Update<cll::Heading>& heading)
{
    for (auto provider : providers)
        provider->on_reference_heading_updated(heading);
}

void cll::FusionProvider::start_position_updates()
{
    for (auto provider : providers)
        provider->state_controller()->start_position_updates();
}

void cll::FusionProvider::stop_position_updates()
{
    for (auto provider : providers)
        provider->state_controller()->stop_position_updates();
}

void cll::FusionProvider::start_heading_updates()
{
    for (auto provider : providers)
        provider->state_controller()->start_heading_updates();
}

void cll::FusionProvider::stop_heading_updates()
{
    for (auto provider : providers)
        provider->state_controller()->stop_heading_updates();
}

void cll::FusionProvider::start_velocity_updates()
{
    for (auto provider : providers)
        provider->state_controller()->start_velocity_updates();
}

void cll::FusionProvider::stop_velocity_updates()
{
    for (auto provider : providers)
        provider->state_controller()->stop_velocity_updates();
}
