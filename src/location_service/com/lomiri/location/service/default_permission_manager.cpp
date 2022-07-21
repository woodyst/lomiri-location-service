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
#include <com/lomiri/location/service/default_permission_manager.h>

#include <iostream>

namespace cll = com::lomiri::location;
namespace clls = com::lomiri::location::service;

clls::DefaultPermissionManager::DefaultPermissionManager()
    : pid(getpid()),
      uid(getuid())
{
}

clls::DefaultPermissionManager::~DefaultPermissionManager() noexcept
{
}

clls::PermissionManager::Result clls::DefaultPermissionManager::check_permission_for_credentials(
    const cll::Criteria&,
    const clls::Credentials& credentials)
{
    if (credentials.pid != pid || credentials.uid != uid)
        return Result::granted; // FIXME(tvoss): This should return rejected.
    return Result::granted;
}
