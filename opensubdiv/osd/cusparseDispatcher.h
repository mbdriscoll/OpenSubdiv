#ifndef OSD_CUSPARSE_DISPATCHER_H
#define OSD_CUSPARSE_DISPATCHER_H

#include "../version.h"
#include "../osd/mklDispatcher.h"
#include "../osd/cudaDispatcher.h"

#include <cusparse_v2.h>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

class OsdCusparseKernelDispatcher : public OsdMklKernelDispatcher
{
public:
    OsdCusparseKernelDispatcher(int levels);
    virtual ~OsdCusparseKernelDispatcher();

    static void Register();

    virtual void ApplyMatrix(int offset);
    virtual void FinalizeMatrix();

    int *d_rows, *d_cols;
    float *d_in, *d_out, *d_vals;

    cusparseMatDescr_t desc;
    cusparseHandle_t handle;
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OSD_CUSPARSE_DISPATCHER_H */
