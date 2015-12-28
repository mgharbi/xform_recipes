#ifndef PERF_MEASURE_H_CG7SWFIK
#define PERF_MEASURE_H_CG7SWFIK


#include <chrono>
namespace {

using namespace std;

inline chrono::steady_clock::time_point get_time(){
    return chrono::steady_clock::now();
}

inline long get_duration(chrono::steady_clock::time_point start,chrono::steady_clock::time_point end) {
    return chrono::duration_cast<chrono::milliseconds>(end-start).count();
}

} // namespace


#endif /* end of include guard: PERF_MEASURE_H_CG7SWFIK */

