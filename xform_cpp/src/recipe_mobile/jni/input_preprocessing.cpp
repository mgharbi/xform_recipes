#include "input_preprocessing.h"
#include "perf_measure.h"

#include "RecipeState.h"
#include <android/bitmap.h>
#include "print_helper.h"

JNIEXPORT jlong JNICALL Java_com_xform_recipe_1mobile_InputPreprocessing_initRecipeState
  (JNIEnv *env, jobject, jobject jinput)
{
    RecipeState *recipe_state = new RecipeState(env, jinput);
    return (jlong)recipe_state;
}

JNIEXPORT jbyteArray JNICALL Java_com_xform_recipe_1mobile_InputPreprocessing_preprocessInput
  (JNIEnv *env, jobject, jlong jrecipePtr)
{
    uint8_t *hdata;
    unsigned long hsize;
    RecipeState *recipe_state = (RecipeState*) jrecipePtr;
    if(!recipe_state) {
        PRINT("recipe state is null !\n");
    }

    recipe_state->preprocess_input(&hdata, &hsize);

    jbyteArray result;
    result = env->NewByteArray(hsize);
    env->SetByteArrayRegion(result,0,hsize,(jbyte*)hdata);

    return result;
}
