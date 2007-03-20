///////////////////////////////////////////////////////////////////////////////
//
// File StdMatrixKey.cpp
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
// Description: Definition of StdMatrixKey
//
///////////////////////////////////////////////////////////////////////////////

#include "StdRegions/StdExpansion.h"
#include "StdRegions/StdMatrixKey.h"

namespace Nektar
{
    namespace StdRegions
    {
	
	// Register Mass Matrix creator. 
	StdMatrixKey::StdMatrixKey(const MatrixType matrixType, 
				   const ShapeType shapeType,
				   const StdExpansion &stdExpansion):
	    m_matrixType(matrixType),
	    m_shapeType(shapeType)
	{
	    m_ncoeffs   = stdExpansion.GetNcoeffs();
	    m_base      = stdExpansion.GetBase();
	}
	    

        bool StdMatrixKey::opLess::operator()(const StdMatrixKey &lhs, 
					      const StdMatrixKey &rhs)
        {	    
	    {
		return (lhs.m_matrixType < rhs.m_matrixType);
	    }
	}

	bool operator<(const StdMatrixKey &lhs, const StdMatrixKey &rhs)
	{
	    int i;
	    
	    if(lhs.m_matrixType < rhs.m_matrixType)
	    {
		return true;
	    }

	    if(lhs.m_matrixType > rhs.m_matrixType)
	    {
		return false;
	    }
	    
	    if(lhs.m_ncoeffs < rhs.m_ncoeffs)
	    {
		return true;
	    }
	    
	    if(lhs.m_ncoeffs > rhs.m_ncoeffs)
	    {
		return false;
	    }
	    
	    for(i = 0; i < ShapeTypeDimMap[lhs.m_shapeType]; ++i)
	    {
		if(lhs.m_base[i] < rhs.m_base[i])
		{
		    return true;
		}
	    }
	    
	    return false;
	}

        std::ostream& operator<<(std::ostream& os, const StdMatrixKey& rhs)
        {

	    int i;

            os << "MatrixType: " << MatrixTypeMap[rhs.GetMatrixType()] << ", ShapeType: " 
	       << ShapeTypeMap[rhs.GetShapeType()] << ", Ncoeffs: " << rhs.GetNcoeffs() 
	       << std::endl;
	    
	    for(i = 0; i < ShapeTypeDimMap[rhs.GetShapeType()]; ++i)
	    {
		os << rhs.GetBase()[i]->GetBasisKey();
	    }

            return os;
        }
    }
}

/**
* $Log: StdMatrixKey.cpp,v $
* Revision 1.2  2007/03/05 08:07:13  sherwin
* Modified so that StdMatrixKey has const calling arguments in its constructor.
*
* Revision 1.1  2007/02/28 19:05:11  sherwin
* Moved key definitions to their own files to make things more transparent
*
* Revision 1.6  2007/02/28 09:53:17  sherwin
* Update including adding GetBasis call to StdExpansion
*
* Revision 1.5  2007/02/24 09:07:25  sherwin
* Working version of stdMatrixManager and stdLinSysMatrix
*
* Revision 1.4  2007/02/23 19:26:04  jfrazier
* General bug fix and formatting.
*
* Revision 1.3  2007/02/22 22:02:27  sherwin
* Update with executing StdMatManager
*
* Revision 1.2  2007/02/22 18:11:31  sherwin
* Version with some create functions introduced for StdMatManagers
*
* Revision 1.1  2007/02/21 22:55:16  sherwin
* First integration of StdMatrixManagers
*
***/

