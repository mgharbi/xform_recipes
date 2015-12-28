#ifndef PRINT_HELPER_H_IAGPKWAE
#define PRINT_HELPER_H_IAGPKWAE

#ifdef __ANDROID__
    #include <android/log.h>
    #ifndef DEBUG_TAG
        #define DEBUG_TAG "MobileLocal"
    #endif
    #define PRINT(...) __android_log_print(ANDROID_LOG_DEBUG, "MobileLocal",__VA_ARGS__)
#else
    #define PRINT(...) fprintf(stderr, __VA_ARGS__)
#endif

#endif /* end of include guard: PRINT_HELPER_H_IAGPKWAE */

