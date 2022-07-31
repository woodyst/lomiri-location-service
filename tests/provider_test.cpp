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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace cll = com::lomiri::location;

namespace
{
template<typename T>
cll::Update<T> update_as_of_now(const T& value = T())
{
    return cll::Update<T>{value, cll::Clock::now()};
}

class DummyProvider : public com::lomiri::location::Provider
{
public:
    DummyProvider(cll::Provider::Features feats = cll::Provider::Features::none,
                  cll::Provider::Requirements requs= cll::Provider::Requirements::none)
        : com::lomiri::location::Provider(feats, requs)
    {
    }

    void inject_update(const com::lomiri::location::Update<com::lomiri::location::Position>& update)
    {
        mutable_updates().position(update);
    }

    void inject_update(const com::lomiri::location::Update<com::lomiri::location::Velocity>& update)
    {
        mutable_updates().velocity(update);
    }

    void inject_update(const com::lomiri::location::Update<com::lomiri::location::Heading>& update)
    {
        mutable_updates().heading(update);
    }
};
}

TEST(Provider, requirement_flags_passed_at_construction_are_correctly_stored)
{
    cll::Provider::Features features = cll::Provider::Features::none;
    cll::Provider::Requirements requirements =
            cll::Provider::Requirements::cell_network |
            cll::Provider::Requirements::data_network |
            cll::Provider::Requirements::monetary_spending |
            cll::Provider::Requirements::satellites;

    DummyProvider provider(features, requirements);

    EXPECT_TRUE(provider.requires(com::lomiri::location::Provider::Requirements::satellites));
    EXPECT_TRUE(provider.requires(com::lomiri::location::Provider::Requirements::cell_network));
    EXPECT_TRUE(provider.requires(com::lomiri::location::Provider::Requirements::data_network));
    EXPECT_TRUE(provider.requires(com::lomiri::location::Provider::Requirements::monetary_spending));
}

TEST(Provider, feature_flags_passed_at_construction_are_correctly_stored)
{
    cll::Provider::Features all_features
        = cll::Provider::Features::heading |
          cll::Provider::Features::position |
          cll::Provider::Features::velocity;
    DummyProvider provider(all_features);

    EXPECT_TRUE(provider.supports(com::lomiri::location::Provider::Features::position));
    EXPECT_TRUE(provider.supports(com::lomiri::location::Provider::Features::velocity));
    EXPECT_TRUE(provider.supports(com::lomiri::location::Provider::Features::heading));
}

TEST(Provider, delivering_a_message_invokes_subscribers)
{
    DummyProvider dp;
    bool position_update_triggered {false};
    bool heading_update_triggered {false};
    bool velocity_update_triggered {false};

    auto c1 = dp.updates().position.connect(
        [&](const com::lomiri::location::Update<com::lomiri::location::Position>&)
        {
            position_update_triggered = true;
        });

    auto c2 = dp.updates().heading.connect(
        [&](const com::lomiri::location::Update<com::lomiri::location::Heading>&)
        {
            heading_update_triggered = true;
        });

    auto c3 = dp.updates().velocity.connect(
        [&](const com::lomiri::location::Update<com::lomiri::location::Velocity>&)
        {
            velocity_update_triggered = true;
        });

    dp.inject_update(update_as_of_now<cll::Position>());
    dp.inject_update(update_as_of_now<cll::Heading>());
    dp.inject_update(update_as_of_now<cll::Velocity>());

    EXPECT_TRUE(position_update_triggered);
    EXPECT_TRUE(heading_update_triggered);
    EXPECT_TRUE(velocity_update_triggered);
}

namespace
{
struct MockProvider : public com::lomiri::location::Provider
{
    MockProvider() : cll::Provider()
    {
    }

    void inject_update(const cll::Update<cll::Position>& update)
    {
        mutable_updates().position(update);
    }

    void inject_update(const cll::Update<cll::Velocity>& update)
    {
        mutable_updates().velocity(update);
    }

    void inject_update(const cll::Update<cll::Heading>& update)
    {
        mutable_updates().heading(update);
    }

    MOCK_METHOD0(start_position_updates, void());
    MOCK_METHOD0(stop_position_updates, void());
    MOCK_METHOD0(start_heading_updates, void());
    MOCK_METHOD0(stop_heading_updates, void());
    MOCK_METHOD0(start_velocity_updates, void());
    MOCK_METHOD0(stop_velocity_updates, void());
};
}

TEST(Provider, starting_and_stopping_multiple_times_results_in_exactly_one_call_to_start_and_stop_on_provider)
{
    using namespace ::testing;
    NiceMock<MockProvider> provider;

    EXPECT_CALL(provider, start_position_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, stop_position_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, start_heading_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, stop_heading_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, start_velocity_updates()).Times(Exactly(1));
    EXPECT_CALL(provider, stop_velocity_updates()).Times(Exactly(1));

    provider.state_controller()->start_position_updates();
    provider.state_controller()->start_position_updates();
    EXPECT_TRUE(provider.state_controller()->are_position_updates_running());
    provider.state_controller()->stop_position_updates();
    provider.state_controller()->stop_position_updates();
    EXPECT_FALSE(provider.state_controller()->are_position_updates_running());

    provider.state_controller()->start_heading_updates();
    provider.state_controller()->start_heading_updates();
    EXPECT_TRUE(provider.state_controller()->are_heading_updates_running());
    provider.state_controller()->stop_heading_updates();
    provider.state_controller()->stop_heading_updates();
    EXPECT_FALSE(provider.state_controller()->are_heading_updates_running());

    provider.state_controller()->start_velocity_updates();
    provider.state_controller()->start_velocity_updates();
    EXPECT_TRUE(provider.state_controller()->are_velocity_updates_running());
    provider.state_controller()->stop_velocity_updates();
    provider.state_controller()->stop_velocity_updates();
    EXPECT_FALSE(provider.state_controller()->are_velocity_updates_running());
}

TEST(Provider, starting_updates_on_a_disabled_provider_does_nothing)
{
    using namespace ::testing;

    NiceMock<MockProvider> p;
    EXPECT_CALL(p, start_position_updates()).Times(0);
    EXPECT_CALL(p, start_heading_updates()).Times(0);
    EXPECT_CALL(p, start_velocity_updates()).Times(0);

    p.state_controller()->disable();

    p.state_controller()->start_position_updates();
    p.state_controller()->start_heading_updates();
    p.state_controller()->start_velocity_updates();
}

TEST(Provider, disabling_a_provider_stops_all_updates)
{
    using namespace ::testing;

    NiceMock<MockProvider> p;
    EXPECT_CALL(p, stop_position_updates()).Times(1);
    EXPECT_CALL(p, stop_heading_updates()).Times(1);
    EXPECT_CALL(p, stop_velocity_updates()).Times(1);

    p.state_controller()->start_position_updates();
    p.state_controller()->start_heading_updates();
    p.state_controller()->start_velocity_updates();

    p.state_controller()->disable();
}

#include <com/lomiri/location/proxy_provider.h>

TEST(ProxyProvider, start_and_stop_of_updates_propagates_to_correct_providers)
{
    using namespace ::testing;
    
    NiceMock<MockProvider> mp1, mp2, mp3;
    EXPECT_CALL(mp1, start_position_updates()).Times(Exactly(1));
    EXPECT_CALL(mp1, stop_position_updates()).Times(Exactly(1));
    EXPECT_CALL(mp2, start_heading_updates()).Times(Exactly(1));
    EXPECT_CALL(mp2, stop_heading_updates()).Times(Exactly(1));
    EXPECT_CALL(mp3, start_velocity_updates()).Times(Exactly(1));
    EXPECT_CALL(mp3, stop_velocity_updates()).Times(Exactly(1));

    cll::Provider::Ptr p1{std::addressof(mp1), [](cll::Provider*){}};
    cll::Provider::Ptr p2{std::addressof(mp2), [](cll::Provider*){}};
    cll::Provider::Ptr p3{std::addressof(mp3), [](cll::Provider*){}};
    
    cll::ProviderSelection selection{p1, p2, p3};

    cll::ProxyProvider pp{selection};

    pp.start_position_updates();
    pp.stop_position_updates();

    pp.start_heading_updates();
    pp.stop_heading_updates();

    pp.start_velocity_updates();
    pp.stop_velocity_updates();
}

struct MockEventConsumer
{
    MOCK_METHOD1(on_new_position, void(const cll::Update<cll::Position>&));
    MOCK_METHOD1(on_new_velocity, void(const cll::Update<cll::Velocity>&));
    MOCK_METHOD1(on_new_heading, void(const cll::Update<cll::Heading>&));
};

TEST(ProxyProvider, update_signals_are_routed_from_correct_providers)
{
    using namespace ::testing;
    
    NiceMock<MockProvider> mp1, mp2, mp3;

    cll::Provider::Ptr p1{std::addressof(mp1), [](cll::Provider*){}};
    cll::Provider::Ptr p2{std::addressof(mp2), [](cll::Provider*){}};
    cll::Provider::Ptr p3{std::addressof(mp3), [](cll::Provider*){}};
    
    cll::ProviderSelection selection{p1, p2, p3};

    cll::ProxyProvider pp{selection};

    NiceMock<MockEventConsumer> mec;
    EXPECT_CALL(mec, on_new_position(_)).Times(1);
    EXPECT_CALL(mec, on_new_velocity(_)).Times(1);
    EXPECT_CALL(mec, on_new_heading(_)).Times(1);

    pp.updates().position.connect([&mec](const cll::Update<cll::Position>& p){mec.on_new_position(p);});
    pp.updates().heading.connect([&mec](const cll::Update<cll::Heading>& h){mec.on_new_heading(h);});
    pp.updates().velocity.connect([&mec](const cll::Update<cll::Velocity>& v){mec.on_new_velocity(v);});

    mp1.inject_update(cll::Update<cll::Position>());
    mp2.inject_update(cll::Update<cll::Heading>());
    mp3.inject_update(cll::Update<cll::Velocity>());
}

#include <com/lomiri/location/fusion_provider.h>
#include <com/lomiri/location/newer_or_more_accurate_update_selector.h>

TEST(FusionProvider, start_and_stop_of_updates_propagates_to_correct_providers)
{
    using namespace ::testing;

    NiceMock<MockProvider> mp1, mp2, mp3;
    EXPECT_CALL(mp1, start_position_updates()).Times(Exactly(1));
    EXPECT_CALL(mp1, stop_position_updates()).Times(Exactly(1));
    EXPECT_CALL(mp2, start_heading_updates()).Times(Exactly(1));
    EXPECT_CALL(mp2, stop_heading_updates()).Times(Exactly(1));
    EXPECT_CALL(mp3, start_velocity_updates()).Times(Exactly(1));
    EXPECT_CALL(mp3, stop_velocity_updates()).Times(Exactly(1));

    cll::Provider::Ptr p1{std::addressof(mp1), [](cll::Provider*){}};
    cll::Provider::Ptr p2{std::addressof(mp2), [](cll::Provider*){}};
    cll::Provider::Ptr p3{std::addressof(mp3), [](cll::Provider*){}};

    cll::ProviderSelection selection{p1, p2, p3};
    std::set<cll::Provider::Ptr> providers{p1, p2, p3};

    //cll::FusionProvider pp{selection};
    cll::FusionProvider fp{providers, std::make_shared<cll::NewerOrMoreAccurateUpdateSelector>()};

    fp.start_position_updates();
    fp.stop_position_updates();

    fp.start_heading_updates();
    fp.stop_heading_updates();

    fp.start_velocity_updates();
    fp.stop_velocity_updates();
}

TEST(FusionProvider, update_signals_are_routed_from_correct_providers)
{
    using namespace ::testing;

    NiceMock<MockProvider> mp1, mp2, mp3;

    cll::Provider::Ptr p1{std::addressof(mp1), [](cll::Provider*){}};
    cll::Provider::Ptr p2{std::addressof(mp2), [](cll::Provider*){}};
    cll::Provider::Ptr p3{std::addressof(mp3), [](cll::Provider*){}};

    std::set<cll::Provider::Ptr> providers{p1, p2, p3};

    cll::FusionProvider fp{providers, std::make_shared<cll::NewerOrMoreAccurateUpdateSelector>()};

    NiceMock<MockEventConsumer> mec;
    EXPECT_CALL(mec, on_new_position(_)).Times(1);
    EXPECT_CALL(mec, on_new_velocity(_)).Times(1);
    EXPECT_CALL(mec, on_new_heading(_)).Times(1);

    fp.updates().position.connect([&mec](const cll::Update<cll::Position>& p){mec.on_new_position(p);});
    fp.updates().heading.connect([&mec](const cll::Update<cll::Heading>& h){mec.on_new_heading(h);});
    fp.updates().velocity.connect([&mec](const cll::Update<cll::Velocity>& v){mec.on_new_velocity(v);});

    mp1.inject_update(cll::Update<cll::Position>());
    mp2.inject_update(cll::Update<cll::Heading>());
    mp3.inject_update(cll::Update<cll::Velocity>());
}

#include <com/lomiri/location/clock.h>

TEST(FusionProvider, more_timely_update_is_chosen)
{
    using namespace ::testing;

    NiceMock<MockProvider> mp1, mp2;

    cll::Provider::Ptr p1{std::addressof(mp1), [](cll::Provider*){}};
    cll::Provider::Ptr p2{std::addressof(mp2), [](cll::Provider*){}};

    std::set<cll::Provider::Ptr> providers{p1, p2};

    cll::FusionProvider fp{providers, std::make_shared<cll::NewerOrMoreAccurateUpdateSelector>()};

    cll::Update<cll::Position> before, after;
    before.when = cll::Clock::now() - std::chrono::seconds(12);
    before.value = cll::Position(cll::wgs84::Latitude(), cll::wgs84::Longitude(), cll::wgs84::Altitude(), cll::Position::Accuracy::Horizontal{50*cll::units::Meters});
    after.when = cll::Clock::now();
    after.value = cll::Position(cll::wgs84::Latitude(), cll::wgs84::Longitude(), cll::wgs84::Altitude(), cll::Position::Accuracy::Horizontal{500*cll::units::Meters});

    NiceMock<MockEventConsumer> mec;
    EXPECT_CALL(mec, on_new_position(before)).Times(1);
    EXPECT_CALL(mec, on_new_position(after)).Times(1);

    fp.updates().position.connect([&mec](const cll::Update<cll::Position>& p){mec.on_new_position(p);});

    mp1.inject_update(before);
    mp2.inject_update(after);

}

TEST(FusionProvider, more_accurate_update_is_chosen)
{
    using namespace ::testing;

    NiceMock<MockProvider> mp1, mp2;

    cll::Provider::Ptr p1{std::addressof(mp1), [](cll::Provider*){}};
    cll::Provider::Ptr p2{std::addressof(mp2), [](cll::Provider*){}};

    std::set<cll::Provider::Ptr> providers{p1, p2};

    cll::FusionProvider fp{providers, std::make_shared<cll::NewerOrMoreAccurateUpdateSelector>()};

    cll::Update<cll::Position> before, after;
    before.when = cll::Clock::now() - std::chrono::seconds(5);
    before.value = cll::Position(cll::wgs84::Latitude(), cll::wgs84::Longitude(), cll::wgs84::Altitude(), cll::Position::Accuracy::Horizontal{50*cll::units::Meters});
    after.when = cll::Clock::now();
    after.value = cll::Position(cll::wgs84::Latitude(), cll::wgs84::Longitude(), cll::wgs84::Altitude(), cll::Position::Accuracy::Horizontal{500*cll::units::Meters});

    NiceMock<MockEventConsumer> mec;
    // We should see the "older" position in two events
    EXPECT_CALL(mec, on_new_position(before)).Times(2);

    fp.updates().position.connect([&mec](const cll::Update<cll::Position>& p){mec.on_new_position(p);});

    mp1.inject_update(before);
    mp2.inject_update(after);

}

TEST(FusionProvider, update_from_same_provider_is_chosen)
{
    using namespace ::testing;

    NiceMock<MockProvider> mp1;

    cll::Provider::Ptr p1{std::addressof(mp1), [](cll::Provider*){}};

    std::set<cll::Provider::Ptr> providers{p1};

    cll::FusionProvider fp{providers, std::make_shared<cll::NewerOrMoreAccurateUpdateSelector>()};

    cll::Update<cll::Position> before, after;
    before.when = cll::Clock::now() - std::chrono::seconds(5);
    before.value = cll::Position(cll::wgs84::Latitude(), cll::wgs84::Longitude(), cll::wgs84::Altitude(), cll::Position::Accuracy::Horizontal{50*cll::units::Meters});
    after.when = cll::Clock::now();
    after.value = cll::Position(cll::wgs84::Latitude(), cll::wgs84::Longitude(), cll::wgs84::Altitude(), cll::Position::Accuracy::Horizontal{500*cll::units::Meters});

    NiceMock<MockEventConsumer> mec;
    // We should see the "newer" position even though it's less accurate since
    // it came from the same source
    EXPECT_CALL(mec, on_new_position(before)).Times(1);
    EXPECT_CALL(mec, on_new_position(after)).Times(1);

    fp.updates().position.connect([&mec](const cll::Update<cll::Position>& p){mec.on_new_position(p);});

    mp1.inject_update(before);
    mp1.inject_update(after);
}
