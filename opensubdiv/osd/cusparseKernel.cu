#include <stdio.h>

#define THREADS_PER_BLOCK 512

__global__ void
expand(int src_numthreads, int nve,
  int* dst_rows, int* dst_cols, float* dst_vals,
  int* src_rows, int* src_cols, float* src_vals)
{
    int thread = threadIdx.x + blockIdx.x * blockDim.x;
    if (thread >= src_numthreads)
        return;

    int r = thread / nve; // src_row
    int k = thread % nve; // replica number

    int i = src_rows[r];
    int stride = src_rows[r+1]-src_rows[r];
    int dst_base = i*nve + k*stride;
    int src_base = src_rows[r];
    dst_rows[r*nve + k] = dst_base;

    for(i = src_rows[r]; i < src_rows[r+1]; i++) {
	    int offset = i - src_base;
	    int col = src_cols[i];
	    float val = src_vals[i];
	    dst_cols[dst_base+offset] = col*nve + k;
	    dst_vals[dst_base+offset] = val;
    }
}

__global__ void
spmv(int m, int nnz, const int* M_rows, const int* M_cols, const float* M_vals, const float* V_in, float* V_out)
{
    int row = threadIdx.x + blockIdx.x * blockDim.x;
    if (row >= m)
        return;

    register float answer = 0.0;
    int lb = M_rows[row],
        ub = M_rows[row+1];

    for(int offset = lb; offset < ub; offset++)
	answer += M_vals[offset] * V_in[ M_cols[offset] ];

    V_out[row] = answer;
}

extern "C" {

#include <cusparse_v2.h>

void
OsdCusparseExpand(int src_numrows, int factor,
    int* dst_rows, int* dst_cols, float* dst_vals,
    int* src_rows, int* src_cols, float* src_vals)
{
    int blks = (src_numrows*factor + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    expand<<<blks,THREADS_PER_BLOCK>>>(src_numrows, factor,
            dst_rows, dst_cols, dst_vals,
            src_rows, src_cols, src_vals);
}

cusparseStatus_t
my_cusparseScsrmv(cusparseHandle_t handle, cusparseOperation_t transA,
    int m, int n, int nnz, float* alpha,
    cusparseMatDescr_t descrA,
    const float *csrValA,
    const int *csrRowPtrA, const int *csrColIndA,
    const float *x, float* beta,
    float *y ) {

    const int* M_rows = csrRowPtrA;
    const int* M_cols = csrColIndA;
    const float* M_vals = csrValA;
    const float* V_in = x;
    float* V_out = y;

    int blks = (m + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    spmv<<<blks,THREADS_PER_BLOCK>>>(m, nnz, M_rows, M_cols, M_vals, V_in, V_out);

    return CUSPARSE_STATUS_SUCCESS;
}

} /* extern C */
