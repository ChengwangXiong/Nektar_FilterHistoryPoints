////////////////////////////////////////////////////////////////////////////////
//
//  File:  GeomFactors.h
//
//  For more information, please see: http://www.nektar.info/
//
//  The MIT License
//
//  Copyright (c) 2006 Division of Applied Mathematics, Brown University (USA),
//  Department of Aeronautics, Imperial College London (UK), and Scientific
//  Computing and Imaging Institute, University of Utah (USA).
//
//  License for the specific language governing rights and limitations under
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//
//  Description: Geometric Factors base class
//
////////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_SPATIALDOMAINS_GEOMFACTORS_H
#define NEKTAR_SPATIALDOMAINS_GEOMFACTORS_H

#include <boost/unordered_set.hpp>

#include <LibUtilities/Foundations/Basis.h>
#include <SpatialDomains/SpatialDomains.hpp>
#include <SpatialDomains/SpatialDomainsDeclspec.h>
#include <StdRegions/StdExpansion.h>
#include <StdRegions/StdRegions.hpp>

namespace Nektar
{
    namespace LibUtilities
    {
        class PointsKey;
    }

    namespace SpatialDomains
    {
        // Forward declarations and useful typedefs
        class GeomFactors;
        class Geometry;

        typedef boost::shared_ptr<Geometry> GeometrySharedPtr;

        SPATIAL_DOMAINS_EXPORT bool operator==(const GeomFactors &lhs,
                                               const GeomFactors &rhs);

        /// Pointer to a GeomFactors object.
        typedef boost::shared_ptr<GeomFactors>      GeomFactorsSharedPtr;
        /// A vector of GeomFactor pointers.
        typedef std::vector< GeomFactorsSharedPtr > GeomFactorsVector;
        /// Iterator for the GeomFactorsVector.
        typedef GeomFactorsVector::iterator         GeomFactorsVectorIter;
        /// An unordered set of GeomFactor pointers.
        typedef boost::unordered_set< GeomFactorsSharedPtr >
                                                    GeomFactorsSet;
        /// Iterator for the GeomFactorsSet
        typedef boost::unordered_set< GeomFactorsSharedPtr >::iterator
                                                    GeomFactorsSetIter;
        /// Storage type for derivative of mapping.
        typedef Array<OneD, Array<OneD, Array<OneD,NekDouble> > >
                                                    DerivStorage;
        typedef Array<OneD, LibUtilities::PointsKey>
                                                    PointsKeyArray;

        /// Calculation and storage of geometric factors associated with the
        /// mapping from StdRegions reference elements to a given LocalRegions
        /// physical element in the mesh.
        class GeomFactors
        {
        public:
            /// Tests if two GeomFactors classes are equal.
            SPATIAL_DOMAINS_EXPORT friend bool operator==( 
                const GeomFactors &lhs,
                const GeomFactors &rhs);

            /// Destructor.
            SPATIAL_DOMAINS_EXPORT virtual ~GeomFactors();

            /// Returns whether the geometry is regular or deformed.
            inline GeomType GetGtype();

            /// Return the Jacobian of the mapping.
            const Array<OneD, const NekDouble> GetJac() const
            {
                return GetJac(m_pointsKey);
            }
            const Array<OneD, const NekDouble> GetJac(
                    const PointsKeyArray &keyTgt) const;

            /// Return the Laplacian coefficients \f$g_{ij}\f$.
            const Array<TwoD, const NekDouble> GetGmat() const
            {
                return GetGmat(m_pointsKey);
            }
            const Array<TwoD, const NekDouble> GetGmat(
                    const PointsKeyArray &keyTgt) const;

            /// Return the derivative of the mapping with respect to the
            /// reference coordinates,
            /// \f$\frac{\partial \chi_i}{\partial \xi_j}\f$.
            DerivStorage GetDeriv() const
            {
                return GetDeriv(m_pointsKey);
            }
            DerivStorage GetDeriv(const PointsKeyArray &tpoints) const;

            /// Return the derivative of the reference coordinates with respect
            /// to the mapping, \f$\frac{\partial \xi_i}{\partial \chi_j}\f$.
            const Array<TwoD, const NekDouble> GetDerivFactors() const
            {
                return GetDerivFactors(m_pointsKey);
            }
            const Array<TwoD, const NekDouble> GetDerivFactors(
                    const PointsKeyArray &keyTgt) const;

            /// Determine if element is valid and not self-intersecting.
            inline bool IsValid() const;

            /// Return the number of dimensions of the coordinate system.
            inline int GetCoordim() const;

            /// Computes the edge tangents with respect to a 2D element
            inline void ComputeEdgeTangents(
                const GeometrySharedPtr       &geom2D,
                const int                      edge,
                const LibUtilities::PointsKey &to_key);
            
            /// Returns the tangent vectors evaluated at each quadrature point
            /// for 1D elements.  The tangent vectors are set using the
            /// function ComputeEdgeTangents.
            inline const Array<OneD, const Array<OneD, NekDouble> >
                &GetEdgeTangent() const;

            /// Returns the LibUtilities::PointsKey object associated with a
            /// co-ordinate direction.
            inline const LibUtilities::PointsKey
                &GetPointsKey(unsigned int i) const;

            /// Computes a hash of this GeomFactors element, based on the
            /// type, expansion/co-ordinate dimensions, metric, Jacobian and
            /// the geometric factors themselves.
            inline size_t GetHash() const;

        protected:
            /// Type of geometry (e.g. eRegular, eDeformed, eMovingRegular).
            GeomType m_type;
            /// Dimension of expansion.
            int m_expDim;
            /// Dimension of coordinate system.
            int m_coordDim;
            /// Validity of element (Jacobian positive)
            bool m_valid;
            /// Stores information about the expansion.
            Array<OneD, StdRegions::StdExpansionSharedPtr> m_coords;

            /// Array of size coordim which stores a key describing the
            /// location of the quadrature points in each dimension.
            Array<OneD,LibUtilities::PointsKey> m_pointsKey;

            /// Array of size (coordim)x(nquad) which holds the components of
            /// the normal vector at each quadrature point. The array is
            /// populated as \a m_coordDim consecutive blocks of size equal to
            /// the number of quadrature points. Each block holds a component
            /// of the normal vectors.
            Array<OneD, Array<OneD,NekDouble> > m_normal;

            /// Array of size (coordim)x(nquad) which holds the components of
            /// the tangent vector at each quadrature point. The array is
            /// populated as \a m_coordDim consecutive blocks of size equal to
            /// the number of quadrature points. Each block holds a component
            /// of the tangent vectors.
            Array<OneD, Array<OneD,NekDouble> > m_tangent;
            
            /// Constructor for GeomFactors class.
            GeomFactors(const GeomType gtype,
                        const int expdim,
                        const int coordim);

            /// Copy constructor.
            GeomFactors(const GeomFactors &S);

            virtual void v_Interp(
                        const PointsKeyArray &map_points,
                        const Array<OneD, const NekDouble> &src,
                        const PointsKeyArray &tpoints,
                        Array<OneD, NekDouble> &tgt) const = 0;

            virtual void v_Adjoint(
                        const Array<TwoD, const NekDouble>& src,
                        Array<TwoD, NekDouble>& tgt) const = 0;

        private:
            /// (2D only) Compute tangents based on a 1D element.
            virtual void v_ComputeEdgeTangents(
            	    	const GeometrySharedPtr &geom2D,
            	    	const int edge,
            	    	const LibUtilities::PointsKey &to_key);
        };


        /// A hash functor for geometric factors. Utilises
        /// GeomFactors::GetHash.
        struct GeomFactorsHash : std::unary_function<GeomFactorsSharedPtr,
                                                     std::size_t>
        {
            std::size_t operator()(GeomFactorsSharedPtr const& p) const
            {
                return p->GetHash();
            }
        };

        /**
         * @returns             The type of geometry.
         * @see GeomType
         */
        inline GeomType GeomFactors::GetGtype()
        {
            return m_type;
        }


        /// Return the number of dimensions of the coordinate system.
        inline int GeomFactors::GetCoordim() const
        {
            return m_coordDim;
        }

        /// Return true if the element is valid (Jacobian is positive)
        inline bool GeomFactors::IsValid() const
        {
            return m_valid;
        }

        /// Computes the edge tangents from a 1D element
        inline void GeomFactors::ComputeEdgeTangents(
            const GeometrySharedPtr &geom2D,
            const int edge,
            const LibUtilities::PointsKey &to_key)
        {
            v_ComputeEdgeTangents(geom2D, edge, to_key);
        }

        /// Returns the tangent vectors evaluated at each quadrature point for
        /// 1D elements.  The tangent vectors are set using the function
        /// ComputeEdgeTangents.
        inline const Array<OneD, const Array<OneD, NekDouble> >
            &GeomFactors::GetEdgeTangent() const
        {
            ASSERTL0(m_tangent.num_elements()>0,
                     "Tangent vectors are not computed for this line");
            return m_tangent;
        }
	
        /// Returns the LibUtilities::PointsKey object associated with a
        /// co-ordinate direction.
        inline const LibUtilities::PointsKey
            &GeomFactors::GetPointsKey(unsigned int i) const
        {
            ASSERTL1(i < m_pointsKey.num_elements(), "PointsKey out of range.");
            return m_pointsKey[i];
        }

        /// Computes a hash of this GeomFactors element, based on the
        /// type, expansion/co-ordinate dimensions, metric, Jacobian and
        /// the geometric factors themselves.
        inline size_t GeomFactors::GetHash() const
        {
            size_t hash = 0;
            boost::hash_combine(hash, (int)m_type);
            boost::hash_combine(hash, m_expDim);
            boost::hash_combine(hash, m_coordDim);
            return hash;
        }

    } //end of namespace
} //end of namespace

#endif
