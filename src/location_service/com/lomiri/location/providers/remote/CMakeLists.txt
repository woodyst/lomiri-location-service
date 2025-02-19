add_library(
    remote
    provider.h provider.cpp
    stub.cpp skeleton.cpp
)

target_link_libraries(remote
    PUBLIC
    glog::glog
)
