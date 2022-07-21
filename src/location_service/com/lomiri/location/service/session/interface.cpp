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
#include <com/lomiri/location/service/session/interface.h>

#include <core/dbus/codec.h>
#include <core/dbus/service.h>
#include <core/dbus/traits/service.h>
#include <core/dbus/types/object_path.h>

namespace cll = com::lomiri::location;
namespace clls = com::lomiri::location::service;
namespace cllss = com::lomiri::location::service::session;

namespace dbus = core::dbus;

struct cllss::Interface::Private
{
    cllss::Interface::Updates updates;
};

cllss::Interface::Interface() : d{new Private{}}
{
}

cllss::Interface::~Interface() noexcept
{
}

cllss::Interface::Updates& cllss::Interface::updates()
{
    return d->updates;
}
