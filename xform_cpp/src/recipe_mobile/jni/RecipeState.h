#ifndef RECIPE_STATE_H_3F0SUKV1
#define RECIPE_STATE_H_3F0SUKV1

#include <vector>
#include <android/bitmap.h>
#include <android/log.h>

#include "recipe/client_preprocessing.h"
#include "recipe/Recipe.h"
#include "image_utils.h"

#include "perf_measure.h"
#include "print_helper.h"

#ifndef DEBUG_TAG
#define DEBUG_TAG "RecipeMobile"
#endif

class RecipeState
{
public:
    RecipeState(JNIEnv *env, jobject jinput);

    virtual ~RecipeState();

    void set_hp_coefficients(JNIEnv* env, jobject jrecipeCoefs);
    void set_lp_residual(JNIEnv* env, jobject jlowpassResidual);
    void set_qtable(JNIEnv *env, jfloatArray qTable);

    void preprocess_input(uint8_t ** hdata, unsigned long * hsize);
    void precompute_features();

    void reconstruct_image(JNIEnv *env, jobject joutput);
    void reconstruct_from_precomputed(JNIEnv *env, jobject joutput);

private:
    xform::Recipe      *recipe;
    Image<uint32_t>     input;
    Image<float>        hp_coefficients;
    Image<uint32_t>     lp_residual;
    std::vector<float>  qtable;
};


#endif /* end of include guard: RECIPE_STATE_H_3F0SUKV1 */

