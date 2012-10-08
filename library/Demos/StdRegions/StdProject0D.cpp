
#include <cstdio>
#include <cstdlib>
#include "StdRegions/StdPointExp.h"

#include "LibUtilities/Foundations/Foundations.hpp"

using namespace Nektar;

// This routine projects a polynomial or trigonmetric functions which 
// has energy in all mdoes of the expansions and report an error.

int main(int argc, char *argv[])
{
  int i,j; 
  int order, nq;
  LibUtilities::PointsType Qtype;
  LibUtilities::BasisType  btype;
  StdRegions::StdExpansion0D  *E;
  Array<OneD,NekDouble> sol;
   
  if(argc != 4)
  {
    fprintf(stderr,"Usage: Project1D Type order nq \n");

    fprintf(stderr,"Where type is an integer value which "
        "dictates the basis as:\n");
    fprintf(stderr,"\t Ortho_A    = 1\n");
    fprintf(stderr,"\t Modified_A = 4\n");
    fprintf(stderr,"\t Fourier    = 7\n");
    fprintf(stderr,"\t Lagrange   = 8\n");
    fprintf(stderr,"\t Legendre   = 9\n"); 
    fprintf(stderr,"\t Chebyshev  = 10\n");
    fprintf(stderr,"\t Monomial   = 11\n");
	fprintf(stderr,"\t FourierSingleMode   = 12\n");
	fprintf(stderr,"\t FourierHalfModeRe   = 13\n");
	fprintf(stderr,"\t FourierHalfModeIm   = 14\n");


 
    fprintf(stderr,"Note type = 1,2,4,5 are for higher dimensional basis\n");

    exit(1);
  }
  
  btype = (LibUtilities::BasisType) atoi(argv[1]);
  

  if(btype == LibUtilities::eNoBasisType)
  {
      NEKERROR(ErrorUtil::efatal,
             "No Basis Type requested");
  }

  // Check to see that only 1D Expansions are used
  if((btype == LibUtilities::eOrtho_B)||(btype == LibUtilities::eOrtho_B)||
     (btype == LibUtilities::eModified_B)||(btype == LibUtilities::eModified_C))
  {
      NEKERROR(ErrorUtil::efatal,
             "This basis is for 2 or 3D expansions");
  }
  
  order =   atoi(argv[2]);
  nq    =   atoi(argv[3]);

  sol = Array<OneD, NekDouble>(nq);
  
 
	if(btype== LibUtilities::eFourier)
	{
		Qtype = LibUtilities::eFourierEvenlySpaced;
	}
	else if(btype== LibUtilities::eFourierSingleMode)
	{
		Qtype = LibUtilities::eFourierSingleModeSpaced;
	}
	else if(btype== LibUtilities::eFourierHalfModeRe  || btype== LibUtilities::eFourierHalfModeIm )
	{
		//Qtype = LibUtilities::eFourierHalfModeSpaced;
		Qtype = LibUtilities::eFourierSingleModeSpaced;

	}
	else
	{
		Qtype = LibUtilities::eGaussLobattoLegendre; 
	}

	
	
  //-----------------------------------------------
  // Define a point expansion based on basis definition
	const LibUtilities::PointsKey Pkey(nq,Qtype);
	const LibUtilities::BasisKey Bkey(btype,order,Pkey);
	E = new StdRegions::StdPointExp(Bkey);

  //-----------------------------------------------

	//----------------------------------------------
  // Define solution to be projected
  Array<OneD,NekDouble> z = Array<OneD,NekDouble>(nq);
  E->GetCoords(z);
	
	if(btype== LibUtilities::eFourier)
	{
		for(i = 0; i < nq; ++i)
		{
			sol[i] = 1.0;
			for(j = 0; j < order/2-1; ++j)
			{
				sol[i] += sin(j*M_PI*z[i]) + cos(j*M_PI*z[i]);
			}
		}
	}
	else if(btype== LibUtilities::eFourierSingleMode)
	{		
		for(i = 0; i < nq; ++i)
		{
			sol[i] = (0.45*sin(M_PI*z[i]) + 0.25*cos(M_PI*z[i]));
		}
		
	}
	else if(btype== LibUtilities::eFourierHalfModeRe)
	{
		for(i = 0; i < nq; ++i)
		{
			sol[i] = (0.45*cos(M_PI*z[i]));
		}
	}
	else if(btype== LibUtilities::eFourierHalfModeIm)
	{
		for(i = 0; i < nq; ++i)
		{
			sol[i] = (0.25*sin(M_PI*z[i]));
		}
	}
	else {
		for(i = 0; i < nq; ++i)
		{
			sol[i] = 0.0;
			for(j = 0; j < order; ++j)
			{
				sol[i] += pow(z[i],j);
			}
		}
	}

  //---------------------------------------------
  // Project onto Expansion 
  E->FwdTrans(sol,E->UpdateCoeffs());
  //---------------------------------------------
	
  cout << "z[0]=  " << z[0] << "   coefficient=   " << E->GetCoeffs()[0] << endl;

  //-------------------------------------------
  // Backward Transform Solution to get projected values
  E->BwdTrans(E->GetCoeffs(),E->UpdatePhys());
  //-------------------------------------------  

  cout << "z[0]=  " << z[0] << "    physical value=  " << E->GetPhys()[0] << endl;

  //--------------------------------------------
  // Calculate L_inf error 
  cout << "L infinity error: " << E->Linf(sol) << endl;
  //--------------------------------------------
 

  //-------------------------------------------

  return 0;    
}
