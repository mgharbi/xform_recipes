add_subdirectory(jni)

add_custom_target(
    recipe_mobile
    DEPENDS android_jni src/com/xform/recipe_mobile/RecipeMobile.java
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ant debug
)

add_custom_target(
    phone
    DEPENDS recipe_mobile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ant installd
)
