#include <com/lomiri/location/init_and_shutdown.h>
#include <com/lomiri/location/logging.h>

#include <gflags/gflags.h>

namespace cll = com::lomiri::location;

namespace
{
}

void cll::init(int* argc, char*** argv)
{
    static const bool remove_parsed_flags = true;
    google::ParseCommandLineFlags(argc, argv, remove_parsed_flags);
}

void cll::shutdown()
{
    google::ShutDownCommandLineFlags();
}
