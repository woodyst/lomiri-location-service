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
#include <com/lomiri/location/heading.h>

#include <gtest/gtest.h>

TEST(Heading, constructing_a_heading_with_invalid_value_throws)
{
    EXPECT_ANY_THROW(com::lomiri::location::Heading h {-std::numeric_limits<double>::min()*com::lomiri::location::units::Degrees};);
    EXPECT_ANY_THROW(com::lomiri::location::Heading h {361.*com::lomiri::location::units::Degrees};);
}

TEST(Heading, constructing_a_heading_with_a_valid_value_does_not_throw)
{
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist(com::lomiri::location::Heading::min().value(),
            com::lomiri::location::Heading::max().value());

    EXPECT_NO_THROW(com::lomiri::location::Heading h {dist(rng)*com::lomiri::location::units::Degrees};);
}

TEST(Heading, a_heading_contains_value_passed_at_construction)
{
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist(com::lomiri::location::Heading::min().value(),
            com::lomiri::location::Heading::max().value());
    double d = dist(rng);
    com::lomiri::location::Heading h {d* com::lomiri::location::units::Degrees};
    EXPECT_DOUBLE_EQ(d, h.value.value());
}
