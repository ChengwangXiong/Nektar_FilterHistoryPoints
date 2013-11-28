///////////////////////////////////////////////////////////////////////////////
//
// File: GeomFactors2D.h
//
// For more information, please see: http://www.nektar.info
//
// The MIT License
//
// Copyright (c) 2006 Division of Applied Mathematics, Brown University (USA),
// Department of Aeronautics, Imperial College London (UK), and Scientific
// Computing and Imaging Institute, University of Utah (USA).
//
// License for the specific language governing rights and limitations under
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// Description: 2D geometric factors.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_SPATIALDOMAINS_GEOMFACTORS2D_H
#define NEKTAR_SPATIALDOMAINS_GEOMFACTORS2D_H

#include <SpatialDomains/GeomFactors.h>
#include <SpatialDomains/SpatialDomainsDeclspec.h>
#include <StdRegions/StdExpansion2D.h>

namespace Nektar
{
    namespace SpatialDomains
    {
        /// Geometric factors relating to the coordinate mapping
        /// \f$\chi:\Omega_{st}\rightarrow \Omega_e \f$ for two-dimensional
        /// elements
        class GeomFactors2D : public GeomFactors
        {
        public:
            /// Constructor.
            SPATIAL_DOMAINS_EXPORT GeomFactors2D(
                    const GeomType gtype,
                    const int coordim,
                    const Array<OneD, const StdRegions
                        ::StdExpansion2DSharedPtr> &Coords,
                    const Array<OneD, const LibUtilities::BasisSharedPtr>
                         &tbasis);
            
            /// Copy constructor.
            SPATIAL_DOMAINS_EXPORT GeomFactors2D(const GeomFactors2D& S);

            /// Destructor.
            SPATIAL_DOMAINS_EXPORT virtual ~GeomFactors2D();

        private:
            /// Tests if the element is valid and not self-intersecting.
            void CheckIfValid();
            
            /// Performs 2D interpolation between two sets of point
            /// distributions.
            virtual void v_Interp(
                        const PointsKeyArray &map_points,
                        const Array<OneD, const NekDouble> &src,
                        const PointsKeyArray &tpoints,
                        Array<OneD, NekDouble> &tgt) const;

            virtual void v_Adjoint(
                        const Array<TwoD, const NekDouble>& src,
                        Array<TwoD, NekDouble>& tgt) const;

        };

        /// Shared pointer to GeomFactors2D object.
        typedef boost::shared_ptr<GeomFactors2D>      GeomFactors2DSharedPtr;
        /// Vector of shared pointers to GeomFactors2D objects.
        typedef std::vector< GeomFactors2DSharedPtr > GeomFactors2DVector;
        /// Iterator for the vector of shared pointers.
        typedef GeomFactors2DVector::iterator GeomFactors2DVectorIter;
    }
}

#endif
