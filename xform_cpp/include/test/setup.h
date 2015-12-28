#ifndef SETUP_H_VHZAE6YU
#define SETUP_H_VHZAE6YU


#include <boost/filesystem.hpp> 
namespace fs=boost::filesystem;

class TestParams
{
public:
    static const fs::path project_root;
    static const fs::path fixture_path;
    static const fs::path output_path;
};

#endif /* end of include guard: SETUP_H_VHZAE6YU */
