#include <com/lomiri/location/provider_factory.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>

namespace cll = com::lomiri::location;

namespace
{
struct Factory
{
    MOCK_METHOD1(create, cll::Provider::Ptr(const cll::ProviderFactory::Configuration&));
};
}

TEST(ProviderFactory, adding_a_factory_works)
{    
    const std::string provider_name{"ATotallyDummyProviderFactory"};
    Factory factory;
    auto factory_function = std::bind(&Factory::create, std::ref(factory), std::placeholders::_1);
    cll::ProviderFactory::instance().add_factory_for_name(provider_name, factory_function);

    bool found = false;
    cll::ProviderFactory::instance().enumerate([&found, &provider_name](const std::string& name, const cll::ProviderFactory::Factory&)
                                               {
                                                   if (!found)
                                                       found = (provider_name == name);
                                               });

    EXPECT_TRUE(found);
}

TEST(ProviderFactory, creating_for_known_name_invokes_factory_function)
{
    using namespace ::testing;

    const std::string provider_name{"ATotallyDummyProviderFactory"};
    Factory factory;
    ON_CALL(factory, create(_)).WillByDefault(Return(cll::Provider::Ptr{}));
    EXPECT_CALL(factory, create(_)).Times(Exactly(1));
    auto factory_function = std::bind(&Factory::create, std::ref(factory), std::placeholders::_1);
    cll::ProviderFactory::instance().add_factory_for_name(provider_name, factory_function);

    cll::ProviderFactory::instance().create_provider_for_name_with_config(provider_name, cll::ProviderFactory::Configuration{});
}

TEST(ProviderFactory, attempt_to_create_for_unknown_name_returns_null_ptr)
{
    EXPECT_EQ(cll::Provider::Ptr{},
              cll::ProviderFactory::instance().create_provider_for_name_with_config(
                  "AnUnknownProvider", 
                  cll::ProviderFactory::Configuration{}));
}

TEST(ProviderFactory, extracting_undecorated_provider_name_works)
{
    EXPECT_EQ("remote::Provider", cll::ProviderFactory::extract_undecorated_name("remote::Provider@gps"));
}
