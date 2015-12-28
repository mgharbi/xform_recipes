package com.xform.recipe_mobile;

import android.util.Log;

import java.lang.Runnable;

public class Precomputation implements Runnable {
    private long JNIRecipePtr;

    public Precomputation(long recipePtr) {
        JNIRecipePtr = recipePtr;
    }

    public void run() {
        Log.d("RecipeMobile", "precomputing features");
        precomputeFeatures(JNIRecipePtr);
    }
    private native void precomputeFeatures(long recipePtr);
}
