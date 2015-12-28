#include "test/setup.h"

const fs::path TestParams::project_root = fs::current_path().parent_path().parent_path();
const fs::path TestParams::fixture_path = TestParams::project_root / "data" / "test_fixtures";
const fs::path TestParams::output_path  = TestParams::project_root / "output" ;
