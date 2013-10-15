///////////////////////////////////////////////////////////////////////////////
//
// File DriverSteadyState.cpp
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
// Description: Incompressible Navier Stokes solver
//
///////////////////////////////////////////////////////////////////////////////

#include <SolverUtils/DriverSteadyState.h>


namespace Nektar
{
    namespace SolverUtils
    {
        string DriverSteadyState::className = GetDriverFactory().RegisterCreatorFunction("SteadyState", DriverSteadyState::create);
        string DriverSteadyState::driverLookupId = LibUtilities::SessionReader::RegisterEnumValue("Driver","SteadyState",0);
        
        /**
         * 
         */
        DriverSteadyState::DriverSteadyState(const LibUtilities::SessionReaderSharedPtr pSession)
        : Driver(pSession)
        {
        }
        
        
        /**
         * 
         */
        DriverSteadyState:: ~DriverSteadyState()
        {
        }
        
        
        /**
         * 
         */
        void DriverSteadyState::v_InitObject(ostream &out)
        {
            Driver::v_InitObject(out);
        }
        
        
        void DriverSteadyState::v_Execute(ostream &out)
        
        {
            //With a loop over "DoSolve", this Driver implements the "encaplulated" Selective Frequency Damping method
            //to find the steady state of a flow above the critical Reynolds number.
            
            m_equ[0]->PrintSummary(out);
            m_equ[0]->DoInitialise();
            
            // - SFD Routine -
            NumElmVelocity = m_equ[0]->GetNumElmVelocity();
            
            Array<OneD, Array<OneD, NekDouble> > q0(NumElmVelocity);
            Array<OneD, Array<OneD, NekDouble> > q1(NumElmVelocity);
            Array<OneD, Array<OneD, NekDouble> > qBar0(NumElmVelocity);
            Array<OneD, Array<OneD, NekDouble> > qBar1(NumElmVelocity);
            
            //For BDF2 implementation
            Array<OneD, Array<OneD, NekDouble> > qMinusHalf(NumElmVelocity);
            Array<OneD, Array<OneD, NekDouble> > qBarMinusHalf(NumElmVelocity);
            
            //For Strang scheme
            Array<OneD, Array<OneD, NekDouble> > qTilde(NumElmVelocity);
            Array<OneD, Array<OneD, NekDouble> > qBarTilde(NumElmVelocity);
            
            //For averaging the flow
            Array<OneD, Array<OneD, NekDouble> > qAvr(NumElmVelocity);
            //Array<OneD, Array<OneD, NekDouble> > qBarAvr(NumElmVelocity);
            
            NekDouble TOL(0);
            m_n=0;
            m_Check=0;			
            
            m_session->LoadParameter("FilterWidth", m_Delta0, 1);
            m_session->LoadParameter("ControlCoeff",m_X0, 1);
            m_session->LoadParameter("TOL", TOL, 1.0e-08);
            m_session->LoadParameter("IO_InfoSteps", m_infosteps, 1000);
            m_session->LoadParameter("IO_CheckSteps", m_checksteps, 100000);
            
            m_dt = m_equ[0]->GetTimeStep();
            
            //m_Delta = m_Delta0;
            //m_X = m_X0;            
            ////m_cst1=m_X*m_dt;
            //m_cst1=m_X;
            //m_cst2=1.0/(1.0 + m_cst1);
            ////m_cst3=m_dt/m_Delta;
            //m_cst3=1.0/m_Delta;
            //m_cst4=m_cst2*m_cst3;
            //m_cst5=1.0/(1.0 + m_cst3*(1.0-m_cst1*m_cst2));
            
            
            
            //To run each integration of the linear filters problem on half a time-step we add:
            //m_X = 0.5*m_X0;
            //m_Delta = 2.0*m_Delta0;
            
            m_X = m_X0;
            m_Delta = m_Delta0;
            
            //BDF1 Matrix
            c1 = 1.0 + m_X;
            c2 = 1.0 + 1.0/m_Delta - m_X/(m_Delta*c1);
            
            a11 = 1.0/c1 + m_X/(m_Delta*c1*c1*c2);
            a12 = m_X/(c1*c2);
            a21 = 1.0/(m_Delta*c1*c2);
            a22 = 1.0/c2;
            
            //BDF2 Matrix
            c1 = 3.0 + 2.0*m_X;
            c2 = 3.0 + 2.0/m_Delta - 4.0*m_X/(m_Delta*c1);
            
            b11 = 4.0/c1 + 16.0*m_X/(m_Delta*c1*c1*c2);
            b12 = 8.0*m_X/(c1*c2);
            b13 = - (1.0/c1 + 4.0*m_X/(m_Delta*c1*c1*c2));
            b14 = - 2.0*m_X/(c1*c2);
            b21 = 8.0/(m_Delta*c1*c2);
            b22 = 4.0/c2;
            b23 = - 2.0/(m_Delta*c1*c2);
            b24 = - 1.0/c2;    
            
            //Exact solution of the Filters equation (ici on a X_tilde et Delta_tile!!!)
            c1 = 1.0/(1.0 + m_X*m_Delta);

            F11 = c1*(1.0 + m_X*m_Delta*exp(-(m_X + 1.0/m_Delta)/2.0));
            F12 = c1*(m_X*m_Delta*(1.0 - exp(-(m_X + 1.0/m_Delta)/2.0)));
            F21 = c1*(1.0 - exp(-(m_X + 1.0/m_Delta)/2.0));
            F22 = c1*(m_X*m_Delta + exp(-(m_X + 1.0/m_Delta)/2.0));
            
            //Veriables for averaging the flow
            bool AveragingFlow;
            AveragingFlow = false; 
            int StartAvrg(20000);
            int EndAvrg(170000);
            NekDouble CoeffAvrg(0.0);
            CoeffAvrg = 1.0/(1.0*EndAvrg - 1.0*StartAvrg);            
            
            //----- Convergence History Parameters ------
            m_Growing=false;
            m_Shrinking=false;
            
            m_MinNormDiff_q_qBar = 9999;
            m_MaxNormDiff_q_qBar = 0;
            m_First_MinNormDiff_q_qBar = 0;
            m_Oscillation = 0;
            //-------------------------------------------
            
            cout << "------------------ SFD Parameters ------------------" << endl;
            cout << "\tDelta = " << m_Delta << endl;
            cout << "\tX = " << m_X << endl;
            cout << "----------------------------------------------------" << endl;
            
            m_equ[0]->SetStepsToOne(); //m_steps is set to 1. Then "m_equ[0]->DoSolve()" will run for only one time step			
            ofstream m_file("ConvergenceHistory.txt", ios::out | ios::trunc);
            
            for(int i = 0; i < NumElmVelocity; ++i)
            {
                q0[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(), 0.0); //q0 is initialised
                qBar0[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(), 0.0);
                m_equ[0]->CopyFromPhysField(i, qBar0[i]); //qBar0 is initially set at beiing equal to the initial conditions provided in the input file
                
                //For BDF2 implentation
                qMinusHalf[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(), 0.0);
                qBarMinusHalf[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(), 0.0);     
                
                //For Strang scheme
                qTilde[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(), 0.0);
                qBarTilde[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(), 0.0);   
                
                //For averaging the flow
                qAvr[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(), 0.0);
                //qBarAvr[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(), 0.0);   
            }
            
            MaxNormDiff_q_qBar = 1.0;
            MaxNormDiff_q1_q0 = 1.0;
            Min_MaxNormDiff_q_qBar = MaxNormDiff_q_qBar;
            
            m_LIM0 = 1.0e-08; //1.0e-03;
            m_LIM = m_LIM0;
            
            
            while (max(MaxNormDiff_q_qBar, MaxNormDiff_q1_q0) > TOL)
            //while (m_n < EndAvrg)
            {
                /*
                //Implementation of the second order sequential operator-splitting method (i.e. Strang splitting)
                
                //First intergration on half a step
                for(int i = 0; i < NumElmVelocity; ++i)
                { 
                    m_equ[0]->CopyFromPhysField(i, q0[i]);
                    
                    if (m_n == 0)
                    {
                        RunBDF1(i, q0, qBar0, q1, qBar1);
                    }
                    else
                    {
                        RunBDF2(i, qMinusHalf, qBarMinusHalf, q0, qBar0, q1, qBar1);
                    }
                    
                    qMinusHalf[i] = q1[i];
                    qBarMinusHalf[i] = qBar1[i];
                    
                    qBarTilde[i] = qBar1[i];                    
                    m_equ[0]->CopyToPhysField(i, q1[i]);                    
                }
                
                //Run NS solver a one time step
                m_equ[0]->DoSolve();
                
                //Second intergration on the other half step
                for(int i = 0; i < NumElmVelocity; ++i)
                {
                    m_equ[0]->CopyFromPhysField(i, qTilde[i]);
                    
                    RunBDF2(i, qMinusHalf, qBarMinusHalf, qTilde, qBarTilde, q1, qBar1);
                    
                    qMinusHalf[i] = qTilde[i];
                    qBarMinusHalf[i] = qBarTilde[i];
                    
                    qBar0[i] = qBar1[i];                    
                    m_equ[0]->CopyToPhysField(i, q1[i]);                    
                }
                */
                
                //*
                //First order Splitting with exact resolution of the filters equation
                m_equ[0]->DoSolve();
                
                for(int i = 0; i < NumElmVelocity; ++i)
                {
                    m_equ[0]->CopyFromPhysField(i, q0[i]);
                    
                    //EvaluateNextSFDVariables(i, q0, qBar0, q1, qBar1);
                    ExactFilters(i, q0, qBar0, q1, qBar1);
                    
                    qBar0[i] = qBar1[i];
                    m_equ[0]->CopyToPhysField(i, q1[i]);     
                    
                    //For averaging the flow
                    if ((AveragingFlow == true) && (m_n > StartAvrg))
                    {
                        Vmath::Vadd(qAvr[i].num_elements(), q1[i], 1, qAvr[i], 1, qAvr[i], 1); 
                    }
                }
                //*/
                
                /*
                //Second Order Splitting with exact resolution of the filters equation
                //First intergration on half a step
                for(int i = 0; i < NumElmVelocity; ++i)
                { 
                    m_equ[0]->CopyFromPhysField(i, q0[i]);
                    
                    ExactFilters(i, q0, qBar0, q1, qBar1);
                    
                    qBar0[i] = qBar1[i];
                    m_equ[0]->CopyToPhysField(i, q1[i]);                      
                }
                
                //Run NS solver a one time step
                m_equ[0]->DoSolve();
                
                //Second intergration on the other half step
                for(int i = 0; i < NumElmVelocity; ++i)
                {
                    m_equ[0]->CopyFromPhysField(i, q0[i]);
                    
                    ExactFilters(i, q0, qBar0, q1, qBar1);
                    
                    qBar0[i] = qBar1[i];
                    m_equ[0]->CopyToPhysField(i, q1[i]);                   
                }
                */
                
                
                if(m_infosteps && !((m_n+1)%m_infosteps))
                {
                    ConvergenceHistory(qBar1, q0, MaxNormDiff_q_qBar, MaxNormDiff_q1_q0);
                }
                
                if(m_checksteps && m_n&&(!((m_n+1)%m_checksteps)))
                {                      
                    m_Check++;                    
                    m_equ[0]->Checkpoint_Output(m_Check);                       
                }
                
                //cout << "Step " << m_n << " just finished!" << endl;
                m_n++;
            }
            
            cout << "\nFINAL Filter Width: Delta = " << m_Delta << "; FINAL Control Coeff: X = " << m_X << "\n" << endl;
            m_Check++;
            m_equ[0]->Checkpoint_Output(m_Check);
            
            m_file.close();
            // - End SFD Routine -
            
            //Put the averaged flow into tho outputed solution
            if (AveragingFlow == true)
            {
                for(int i = 0; i < NumElmVelocity; ++i)
                {
                    Vmath::Smul(qAvr[i].num_elements(), CoeffAvrg, qAvr[i], 1, qAvr[i], 1); 
                    m_equ[0]->CopyToPhysField(i, qAvr[i]);   
                    m_equ[0]->TransPhysToCoeff();
                }
            }
            
            m_equ[0]->Output();
            
            // Evaluate and output computation time and solution accuracy.
            // The specific format of the error output is essential for the
            // regression tests to work.
            // Evaluate L2 Error
            for(int i = 0; i < m_equ[0]->GetNvariables(); ++i)
            {
                NekDouble vL2Error = m_equ[0]->L2Error(i,false);
                NekDouble vLinfError = m_equ[0]->LinfError(i);
                if (m_comm->GetRank() == 0)
                {
                    out << "L 2 error (variable " << m_equ[0]->GetVariable(i) << ") : " << vL2Error << endl;
                    out << "L inf error (variable " << m_equ[0]->GetVariable(i) << ") : " << vLinfError << endl;
                }
            }
        }
        
        
        void DriverSteadyState::ExactFilters(const int i,
                                        const Array<OneD, const Array<OneD, NekDouble> > &q0,
                                        const Array<OneD, const Array<OneD, NekDouble> > &qBar0,
                                        Array<OneD, Array<OneD, NekDouble> > &q1,
                                        Array<OneD, Array<OneD, NekDouble> > &qBar1)
        {
            q1[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(),0.0);
            qBar1[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(),0.0);
            
            //Exact solution of the Filters equation            
            Vmath::Svtvp(q1[i].num_elements(), F11, q0[i], 1, q1[i], 1, q1[i], 1 );    
            Vmath::Svtvp(q1[i].num_elements(), F12, qBar0[i], 1, q1[i], 1, q1[i], 1 ); 
            
            Vmath::Svtvp(qBar1[i].num_elements(), F21, q0[i], 1, qBar1[i], 1, qBar1[i], 1 );    
            Vmath::Svtvp(qBar1[i].num_elements(), F22, qBar0[i], 1, qBar1[i], 1, qBar1[i], 1 );             
        }
        
        
        void DriverSteadyState::RunBDF1(const int i,
                                        const Array<OneD, const Array<OneD, NekDouble> > &q0,
                                        const Array<OneD, const Array<OneD, NekDouble> > &qBar0,
                                        Array<OneD, Array<OneD, NekDouble> > &q1,
                                        Array<OneD, Array<OneD, NekDouble> > &qBar1)
        {
            q1[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(),0.0);
            qBar1[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(),0.0);
            
            //BDF1            
            Vmath::Svtvp(q1[i].num_elements(), a11, q0[i], 1, q1[i], 1, q1[i], 1 );    
            Vmath::Svtvp(q1[i].num_elements(), a12, qBar0[i], 1, q1[i], 1, q1[i], 1 ); 
            
            Vmath::Svtvp(qBar1[i].num_elements(), a21, q0[i], 1, qBar1[i], 1, qBar1[i], 1 );    
            Vmath::Svtvp(qBar1[i].num_elements(), a22, qBar0[i], 1, qBar1[i], 1, qBar1[i], 1 );             
        }
        
        void DriverSteadyState::RunBDF2(const int i,
                                        const Array<OneD, const Array<OneD, NekDouble> > &qMinus1,
                                        const Array<OneD, const Array<OneD, NekDouble> > &qBarMinus1,
                                        const Array<OneD, const Array<OneD, NekDouble> > &q0,
                                        const Array<OneD, const Array<OneD, NekDouble> > &qBar0,
                                        Array<OneD, Array<OneD, NekDouble> > &q1,
                                        Array<OneD, Array<OneD, NekDouble> > &qBar1)
        {
            q1[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(),0.0);
            qBar1[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(),0.0);
            
            //BDF2            
            Vmath::Svtvp(q1[i].num_elements(), b11, q0[i], 1, q1[i], 1, q1[i], 1 );    
            Vmath::Svtvp(q1[i].num_elements(), b12, qBar0[i], 1, q1[i], 1, q1[i], 1 ); 
            Vmath::Svtvp(q1[i].num_elements(), b13, qMinus1[i], 1, q1[i], 1, q1[i], 1 ); 
            Vmath::Svtvp(q1[i].num_elements(), b14, qBarMinus1[i], 1, q1[i], 1, q1[i], 1 ); 
            
            Vmath::Svtvp(qBar1[i].num_elements(), b21, q0[i], 1, qBar1[i], 1, qBar1[i], 1 );    
            Vmath::Svtvp(qBar1[i].num_elements(), b22, qBar0[i], 1, qBar1[i], 1, qBar1[i], 1 ); 
            Vmath::Svtvp(qBar1[i].num_elements(), b23, qMinus1[i], 1, qBar1[i], 1, qBar1[i], 1 ); 
            Vmath::Svtvp(qBar1[i].num_elements(), b24, qBarMinus1[i], 1, qBar1[i], 1, qBar1[i], 1 ); 
        }
        
        void DriverSteadyState::EvaluateNextSFDVariables(const int i,
                                                         const Array<OneD, const Array<OneD, NekDouble> > &q0,
                                                         const Array<OneD, const Array<OneD, NekDouble> > &qBar0,
                                                         Array<OneD, Array<OneD, NekDouble> > &q1,
                                                         Array<OneD, Array<OneD, NekDouble> > &qBar1)
        {
            q1[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(),0.0);
            qBar1[i] = Array<OneD, NekDouble> (m_equ[0]->GetTotPoints(),0.0);
            
            //Implicit formulation -
            Vmath::Smul(qBar1[i].num_elements(), m_cst4, q0[i], 1, qBar1[i], 1);
            Vmath::Vadd(qBar1[i].num_elements(), qBar0[i], 1, qBar1[i], 1, qBar1[i], 1);
            Vmath::Smul(qBar1[i].num_elements(), m_cst5, qBar1[i], 1, qBar1[i], 1);
            
            Vmath::Smul(q1[i].num_elements(), m_cst1, qBar1[i], 1, q1[i], 1);
            Vmath::Vadd(q1[i].num_elements(), q0[i], 1, q1[i], 1, q1[i], 1);
            Vmath::Smul(q1[i].num_elements(), m_cst2, q1[i], 1, q1[i], 1);
            
            
            //Explicit formulation -
            /*Vmath::Svtvm(q1[i].num_elements(), 1.0, qBar0[i], 1, q0[i], 1, q1[i], 1); //q1 = 1.0*qbar0 - q0
             *            Vmath::Smul(q1[i].num_elements(), m_cst1, q1[i], 1, q1[i], 1); //q1 = X*dt*q1
             *            Vmath::Vadd(q1[i].num_elements(), q0[i], 1, q1[i], 1, q1[i], 1); // q1 = q0 + q1
             *            
             *            Vmath::Svtvm(q1[i].num_elements(), 1.0, q0[i], 1, qBar0[i], 1, qBar1[i], 1);
             *            Vmath::Smul(q1[i].num_elements(), m_cst3, qBar1[i], 1, qBar1[i], 1);
             *            Vmath::Vadd(q1[i].num_elements(), qBar0[i], 1, qBar1[i], 1, qBar1[i], 1);*/
            
        }
        
        
        void DriverSteadyState::ConvergenceHistory(const Array<OneD, const Array<OneD, NekDouble> > &qBar1,
                                                   const Array<OneD, const Array<OneD, NekDouble> > &q0,
                                                   NekDouble &MaxNormDiff_q_qBar,
                                                   NekDouble &MaxNormDiff_q1_q0)
        {
            //This routine evaluates |q-qBar|_L2 and save the value in "ConvergenceHistory.txt"
            
            Array<OneD, NekDouble > NormDiff_q_qBar(NumElmVelocity, 1.0);
            Array<OneD, NekDouble > NormDiff_q1_q0(NumElmVelocity, 1.0);
            
            MaxNormDiff_q_qBar=0.0;
            MaxNormDiff_q1_q0=0.0;
            
            //Norm Calculation
            for(int i = 0; i < NumElmVelocity; ++i)
            {
                //To check convergence of SFD
                //NormDiff_q_qBar[i] = m_equ[0]->L2Error(i, qBar1[i], false);
                NormDiff_q_qBar[i] = m_equ[0]->LinfError(i, qBar1[i]);
                
                //To check convergence of Navier-Stokes
                NormDiff_q1_q0[i] = m_equ[0]->LinfError(i, q0[i]);
                
                if (MaxNormDiff_q_qBar < NormDiff_q_qBar[i])
                {
                    MaxNormDiff_q_qBar = NormDiff_q_qBar[i];
                }
                
                if (MaxNormDiff_q1_q0 < NormDiff_q1_q0[i])
                {
                    MaxNormDiff_q1_q0 = NormDiff_q1_q0[i];
                }
            }
            
            #if NEKTAR_USE_MPI   
            MPI_Comm_rank(MPI_COMM_WORLD,&MPIrank);
            if (MPIrank==0)
            {
                //cout << "SFD - Step: " << m_n+1 << "; Time: " << m_equ[0]->GetFinalTime() <<  "; |q-qBar|L2 = " << MaxNormDiff_q_qBar <<endl;
                cout << "SFD (MPI) - Step: " << m_n+1 << "; Time: " << m_equ[0]->GetFinalTime() <<  "; |q-qBar|inf = " << MaxNormDiff_q_qBar << "; |q1-q0|inf = " << MaxNormDiff_q1_q0 << ";\t for X_tilde = " << m_X0 <<" and Delta_tilde = " << m_Delta0 <<endl;
                std::ofstream m_file( "ConvergenceHistory.txt", std::ios_base::app); 
                m_file << m_n+1 << "\t" << m_equ[0]->GetFinalTime() << "\t" << MaxNormDiff_q_qBar << "\t" << MaxNormDiff_q1_q0 << endl;
                m_file.close();
            }
            #else
            //cout << "SFD - Step: " << m_n+1 << "; Time: " << m_equ[0]->GetFinalTime() <<  "; |q-qBar|L2 = " << MaxNormDiff_q_qBar <<endl;
            cout << "SFD - Step: " << m_n+1 << "; Time: " << m_equ[0]->GetFinalTime() <<  "; |q-qBar|inf = " << MaxNormDiff_q_qBar << "; |q1-q0|inf = " << MaxNormDiff_q1_q0 << ";\t for X_tilde = " << m_X0 <<" and Delta_tilde = " << m_Delta0 <<endl;
            std::ofstream m_file( "ConvergenceHistory.txt", std::ios_base::app); 
            m_file << m_n+1 << "\t" << m_equ[0]->GetFinalTime() << "\t" << MaxNormDiff_q_qBar << "\t" << MaxNormDiff_q1_q0 << endl;
            m_file.close();
            #endif  
            
            /*if (m_Shrinking==false && MaxNormDiff_q_qBar > m_MaxNormDiff_q_qBar)
             *            {
             *                m_MaxNormDiff_q_qBar=MaxNormDiff_q_qBar;
             *                m_Growing=true;
             *                if (m_MaxNormDiff_q_qBar < m_First_MinNormDiff_q_qBar)
             *                {
             *                    m_Oscillation = 0;
             *                    m_First_MinNormDiff_q_qBar=0;
        }
        }
        if (MaxNormDiff_q_qBar < m_MaxNormDiff_q_qBar && m_Growing==true)
        {
            m_Growing=false;
            m_MaxNormDiff_q_qBar=0;
        }
        if (m_Growing==false && MaxNormDiff_q_qBar < m_MinNormDiff_q_qBar)
        {
            m_MinNormDiff_q_qBar=MaxNormDiff_q_qBar;
            m_Shrinking=true;
            if (m_Oscillation==0)
            {
                m_First_MinNormDiff_q_qBar=m_MinNormDiff_q_qBar;
        }
        }
        if (MaxNormDiff_q_qBar > m_MinNormDiff_q_qBar && m_Shrinking==true)
        {
            m_Shrinking=false;
            m_MinNormDiff_q_qBar=1000;
            m_Oscillation=m_Oscillation+1;
        }
        
        if (m_Oscillation==10)
        {                   
            //m_Delta = m_Delta + 0.25;
            //m_X = 0.99*(1.0/m_Delta);
            
            mult = mult + mult*0.25;
            
            m_X = mult*(coeff - 1.0)/m_dt;
            m_Delta = 100*mult*( ( coeff*m_X*m_dt*m_dt/(1.0+m_X*m_dt-coeff) - m_dt )/(1.0+m_X*m_dt) );
            
            m_cst1=m_X*m_dt;
            m_cst2=1.0/(1.0 + m_cst1);
            m_cst3=m_dt/m_Delta;
            m_cst4=m_cst2*m_cst3;
            m_cst5=1.0/(1.0 + m_cst3*(1.0-m_cst1*m_cst2));
            
            cout << "\nNew Filter Width: Delta = " << m_Delta << "; New Control Coeff: X = " << m_X << "\n" << endl;
            
            m_Oscillation=0;
            //m_equ[0]->DoInitialise();
        }*/
            
            
            //Algo for updating parameters (late 2012)
            /*if (MaxNormDiff_q_qBar < Min_MaxNormDiff_q_qBar)
             *            {
             *                Min_MaxNormDiff_q_qBar = MaxNormDiff_q_qBar;               
        }
        
        if (MaxNormDiff_q_qBar < m_LIM)
        {                   
            m_Delta = m_Delta + m_Delta*0.1;
            m_X = 1.0/m_Delta;
            
            m_cst1=m_X;
            m_cst2=1.0/(1.0 + m_cst1);
            //m_cst3=m_dt/m_Delta;
            m_cst3=1.0/m_Delta;
            m_cst4=m_cst2*m_cst3;
            m_cst5=1.0/(1.0 + m_cst3*(1.0-m_cst1*m_cst2));
            
            cout << "\n New Control Coeff: X = " << m_X << "; New Filter Width: Delta =" << m_Delta << "\n" << endl;
            
            m_LIM = m_LIM/5.0;
            //m_LIM = m_LIM/1000.0;
        }
        
        if (MaxNormDiff_q_qBar > 9999999.0*Min_MaxNormDiff_q_qBar) // It means that the algo has failed to converge
            {        
                Min_MaxNormDiff_q_qBar = 1.0;
                
                m_Delta0 = m_Delta0 + m_Delta0*0.5;
                m_Delta = m_Delta0;
                
                m_X0 = 1.0/m_Delta0; //m_X0 + m_X0*0.1;
                m_X = m_X0;
                
                m_cst1=m_X;
                m_cst2=1.0/(1.0 + m_cst1);
                //m_cst3=m_dt/m_Delta;
                m_cst3=1.0/m_Delta;
                m_cst4=m_cst2*m_cst3;
                m_cst5=1.0/(1.0 + m_cst3*(1.0-m_cst1*m_cst2));
                
                m_LIM = m_LIM0;
                
                cout << "\n THE PROBLEM HAS BEEN REINITIALIZED!!! - New Control Coeff: X = " << m_X << "; New Filter Width: Delta =" << m_Delta << "\n" << endl;
                
                for(int i = 0; i < NumElmVelocity; ++i)
                {
                    m_equ[0]->CopyToPhysField(i, qMin[i]); //We reinitialize the problem with the value where |q-qBar| was minimal 
    }
    m_equ[0]->DoInitialise();
    
    m_Check++;                    
    m_equ[0]->Checkpoint_Output(m_Check);
    
    }*/
            
            
    }
}
}

