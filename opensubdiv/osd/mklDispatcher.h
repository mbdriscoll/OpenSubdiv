#ifndef OSD_MKL_DISPATCHER_H
#define OSD_MKL_DISPATCHER_H

#include <stdlib.h>

#include "../version.h"
#include "../osd/spmvDispatcher.h"

extern "C" {
#include <mkl_spblas.h>
}

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

class CpuCsrMatrix;

class CpuCooMatrix : public CooMatrix {
public:
    CpuCooMatrix(int m, int n);

    virtual CpuCsrMatrix* gemm(CpuCsrMatrix* rhs);
    virtual void append_element(int i, int j, float val);

    std::vector<int> rows;
    std::vector<int> cols;
    std::vector<float> vals;
};

class CpuCsrMatrix : public CsrMatrix {
public:
    int* rows;
    int* cols;
    float* vals;

    CpuCsrMatrix(int m, int n, int nnz, int nve=1);
    CpuCsrMatrix(const CpuCooMatrix* StagedOp, int nve=1);
    virtual ~CpuCsrMatrix();

    virtual void spmv(float* d_out, float* d_in);
    virtual void logical_spmv(float* d_out, float* d_in, float *h_in);
    virtual CpuCsrMatrix* gemm(CpuCsrMatrix* rhs);
    virtual void dump(std::string ofilename);
};


class OsdMklKernelDispatcher :
    public OsdSpMVKernelDispatcher<CpuCooMatrix,CpuCsrMatrix,OsdCpuVertexBuffer>
{
public:
    typedef OsdSpMVKernelDispatcher<CpuCooMatrix,CpuCsrMatrix,OsdCpuVertexBuffer> super;
    OsdMklKernelDispatcher(int levels, bool logical=false);
    virtual void FinalizeMatrix();
    static void Register();
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OSD_MKL_DISPATCHER_H */
