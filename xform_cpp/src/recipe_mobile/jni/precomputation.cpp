#include "precomputation.h"

#include "perf_measure.h"

#include "RecipeState.h"
#include <android/bitmap.h>
#include "print_helper.h"

JNIEXPORT void JNICALL Java_com_xform_recipe_1mobile_Precomputation_precomputeFeatures
  (JNIEnv *, jobject, jlong jrecipePtr)
{

    RecipeState *recipe_state = (RecipeState*) jrecipePtr;
    if(!recipe_state) {
        PRINT("recipe state is null !\n");
    }

    recipe_state->precompute_features();
}
