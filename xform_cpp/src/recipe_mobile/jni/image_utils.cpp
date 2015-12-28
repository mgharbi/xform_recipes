#include "image_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t createPixel(int r, int g, int b, int a) {
  return ((a & 0xff) << 24)
       | ((r & 0xff) << 16)
       | ((g & 0xff) << 8)
       | ((b & 0xff));
}

Image<float> copy_to_HImage(JNIEnv * env, const jobject& bitmap, int nchannel, float scale){

	AndroidBitmapInfo  info;

	AndroidBitmap_getInfo(env, bitmap, &info);

	void* bitmapPixels;
	AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels);
	uint32_t* src = (uint32_t*) bitmapPixels;

	Image<float> input(info.width, info.height, nchannel);
	/* Parse */
    for (int x = 0; x < (int) info.width; ++x){
        for (int y = 0; y < (int) info.height; ++y)
        {
            uint32_t zz = src[info.width * y + x];
            float b = static_cast<float>((zz%256))/scale;  zz >>= 8;
            float g = static_cast<float>((zz%256))/scale;  zz >>= 8;
            float r = static_cast<float>((zz%256))/scale;  zz >>= 8;
            (input)(x,y,0) = r;

            if (nchannel > 1){
                (input)(x,y,1) = g;
                (input)(x,y,2) = b;
            }
        }
    }
    return input;
}

void copy_to_jBuffer(JNIEnv * env, const Image<float>& output, jobject& bitmap, float scale){

	AndroidBitmapInfo  info;

	AndroidBitmap_getInfo(env, bitmap, &info);


	void* bitmapPixels;
	AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels);
	uint32_t* src = (uint32_t*) bitmapPixels;

    for (int x = 0; x < (int) info.width; ++x){
        for (int y = 0; y < (int) info.height; ++y){
            int r = static_cast<int>(output(x,y,0)*scale);
            int g = static_cast<int>(output(x,y,1)*scale);
            int b = static_cast<int>(output(x,y,2)*scale);
            src[info.width * y + x] = createPixel(r, g, b, 0xff);
        }
    }
}

// ----

Image<uint32_t> copy_to_uint_HImage(JNIEnv *env, const jobject &bitmap) {
	void* bitmapPixels = NULL;

	AndroidBitmapInfo       info;
	AndroidBitmap_getInfo   (env, bitmap, &info);
	AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels);

    size_t dataSize = info.width * info.height * sizeof(uint32_t);

	Image<uint32_t> input(info.width, info.height);
    memcpy(static_cast<void*>(input.data()), bitmapPixels, dataSize);

    return input;
}

void copy_to_uint_jBuffer(JNIEnv *env, Image<uint32_t> output, jobject &bitmap) {
	void* bitmapPixels = NULL;

    AndroidBitmapInfo       info;
	AndroidBitmap_getInfo   (env, bitmap, &info);
	AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels);

    size_t dataSize = info.width * info.height * sizeof(uint32_t);
    memcpy(bitmapPixels, static_cast<void*>(output.data()), dataSize);
}

#ifdef __cplusplus
}
#endif
