#include <jni.h>

#ifndef IMAGE_UTILS_H_MYW5VGZA
#define IMAGE_UTILS_H_MYW5VGZA

#include <string.h>
#include <stdio.h>
#include <android/log.h>
#include <android/bitmap.h>

#include "utils/static_image.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEBUG_TAG
#define DEBUG_TAG "RecipeMobile"
#endif

uint32_t createPixel(int r, int g, int b, int a);

Image<float> copy_to_HImage(JNIEnv * env, const jobject& bitmap, int nchannel, float scale = 255.0f);
Image<uint32_t> copy_to_uint_HImage(JNIEnv *env, const jobject &bitmap);

void copy_to_jBuffer(JNIEnv * env, const Image<float>& output, jobject& bitmap, float scale = 255.0f);
void copy_to_uint_jBuffer(JNIEnv *env, Image<uint32_t> output, jobject &bitmap);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: IMAGE_UTILS_H_MYW5VGZA */
