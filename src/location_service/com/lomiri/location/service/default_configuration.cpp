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
#include <com/lomiri/location/service/default_configuration.h>
#include <com/lomiri/location/service/default_permission_manager.h>
#include <com/lomiri/location/service/system_configuration.h>

#include <com/lomiri/location/engine.h>
#include <com/lomiri/location/fusion_provider_selection_policy.h>

namespace cll = com::lomiri::location;
namespace clls = com::lomiri::location::service;

cll::Engine::Ptr clls::DefaultConfiguration::the_engine(
    const std::set<cll::Provider::Ptr>& provider_set,
    const cll::ProviderSelectionPolicy::Ptr& provider_selection_policy,
    const cll::Settings::Ptr& settings)
{
    auto engine = std::make_shared<cll::Engine>(provider_selection_policy, settings);
    for (const auto& provider : provider_set)
        engine->add_provider(provider);

    return engine;
}

cll::ProviderSelectionPolicy::Ptr clls::DefaultConfiguration::the_provider_selection_policy()
{
    return std::make_shared<cll::FusionProviderSelectionPolicy>();
}

std::set<cll::Provider::Ptr> clls::DefaultConfiguration::the_provider_set(
    const cll::Provider::Ptr& seed)
{
    return std::set<cll::Provider::Ptr>{seed};
}

clls::PermissionManager::Ptr clls::DefaultConfiguration::the_permission_manager(const core::dbus::Bus::Ptr& bus)
{
    return clls::SystemConfiguration::instance().create_permission_manager(bus);
}

