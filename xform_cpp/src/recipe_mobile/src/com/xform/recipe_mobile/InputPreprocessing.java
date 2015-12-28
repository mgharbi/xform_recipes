package com.xform.recipe_mobile;

import android.util.Log;
import android.graphics.Bitmap;
import android.os.SystemClock;

import java.lang.Runnable;

public class InputPreprocessing implements Runnable {
    Bitmap _input;
    int    _scaleFactor;
    byte[] _histograms;
    long JNIRecipePtr;

    public InputPreprocessing(Bitmap input, int scaleFactor) {
        _input       = input;
        _scaleFactor = scaleFactor;
    }

    public long getJNIRecipePtr() {
        return JNIRecipePtr;
    }

    public void run() {
        Log.d("RecipeMobile", "Preprocessing input");
        long t_start = SystemClock.uptimeMillis();

        JNIRecipePtr = initRecipeState(_input);

        if(_scaleFactor > 1) {
            _histograms = preprocessInput(JNIRecipePtr);
        }
        Log.d("RecipeMobile", "  - TOT preprocessing: "+(SystemClock.uptimeMillis()-t_start)+"ms");
    }

    private native long initRecipeState(Bitmap input);
    private native byte[] preprocessInput(long recipePtr);
}
