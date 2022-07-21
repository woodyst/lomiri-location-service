#include <com/lomiri/location/provider_selection_policy.h>

#include "mock_event_consumer.h"
#include "mock_provider.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace cll = com::lomiri::location;

namespace
{
struct ProviderSetEnumerator : public cll::ProviderEnumerator
{
    ProviderSetEnumerator(const std::set<std::shared_ptr<cll::Provider>>& providers)
        : providers(providers)
    {
    }

    void for_each_provider(const std::function<void(const std::shared_ptr<cll::Provider>&)>& f) const
    {
        for (const auto& provider : providers)
            f(provider);
    }

    std::set<std::shared_ptr<cll::Provider>> providers;
};

class DummyProvider : public cll::Provider
{
public:
    DummyProvider(cll::Provider::Features feats = cll::Provider::Features::none,
                  cll::Provider::Requirements requs= cll::Provider::Requirements::none)
        : com::lomiri::location::Provider(feats, requs)
    {
    }

    MOCK_METHOD1(matches_criteria, bool(const cll::Criteria&));
};

static const cll::Provider::Features all_features
    = cll::Provider::Features::heading | cll::Provider::Features::position | cll::Provider::Features::velocity;

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

TEST(ProviderSelection, feature_flags_calculation_works_correctly)
{
    cll::Provider::Ptr provider{new DummyProvider{}};

    cll::ProviderSelection selection{provider, provider, provider};

    EXPECT_EQ(all_features, selection.to_feature_flags());
}

#include <com/lomiri/location/default_provider_selection_policy.h>

TEST(DefaultProviderSelectionPolicy, if_no_provider_matches_criteria_null_is_returned)
{
    using namespace testing;

    cll::DefaultProviderSelectionPolicy policy;

    NiceMock<DummyProvider> provider1, provider2;
    ON_CALL(provider1, matches_criteria(_)).WillByDefault(Return(false));
    ON_CALL(provider2, matches_criteria(_)).WillByDefault(Return(false));
    
    std::set<cll::Provider::Ptr> providers;
    providers.insert(cll::Provider::Ptr{&provider1, [](cll::Provider*){}});
    providers.insert(cll::Provider::Ptr{&provider2, [](cll::Provider*){}});

    ProviderSetEnumerator enumerator{providers};

    EXPECT_EQ(cll::ProviderSelectionPolicy::null_provider(),
              policy.determine_position_updates_provider(cll::Criteria{}, enumerator));
    EXPECT_EQ(cll::ProviderSelectionPolicy::null_provider(),
              policy.determine_heading_updates_provider(cll::Criteria{}, enumerator));
    EXPECT_EQ(cll::ProviderSelectionPolicy::null_provider(),
              policy.determine_velocity_updates_provider(cll::Criteria{}, enumerator));

    cll::ProviderSelection empty_selection
    {
        cll::ProviderSelectionPolicy::null_provider(),
        cll::ProviderSelectionPolicy::null_provider(),
        cll::ProviderSelectionPolicy::null_provider()
    };
    EXPECT_EQ(empty_selection,
              policy.determine_provider_selection_for_criteria(cll::Criteria{}, enumerator));
}

TEST(DefaultProviderSelectionPolicy, an_already_running_provider_is_preferred)
{
    using namespace testing;

    cll::DefaultProviderSelectionPolicy policy;

    NiceMock<DummyProvider> provider1, provider2;
    ON_CALL(provider1, matches_criteria(_)).WillByDefault(Return(true));
    ON_CALL(provider2, matches_criteria(_)).WillByDefault(Return(true));
    
    provider1.state_controller()->start_position_updates();
    provider1.state_controller()->start_heading_updates();
    provider1.state_controller()->start_velocity_updates();

    cll::Provider::Ptr p1{&provider1, [](cll::Provider*){}};
    cll::Provider::Ptr p2{&provider2, [](cll::Provider*){}};
    
    std::set<cll::Provider::Ptr> providers{{p1, p2}};
    ProviderSetEnumerator enumerator{providers};

    EXPECT_EQ(p1,
              policy.determine_position_updates_provider(cll::Criteria{}, enumerator));
    EXPECT_EQ(p1,
              policy.determine_heading_updates_provider(cll::Criteria{}, enumerator));
    EXPECT_EQ(p1,
              policy.determine_velocity_updates_provider(cll::Criteria{}, enumerator));
    cll::ProviderSelection ps{p1, p1, p1};
    EXPECT_EQ(ps,
              policy.determine_provider_selection_for_criteria(cll::Criteria{}, enumerator));
}

#include <com/lomiri/location/non_selecting_provider_selection_policy.h>

TEST(NonSelectingProviderSelectionPolicy, returns_a_selection_of_providers_that_dispatches_to_all_underlying_providers)
{
    using namespace ::testing;

    auto provider1 = std::make_shared<NiceMock<MockProvider>>();
    auto provider2 = std::make_shared<NiceMock<MockProvider>>();

    MockEventConsumer mec;
    EXPECT_CALL(mec, on_new_position(_)).Times(2);
    EXPECT_CALL(mec, on_new_heading(_)).Times(2);
    EXPECT_CALL(mec, on_new_velocity(_)).Times(2);

    EXPECT_CALL(*provider1, start_position_updates()).Times(1);
    EXPECT_CALL(*provider1, stop_position_updates()).Times(1);

    EXPECT_CALL(*provider1, start_velocity_updates()).Times(1);
    EXPECT_CALL(*provider1, stop_velocity_updates()).Times(1);

    EXPECT_CALL(*provider1, start_heading_updates()).Times(1);
    EXPECT_CALL(*provider1, stop_heading_updates()).Times(1);

    EXPECT_CALL(*provider2, start_position_updates()).Times(1);
    EXPECT_CALL(*provider2, stop_position_updates()).Times(1);

    EXPECT_CALL(*provider2, start_velocity_updates()).Times(1);
    EXPECT_CALL(*provider2, stop_velocity_updates()).Times(1);

    EXPECT_CALL(*provider2, start_heading_updates()).Times(1);
    EXPECT_CALL(*provider2, stop_heading_updates()).Times(1);

    std::set<cll::Provider::Ptr> providers{{provider1, provider2}};

    cll::NonSelectingProviderSelectionPolicy policy;

    auto selection = policy.determine_provider_selection_for_criteria(
                cll::Criteria{},
                ProviderSetEnumerator{providers});

    selection.position_updates_provider->updates().position.connect([&mec](const cll::Update<cll::Position>& update)
    {
        mec.on_new_position(update);
    });

    selection.heading_updates_provider->updates().heading.connect([&mec](const cll::Update<cll::Heading>& update)
    {
        mec.on_new_heading(update);
    });

    selection.velocity_updates_provider->updates().velocity.connect([&mec](const cll::Update<cll::Velocity>& update)
    {
        mec.on_new_velocity(update);
    });

    selection.position_updates_provider->state_controller()->start_position_updates();
    selection.position_updates_provider->state_controller()->stop_position_updates();

    selection.heading_updates_provider->state_controller()->start_heading_updates();
    selection.heading_updates_provider->state_controller()->stop_heading_updates();

    selection.velocity_updates_provider->state_controller()->start_velocity_updates();
    selection.velocity_updates_provider->state_controller()->stop_velocity_updates();

    provider1->inject_update(reference_position_update);
    provider2->inject_update(reference_position_update);

    provider1->inject_update(reference_heading_update);
    provider2->inject_update(reference_heading_update);

    provider1->inject_update(reference_velocity_update);
    provider2->inject_update(reference_velocity_update);
}
