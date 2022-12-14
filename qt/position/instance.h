/*
 * Copyright © 2013 Canonical Ltd.
 * Copyright 2022 UBports Foundation.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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
 *              Marius Gripsgard <marius@ubports.com>
 */

#ifndef INSTANCE_H_
#define INSTANCE_H_

#include <com/lomiri/location/service/stub.h>

#include <core/dbus/resolver.h>
#include <core/dbus/asio/executor.h>

#include <thread>

class Instance
{
  public:
    Instance()
        : bus(std::make_shared<core::dbus::Bus>(core::dbus::WellKnownBus::system)),
          executor(core::dbus::asio::make_executor(bus)),
          service(core::dbus::resolve_service_on_bus<
                    com::lomiri::location::service::Interface,
                    com::lomiri::location::service::Stub
                  >(bus))
    {
        bus->install_executor(executor);
        worker = std::move(std::thread([&]() { bus->run(); }));
    }

    ~Instance() noexcept
    {
        try
        {
            bus->stop();

            if (worker.joinable())
                worker.join();
        } catch(...)
        {
            // We silently ignore errors to fulfill our noexcept guarantee.
        }
    }

    const com::lomiri::location::service::Interface::Ptr& getService() const
    {
        return service;
    }

private:

    core::dbus::Bus::Ptr bus;
    core::dbus::Executor::Ptr executor;
    std::thread worker;

    com::lomiri::location::service::Interface::Ptr service;
};

#endif // INSTANCE_H_
