#ifndef OSD_SPMV_KERNEL_H
#define OSD_SPMV_KERNEL_H

#include "../version.h"
#include "osd/spmvDispatcher.h"
#include "osd/cpuKernel.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

class SpMVVertexDescriptor : public VertexDescriptor {
public:
    SpMVVertexDescriptor(OsdKernelDispatcher* dispatcher, int numVertexElem, int numVaryingElem)
        : VertexDescriptor(numVertexElem, numVaryingElem), _dispatcher(dispatcher) { }

    virtual void Clear(float *vertex, float *varying, int index) const { }

    virtual void AddWithWeight(float *vertex, int dstIndex, int srcIndex, float weight) const {
        int srcOffset = _dispatcher->srcOffset;
        int d = dstIndex * numVertexElements;
        int s = (srcIndex-srcOffset) * numVertexElements;
        for (int i = 0; i < numVertexElements; ++i)
            _dispatcher->StageElem(d+i,s+i,weight);
    }

    virtual void AddVaryingWithWeight(float *varying, int dstIndex, int srcIndex, float weight) const {
        assert(numVaryingElements == 0);
    }

    virtual void ApplyVertexEditAdd(float *vertex, int primVarOffset, int primVarWidth, int editIndex, const float *editValues) const {
        printf("Warning: spmv kernels don't support vertex add yet.\n");
        return;
        int d = editIndex * numVertexElements + primVarOffset;
        for (int i = 0; i < primVarWidth; ++i) {
            vertex[d++] += editValues[i];
        }
    }

    virtual void ApplyVertexEditSet(float *vertex, int primVarOffset, int primVarWidth, int editIndex, const float *editValues) const {
        printf("Warning: spmv kernels don't support vertex set.\n");
        return;
        int d = editIndex * numVertexElements + primVarOffset;
        for (int i = 0; i < primVarWidth; ++i) {
            vertex[d++] = editValues[i];
        }
    }

    OsdKernelDispatcher* _dispatcher;
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OSD_SPMV_KERNEL_H */