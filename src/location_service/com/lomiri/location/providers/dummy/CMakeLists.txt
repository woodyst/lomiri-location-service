add_library(
  dummy
  
  provider.h
  provider.cpp
  delayed_provider.h
  delayed_provider.cpp)

target_link_libraries(dummy
  PUBLIC
  glog::glog)

set(
  ENABLED_PROVIDER_TARGETS
  ${ENABLED_PROVIDER_TARGETS} dummy
  PARENT_SCOPE)
