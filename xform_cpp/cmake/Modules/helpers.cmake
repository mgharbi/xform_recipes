function(halide_aot_compile f)
    set(bin_dir BIN RENAME)
    set(inc_dir INC RENAME)
    set(obj_dir OBJ RENAME)
    set(extra_libs LIBRARIES CONFIGURATIONS)
    set(halide_args HALIDE_ARGS RENAME)
    set(no_compilation NO_COMPILATION RENAME)
    cmake_parse_arguments(AOT "" "${bin_dir};${inc_dir};${obj_dir};${halide_args};${no_compilation}" "${extra_libs}" ${ARGN})

    add_executable(${f} "${f}.cpp")
    set_target_properties(${f} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${AOT_BIN}
    )
    target_link_libraries(${f}
        ${HALIDE_LIBRARY}
        ${AOT_LIBRARIES}
    )

    # Run the halide file to generate .h and .o
    add_custom_command(
        OUTPUT ${AOT_OBJ}/${f}.o ${AOT_INC}/${f}.h
        DEPENDS ${f}
        WORKING_DIRECTORY ${AOT_BIN}
        COMMAND "./${f}"
        ARGS  ${AOT_HALIDE_ARGS}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${AOT_BIN}/${f}.o ${AOT_OBJ}/${f}.o
        COMMAND ${CMAKE_COMMAND}
        ARGS -E remove ${AOT_BIN}/${f}.o
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${AOT_BIN}/${f}.h ${AOT_INC}/${f}.h
        COMMAND ${CMAKE_COMMAND}
        ARGS -E remove ${AOT_BIN}/${f}.h
    )
endfunction()

function(halide_compile_with_args f)
    set(bin_dir BIN RENAME)
    set(inc_dir INC RENAME)
    set(obj_dir OBJ RENAME)
    set(halide_args HALIDE_ARGS RENAME)
    set(output_name OUTPUT_NAME RENAME)
    cmake_parse_arguments(AOT "" "${bin_dir};${inc_dir};${obj_dir};${halide_args};${output_name}" "${extra_libs}" ${ARGN})

    add_custom_command(
        OUTPUT ${AOT_OBJ}/${AOT_OUTPUT_NAME}.o ${AOT_INC}/${AOT_OUTPUT_NAME}.h
        WORKING_DIRECTORY ${AOT_BIN}
        DEPENDS ${f}
        COMMAND ./${f} ${AOT_HALIDE_ARGS}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E make_directory ${AOT_OBJ}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${AOT_BIN}/${AOT_OUTPUT_NAME}.o ${AOT_OBJ}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E remove ${AOT_BIN}/${AOT_OUTPUT_NAME}.o
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${AOT_BIN}/${AOT_OUTPUT_NAME}.h ${AOT_INC}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E remove ${AOT_BIN}/${AOT_OUTPUT_NAME}.h
    )
endfunction()

function(halide_arm_compile f)
    set(bin_dir BIN RENAME)
    set(inc_dir INC RENAME)
    set(obj_dir OBJ RENAME)
    cmake_parse_arguments(AOT "" "${bin_dir};${inc_dir};${obj_dir}" "${extra_libs}" ${ARGN})

    add_custom_command(
        OUTPUT ${AOT_OBJ}/${f}.o
        WORKING_DIRECTORY ${AOT_BIN}
        DEPENDS ${f}
        COMMAND HL_TARGET=${HL_TARGET} ./${f}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E make_directory ${AOT_OBJ}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${AOT_BIN}/${f}.o ${AOT_OBJ}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E remove ${AOT_BIN}/${f}.o
        COMMAND ${CMAKE_COMMAND}
        ARGS -E remove ${AOT_BIN}/${f}.h
    )
endfunction()

function(halide_gpu_compile f)
    set(bin_dir BIN RENAME)
    set(inc_dir INC RENAME)
    set(obj_dir OBJ RENAME)
    set(extra_libs LIBRARIES CONFIGURATIONS)
    cmake_parse_arguments(AOT "" "${bin_dir};${inc_dir};${obj_dir};${target}" "${extra_libs}" ${ARGN})

    add_custom_command(
        OUTPUT ${AOT_OBJ}/${f}.o
        WORKING_DIRECTORY ${AOT_BIN}
        DEPENDS ${f}
        COMMAND HL_TARGET=host-opencl ./${f}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${AOT_BIN}/${f}.o ${AOT_OBJ}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E remove ${AOT_BIN}/${f}.o
        COMMAND ${CMAKE_COMMAND}
        ARGS -E remove ${AOT_BIN}/${f}.h
    )
endfunction()
