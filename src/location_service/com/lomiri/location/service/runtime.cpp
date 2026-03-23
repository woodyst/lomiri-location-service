/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTIlocationAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */
#include <com/lomiri/location/service/runtime.h>

#include <com/lomiri/location/logging.h>

namespace clls = com::lomiri::location::service;

namespace
{
// exception_safe_run runs service, catching all exceptions and
// restarting operation until an explicit shutdown has been requested.
//
// TODO(tvoss): Catching all exceptions is risky as they might signal unrecoverable
// errors. We should enable calling code to decide whether an exception should be considered
// fatal or not.
void exception_safe_run(boost::asio::io_context& service)
{
    while (true)
    {
        try
        {
            service.run();
            // a clean return from run only happens in case of
            // stop() being called (we are keeping the service alive with
            // a service::work instance).
            break;
        }
        catch (const std::exception& e)
        {
            LOG(WARNING) << e.what();
        }
        catch (...)
        {
            LOG(WARNING) << "Unknown exception caught while executing boost::asio::io_context";
        }
    }
}
}

std::shared_ptr<clls::Runtime> clls::Runtime::create(std::uint32_t pool_size)
{
    return std::shared_ptr<clls::Runtime>(new clls::Runtime(pool_size));
}

clls::Runtime::Runtime(std::uint32_t pool_size)
    : pool_size_{pool_size},
      service_{pool_size_},
      strand_{service_},
      keep_alive_{service_.get_executor()}
{
}

clls::Runtime::~Runtime()
{
    try
    {
        stop();
    } catch(...)
    {
        // Dropping all exceptions to satisfy the nothrow guarantee.
    }
}

void clls::Runtime::start()
{
    for (unsigned int i = 0; i < pool_size_; i++)
        workers_.push_back(std::thread{exception_safe_run, std::ref(service_)});
}

void clls::Runtime::stop()
{
    service_.stop();

    for (auto& worker : workers_)
        if (worker.joinable())
            worker.join();
}

std::function<void(std::function<void()>)> clls::Runtime::to_dispatcher_functional()
{
    // We have to make sure that we stay alive for as long as
    // calling code requires the dispatcher to work.
    auto sp = shared_from_this();
    return [sp](std::function<void()> task)
    {
        boost::asio::post(sp->strand_, task);
    };
}

boost::asio::io_context& clls::Runtime::service()
{
    return service_;
}
