///////////////////////////////////////////////////////////////////////////////
//
// File OptimizationParametersAccess.cpp
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
// Description: Header file of optimisation parameters class
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/array.hpp>

#ifndef TIXML_USE_STL
#define TIXML_USE_STL
#endif
#include <tinyxml/tinyxml.h>
#include <StdRegions/OptimizationParametersAccess.h>



namespace Nektar
{
    namespace NekOptimize 
    {        

        void LoadElementalOptimizationParameters(const std::string& fileName)
        {
            LoadOptimizationParametersInterface::LoadElemental2DOptimizationParameters(fileName);
        }

        void LoadOptimizationParametersInterface::LoadElemental2DOptimizationParameters(const std::string& fileName)
        {
            TiXmlDocument doc(fileName);
            bool loadOkay = doc.LoadFile();
            
            ASSERTL0(loadOkay, (std::string("Unable to load file: ") + 
                                fileName).c_str());

            TiXmlHandle docHandle(&doc);
            TiXmlElement* master    = docHandle.FirstChildElement("NEKTAR").Element();
            TiXmlElement* paramList = docHandle.FirstChildElement("NEKTAR").FirstChildElement("ELEMENTALOPTIMIZATIONPARAMETERS").Element();
                
            ASSERTL0(master   , "Unable to find NEKTAR tag in file.");
            ASSERTL0(paramList, "Unable to find ELEMENTALOPTIMIZATIONPARAMETERS tag in file.");

#define OPTIMIZE2DELEMENTS (6, (StdRegions::eStdQuadExp,                \
                                StdRegions::eStdTriExp,                 \
                                StdRegions::eStdNodalTriExp,            \
                                StdRegions::eQuadExp,                   \
                                StdRegions::eTriExp,                    \
                                StdRegions::eNodalTriExp))                  
            
#define OPTIMIZEOPERATIONS (8, (eBwdTrans,                      \
                                eIProductWRTBase,               \
                                eIProductWRTDerivBase,          \
                                eMassMatrixOp,                  \
                                eLaplacianMatrixOp,             \
                                eLaplacianMatrixIJOp,           \
                                eWeakDerivMatrixOp,             \
                                eHelmholtzMatrixOp))
            
#define PARSE_OPERATIONS(z,n,elType)                                    \
            {                                                           \
                const ElementalOptimizationOperationType opType = BOOST_PP_ARRAY_ELEM(n,OPTIMIZEOPERATIONS); \
                TiXmlElement* operationType = elementType->FirstChildElement(ElementalOptimizationOperationTypeMap[n]); \
                if(operationType)                                       \
                {                                                       \
                    TiXmlElement* arrayElement = operationType->FirstChildElement("DO_MAT_OP"); \
                                                                        \
                    ASSERTL0(arrayElement, (std::string("Unable to find DO_MAT_OP tag for ") + \
                                            std::string(StdRegions::ElementTypeMap[elType]) + std::string("_") + \
                                            std::string(ElementalOptimizationOperationTypeMap[opType]) + std::string("."))); \
                    while(arrayElement)                                 \
                    {                                                   \
                        int nummodes0;                                  \
                        int nummodes1;                                  \
                        int value;                                      \
                        int err;                                        \
                        err = arrayElement->QueryIntAttribute("NUMMODES0", &nummodes0); \
                        ASSERTL0(err == TIXML_SUCCESS, (std::string("Unable to read DO_MAT_OP attribute NUMMODES0 for ") + \
                                                        std::string(StdRegions::ElementTypeMap[elType]) + std::string("_") + \
                                                        std::string(ElementalOptimizationOperationTypeMap[opType]) + std::string("."))); \
                        err = arrayElement->QueryIntAttribute("NUMMODES1", &nummodes1); \
                        ASSERTL0(err == TIXML_SUCCESS, (std::string("Unable to read DO_MAT_OP attribute NUMMODES1 for ") + \
                                                        std::string(StdRegions::ElementTypeMap[elType]) + std::string("_") + \
                                                        std::string(ElementalOptimizationOperationTypeMap[opType]) + std::string("."))); \
                        err = arrayElement->QueryIntAttribute("VALUE", &value); \
                        ASSERTL0(err == TIXML_SUCCESS, (std::string("Unable to read DO_MAT_OP attribute VALUE for ") + \
                                                        std::string(StdRegions::ElementTypeMap[elType]) + std::string("_") + \
                                                        std::string(ElementalOptimizationOperationTypeMap[opType]) + std::string("."))); \
                        ElementalOptimization<elType,opType>::SetDoMatOp(nummodes0,nummodes1,(bool) value); \
                        arrayElement = arrayElement->NextSiblingElement(); \
                    }                                                   \
                }                                                       \
            }                                                           

#define PARSE_ELEMENTS(z,n,i)                                           \
            {                                                           \
                const StdRegions::ElementType el = BOOST_PP_ARRAY_ELEM(n,OPTIMIZE2DELEMENTS); \
                TiXmlElement* elementType = paramList->FirstChildElement(StdRegions::ElementTypeMap[el]); \
                if(elementType)                                         \
                {                                                       \
                    BOOST_PP_REPEAT_FROM_TO(0,BOOST_PP_ARRAY_SIZE(OPTIMIZEOPERATIONS),PARSE_OPERATIONS,BOOST_PP_ARRAY_ELEM(n,OPTIMIZE2DELEMENTS)); \
                }                                                       \
            }                 
            
            BOOST_PP_REPEAT_FROM_TO(0,BOOST_PP_ARRAY_SIZE(OPTIMIZE2DELEMENTS),PARSE_ELEMENTS,dummy);
        }

        void DumpElementalOptimizationParameters(std::ostream &outfile)
        {
            DumpOptimizationParametersInterface::DumpElemental2DOptimizationParameters(outfile);
        }

        void DumpOptimizationParametersInterface::DumpElemental2DOptimizationParameters(std::ostream &outfile)
        {
#define DUMP_OPERATIONS(z,n,elType)                                     \
            {                                                           \
                const ElementalOptimizationOperationType opType = BOOST_PP_ARRAY_ELEM(n,OPTIMIZEOPERATIONS); \
                ElementalOptimization<elType,opType>::DumpParameters(outfile); \
            }                                     

#define DUMP_ELEMENTS(z,n,i)                                            \
            {                                                           \
                const StdRegions::ElementType el = BOOST_PP_ARRAY_ELEM(n,OPTIMIZE2DELEMENTS); \
                BOOST_PP_REPEAT_FROM_TO(0,BOOST_PP_ARRAY_SIZE(OPTIMIZEOPERATIONS),DUMP_OPERATIONS,BOOST_PP_ARRAY_ELEM(n,OPTIMIZE2DELEMENTS)); \
            }          
            
            BOOST_PP_REPEAT_FROM_TO(0,BOOST_PP_ARRAY_SIZE(OPTIMIZE2DELEMENTS),DUMP_ELEMENTS,dummy);
        }



    } // end of namespace
} // end of namespace
