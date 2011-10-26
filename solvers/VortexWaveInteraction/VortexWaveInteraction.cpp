///////////////////////////////////////////////////////////////////////////////
//
// File VortexWaveInteraction.cpp
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
// Description: Vortex Wave Interaction class
//
///////////////////////////////////////////////////////////////////////////////
#include <VortexWaveInteraction/VortexWaveInteraction.h>

namespace Nektar
{

    VortexWaveInteraction::VortexWaveInteraction(int argc, char * argv[]):
        m_neutralPointTol(1e-4),
        m_maxOuterIterations(100),
        m_alphaStep(0.1)
    {

        int storesize = 4;// number of previous iterations to store

        m_sessionName = argv[argc-1];
        string meshfile = m_sessionName + ".xml";

        LibUtilities::SessionReaderSharedPtr session; 

        std::string VWICondFile(argv[argc-1]); 
        VWICondFile += "_VWI.xml"; 
        std::vector<std::string> VWIFilenames;
        VWIFilenames.push_back(meshfile);   
        VWIFilenames.push_back(VWICondFile);

        // Create Incompressible NavierStokesSolver session reader.
        m_sessionVWI = LibUtilities::SessionReader::CreateInstance(argc, argv, VWIFilenames);
        
        
        if(m_sessionVWI->DefinesParameter("StartIteration"))
        {
            m_iterStart = m_sessionVWI->GetParameter("StartIteration");
        }
        else
        {
            m_iterStart = 0; 
        }

        if(m_sessionVWI->DefinesParameter("EndIteration"))
        {
            m_iterEnd = m_sessionVWI->GetParameter("EndIteration");
        }
        else
        {
            m_iterEnd = 1; 
        }

        m_waveForceMag = m_sessionVWI->GetParameter("WaveForceMag");

        m_alpha = Array<OneD, NekDouble> (storesize);
        m_leading_real_evl = Array<OneD, NekDouble> (storesize);
        m_leading_imag_evl = Array<OneD, NekDouble> (storesize);
        m_alpha[0]        = m_sessionVWI->GetParameter("Alpha");
        
        if(m_sessionVWI->DefinesParameter("Relaxation"))
        {
            m_vwiRelaxation = m_sessionVWI->GetParameter("Relaxation");
        }
        else
        {
            m_vwiRelaxation = 0.0;
        }
        
        // Initialise NS Roll solver 
        std::string IncCondFile(argv[argc-1]); 
        IncCondFile += "_IncNSCond.xml"; 
        std::vector<std::string> IncNSFilenames;
        IncNSFilenames.push_back(meshfile);   
        IncNSFilenames.push_back(IncCondFile);

        // Create Incompressible NavierStokesSolver session reader.
        m_sessionRoll = LibUtilities::SessionReader::CreateInstance(argc, argv, IncNSFilenames);
        std::string vEquation = m_sessionRoll->GetSolverInfo("SolverType");
        m_solverRoll = GetEquationSystemFactory().CreateInstance(vEquation, m_sessionRoll);


        int ncoeffs = m_solverRoll->UpdateFields()[0]->GetNcoeffs();
        m_vwiForcing = Array<OneD, Array<OneD, NekDouble> > (4);
        m_vwiForcing[0] = Array<OneD, NekDouble> (4*ncoeffs);
        for(int i = 1; i < 4; ++i)
        {
            m_vwiForcing[i] = m_vwiForcing[i-1] + ncoeffs;
        }

        // Fill forcing into m_vwiForcing
        Vmath::Vcopy(ncoeffs,m_solverRoll->UpdateForces()[0]->GetCoeffs(),1,m_vwiForcing[0],1);
        Vmath::Vcopy(ncoeffs,m_solverRoll->UpdateForces()[1]->GetCoeffs(),1,m_vwiForcing[1],1);
        

        // Create AdvDiff Streak solver 
        std::string AdvDiffCondFile(argv[argc-1]); 
        AdvDiffCondFile += "_AdvDiffCond.xml"; 
        std::vector<std::string> AdvDiffFilenames;
        AdvDiffFilenames.push_back(meshfile);   
        AdvDiffFilenames.push_back(AdvDiffCondFile);

        // Create AdvDiffusion session reader.
        m_sessionStreak = LibUtilities::SessionReader::CreateInstance(argc, argv, AdvDiffFilenames);

        // Initialise LinNS solver 
        std::string LinNSCondFile(argv[argc-1]); 
        LinNSCondFile += "_LinNSCond.xml"; 
        std::vector<std::string> LinNSFilenames;
        LinNSFilenames.push_back(meshfile);   
        LinNSFilenames.push_back(LinNSCondFile);
        
        // Create Linearised NS stability session reader.
        m_sessionWave = LibUtilities::SessionReader::CreateInstance(argc, argv, LinNSFilenames);

        // Set the initial beta value in stability to be equal to VWI file
        std::string LZstr("LZ");
        NekDouble LZ = 2*M_PI/m_alpha[0];
        cout << "Setting LZ in Linearised solver to " << LZ << endl;
        m_sessionWave->SetParameter(LZstr,LZ);

        // Check for iteration type 
        if(m_sessionVWI->DefinesSolverInfo("VWIIterationType"))
        {
            std::string IterationTypeStr = m_sessionVWI->GetSolverInfo("VWIIterationType");
            for(int i = 0; i < (int) eVWIIterationTypeSize; ++i)
            {
                if(m_solverRoll->NoCaseStringCompare(VWIIterationTypeMap[i],IterationTypeStr) == 0 )
                {
                    m_VWIIterationType = (VWIIterationType)i; 
                    break;
                }
            }
        }
        else
        {
            m_VWIIterationType = eFixedAlphaWaveForcing;
        }

        // Check for restart
        bool restart;
        m_sessionVWI->MatchSolverInfo("RestartIteration","True",restart,false);
        if(restart)
        {
            string nstr =  boost::lexical_cast<std::string>(m_iterStart);
            cout << "Restarting from iteration " << m_iterStart << endl;
            std::string rstfile = "cp -f Save/" + m_sessionName + ".rst." + nstr + " " + m_sessionName + ".rst"; 
            cout << "      " << rstfile << endl;
            system(rstfile.c_str());
            std::string vwifile = "cp -f Save/" + m_sessionName + ".vwi." + nstr + " " + m_sessionName + ".vwi"; 
            cout << "      " << vwifile << endl;
            system(vwifile.c_str());
        }
    }


    VortexWaveInteraction::~VortexWaveInteraction()
    {
        m_solverRoll->GetSession()->Finalise();
        m_sessionStreak->Finalise();
        m_sessionWave->Finalise();
    }
    
    void VortexWaveInteraction::ExecuteRoll(void)
    {
        
        // Read vwi file
        std::string forcefile
            = m_sessionRoll->GetFunctionFilename("BodyForce");
        m_solverRoll->ImportFld(forcefile,m_solverRoll->UpdateForces());

        // Execute Roll 
        cout << "Executing Roll solver" << endl;
        m_solverRoll->DoInitialise();
        m_solverRoll->DoSolve();
        m_solverRoll->Output();
        
        // Copy .fld file to .rst and base.fld
        cout << "Executing cp -f session.fld session.rst" << endl;
        CopyFile(".fld",".rst");
        cout << "Executing cp -f session.fld session-Base.fld" << endl;
        CopyFile(".fld","-Base.fld");


    }


    void VortexWaveInteraction::ExecuteStreak(void)
    {
        // Setup and execute Advection Diffusion solver 
        string vEquation = m_sessionStreak->GetSolverInfo("EqType");
        EquationSystemSharedPtr solverStreak = GetEquationSystemFactory().CreateInstance(vEquation,m_sessionStreak);

        cout << "Executing Streak Solver" << endl;
        solverStreak->DoInitialise();
        solverStreak->DoSolve();
        solverStreak->Output();
        
        cout << "Executing cp -f session.fld session_streak.fld" << endl;
        CopyFile(".fld","_streak.fld");

    }

    void VortexWaveInteraction::ExecuteWave(void)
    {

        // Set the initial beta value in stability to be equal to VWI file
        std::string LZstr("LZ");
        NekDouble LZ = 2*M_PI/m_alpha[0];
        cout << "Setting LZ in Linearised solver to " << LZ << endl;
        m_sessionWave->SetParameter(LZstr,LZ);

        // Create driver
        std::string vDriverModule;
        m_sessionWave->LoadSolverInfo("Driver", vDriverModule, "ModifiedArnoldi");
        cout << "Setting up linearised NS sovler" << endl;
        DriverSharedPtr solverWave = GetDriverFactory().CreateInstance(vDriverModule, m_sessionWave);  

        /// Do linearised NavierStokes Session  with Modified Arnoldi
        cout << "Executing wave solution " << endl;
        solverWave->Execute();

        // Copy file to a rst location for next restart
        cout << "Executing cp -f session_eig_0 session_eig_0.rst" << endl;
        CopyFile("_eig_0","_eig_0.rst");

        // Store data relevant to other operations 
        m_leading_real_evl[0] = solverWave->GetRealEvl()[0];
        m_leading_imag_evl[0] = solverWave->GetImagEvl()[0];

        // Set up leading eigenvalue from inverse 
        NekDouble invmag = 1.0/(m_leading_real_evl[0]*m_leading_real_evl[0] + 
                                m_leading_imag_evl[0]*m_leading_imag_evl[0]);
        m_leading_real_evl[0] *= -invmag;
        m_leading_imag_evl[0] *= invmag;


        m_waveVelocities = solverWave->GetEqu()[0]->UpdateFields();
        m_wavePressure   = solverWave->GetEqu()[0]->GetPressure();
        
    }

    void VortexWaveInteraction::CalcNonLinearWaveForce(void)
    {
        
        int npts    = m_waveVelocities[0]->GetPlane(0)->GetNpoints();
        int ncoeffs = m_waveVelocities[0]->GetPlane(0)->GetNcoeffs();
        Array<OneD, NekDouble> val(npts), der1(2*npts);
        Array<OneD, NekDouble> der2 = der1 + npts; 

        // Shift m_vwiForcing in case of relaxation 
        Vmath::Vcopy(ncoeffs,m_vwiForcing[0],1,m_vwiForcing[2],1);
        Vmath::Vcopy(ncoeffs,m_vwiForcing[1],1,m_vwiForcing[3],1);

        
        // determine inverse of area normalised field. 
        m_wavePressure->GetPlane(0)->BwdTrans(m_wavePressure->GetPlane(0)->GetCoeffs(),
                                              m_wavePressure->GetPlane(0)->UpdatePhys());
        m_wavePressure->GetPlane(0)->BwdTrans(m_wavePressure->GetPlane(1)->GetCoeffs(),
                                              m_wavePressure->GetPlane(1)->UpdatePhys());
        NekDouble norm = m_wavePressure->L2();
        Vmath::Fill(2*npts,1.0,der1,1);
        norm = sqrt(m_waveVelocities[0]->PhysIntegral(der1)/(norm*norm));
        
        // Get hold of arrays. 
        m_waveVelocities[0]->GetPlane(0)->BwdTrans(m_waveVelocities[0]->GetPlane(0)->GetCoeffs(),m_waveVelocities[0]->GetPlane(0)->UpdatePhys());
        Array<OneD, NekDouble> u_real = m_waveVelocities[0]->GetPlane(0)->UpdatePhys();
        Vmath::Smul(npts,norm,u_real,1,u_real,1);
        m_waveVelocities[0]->GetPlane(1)->BwdTrans(m_waveVelocities[0]->GetPlane(1)->GetCoeffs(),m_waveVelocities[0]->GetPlane(1)->UpdatePhys());
        Array<OneD, NekDouble> u_imag = m_waveVelocities[0]->GetPlane(1)->UpdatePhys();
        Vmath::Smul(npts,norm,u_imag,1,u_imag,1);
        m_waveVelocities[1]->GetPlane(0)->BwdTrans(m_waveVelocities[1]->GetPlane(0)->GetCoeffs(),m_waveVelocities[1]->GetPlane(0)->UpdatePhys());
        Array<OneD, NekDouble> v_real = m_waveVelocities[1]->GetPlane(0)->UpdatePhys(); 
        Vmath::Smul(npts,norm,v_real,1,v_real,1);
        m_waveVelocities[1]->GetPlane(1)->BwdTrans(m_waveVelocities[1]->GetPlane(1)->GetCoeffs(),m_waveVelocities[1]->GetPlane(1)->UpdatePhys());
        Array<OneD, NekDouble> v_imag = m_waveVelocities[1]->GetPlane(1)->UpdatePhys();
        Vmath::Smul(npts,norm,v_imag,1,v_imag,1);
        
        // Calculate non-linear terms for x and y directions
        // d/dx(u u* + u* u)
        Vmath::Vmul (npts,u_real,1,u_real,1,val,1);
        Vmath::Vvtvp(npts,u_imag,1,u_imag,1,val,1,val,1);
        Vmath::Smul (npts,2.0,val,1,val,1);
        m_waveVelocities[0]->GetPlane(0)->PhysDeriv(0,val,der1);
        
        // d/dy(v u* + v* u)
        Vmath::Vmul (npts,u_real,1,v_real,1,val,1);
        Vmath::Vvtvp(npts,u_imag,1,v_imag,1,val,1,val,1);
        Vmath::Smul (npts,2.0,val,1,val,1);
        m_waveVelocities[0]->GetPlane(0)->PhysDeriv(1,val,der2);
        
        Vmath::Vadd(npts,der1,1,der2,1,der1,1);
        
        m_waveVelocities[0]->GetPlane(0)->FwdTrans_BndConstrained(der1,m_vwiForcing[0]);
        Vmath::Smul(ncoeffs,-m_waveForceMag,m_vwiForcing[0],1,m_vwiForcing[0],1);
        
        // d/dx(u v* + u* v)
        m_waveVelocities[0]->GetPlane(0)->PhysDeriv(0,val,der1);
        
        // d/dy(v v* + v* v)
        Vmath::Vmul(npts,v_real,1,v_real,1,val,1);
        Vmath::Vvtvp(npts,v_imag,1,v_imag,1,val,1,val,1);
        Vmath::Smul (npts,2.0,val,1,val,1);
        m_waveVelocities[0]->GetPlane(0)->PhysDeriv(1,val,der2);
        
        m_waveVelocities[0]->GetPlane(0)->FwdTrans_BndConstrained(der1,m_vwiForcing[1]);
        Vmath::Smul(ncoeffs,-m_waveForceMag,m_vwiForcing[1],1,m_vwiForcing[1],1);
        
        // Symmetrise forcing
        //-> Get coordinates 
        Array<OneD, NekDouble> coord(2);
        Array<OneD, NekDouble> coord_x(npts);
        Array<OneD, NekDouble> coord_y(npts);
        
        //-> Impose symmetry (x -> -x + Lx/2, y-> -y) on coordinates
        m_waveVelocities[0]->GetPlane(0)->GetCoords(coord_x,coord_y);
        NekDouble xmax = Vmath::Vmax(npts,coord_x,1);
        Vmath::Neg(npts,coord_x,1);
        Vmath::Sadd(npts,xmax,coord_x,1,coord_x,1);
        Vmath::Neg(npts,coord_y,1);
        
        int i, physoffset;
        
        //-> Obtain list of expansion element ids for each point. 
        Array<OneD, int> Eid(npts);
        // This search may not be necessary every iteration
        for(i = 0; i < npts; ++i)
        {
            coord[0] = coord_x[i];
            coord[1] = coord_y[i];
            
            // Note this will not quite be symmetric. 
            Eid[i] = m_waveVelocities[0]->GetPlane(0)->GetExpIndex(coord,1e-6);
        }
        
        // Interpolate field 0 
        m_waveVelocities[0]->GetPlane(0)->BwdTrans_IterPerExp(m_vwiForcing[0],der1);
        for(i = 0; i < npts; ++i)
        {
            physoffset = m_waveVelocities[0]->GetPlane(0)->GetPhys_Offset(Eid[i]);
            coord[0] = coord_x[i];
            coord[1] = coord_y[i];
            der2 [i] = m_waveVelocities[0]->GetPlane(0)->GetExp(Eid[i])->PhysEvaluate(coord,
                                                                            der1 + physoffset);
        }
        //-> Average field 0 
        Vmath::Vsub(npts,der1,1,der2,1,der2,1);
        Vmath::Smul(npts,0.5,der2,1,der2,1);
        m_waveVelocities[0]->GetPlane(0)->FwdTrans_BndConstrained(der2, m_vwiForcing[0]);
        
        //-> Interpoloate field 1
        m_waveVelocities[0]->GetPlane(0)->BwdTrans_IterPerExp(m_vwiForcing[1],der1);
        for(i = 0; i < npts; ++i)
        {
            physoffset = m_waveVelocities[0]->GetPlane(0)->GetPhys_Offset(Eid[i]);
            coord[0] = coord_x[i];
            coord[1] = coord_y[i];
            der2[i]  = m_waveVelocities[0]->GetPlane(0)->GetExp(Eid[i])->PhysEvaluate(coord,
                                                                           der1 + physoffset);
        }
        
        //-> Average field 1
        Vmath::Vsub(npts,der1,1,der2,1,der2,1);
        Vmath::Smul(npts,0.5,der2,1,der2,1);
        m_waveVelocities[0]->GetPlane(0)->FwdTrans_BndConstrained(der2, m_vwiForcing[1]);

        // Apply relaxation 
        if(m_vwiRelaxation)
        {
            Vmath::Smul(ncoeffs,1.0-m_vwiRelaxation,
                        m_vwiForcing[0],1,m_vwiForcing[0],1);
            Vmath::Svtvp(ncoeffs,m_vwiRelaxation,m_vwiForcing[2],1,
                          m_vwiForcing[0],1,m_vwiForcing[0],1);

            Vmath::Smul(ncoeffs,1.0-m_vwiRelaxation,
                        m_vwiForcing[1],1,m_vwiForcing[1],1);
            Vmath::Svtvp(ncoeffs,m_vwiRelaxation,m_vwiForcing[3],1,
                          m_vwiForcing[1],1,m_vwiForcing[1],1);
        }


        
        // dump output
        Array<OneD, std::string> variables(2);
        Array<OneD, Array<OneD, NekDouble> > outfield(2);
        variables[0] = "u";   variables[1] = "v";
        outfield[0]  = m_vwiForcing[0];
        outfield[1]  = m_vwiForcing[1];
        
        std::string outname = m_sessionName  + ".vwi";
        
        m_solverRoll->WriteFld(outname, m_waveVelocities[0]->GetPlane(0), outfield, variables);
    }
    
    void VortexWaveInteraction::SaveFile(string fileend, string dir, int n)
    {
        static bool init = true; 

        if(init == true)
        {
            // make directory and presume will fail if it already exists
            string mkdir = "mkdir " + dir;
            system(mkdir.c_str());
            init = false;
        }
        
        string cpfile   = m_sessionName + fileend;
        string savefile = dir + "/" + cpfile + "." + boost::lexical_cast<std::string>(n);
        string syscall  = "cp -f "  + cpfile + " " + savefile; 

        system(syscall.c_str());
    }

    void VortexWaveInteraction::CopyFile(string file1end, string file2end)
    {
        string cpfile1   = m_sessionName + file1end;
        string cpfile2   = m_sessionName + file2end;
        string syscall  = "cp -f "  + cpfile1 + " " + cpfile2; 

        system(syscall.c_str());
    }

    void VortexWaveInteraction::AppendEvlToFile(string file, int n)
    {
        FILE *fp;
        fp = fopen(file.c_str(),"a");
        fprintf(fp, "%d: %lf %16.12le  %16.12le\n",n, m_alpha[0], m_leading_real_evl[0],m_leading_imag_evl[0]);
        fclose(fp);
    }

    void VortexWaveInteraction::SaveLoopDetails(int i)

    {
        // Save NS restart file
        SaveFile(".rst","Save",i);
        // Save Streak Solution
        SaveFile("_streak.fld","Save",i);
        // Save Wave solution output
        SaveFile(".evl","Save",i);
        SaveFile("_eig_0","Save",i);
        // Save new forcing file
        SaveFile(".vwi","Save",i+1);
    }

    void VortexWaveInteraction::ExecuteLoop(void)
    {
        ExecuteRoll();

        ExecuteStreak();
 
        ExecuteWave();
                   
        CalcNonLinearWaveForce();
    }

    bool VortexWaveInteraction::CheckGrowthConverged(void)
    {
        static NekDouble previous_real_evl = -1.0; 
        
        if(previous_real_evl == -1.0)
        {
            previous_real_evl = m_leading_real_evl[0];
            return false;
        }

        cout << "Growth tolerance: " << fabs((m_leading_real_evl[0] - previous_real_evl)/m_leading_real_evl[0]) << endl; 
            
        if(fabs((m_leading_real_evl[0] - previous_real_evl)/m_leading_real_evl[0]) < m_neutralPointTol)
        {
            previous_real_evl = m_leading_real_evl[0];
            return true;
        }
        else
        {
            if(fabs(m_leading_imag_evl[0]) > 1e-2)
            {
                cout << "Warning: imaginary eigenvalue is greater than 1e-2" << endl;
            }
            previous_real_evl = m_leading_real_evl[0];
            return false;
        }
    }

    // Check to see if leading eigenvalue is within tolerance defined
    // in m_neutralPointTol
    bool VortexWaveInteraction::CheckIfAtNeutralPoint(void)
    {
        if((m_leading_real_evl[0]*m_leading_real_evl[0] + m_leading_imag_evl[0]*m_leading_imag_evl[0]) < m_neutralPointTol*m_neutralPointTol)
        {
            return true;
        }
        else
        {
            if(fabs(m_leading_imag_evl[0]) > 1e-2)
            {
                cout << "Warning: imaginary eigenvalue is greater than 1e-2" << endl;
            }
            return false;
        }

    }
    
    void VortexWaveInteraction::UpdateAlpha(int outeriter)
    {
        NekDouble alp_new;

        if(outeriter == 1)
        {
            m_alpha[1] = m_alpha[0];
            if(m_leading_real_evl[0] > 0.0)
            {
                alp_new = m_alpha[0] + m_alphaStep;
            }
            else
            {
                alp_new = m_alpha[0] - m_alphaStep;
            }
        }
        else
        {
            int i,j;
            int nstore = (m_alpha.num_elements() < outeriter)? m_alpha.num_elements(): outeriter;
            Array<OneD, NekDouble> Alpha(nstore);
            Array<OneD, NekDouble> Growth(nstore);
            
            Vmath::Vcopy(nstore,m_alpha,1,Alpha,1);

            // Sort Alpha Growth values; 
            double store;
            int k;
            for(i = 0; i < nstore; ++i)
            {
                k = Vmath::Imin(nstore-i,&Alpha[i],1);
                
                store    = Alpha[i]; 
                Alpha[i] = Alpha[k];
                Alpha[k] = store;

                store     = Growth[i];
                Growth[i] = Growth[k];
                Growth[k] = store; 
            }

            // See if we have any values that cross zero
            for(i = 0; i < nstore-1; ++i)
            {
                if(Growth[i]*Growth[i+1] < 0.0)
                {
                    break;
                }
            }
            
            if(i != nstore-1)
            {
                if(nstore == 2)
                {
                    alp_new = (Alpha[0]*Growth[1] - Alpha[1]*Growth[0])/(Growth[1]-Growth[0]);
                }
                else
                {
                    // use a quadratic fit and step through 10000 points
                    // to find zero.
                    int     j; 
                    int     nsteps = 10000;
                    int     idx = (i == 0)?1:i;
                    double  da = Alpha[idx+1] - Alpha[idx-1];
                    double  gval_m1 = Growth[idx-1],a,gval;
                    double  c1 = Growth[idx-1]/(Alpha[idx-1]-Alpha[idx])/
                        (Alpha[idx-1]-Alpha[idx+1]);
                    double  c2 = Growth[idx]/(Alpha[idx]-Alpha[idx-1])/
                        (Alpha[idx]-Alpha[idx+1]);
                    double  c3 = Growth[idx+1]/(Alpha[idx+1]-Alpha[idx-1])/
                        (Alpha[idx+1]-Alpha[idx]);
                    
                    for(j = 1; j < nsteps+1; ++j)
                    {
                        a = Alpha[i] + j*da/(double) nsteps;
                        gval = c1*(a - Alpha[idx  ])*(a - Alpha[idx+1]) 
                            +  c2*(a - Alpha[idx-1])*(a - Alpha[idx+1])
                            +  c3*(a - Alpha[idx-1])*(a - Alpha[idx]);
                        
                        if(gval*gval_m1 < 0.0)
                        {
                            alp_new = ((a+da/(double)nsteps)*gval - a*gval_m1)/
                                (gval - gval_m1);
                            break;
                        }
                    }
                }
            }
            else // step backward/forward by 0.1 depending on sign
            {
                if(Growth[i] > 0.0)
                {
                    alp_new = m_alpha[0] + m_alphaStep;
                }
                else
                {
                    alp_new = m_alpha[0] - m_alphaStep;
                }
            }
        }
        
        for(int i = m_alpha.num_elements()-1; i > 0; --i)
        {
            m_alpha[i] = m_alpha[i-1];
            m_leading_real_evl[i] = m_leading_real_evl[i-1];
            m_leading_imag_evl[i] = m_leading_imag_evl[i-1];
        }

        m_alpha[0] = alp_new;
        
    }

}
