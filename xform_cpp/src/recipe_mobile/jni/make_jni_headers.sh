#!/bin/sh
# Script to generate c++ header from java code 

javah -o precomputation.h -classpath ../../../bin/recipe_mobile/classes/:$ANDROID_SDK/platforms/android-22/android.jar com.xform.recipe_mobile.Precomputation
javah -o input_preprocessing.h -classpath ../../../bin/recipe_mobile/classes/:$ANDROID_SDK/platforms/android-22/android.jar com.xform.recipe_mobile.InputPreprocessing
javah -o recipe_mobile.h -classpath ../../../bin/recipe_mobile/classes/:$ANDROID_SDK/platforms/android-22/android.jar com.xform.recipe_mobile.RecipeMobile 
