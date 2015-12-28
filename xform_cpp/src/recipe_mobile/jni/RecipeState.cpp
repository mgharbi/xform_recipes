#include "RecipeState.h"

RecipeState::RecipeState(JNIEnv *env, jobject jinput) {
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, jinput, &info);
    input = copy_to_uint_HImage(env, jinput);
    recipe = new xform::Recipe(input);
}

RecipeState::~RecipeState() {
    if(recipe) {
        delete recipe;
        recipe = nullptr;
    }
}

void RecipeState::set_hp_coefficients(JNIEnv* env, jobject jrecipeCoefs) {
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, jrecipeCoefs, &info);
    hp_coefficients = copy_to_HImage(env, jrecipeCoefs, 1,1.0f);
    recipe->set_hp_coefs(hp_coefficients);
}

void RecipeState::set_lp_residual(JNIEnv* env, jobject jlowpassResidual) {
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, jlowpassResidual, &info);
    lp_residual = copy_to_uint_HImage(env, jlowpassResidual);
    recipe->set_lp_residual(lp_residual);
}

void RecipeState::set_qtable(JNIEnv *env, jfloatArray qTable) {
    jfloat* jqtable = env->GetFloatArrayElements(qTable,0);
    int qlength = env->GetArrayLength(qTable);
    qtable = std::vector<float>(qlength);
    qtable.assign(jqtable, jqtable+qlength);
    env->ReleaseFloatArrayElements(qTable, jqtable,0);
    recipe->set_qtable(qtable);
}

void RecipeState::preprocess_input(uint8_t ** hdata, unsigned long * hsize) {
    client_preprocessing(input,hdata,hsize);
}

void RecipeState::precompute_features() {
    auto start = get_time();
    recipe->precompute_features(input);
    PRINT("  - TOT precomputation: %ldms\n", get_duration(start,get_time()));
}

void RecipeState::reconstruct_from_precomputed(JNIEnv *env, jobject joutput) {
    Image<uint32_t> output(input.width(), input.height());

    auto start = get_time();
    recipe->reconstruct_with_features(output);
    PRINT("  - TOT reconstruction: %ldms\n", get_duration(start,get_time()));

    start = get_time();
    copy_to_uint_jBuffer(env, output, joutput);
}
