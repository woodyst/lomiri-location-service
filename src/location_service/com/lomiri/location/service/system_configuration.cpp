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
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 *              Scott Sweeny <scott.sweeny@canonical.com>
 */
#include <com/lomiri/location/service/system_configuration.h>

#include <com/lomiri/location/service/always_granting_permission_manager.h>
#include <com/lomiri/location/service/default_permission_manager.h>
#ifdef ENABLE_TRUST_STORE
#include <com/lomiri/location/service/trust_store_permission_manager.h>
#endif

#include <com/lomiri/location/logging.h>

#include <core/posix/this_process.h>

namespace clls = com::lomiri::location::service;
namespace env = core::posix::this_process::env;

namespace
{
#ifdef ENABLE_TRUST_STORE
struct UbuntuSystemConfiguration : public clls::SystemConfiguration
{
    fs::path runtime_persistent_data_dir() const
    {
        return "/var/lib/lomiri-location-service";
    }
    
    clls::PermissionManager::Ptr create_permission_manager(const std::shared_ptr<core::dbus::Bus>& bus) const
    {
        return clls::TrustStorePermissionManager::create_default_instance_with_bus(bus);
    }    
};
#endif

struct DefaultSystemConfiguration : public clls::SystemConfiguration
{
    fs::path runtime_persistent_data_dir() const
    {
        return "/var/lib/lomiri-location-service";
    }
    
    clls::PermissionManager::Ptr create_permission_manager(const std::shared_ptr<core::dbus::Bus>& bus) const
    {
        return clls::PermissionManager::Ptr{new clls::DefaultPermissionManager()};
    }    
};
}

clls::SystemConfiguration& clls::SystemConfiguration::instance()
{
#ifdef ENABLE_TRUST_STORE
    static UbuntuSystemConfiguration config;
#else
    static DefaultSystemConfiguration config;
#endif
    return config;
}
