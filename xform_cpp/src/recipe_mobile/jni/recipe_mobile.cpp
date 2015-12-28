#include <string.h>
#include <stdio.h>
#include <vector>


#include "perf_measure.h"

#include "RecipeState.h"
#include <android/bitmap.h>
#include "print_helper.h"

#include "filters/local_laplacian/hl_local_laplacian.h"
#include "filters/style_transfer/style_transfer.h"
#include "filters/colorization/colorize.h"

extern RecipeState* recipe_state;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_xform_recipe_1mobile_RecipeMobile_filterLocalLaplacian
  (JNIEnv * env, jobject, jobject jinput, jobject joutput, jint levels, jfloat alpha)
{
    auto start = get_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, jinput, &info);

    Image<uint32_t> input = copy_to_uint_HImage(env, jinput);

    Image<uint32_t> output(input.width(), input.height());
    float beta = 1.0f;

    start = get_time();
    hl_local_laplacian(levels, alpha/(levels-1), beta, input, output);

    PRINT("process local laplacian: %ldms\n", get_duration(start,get_time()));

    start = get_time();
    copy_to_uint_jBuffer(env, output, joutput);

    PRINT("convert Halide buffer to output: %ldms\n", get_duration(start,get_time()));
}


JNIEXPORT void JNICALL Java_com_xform_recipe_1mobile_RecipeMobile_filterColorization
  (JNIEnv * env, jobject, jobject jinput, jobject jscribbles, jobject joutput)
{
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, jinput, &info);
    AndroidBitmap_getInfo(env, jscribbles, &info);

    Image<uint32_t> input = copy_to_uint_HImage(env, jinput);
    Image<uint32_t> scribbles= copy_to_uint_HImage(env, jscribbles);

    Image<uint32_t> output(input.width(), input.height());

    colorize(input, scribbles, output);

    copy_to_uint_jBuffer(env, output, joutput);
}


JNIEXPORT void JNICALL Java_com_xform_recipe_1mobile_RecipeMobile_filterStyleTransfer
  (JNIEnv * env, jobject, jobject jinput, jobject jtarget, jobject joutput, jint levels, jint iterations)
{
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, jinput, &info);
    AndroidBitmap_getInfo(env, jtarget, &info);

    Image<uint32_t> input = copy_to_uint_HImage(env, jinput);
    Image<uint32_t> target= copy_to_uint_HImage(env, jtarget);

    Image<uint32_t> output(input.width(), input.height());

    style_transfer(input, target, levels, output, iterations);

    copy_to_uint_jBuffer(env, output, joutput);
}


JNIEXPORT void JNICALL Java_com_xform_recipe_1mobile_RecipeMobile_recipeReconstruct
  (JNIEnv *env, jobject,
   jobject jrecipeCoefs,
   jobject jlowpassResidual,
   jfloatArray qTable,
   jobject joutput,
   jlong jrecipePtr
   )
{
    RecipeState *recipe_state = (RecipeState*) jrecipePtr;
    if(!recipe_state) {
        PRINT("recipe state is null !\n");
    }

    recipe_state->set_hp_coefficients(env, jrecipeCoefs);
    recipe_state->set_lp_residual(env, jlowpassResidual);
    recipe_state->set_qtable(env, qTable);

    recipe_state->reconstruct_from_precomputed(env, joutput);
}


JNIEXPORT void JNICALL Java_com_xform_recipe_1mobile_RecipeMobile_destroyRecipeState
  (JNIEnv *, jobject, jlong jrecipePtr)
{
    RecipeState *recipe_state = (RecipeState*) jrecipePtr;
    if(!recipe_state) {
        PRINT("recipe state is null !\n");
    }

    PRINT("destroy recipe state\n");
    if(recipe_state) {
        delete recipe_state;
        recipe_state = NULL;
    }
}

#ifdef __cplusplus
}
#endif
