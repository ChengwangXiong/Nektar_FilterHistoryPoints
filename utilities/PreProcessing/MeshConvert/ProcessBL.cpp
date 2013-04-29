////////////////////////////////////////////////////////////////////////////////
//
//  File: ProcessBL.cpp
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
//  Description: Refine prismatic boundary layer elements.
//
////////////////////////////////////////////////////////////////////////////////

#include <string>
using namespace std;

#include "MeshElements.h"
#include "ProcessBL.h"

#include <LibUtilities/Foundations/BLPoints.h>
#include <LocalRegions/PrismExp.h>
#include <LibUtilities/BasicUtils/SharedArray.hpp>

namespace Nektar
{
    namespace Utilities
    {
        ModuleKey ProcessBL::className =
            GetModuleFactory().RegisterCreatorFunction(
                ModuleKey(eProcessModule, "bl"), ProcessBL::create,
                "Refines a prismatic boundary layer.");

        ProcessBL::ProcessBL(MeshSharedPtr m) : ProcessModule(m)
        {
            // BL mesh configuration.
            config["layers"]     = ConfigOption(false, "2",
                "Number of layers to refine.");
            config["nq"]         = ConfigOption(false, "5",
                "Number of points in high order elements.");
            config["surf"]       = ConfigOption(false, "-1",
                "Tag identifying surface connected to prism.");
            config["r"]          = ConfigOption(false, "2.0",
                "Ratio to use in geometry progression.");
        }

        ProcessBL::~ProcessBL()
        {

        }

        void ProcessBL::Process()
        {
            if (m->verbose)
            {
                cout << "ProcessBL: Refining prismatic boundary layer..."
                     << endl;
            }

            int nodeId  = m->vertexSet.size();
            int nl      = config["layers"].as<int>();
            int nq      = config["nq"].    as<int>();
            int surf    = config["surf"].  as<int>();

            LibUtilities::BLPoints::delta_star = config["r"].as<NekDouble>();

            // Prismatic node -> face map.
            int prismFaceNodes[5][4] = {
                {0,1,2,3},{0,1,4,-1},{1,2,5,4},{3,2,5,-1},{0,3,5,4}};

            // Default PointsType.
            LibUtilities::PointsType pt = LibUtilities::eGaussLobattoLegendre;

            // Map which takes element ID to face on surface. This enables
            // splitting to occur in either y-direction of the prism.
            map<int, int> splitEls;

            // Set up maps which takes an edge (in nektar++ ordering) and return
            // their offset and stride in the 3d array of collapsed quadrature
            // points. Note that this map includes only the edges that are on
            // the triangular faces as the edges in the normal direction are
            // linear.
            int splitMapEdge[6] = {0,2,4,5,6,7};
            int splitMapOffset[6][2] = {
                {0,       1        },
                {nq,      1        },
                {0,       nq*(nl+1)},
                {nq-1,    nq*(nl+1)},
                {nq+nq-1, nq*(nl+1)},
                {nq,      nq*(nl+1)}
            };

            // splitEdge enumerates the edges in the standard prism along which
            // new nodes should be generated. These edges are the three between
            // the two triangular faces.
            int splitEdge[3] = {3,1,8};

            // edgeVertMap specifies the vertices which comprise those edges in
            // splitEdge; for example splitEdge[0] = 3 which connects vertices 0
            // and 3.
            int edgeVertMap[3][2] = {{0,3}, {1,2}, {4,5}};

            // edgeOffset holds the offset of each of edges 3, 1 and 8
            // respectively inside the collapsed coordinate system.
            int edgeOffset[3] = {0, nq-1, nq*(nl+1)*(nq-1)};

            // edgeMap associates geometry edge IDs to the (nl+1) vertices which
            // are generated along that edge when a prism is split, and is used
            // to avoid generation of duplicate vertices. It is stored as an
            // unordered map for speed.
            boost::unordered_map<int, vector<NodeSharedPtr> > edgeMap;
            boost::unordered_map<int, vector<NodeSharedPtr> >::iterator eIt;

            int surfTag = config["surf"].as<int>();
            if (surfTag != -1)
            {
                // If surface is defined, process list of elements to find those
                // that are connected to it.
                for (int i = 0; i < m->element[m->expDim].size(); ++i)
                {
                    ElementSharedPtr el = m->element[m->expDim][i];
                    int nSurf = el->GetFaceCount();

                    for (int j = 0; j < nSurf; ++j)
                    {
                        int bl = el->GetBoundaryLink(j);
                        if (bl == -1)
                        {
                            continue;
                        }

                        ElementSharedPtr bEl  = m->element[m->expDim-1][bl];
                        vector<int>      tags = bEl->GetTagList();

                        if (find(tags.begin(), tags.end(), surfTag) !=
                            tags.end())
                        {
                            if (el->GetConf().e != ePrism)
                            {
                                cerr << "WARNING: Found non-prismatic element "
                                     << "to split in surface " << surf
                                     << "; ignoring" << endl;
                                continue;
                            }

                            if (j % 2 == 0)
                            {
                                cerr << "WARNING: Found quadrilateral face "
                                     << j << " on surface " << surf
                                     << " connected to prism; ignoring."
                                     << endl;
                                continue;
                            }

                            if (splitEls.count(el->GetId()) > 0)
                            {
                                cerr << "WARNING: prism already found; "
                                     << "ignoring" << endl;
                            }

                            splitEls[el->GetId()] = j;
                        }
                    }
                }
            }
            else
            {
                // Otherwise, add all prismatic elements and assume face 1 of
                // the prism lies on the surface.
                for (int i = 0; i < m->element[m->expDim].size(); ++i)
                {
                    ElementSharedPtr el = m->element[m->expDim][i];

                    if (el->GetConf().e != ePrism)
                    {
                        continue;
                    }

                    splitEls[el->GetId()] = 1;
                }
            }

            if (splitEls.size() == 0)
            {
                cerr << "WARNING: No elements detected to split." << endl;
                return;
            }

            // Erase all elements from the element list. Elements will be
            // re-added as they are split.
            vector<ElementSharedPtr> el = m->element[m->expDim];
            m->element[m->expDim].clear();

            // Iterate over list of elements of expansion dimension.
            for (int i = 0; i < el.size(); ++i)
            {
                if (splitEls.count(el[i]->GetId()) == 0)
                {
                    m->element[m->expDim].push_back(el[i]);
                    continue;
                }

                // Find quadrilateral boundary faces if any
                std::map<int, int> bLink;
                for (int j = 0; j < 5; j += 2)
                {
                    int bl = el[i]->GetBoundaryLink(j);
                    if (bl != -1)
                    {
                        bLink[j] = bl;
                    }
                }

                // Get elemental geometry object.
                SpatialDomains::PrismGeomSharedPtr geom =
                    boost::dynamic_pointer_cast<SpatialDomains::PrismGeom>(
                        el[i]->GetGeom(m->spaceDim));

                // Determine whether to use reverse points.
                LibUtilities::PointsType t = splitEls[el[i]->GetId()] == 1 ?
                    LibUtilities::eBoundaryLayerPoints :
                    LibUtilities::eBoundaryLayerPointsRev;

                // Create basis.
                LibUtilities::BasisKey B0(
                    LibUtilities::eModified_A, nq,
                    LibUtilities::PointsKey(nq,pt));
                LibUtilities::BasisKey B1(
                    LibUtilities::eModified_A, 2,
                    LibUtilities::PointsKey(nl+1, t));
                LibUtilities::BasisKey B2(
                    LibUtilities::eModified_B, nq,
                    LibUtilities::PointsKey(nq,pt));

                // Create local region.
                LocalRegions::PrismExpSharedPtr q =
                    MemoryManager<LocalRegions::PrismExp>::AllocateSharedPtr(
                        B0,B1,B2,geom);

                // Grab co-ordinates.
                Array<OneD, NekDouble> x(nq*nq*(nl+1));
                Array<OneD, NekDouble> y(nq*nq*(nl+1));
                Array<OneD, NekDouble> z(nq*nq*(nl+1));
                q->GetCoords(x,y,z);

                vector<vector<NodeSharedPtr> > edgeNodes(3);

                // Loop over edges to be split.
                for (int j = 0; j < 3; ++j)
                {
                    int locEdge = splitEdge[j];
                    int edgeId  = el[i]->GetEdge(locEdge)->id;

                    // Determine whether we have already generated vertices
                    // along this edge.
                    eIt = edgeMap.find(edgeId);

                    if (eIt == edgeMap.end())
                    {
                        // If not then resize storage to hold new points.
                        edgeNodes[j].resize(nl+1);

                        // Re-use existing vertices at endpoints of edge to
                        // avoid duplicating the existing vertices.
                        edgeNodes[j][0]  = el[i]->GetVertex(edgeVertMap[j][0]);
                        edgeNodes[j][nl] = el[i]->GetVertex(edgeVertMap[j][1]);

                        // Create new interior nodes.
                        for (int k = 1; k < nl; ++k)
                        {
                            int pos = edgeOffset[j] + k*nq;
                            edgeNodes[j][k] = NodeSharedPtr(
                                new Node(nodeId++, x[pos], y[pos], z[pos]));
                        }

                        // Store these edges in edgeMap.
                        edgeMap[edgeId] = edgeNodes[j];
                    }
                    else
                    {
                        edgeNodes[j] = eIt->second;
                    }
                }

                // Create element layers.
                for (int j = 0; j < nl; ++j)
                {
                    // Offset of this layer within the collapsed coordinate
                    // system.
                    int offset = j*nq;

                    // Get corner vertices.
                    vector<NodeSharedPtr> nodeList(6);
                    nodeList[0] = edgeNodes[0][j  ];
                    nodeList[1] = edgeNodes[1][j  ];
                    nodeList[2] = edgeNodes[1][j+1];
                    nodeList[3] = edgeNodes[0][j+1];
                    nodeList[4] = edgeNodes[2][j  ];
                    nodeList[5] = edgeNodes[2][j+1];

                    // Create the element.
                    ElmtConfig conf(ePrism, 1, true, true, false);
                    ElementSharedPtr elmt = GetElementFactory().
                        CreateInstance(
                            ePrism,conf,nodeList,el[i]->GetTagList());

                    // Add high order nodes to split prismatic edges.
                    for (int l = 0; l < 6; ++l)
                    {
                        EdgeSharedPtr HOedge = elmt->GetEdge(
                            splitMapEdge[l]);
                        for (int k = 1; k < nq-1; ++k)
                        {
                            int pos = offset + splitMapOffset[l][0] +
                                k*splitMapOffset[l][1];
                            HOedge->edgeNodes.push_back(
                                NodeSharedPtr(
                                    new Node(nodeId++,x[pos],y[pos],z[pos])));
                        }
                        HOedge->curveType = pt;
                    }

                    // Change the surface elements of the quad face on the
                    // symmetry plane to match the layers of prisms.
                    map<int,int>::iterator it;
                    for (it = bLink.begin(); it != bLink.end(); ++it)
                    {
                        int fid = it->first;
                        int bl  = it->second;

                        if (j == 0)
                        {
                            // For first layer reuse existing 2D element.
                            ElementSharedPtr e = m->element[m->expDim-1][bl];
                            for (int k = 0; k < 4; ++k)
                            {
                                e->SetVertex(k, nodeList[prismFaceNodes[fid][k]]);
                            }
                        }
                        else
                        {
                            // For all other layers create new element.
                            vector<NodeSharedPtr> qNodeList(4);
                            for (int k = 0; k < 4; ++k)
                            {
                                qNodeList[k] = nodeList[prismFaceNodes[fid][k]];
                            }
                            vector<int> tagBE;
                            tagBE = m->element[m->expDim-1][bl]->GetTagList();
                            ElmtConfig bconf(eQuadrilateral, 1, true, true, false);
                            ElementSharedPtr boundaryElmt = GetElementFactory().
                                CreateInstance(eQuadrilateral,bconf,qNodeList,tagBE);
                            m->element[m->expDim-1].push_back(boundaryElmt);
                        }
                    }

                    m->element[m->expDim].push_back(elmt);
                }
            }

            // Re-process mesh to eliminate duplicate vertices and edges.
            ProcessVertices();
            ProcessEdges();
            ProcessFaces();
            ProcessElements();
            ProcessComposites();
        }
    }
}
