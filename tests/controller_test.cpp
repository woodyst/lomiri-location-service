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

#include "mock_provider.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace cll = com::lomiri::location;

namespace
{
auto timestamp = com::lomiri::location::Clock::now();

com::lomiri::location::Update<com::lomiri::location::Position> reference_position_update
{
    {
        com::lomiri::location::wgs84::Latitude{9. * com::lomiri::location::units::Degrees},
        com::lomiri::location::wgs84::Longitude{53. * com::lomiri::location::units::Degrees},
        com::lomiri::location::wgs84::Altitude{-2. * com::lomiri::location::units::Meters}
    },
    timestamp
};

com::lomiri::location::Update<com::lomiri::location::Velocity> reference_velocity_update
{
    {5. * com::lomiri::location::units::MetersPerSecond},
    timestamp
};

com::lomiri::location::Update<com::lomiri::location::Heading> reference_heading_update
{
    {120. * com::lomiri::location::units::Degrees},
    timestamp
};
}

TEST(Controller, controller_starts_and_stops_updates_on_provider)
{
    using namespace ::testing;

    MockProvider provider;

    EXPECT_CALL(provider, start_position_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, start_velocity_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, start_heading_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, stop_position_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, stop_velocity_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, stop_heading_updates()).Times(Exactly(1));

    auto controller = provider.state_controller();

    controller->start_position_updates();
    controller->start_velocity_updates();
    controller->start_heading_updates();

    EXPECT_TRUE(controller->are_position_updates_running());
    EXPECT_TRUE(controller->are_velocity_updates_running());
    EXPECT_TRUE(controller->are_heading_updates_running());

    controller->stop_position_updates();
    controller->stop_velocity_updates();
    controller->stop_heading_updates();

    EXPECT_FALSE(controller->are_position_updates_running());
    EXPECT_FALSE(controller->are_velocity_updates_running());
    EXPECT_FALSE(controller->are_heading_updates_running());
}

TEST(Controller, controller_starts_and_stops_updates_on_provider_only_once_requires_hardware)
{
    using namespace ::testing;

    MockProvider provider;

    EXPECT_CALL(provider, start_position_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, start_velocity_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, start_heading_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, stop_position_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, stop_velocity_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, stop_heading_updates()).Times(Exactly(1));

    auto controller = provider.state_controller();

    controller->start_position_updates();
    controller->start_velocity_updates();
    controller->start_heading_updates();

    EXPECT_TRUE(controller->are_position_updates_running());
    EXPECT_TRUE(controller->are_velocity_updates_running());
    EXPECT_TRUE(controller->are_heading_updates_running());

    controller->start_position_updates();
    controller->start_velocity_updates();
    controller->start_heading_updates();

    EXPECT_TRUE(controller->are_position_updates_running());
    EXPECT_TRUE(controller->are_velocity_updates_running());
    EXPECT_TRUE(controller->are_heading_updates_running());

    controller->stop_position_updates();
    controller->stop_velocity_updates();
    controller->stop_heading_updates();

    EXPECT_TRUE(controller->are_position_updates_running());
    EXPECT_TRUE(controller->are_velocity_updates_running());
    EXPECT_TRUE(controller->are_heading_updates_running());

    controller->stop_position_updates();
    controller->stop_velocity_updates();
    controller->stop_heading_updates();

    EXPECT_FALSE(controller->are_position_updates_running());
    EXPECT_FALSE(controller->are_velocity_updates_running());
    EXPECT_FALSE(controller->are_heading_updates_running());
}
