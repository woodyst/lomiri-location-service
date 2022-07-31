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

#include <bitset>
#include <ostream>

namespace cll = com::lomiri::location;

cll::Position::Position(
        const cll::wgs84::Latitude& latitude,
        const cll::wgs84::Longitude& longitude)
    : latitude(latitude),
      longitude(longitude)
{
}

cll::Position::Position(
        const cll::wgs84::Latitude& latitude,
        const cll::wgs84::Longitude& longitude,
        const cll::wgs84::Altitude& altitude)
    : latitude(latitude),
      longitude(longitude),
      altitude(altitude)
{
}

cll::Position::Position(const cll::wgs84::Latitude& lat,
                        const cll::wgs84::Longitude& lon,
                        const cll::wgs84::Altitude& alt,
                        const cll::units::Quantity<cll::units::Length>& hor_acc)
    : cll::Position::Position(lat, lon, alt)
{
    accuracy.horizontal = hor_acc;
}

cll::Position::Position(const cll::wgs84::Latitude& lat,
                        const cll::wgs84::Longitude& lon,
                        const cll::wgs84::Altitude& alt,
                        const cll::units::Quantity<cll::units::Length>& hor_acc,
                        const cll::units::Quantity<cll::units::Length>& ver_acc)
    : cll::Position::Position(lat, lon, alt, hor_acc)
{
    accuracy.vertical = ver_acc;
}

bool cll::Position::operator==(const cll::Position& rhs) const
{
    return latitude == rhs.latitude &&
            longitude == rhs.longitude &&
            altitude == rhs.altitude &&
            accuracy.horizontal == rhs.accuracy.horizontal &&
            accuracy.vertical == rhs.accuracy.vertical;
}

bool cll::Position::operator!=(const cll::Position& rhs) const
{
    return !(*this == rhs);
}

std::ostream& cll::operator<<(std::ostream& out, const cll::Position& position)
{
    out << "Position("
        << "lat: " << position.latitude << ", "
        << "lon: " << position.longitude << ", ";
    out << "alt: ";
    if (position.altitude)
        out << *position.altitude;
    else
        out << "n/a";
    out << ", ";
    out << "hor.acc.: ";
    if (position.accuracy.horizontal)
        out << *position.accuracy.horizontal;
    else
        out << "n/a";
    out << ", ";
    out << "ver.acc.: ";
    if (position.accuracy.vertical)
        out << *position.accuracy.vertical;
    else
        out << "n/a";
    out << ")";
    return out;
}

cll::units::Quantity<cll::units::Length> cll::haversine_distance(const cll::Position& p1, const cll::Position& p2)
{
    static const units::Quantity<units::Length> radius_of_earth {6371 * units::kilo * units::Meters};
    auto dLat = p2.latitude - p1.latitude;
    auto dLon = p2.longitude - p1.longitude;
    auto a =
        std::pow(units::sin(dLat.value/2.), 2) +
        std::pow(units::sin(dLon.value/2.), 2) * units::cos(p1.latitude.value) * units::cos(p2.latitude.value);

    auto c = 2. * std::atan2(std::sqrt(a), std::sqrt(1.-a));
    return radius_of_earth * c;
}
