set(halide_local_laplacian_aot
    hl_local_laplacian
)

set(halide_objs)
set(objdir ${obj}/filters/local_laplacian)
foreach(f ${halide_local_laplacian_aot})
    halide_aot_compile(${f}
        BIN ${bin}/filters/local_laplacian
        OBJ ${objdir}
        INC ${inc}/filters/local_laplacian
    )
    list(APPEND halide_objs ${objdir}/${f}.o)
endforeach(f)

add_library(local_laplacian STATIC
    ${halide_objs}
)

SET_TARGET_PROPERTIES (
    local_laplacian
    PROPERTIES LINKER_LANGUAGE CXX
)
