// wsclient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define _CLIENT_BUILD_

#include "apphelper.h"

// @TODO: testing..
#include <iostream>
#include <iomanip>


using namespace std;
using namespace WSA;


// generates uniformly distributed random integer values in the range [0-1023]
//
template<typename T = int>
class RandomGenerator
{
    static const T M = KEY_MAXIMUM;
public:
    RandomGenerator()
        : distribution(0, M)
        , generator((int)chrono::system_clock::now().time_since_epoch().count())
    {
    }

    T number(void) throw()
    {
        return distribution(generator);
    }
    T lo(void) const
    {
        return distribution.a();
    }
    T hi(void) const
    {
        return distribution.b();
    }

private:
    default_random_engine generator;
    uniform_int_distribution<T> distribution;
};


// CExeModule

class CExeModule : public IExeModuleImpl<CExeModule, CLog> // @TODO: km 20160705 - move into header file..
{

};

CExeModule _Module;     // reserved name, DO NOT change


int _tmain(int argc, LPTSTR argv[])
{
    int nRet = 0;

    try
    {
        MSG(0, _T("greetings from client!\n\n"));

        // @TODO: testing..
        RandomGenerator<int> rg;

        cout << "uniform_int_distribution ("<< rg.lo() << ", " << rg.hi() << "):" << endl;

        for (int i = 0; i < 20; ++i)
        {
            for (int ii = 0; ii < 15; ++ii)
            {
                int n = rg.number();
                cout << setw(5) << n;
            }
            cout << endl;
        }


        nRet = _Module.LoadPreferences(argc, argv);
        if (nRet == ERROR_SUCCESS)
        {
            CWsaInitialize _;

            _Module.Connect();

        }

    }
    catch (int n) { return n; }
    catch (std::exception& e)
    {
        ERR(_T("exception caught: %S\n"), e.what()); // !! ASCII
    }
    catch (...)
    {
        ERR(_T("exception caught: unspecified\n"));
    }

    return nRet;
}

