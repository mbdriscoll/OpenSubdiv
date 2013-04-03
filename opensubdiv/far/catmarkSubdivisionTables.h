//
//     Copyright (C) Pixar. All rights reserved.
//
//     This license governs use of the accompanying software. If you
//     use the software, you accept this license. If you do not accept
//     the license, do not use the software.
//
//     1. Definitions
//     The terms "reproduce," "reproduction," "derivative works," and
//     "distribution" have the same meaning here as under U.S.
//     copyright law.  A "contribution" is the original software, or
//     any additions or changes to the software.
//     A "contributor" is any person or entity that distributes its
//     contribution under this license.
//     "Licensed patents" are a contributor's patent claims that read
//     directly on its contribution.
//
//     2. Grant of Rights
//     (A) Copyright Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free copyright license to reproduce its contribution,
//     prepare derivative works of its contribution, and distribute
//     its contribution or any derivative works that you create.
//     (B) Patent Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free license under its licensed patents to make, have
//     made, use, sell, offer for sale, import, and/or otherwise
//     dispose of its contribution in the software or derivative works
//     of the contribution in the software.
//
//     3. Conditions and Limitations
//     (A) No Trademark License- This license does not grant you
//     rights to use any contributor's name, logo, or trademarks.
//     (B) If you bring a patent claim against any contributor over
//     patents that you claim are infringed by the software, your
//     patent license from such contributor to the software ends
//     automatically.
//     (C) If you distribute any portion of the software, you must
//     retain all copyright, patent, trademark, and attribution
//     notices that are present in the software.
//     (D) If you distribute any portion of the software in source
//     code form, you may do so only under this license by including a
//     complete copy of this license with your distribution. If you
//     distribute any portion of the software in compiled or object
//     code form, you may only do so under a license that complies
//     with this license.
//     (E) The software is licensed "as-is." You bear the risk of
//     using it. The contributors give no express warranties,
//     guarantees or conditions. You may have additional consumer
//     rights under your local laws which this license cannot change.
//     To the extent permitted under your local laws, the contributors
//     exclude the implied warranties of merchantability, fitness for
//     a particular purpose and non-infringement.
//
#ifndef FAR_CATMARK_SUBDIVISION_TABLES_H
#define FAR_CATMARK_SUBDIVISION_TABLES_H

#include <cassert>
#include <cmath>
#include <cfloat>
#include <vector>

#include "../version.h"

#include "../far/subdivisionTables.h"


#define EIGEN(n) (this->eigen[(n)-3])

// returns integer log2(1.0/x)
inline int invilog2_roundup(double x)
{
	if (x==0.0) return 33;
	int xi = (int)(1.0/x);
	int lg2 = 0;
	while (xi > 0) {
		lg2++;
		xi >>= 1;
	}
	return lg2;
}

// pow(0,0) returns error, don't!
inline double mypow(double x, double y)
{
	if ((x < 0.0) or ((x == 0.0) and (y == 0.0))) return 1.0;
	return pow(x, y);
}

using namespace std;
namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

/// \brief Catmark subdivision scheme tables.
///
/// Catmull-Clark tables store the indexing tables required in order to compute
/// the refined positions of a mesh without the help of a hierarchical data
/// structure. The advantage of this representation is its ability to be executed
/// in a massively parallel environment without data dependencies.
///
template <class U> class FarCatmarkSubdivisionTables : public FarSubdivisionTables<U> {

public:

    /// Memory required to store the indexing tables
    virtual int GetMemoryUsed() const;

    /// Compute the positions of refined vertices using the specified kernels
    virtual void Apply( int level, void * data=0 ) const;
    virtual void PushLimitMatrix(int nverts, int offset);
    static double EvalSpline(const double eb[16], const double u, const double v);
    virtual std::vector<HbrVertex<U>*> Orient(HbrHalfedge<U> *edge, double& u, double& v);

    /// Face-vertices indexing table accessor
    FarTable<unsigned int> const & Get_F_IT( ) const { return _F_IT; }

    /// Face-vertices indexing table accessor
    FarTable<int> const & Get_F_ITa( ) const { return _F_ITa; }

    /// Returns the number of indexing tables needed to represent this particular
    /// subdivision scheme.
    virtual int GetNumTables() const { return 7; }

private:
    template <class X, class Y> friend struct FarCatmarkSubdivisionTablesFactory;
    friend class FarDispatcher<U>;

    // Private constructor called by factory
    FarCatmarkSubdivisionTables( FarMesh<U> * mesh, int maxlevel );

    // Compute-kernel applied to vertices resulting from the refinement of a face.
    void computeFacePoints(int offset, int level, int start, int end, void * clientdata) const;

    // Compute-kernel applied to vertices resulting from the refinement of an edge.
    void computeEdgePoints(int offset, int level, int start, int end, void * clientdata) const;

    // Compute-kernel applied to vertices resulting from the refinement of a vertex
    // Kernel "A" Handles the k_Smooth and k_Dart rules
    void computeVertexPointsA(int offset, bool pass, int level, int start, int end, void * clientdata) const;

    // Compute-kernel applied to vertices resulting from the refinement of a vertex
    // Kernel "B" Handles the k_Crease and k_Corner rules
    void computeVertexPointsB(int offset, int level, int start, int end, void * clientdata) const;

private:

    FarTable<int>           _F_ITa;
    FarTable<unsigned int>  _F_IT;

};

template <class U>
FarCatmarkSubdivisionTables<U>::FarCatmarkSubdivisionTables( FarMesh<U> * mesh, int maxlevel ) :
    FarSubdivisionTables<U>(mesh, maxlevel),
    _F_ITa(maxlevel+1),
    _F_IT(maxlevel+1)
{ }

template <class U> int
FarCatmarkSubdivisionTables<U>::GetMemoryUsed() const {
    return FarSubdivisionTables<U>::GetMemoryUsed()+
        _F_ITa.GetMemoryUsed()+
        _F_IT.GetMemoryUsed();
}

#if 0 // REMOVEME
template <class U> void
FarCatmarkSubdivisionTables<U>::Apply( int level, void * clientdata ) const {

    assert(this->_mesh and level>0);

    typename FarSubdivisionTables<U>::VertexKernelBatch const * batch = & (this->_batches[level-1]);

    FarDispatcher<U> const * dispatch = this->_mesh->GetDispatcher();
    assert(dispatch);

    int offset = this->GetFirstVertexOffset(level);
    if (batch->kernelF>0)
        dispatch->ApplyCatmarkFaceVerticesKernel(this->_mesh, offset, level, 0, batch->kernelF, clientdata);

    offset += this->GetNumFaceVertices(level);
    if (batch->kernelE>0)
        dispatch->ApplyCatmarkEdgeVerticesKernel(this->_mesh, offset, level, 0, batch->kernelE, clientdata);

    offset += this->GetNumEdgeVertices(level);
    if (batch->kernelB.first < batch->kernelB.second)
        dispatch->ApplyCatmarkVertexVerticesKernelB(this->_mesh, offset, level, batch->kernelB.first, batch->kernelB.second, clientdata);
    if (batch->kernelA1.first < batch->kernelA1.second)
        dispatch->ApplyCatmarkVertexVerticesKernelA(this->_mesh, offset, false, level, batch->kernelA1.first, batch->kernelA1.second, clientdata);
    if (batch->kernelA2.first < batch->kernelA2.second)
        dispatch->ApplyCatmarkVertexVerticesKernelA(this->_mesh, offset, true, level, batch->kernelA2.first, batch->kernelA2.second, clientdata);
}
#endif

template <class U> void
FarCatmarkSubdivisionTables<U>::Apply( int level, void * clientdata ) const {

    assert(this->_mesh and level>0);

    typename FarSubdivisionTables<U>::VertexKernelBatch const * batch = & (this->_batches[level-1]);

    FarDispatcher<U> * dispatch = this->_mesh->GetDispatcher();
    assert(dispatch);

    int prevLevel = std::max(level-1,0);
    int prevOffset = this->GetFirstVertexOffset( prevLevel );
    int offset =     this->GetFirstVertexOffset( level );
    int jop = this->GetNumVertices(prevLevel);
    int iop = this->GetNumVertices(prevLevel) + batch->kernelF;

    dispatch->SetSrcOffset(prevOffset);
    dispatch->SetDstOffset(prevOffset);

    dispatch->StageMatrix(iop, jop);
    {
        dispatch->CopyNVerts(jop, prevOffset);

        if (batch->kernelF>0)
            dispatch->ApplyCatmarkFaceVerticesKernel(this->_mesh, offset, level, 0, batch->kernelF, clientdata);
    }
    dispatch->PushMatrix();

    jop = this->GetNumVertices(prevLevel) + batch->kernelF;
    iop = this->GetNumVertices(level);

    dispatch->SetSrcOffset(prevOffset);
    dispatch->SetDstOffset(offset);

    dispatch->StageMatrix(iop,jop);
    {
        dispatch->CopyNVerts(batch->kernelF, offset);

        offset += this->GetNumFaceVertices(level);
        if (batch->kernelE>0)
            dispatch->ApplyCatmarkEdgeVerticesKernel(this->_mesh, offset, level, 0, batch->kernelE, clientdata);

        offset += this->GetNumEdgeVertices(level);
        if (batch->kernelB.first < batch->kernelB.second)
            dispatch->ApplyCatmarkVertexVerticesKernelB
                (this->_mesh, offset, level, batch->kernelB.first, batch->kernelB.second, clientdata);
        if (batch->kernelA1.first < batch->kernelA1.second)
            dispatch->ApplyCatmarkVertexVerticesKernelA
                (this->_mesh, offset, false, level, batch->kernelA1.first, batch->kernelA1.second, clientdata);
        if (batch->kernelA2.first < batch->kernelA2.second)
            dispatch->ApplyCatmarkVertexVerticesKernelA
                (this->_mesh, offset, true, level, batch->kernelA2.first, batch->kernelA2.second, clientdata);
    }
    dispatch->PushMatrix();
}

//
// Face-vertices compute Kernel - completely re-entrant
//

template <class U> void
FarCatmarkSubdivisionTables<U>::computeFacePoints( int offset, int level, int start, int end, void * clientdata ) const {

    assert(this->_mesh);

    U * vsrc = &this->_mesh->GetVertices().at(0),
      * vdst = vsrc + offset + start;

    const int * F_ITa = _F_ITa[level-1];
    const unsigned int * F_IT = _F_IT[level-1];

    for (int i=start; i<end; ++i, ++vdst ) {

        vdst->Clear(clientdata);

        int h = F_ITa[2*i  ],
            n = F_ITa[2*i+1];
        float weight = 1.0f/n;

        for (int j=0; j<n; ++j) {
             vdst->AddWithWeight( vsrc[ F_IT[h+j] ], weight, clientdata );
             vdst->AddVaryingWithWeight( vsrc[ F_IT[h+j] ], weight, clientdata );
        }
    }
}

//
// Edge-vertices compute Kernel - completely re-entrant
//

template <class U> void
FarCatmarkSubdivisionTables<U>::computeEdgePoints( int offset,  int level, int start, int end, void * clientdata ) const {

    assert(this->_mesh);

    U * vsrc = &this->_mesh->GetVertices().at(0),
      * vdst = vsrc + offset + start;

    const int * E_IT = this->_E_IT[level-1];
    const float * E_W = this->_E_W[level-1];

    for (int i=start; i<end; ++i, ++vdst ) {

        vdst->Clear(clientdata);

        int eidx0 = E_IT[4*i+0],
            eidx1 = E_IT[4*i+1],
            eidx2 = E_IT[4*i+2],
            eidx3 = E_IT[4*i+3];

        float vertWeight = E_W[i*2+0];

        // Fully sharp edge : vertWeight = 0.5f
        vdst->AddWithWeight( vsrc[eidx0], vertWeight, clientdata );
        vdst->AddWithWeight( vsrc[eidx1], vertWeight, clientdata );

        if (eidx2!=-1) {
            // Apply fractional sharpness
            float faceWeight = E_W[i*2+1];

            vdst->AddWithWeight( vsrc[eidx2], faceWeight, clientdata );
            vdst->AddWithWeight( vsrc[eidx3], faceWeight, clientdata );
        }

        vdst->AddVaryingWithWeight( vsrc[eidx0], 0.5f, clientdata );
        vdst->AddVaryingWithWeight( vsrc[eidx1], 0.5f, clientdata );
    }
}

//
// Vertex-vertices compute Kernels "A" and "B" - completely re-entrant
//

// multi-pass kernel handling k_Crease and k_Corner rules
template <class U> void
FarCatmarkSubdivisionTables<U>::computeVertexPointsA( int offset, bool pass, int level, int start, int end, void * clientdata ) const {

    assert(this->_mesh);

    U * vsrc = &this->_mesh->GetVertices().at(0),
      * vdst = vsrc + offset + start;

    const int * V_ITa = this->_V_ITa[level-1];
    const float * V_W = this->_V_W[level-1];

    for (int i=start; i<end; ++i, ++vdst ) {

        if (not pass)
            vdst->Clear(clientdata);

        int     n=V_ITa[5*i+1],   // number of vertices in the _VO_IT array (valence)
                p=V_ITa[5*i+2],   // index of the parent vertex
            eidx0=V_ITa[5*i+3],   // index of the first crease rule edge
            eidx1=V_ITa[5*i+4];   // index of the second crease rule edge

        float weight = pass ? V_W[i] : 1.0f - V_W[i];

        // In the case of fractional weight, the weight must be inverted since
        // the value is shared with the k_Smooth kernel (statistically the
        // k_Smooth kernel runs much more often than this one)
        if (weight>0.0f and weight<1.0f and n>0)
            weight=1.0f-weight;

        // In the case of a k_Corner / k_Crease combination, the edge indices
        // won't be null,  so we use a -1 valence to detect that particular case
        if (eidx0==-1 or (pass==false and (n==-1)) ) {
            // k_Corner case
            vdst->AddWithWeight( vsrc[p], weight, clientdata );
        } else {
            // k_Crease case
            vdst->AddWithWeight( vsrc[p], weight * 0.75f, clientdata );
            vdst->AddWithWeight( vsrc[eidx0], weight * 0.125f, clientdata );
            vdst->AddWithWeight( vsrc[eidx1], weight * 0.125f, clientdata );
        }
        vdst->AddVaryingWithWeight( vsrc[p], 1.0f, clientdata );
    }
}

// multi-pass kernel handling k_Dart and k_Smooth rules
template <class U> void
FarCatmarkSubdivisionTables<U>::computeVertexPointsB( int offset, int level, int start, int end, void * clientdata ) const {

    assert(this->_mesh);

    U * vsrc = &this->_mesh->GetVertices().at(0),
      * vdst = vsrc + offset + start;

    const int * V_ITa = this->_V_ITa[level-1];
    const unsigned int * V_IT = this->_V_IT[level-1];
    const float * V_W = this->_V_W[level-1];

    for (int i=start; i<end; ++i, ++vdst ) {

        vdst->Clear(clientdata);

        int h = V_ITa[5*i  ],     // offset of the vertices in the _V0_IT array
            n = V_ITa[5*i+1],     // number of vertices in the _VO_IT array (valence)
            p = V_ITa[5*i+2];     // index of the parent vertex

        float weight = V_W[i],
                  wp = 1.0f/(n*n),
                  wv = (n-2.0f)*n*wp;

        vdst->AddWithWeight( vsrc[p], weight * wv, clientdata );

        for (int j=0; j<n; ++j) {
            vdst->AddWithWeight( vsrc[V_IT[h+j*2  ]], weight * wp, clientdata );
            vdst->AddWithWeight( vsrc[V_IT[h+j*2+1]], weight * wp, clientdata );
        }
        vdst->AddVaryingWithWeight( vsrc[p], 1.0f, clientdata );
    }
}

template <class U> std::vector<HbrVertex<U>*>
FarCatmarkSubdivisionTables<U>::Orient(HbrHalfedge<U> *edge, double& u, double& v)  {
    /* find extraordinary vertex */
    HbrHalfedge<U> *e0 = NULL,
                   *eA = edge,
                   *eB = edge->GetNext(),
                   *eC = edge->GetNext()->GetNext(),
                   *eD = edge->GetNext()->GetNext()->GetNext();

    /* find e0 pointing to extraordinary vertex, if one exists */
    // XXX shouldn't we set e0 based on orientation in patch?
    if      (eD->GetOrgVertex()->GetValence() != 4) { e0 = eA; u = 0.0; v = 1.0; }
    else if (eC->GetOrgVertex()->GetValence() != 4) { e0 = eA; u = 1.0; v = 1.0; }
    else if (eB->GetOrgVertex()->GetValence() != 4) { e0 = eA; u = 1.0; v = 0.0; }
    else    /* eA has irreg vert or patch is reg */ { e0 = eA; u = 0.0; v = 0.0; }
    assert(e0 != NULL);

    int N = e0->GetOrgVertex()->GetValence();
    vector<HbrVertex<U>*> PatchCV(2*N+8, NULL);

    PatchCV[0] = e0->GetOrgVertex();
    PatchCV[1] = e0->GetOpposite()->GetNext()->GetDestVertex();
    PatchCV[2] = e0->GetOpposite()->GetPrev()->GetOrgVertex();
    PatchCV[3] = e0->GetDestVertex();
    PatchCV[4] = e0->GetNext()->GetDestVertex();

    int i = 5;
    for (HbrHalfedge<U> *h = e0->GetPrev()->GetOpposite();
            h->GetDestVertex() != PatchCV[1];
            h = h->GetPrev()->GetOpposite()) {
        PatchCV[i++] = h->GetNext()->GetOrgVertex();
        PatchCV[i++] = h->GetNext()->GetDestVertex();
    }
    assert(i == 2*N+1);

    PatchCV[2*N+1] = e0->GetNext()->GetOpposite()->GetPrev()->GetOpposite()->GetNext()->GetDestVertex();
    PatchCV[2*N+2] = e0->GetNext()->GetNext()->GetOpposite()->GetNext()->GetDestVertex();
    PatchCV[2*N+3] = e0->GetNext()->GetNext()->GetOpposite()->GetPrev()->GetOrgVertex();
    PatchCV[2*N+4] = e0->GetPrev()->GetOpposite()->GetNext()->GetOpposite()->GetPrev()->GetOrgVertex();
    PatchCV[2*N+5] = e0->GetNext()->GetOpposite()->GetPrev()->GetOrgVertex();
    PatchCV[2*N+6] = e0->GetNext()->GetOpposite()->GetNext()->GetDestVertex();
    PatchCV[2*N+7] = e0->GetOpposite()->GetPrev()->GetOpposite()->GetNext()->GetDestVertex();

    return PatchCV;
}

template <class U> double
FarCatmarkSubdivisionTables<U>::EvalSpline(const double eb[16], const double u, const double v) {
    return ((((((eb[ 0]*v + eb[ 1])*v + eb[ 2])*v + eb[ 3]) *u +
              (((eb[ 4]*v + eb[ 5])*v + eb[ 6])*v + eb[ 7]))*u +
              (((eb[ 8]*v + eb[ 9])*v + eb[10])*v + eb[11]))*u +
              (((eb[12]*v + eb[13])*v + eb[14])*v + eb[15]));
}

template <class U> void
FarCatmarkSubdivisionTables<U>::PushLimitMatrix( int nverts, int offset ) {

    assert(this->_mesh);
    FarDispatcher<U> * dispatch = this->_mesh->GetDispatcher();

    char* filename = (char*)
#if defined(_WIN32) || defined(__APPLE__)
      "../data/ccdata50NT.dat";
#else
      "../data/ccdata50.dat";
#endif

    this->read_eval(filename);
    assert(this->eigen != NULL);

    dispatch->StageMatrix(nverts, nverts);
    {
        for(int vi = 0; vi < nverts; vi++) {
            /* Get Hbr handles */
            HbrVertex<U> *vertex = this->_mesh->GetHbrVertex(offset + vi);
            HbrHalfedge<U> *edge = vertex->GetIncidentEdge();
            HbrFace<U>     *face = edge->GetFace();

            /* Ground truth */
            assert(face->GetNumVertices() == 4);

            /* determine which vertices to combine (specified by global far indices)
             * and the vertex's u-v parameterization within */
            double u = 0, v = 0;
            vector<HbrVertex<U>*> PatchVertices = this->Orient(edge, u, v);
            double uf = u, vf = v;

            // determine in which domain omega_nk the parameter lies
            const int n = invilog2_roundup(std::max(u, v));
            const double pow2 = (double)(1 << (n-1));
            u *= pow2, v *= pow2;
            int k;
            if (v < 0.5)
                k = 0,  u = 2.0*u - 1.0,  v *= 2.0;
            else if (u < 0.5)
                k = 2,  u *= 2.0,  v = 2.0*v - 1.0;
            else
                k = 1,  u = 2.0*u - 1.0,  v = 2.0*v - 1.0;

            const int K = PatchVertices.size(), N = (K-8) / 2;
            assert(K == 2*N+8);

            const double *vecI = EIGEN(N).iV;
            const double *eb = EIGEN(N).x[k];
            const double *L = EIGEN(N).L;

            vector<double> Eval(K);
            for (int i=0; i<K; ++i, eb += 16)
                Eval[i] = mypow(L[i], n-1) * EvalSpline(eb, u, v);

            /* Compute Eval * eigen[N].iV matvec (aka the final weights) */
            vector<double> Weights(K, 0.0);
            for (int i = 0; i < K; i++)
                for (int j = 0; j < K; j++)
                    Weights[i] += vecI[i+K*j] * Eval[i];

            /* Compute the mapping from patch index to relative FAR index */
            vector<int> IndexMap(K, 0);
            for (int i = 0; i < K; i++)
                IndexMap[i] = this->_mesh->GetFarVertexID( PatchVertices[i] ) - offset;

            /* insert weights into staged matrix */
            for (int i = 0; i < K; i++)
                dispatch->StageElem(vi, IndexMap[i], Weights[i]);
        }
    }
    dispatch->PushMatrix();
}

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* FAR_CATMARK_SUBDIVISION_TABLES_H */
