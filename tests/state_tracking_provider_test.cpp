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
 */

#include <com/lomiri/location/state_tracking_provider.h>

#include <com/lomiri/location/clock.h>
#include <com/lomiri/location/update.h>
#include <com/lomiri/location/position.h>
#include <com/lomiri/location/units/units.h>
#include <com/lomiri/location/wgs84/latitude.h>
#include <com/lomiri/location/wgs84/longitude.h>
#include <com/lomiri/location/wgs84/altitude.h>

#include "mock_provider.h"

#include <gtest/gtest.h>

namespace cll = com::lomiri::location;

namespace
{
auto timestamp = cll::Clock::now();

// Create reference objects for injecting and validating updates.
cll::Update<cll::Position> reference_position_update
{
    {
        cll::wgs84::Latitude{9. * cll::units::Degrees},
        cll::wgs84::Longitude{53. * cll::units::Degrees},
        cll::wgs84::Altitude{-2. * cll::units::Meters}
    },
    timestamp
};

cll::Update<cll::Velocity> reference_velocity_update
{
    {5. * cll::units::MetersPerSecond},
    timestamp
};

cll::Update<cll::Heading> reference_heading_update
{
    {120. * cll::units::Degrees},
    timestamp
};
}
TEST(StateTrackingProviderTest, forwards_calls_to_impl)
{
    using namespace testing;

    auto impl = std::make_shared<MockProvider>();
    EXPECT_CALL(*impl, supports(_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*impl, requires(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*impl, matches_criteria(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*impl, on_wifi_and_cell_reporting_state_changed(_)).Times(1);
    EXPECT_CALL(*impl, on_reference_location_updated(_)).Times(1);
    EXPECT_CALL(*impl, on_reference_velocity_updated(_)).Times(1);
    EXPECT_CALL(*impl, on_reference_heading_updated(_)).Times(1);
    EXPECT_CALL(*impl, start_position_updates()).Times(1);
    EXPECT_CALL(*impl, stop_position_updates()).Times(1);
    EXPECT_CALL(*impl, start_velocity_updates()).Times(1);
    EXPECT_CALL(*impl, stop_velocity_updates()).Times(1);
    EXPECT_CALL(*impl, start_heading_updates()).Times(1);
    EXPECT_CALL(*impl, stop_heading_updates()).Times(1);

    com::lomiri::location::StateTrackingProvider stp{impl};
    EXPECT_TRUE(stp.supports(com::lomiri::location::Provider::Features::none));
    EXPECT_FALSE(stp.requires(com::lomiri::location::Provider::Requirements::none));
    EXPECT_FALSE(stp.matches_criteria(com::lomiri::location::Criteria{}));
    stp.on_wifi_and_cell_reporting_state_changed(com::lomiri::location::WifiAndCellIdReportingState::on);
    stp.on_reference_location_updated(reference_position_update);
    stp.on_reference_heading_updated(reference_heading_update);
    stp.on_reference_velocity_updated(reference_velocity_update);
    stp.start_position_updates();
    stp.stop_position_updates();
    stp.start_heading_updates();
    stp.stop_heading_updates();
    stp.start_velocity_updates();
    stp.stop_velocity_updates();
}

TEST(StateTrackingProviderTest, state_after_construction_is_enabled)
{
    using namespace ::testing;
    cll::StateTrackingProvider stp{std::make_shared<NiceMock<MockProvider>>()};
    EXPECT_EQ(cll::StateTrackingProvider::State::enabled, stp.state());
}

TEST(StateTrackingProviderTest, state_after_start_is_active)
{
    using namespace ::testing;
    cll::StateTrackingProvider stp{std::make_shared<NiceMock<MockProvider>>()};
    stp.start_position_updates();
    EXPECT_EQ(cll::StateTrackingProvider::State::active, stp.state());
}

TEST(StateTrackingProviderTest, stop_switches_to_enabled)
{
    using namespace ::testing;
    cll::StateTrackingProvider stp{std::make_shared<NiceMock<MockProvider>>()};
    stp.start_position_updates();
    stp.stop_position_updates();
    EXPECT_EQ(cll::StateTrackingProvider::State::enabled, stp.state());
}
