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
#include <com/lomiri/location/position.h>

#include <gtest/gtest.h>

namespace cll = com::lomiri::location;

TEST(Position, AllFieldsAreInvalidForDefaultConstructor)
{
    cll::Position p;
    EXPECT_FALSE(p.altitude);
    EXPECT_FALSE(p.accuracy.vertical);
}

TEST(Position, InitWithLatLonGivesValidFieldsForLatLon)
{
    cll::Position p{cll::wgs84::Latitude{}, cll::wgs84::Longitude{}};
    EXPECT_FALSE(p.altitude);
}

TEST(Position, InitWithLatLonAltGivesValidFieldsForLatLonAlt)
{
    cll::Position p{
        cll::wgs84::Latitude{},
        cll::wgs84::Longitude{},
        cll::wgs84::Altitude{}};
    EXPECT_TRUE(p.altitude ? true : false);
}

#include <com/lomiri/location/codec.h>

#include <core/dbus/message_streaming_operators.h>

TEST(Position, EncodingAndDecodingGivesSameResults)
{
    cll::Position p
    {
        cll::wgs84::Latitude{9. * cll::units::Degrees},
        cll::wgs84::Longitude{53. * cll::units::Degrees},
        cll::wgs84::Altitude{-2. * cll::units::Meters}
    };

    p.accuracy.horizontal =  cll::Position::Accuracy::Horizontal{300*cll::units::Meters};
    p.accuracy.vertical = cll::Position::Accuracy::Vertical{100*cll::units::Meters};

    auto msg = core::dbus::Message::make_method_call(
        "org.freedesktop.DBus",
        core::dbus::types::ObjectPath("/org/freedesktop/DBus"),
        "org.freedesktop.DBus",
        "ListNames");

    msg->writer() << p;

    cll::Position pp;
    msg->reader() >> pp;

    EXPECT_EQ(p, pp);
}
