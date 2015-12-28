#ifndef LINEAR_SOLVER_H_Z2TBX1YC
#define LINEAR_SOLVER_H_Z2TBX1YC

void linear_solver(  
        int n,
        int nnz,
        int *row_idx,
        int *col_idx,
        float *values,
        float *rhs_u,
        float *rhs_v,
        float *result_u,
        float *result_v,
        int max_iter = 100,
        float tol = 1e-4
);


#endif /* end of include guard: LINEAR_SOLVER_H_Z2TBX1YC */

