#include "IntWrapper.h"
#include <boost/lexical_cast.hpp>
#include <LibUtilities/LinearAlgebra/NekMatrix.hpp>
#include <Profile/StringConcat.h>
#include <Profile/StringConcatExprTemp.h>
#include <boost/timer.hpp>
#include <boost/progress.hpp>
#include "VectorOps.h"
#include "VectorOpsExprTemp.h"
#include <iostream>


using namespace std;

void Run2VectorTests(unsigned int numTests, int stringSize)
{
    boost::timer t;
    double elapsedTime = 0.0;
    
    Nektar::NekVector<double> strVals[] = { Nektar::NekVector<double>(stringSize), 
        Nektar::NekVector<double>(stringSize),
        Nektar::NekVector<double>(stringSize),
        Nektar::NekVector<double>(stringSize)};
        
    Nektar::NekVector<double> result(stringSize);
    
    cout << "2 Vector Addition Tests" << endl;
    cout << "---------------------" << endl;
    t.restart();
    for(unsigned int i = 0; i < numTests; ++i)
    {
       AddVectors(result, strVals[0], strVals[1]);    
    }
    elapsedTime = t.elapsed();
    cout << "Straight Addition - Total : " << elapsedTime << endl;
    cout << "Straight Addition - PerOp : " << elapsedTime/(double)numTests << endl;
    cout << result[0] << endl;
    
    t.restart();
    for(unsigned int i = 0; i < numTests; ++i)
    {
        AddVectorsAccum(result, strVals[0], strVals[1]);
    }
    elapsedTime = t.elapsed();
    cout << "Manual Accumulator - Total : " << elapsedTime << endl;
    cout << "Manual Accumulator - PerOp : " << elapsedTime/(double)numTests << endl;
    cout << result[1] << endl;
    t.restart();
    for(unsigned int i = 0; i < numTests; ++i)
    {
        AddVectorsExprTemp(result, strVals[0], strVals[1]);
    }
    elapsedTime = t.elapsed();
    cout << "Expression Templates - Total : " << elapsedTime << endl;
    cout << "Expression Templates - PerOp : " << elapsedTime/(double)numTests << endl;
    cout << result[2] << endl;

}

void Run2StringTests(unsigned int numTests, int stringSize)
{
    boost::timer t;
    double elapsedTime = 0.0;
    
    std::string strVals[] = { std::string(stringSize, 'a'), std::string(stringSize, 'a'),
                              std::string(stringSize, 'a'), std::string(stringSize, 'a')};
    std::string result;
    
    cout << "2 String Addition Tests" << endl;
    cout << "---------------------" << endl;
    t.restart();
    for(unsigned int i = 0; i < numTests; ++i)
    {
       AddStrings(result, strVals[0], strVals[1], strVals[2], strVals[3]);    
    }
    elapsedTime = t.elapsed();
    cout << "Straight Addition - Total : " << elapsedTime << endl;
    cout << "Straight Addition - PerOp : " << elapsedTime/(double)numTests << endl;
    
    t.restart();
    for(unsigned int i = 0; i < numTests; ++i)
    {
        AddStringsAccum(result, strVals[0], strVals[1], strVals[2], strVals[3]);
    }
    elapsedTime = t.elapsed();
    cout << "Manual Accumulator - Total : " << elapsedTime << endl;
    cout << "Manual Accumulator - PerOp : " << elapsedTime/(double)numTests << endl;
    
    t.restart();
    for(unsigned int i = 0; i < numTests; ++i)
    {
        AddStringsExprTemp(result, strVals[0], strVals[1], strVals[2], strVals[3]);
    }
    elapsedTime = t.elapsed();
    cout << "Expression Templates - Total : " << elapsedTime << endl;
    cout << "Expression Templates - PerOp : " << elapsedTime/(double)numTests << endl;
    
    
}

void RunIntWrapperTests(unsigned int numTests)
{
    boost::timer t;
    double elapsedTime = 0.0;
    
    IntWrapper strVals[] = { IntWrapper(1), IntWrapper(2), IntWrapper(3), IntWrapper(4),
                             IntWrapper(5), IntWrapper(6) };
    IntWrapper result;
    
    cout << "Int Wrapper Tests" << endl;
    cout << "---------------------" << endl;
    t.restart();
    for(unsigned int i = 0; i < numTests; ++i)
    {
       AddIntWrapper(result, strVals[0], strVals[1]);    
    }
    elapsedTime = t.elapsed();
    cout << "Straight Addition - Total : " << elapsedTime << endl;
    cout << "Straight Addition - PerOp : " << elapsedTime/(double)numTests << endl;
    
    t.restart();
    for(unsigned int i = 0; i < numTests; ++i)
    {
        AddIntWrapperManualAccum(result, strVals[0], strVals[1]);
    }
    elapsedTime = t.elapsed();
    cout << "Manual Accumulator - Total : " << elapsedTime << endl;
    cout << "Manual Accumulator - PerOp : " << elapsedTime/(double)numTests << endl;
    
    t.restart();
    for(unsigned int i = 0; i < numTests; ++i)
    {
        AddIntWrapperExprTemp(result, strVals[0], strVals[1]);
    }
    elapsedTime = t.elapsed();
    cout << "Expression Templates - Total : " << elapsedTime << endl;
    cout << "Expression Templates - PerOp : " << elapsedTime/(double)numTests << endl;
    
    
}

int main(int argc, char** argv)
{
    if( argc != 3 )
    {
        std::cerr << "Usage: perf <ProblemSize> <NumTests>" << std::endl;
        return 1;
    }
    
    unsigned int n = boost::lexical_cast<unsigned int>(argv[1]);
    unsigned int numTests = boost::lexical_cast<unsigned int>(argv[2]);
    
    //Run2StringTests(numTests, n);
    //RunIntWrapperTests(numTests);
    Run2VectorTests(numTests, n);
    return 0;
}
