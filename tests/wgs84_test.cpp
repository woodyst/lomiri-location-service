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
#include <com/lomiri/location/wgs84/coordinate.h>
#include <com/lomiri/location/wgs84/altitude.h>
#include <com/lomiri/location/wgs84/latitude.h>
#include <com/lomiri/location/wgs84/longitude.h>
#include <com/lomiri/location/position.h>

#include <gtest/gtest.h>

#include <random>

TEST(Latitude, constructing_a_latitude_with_invalid_value_throws)
{
    static const double min_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Latitude>::min();
    static const double max_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Latitude>::max();
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist1(
        -std::numeric_limits<double>::max(),
        min_value - std::numeric_limits<double>::min());
    std::uniform_real_distribution<double> dist2(
        max_value + std::numeric_limits<double>::min(),
        std::numeric_limits<double>::max());
    double dl = dist1(rng);
    double du = dist2(rng);
    EXPECT_ANY_THROW(com::lomiri::location::wgs84::Latitude l {dl*com::lomiri::location::units::Degrees};);
    EXPECT_ANY_THROW(com::lomiri::location::wgs84::Latitude l {du*com::lomiri::location::units::Degrees};);
}

TEST(Latitude, constructing_a_latitude_with_a_valid_value_does_not_throw)
{
    const double min_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Latitude>::min();
    const double max_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Latitude>::max();
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist(min_value,max_value);
    EXPECT_NO_THROW(com::lomiri::location::wgs84::Latitude l {dist(rng)*com::lomiri::location::units::Degrees};);
}

TEST(Latitude, a_latitude_contains_value_passed_at_construction)
{
    const double min_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Latitude>::min();
    const double max_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Latitude>::max();
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist(min_value,max_value);
    double d = dist(rng);
    com::lomiri::location::wgs84::Latitude l {d* com::lomiri::location::units::Degrees};
    EXPECT_EQ(d * com::lomiri::location::units::Degrees, l.value);
}

TEST(Longitude, constructing_a_longitude_with_invalid_value_throws)
{
    static const double min_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Longitude>::min();
    static const double max_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Longitude>::max();
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist1(
        -std::numeric_limits<double>::max(),
        min_value - std::numeric_limits<double>::min());
    std::uniform_real_distribution<double> dist2(
        max_value + std::numeric_limits<double>::min(),
        std::numeric_limits<double>::max());
    double dl = dist1(rng);
    double du = dist2(rng);
    EXPECT_ANY_THROW(com::lomiri::location::wgs84::Longitude l {dl*com::lomiri::location::units::Degrees};);
    EXPECT_ANY_THROW(com::lomiri::location::wgs84::Longitude l {du*com::lomiri::location::units::Degrees};);
}

TEST(Longitude, constructing_a_longitude_with_a_valid_value_does_not_throw)
{
    const double min_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Longitude>::min();
    const double max_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Longitude>::max();
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist(min_value,max_value);
    EXPECT_NO_THROW(com::lomiri::location::wgs84::Longitude l {dist(rng)*com::lomiri::location::units::Degrees};);
}

TEST(Longitude, a_longitude_contains_value_passed_at_construction)
{
    const double min_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Longitude>::min();
    const double max_value = com::lomiri::location::wgs84::CoordinateTraits<com::lomiri::location::wgs84::Longitude>::max();
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist(min_value,max_value);
    double d = dist(rng);
    com::lomiri::location::wgs84::Longitude l {d* com::lomiri::location::units::Degrees};
    EXPECT_EQ(d * com::lomiri::location::units::Degrees, l.value);
}

TEST(Altitude, an_altitude_contains_value_passed_at_construction)
{
    const double min_value = -std::numeric_limits<double>::max();
    const double max_value = std::numeric_limits<double>::max();
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist(min_value,max_value);
    double d = dist(rng);
    com::lomiri::location::wgs84::Altitude a {d* com::lomiri::location::units::Meters};
    EXPECT_EQ(d * com::lomiri::location::units::Meters, a.value);
}

TEST(HaversineDistance, calculating_the_haverstine_distance_yields_correct_values)
{
    com::lomiri::location::wgs84::Latitude lat1 {47.621800*com::lomiri::location::units::Degrees};
    com::lomiri::location::wgs84::Longitude lon1 {-122.350326*com::lomiri::location::units::Degrees};

    com::lomiri::location::wgs84::Latitude lat2 {47.041917*com::lomiri::location::units::Degrees};
    com::lomiri::location::wgs84::Longitude lon2 {-122.893766*com::lomiri::location::units::Degrees};

    com::lomiri::location::Position seattle {lat1, lon1};
    com::lomiri::location::Position olympia {lat2, lon2};

    com::lomiri::location::units::Quantity<com::lomiri::location::units::Length> expected_distance
    {
        76.386615799548693 * com::lomiri::location::units::kilo* com::lomiri::location::units::Meters
    };
    EXPECT_NEAR(expected_distance.value(), com::lomiri::location::haversine_distance(seattle, olympia).value(), 1E-3);
}
