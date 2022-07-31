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

#include <string>
#include <com/lomiri/location/service/state.h>

#include <map>
#include <type_traits>

namespace clls = com::lomiri::location::service;

namespace
{
typedef typename std::underlying_type<clls::State>::type UT;

typedef std::pair<std::map<clls::State, std::string>, std::map<std::string, clls::State>> Lut;

const Lut& lut()
{
    static const Lut instance
    {
        {
            {clls::State::disabled, "disabled"},
            {clls::State::enabled, "enabled"},
            {clls::State::active, "active"}
        },
        {
            {"disabled", clls::State::disabled},
            {"enabled", clls::State::enabled},
            {"active", clls::State::active}
        }
    };

    return instance;
}
}

bool clls::operator==(clls::State lhs, clls::State rhs)
{
    return static_cast<UT>(lhs) == static_cast<UT>(rhs);
}

bool clls::operator!=(clls::State lhs, clls::State rhs)
{
    return static_cast<UT>(lhs) != static_cast<UT>(rhs);
}

std::ostream& clls::operator<<(std::ostream& out, clls::State state)
{
    return out << lut().first.at(state);
}

std::istream& clls::operator>>(std::istream& in, clls::State& state)
{
    std::string s; in >> s; state = lut().second.at(s);
    return in;
}
