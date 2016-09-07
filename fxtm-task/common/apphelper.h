///////////////////////////////////////////////////////////////////////////////
//  Application base class.
//
#ifndef __APPHELP_H__
#define __APPHELP_H__

#pragma once


// Application defines
#ifdef _DEBUG
#define MSG_LEVEL           3 // verbosity level (messages with level below are displayed)
#else
#define MSG_LEVEL           2 // verbosity level (messages with level below are displayed)
#endif // _DEBUG


#ifdef ERR
#undef ERR
#endif

#ifdef MSG
#undef MSG
#endif

#define MSG(_l_, _s_, ...)  if (_l_ < MSG_LEVEL) _tprintf_s(_s_, __VA_ARGS__)
#define ERR(_s_, ...)       _ftprintf_s(stderr, _s_, __VA_ARGS__)

#define PAUSE(_s_) \
        __pragma(warning(push)) \
        __pragma(warning(disable : 4127)) \
        do { _tprintf_s(_s_); _gettch(); } while (0) \
        __pragma(warning(pop))


// Windows socket defaults
#define DEFAULT_FAMILY      AF_UNSPEC       // accept either IPv4 or IPv6
#define DEFAULT_SOCKTYPE    SOCK_DGRAM      // UDP
#define DEFAULT_ADDRESS     NULL            // loop-back interface
#define DEFAULT_PORT        _T("40001")
#define DEFAULT_EXTRA       0               // Number of "extra" bytes to send
#define BUFFER_SIZE         128
#define UNKNOWN_NAME        _T("<unknown>")

#define MAX_ADDRSTRLEN      INET6_ADDRSTRLEN // NI_MAXHOST

// Task related defaults
#define STAT_INTERVAL       200             // statistics update interval, ms
#define KEY_MAXIMUM         1023
#define STORAGE_NAME        _T("task_state.bin")



// forward reference
class CLog;


__interface IExeModule
{
    int LoadPreferences(int argc, LPTSTR argv[]);
    int Connect(void);
    int Disconnect(void);
    int Run(void);
};


template<class T, class Log>
class ATL_NO_VTABLE IExeModuleImpl : public IExeModule
{
public:
    IExeModuleImpl()        
        : m_port(DEFAULT_PORT)
        , m_family(DEFAULT_FAMILY)
        , m_socktype(DEFAULT_SOCKTYPE)
        , m_buffsize(BUFFER_SIZE)
        , m_bConnected(false)
    {
    };

    ~IExeModuleImpl() throw() {};


    // IExeModule
    int LoadPreferences(int argc, LPTSTR argv[])
    {
#ifdef _CONSOLE
        _tsetlocale(0 /* LC_ALL */, _T(".OCP"));
#endif // _CONSOLE

        T* pT = static_cast<T*>(this);
        int err = pT->ReadConfig();

        if (err != ERROR_SUCCESS) return err;

        return pT->ParseArgs(argc, argv);
    }
    int Connect(void) throw()
    {
        ATLTRACENOTIMPL(_T("IExeModuleImpl::Connect()"));
    }
    int Disconnect(void) throw()
    {
        ATLTRACENOTIMPL(_T("IExeModuleImpl::Disconnect()"));
    }
    int Run(void) throw()
    {
        ATLTRACE(_T("IExeModuleImpl::Run() must be implemented.\n"));
        ATLASSERT(false);
        return 0;
    }


protected:
    // helper methods
    int DisplayError(LPCTSTR msg, int err = ::WSAGetLastError()) throw()
    {
        return Log::Error(msg, err);
    }

    int DisplayHelp(LPCTSTR arg0) throw()
    {
        CPath name(arg0);
        name.StripPath();
        name.RemoveExtension();

        MSG(0, _T("\nUsage: %s [-f 4|6] [-p port] [-s addr] [-t udp|tcp] [-b size]"), (LPCTSTR)name);
#ifdef _CLIENT_BUILD_ // client only parameters
        MSG(0, _T(" [-n number] [-d ticks] [-i id] [-w time]\n"));
#else
        MSG(0, _T("\n"));
#endif
        MSG(0, _T("  -f 4|6     Address family, 4 = IPv4, 6 = IPv6 (default: both)\n"));
        MSG(0, _T("  -p port    Port number (default: %s)\n"), DEFAULT_PORT);
        MSG(0, _T("  -s addr    Server ip address (default: INADDR_ANY,INADDR6_ANY)\n"));
        MSG(0, _T("  -t tcp|udp Transport protocol to use (default: UDP)\n"));
        MSG(0, _T("  -b size    Buffer size for send/recv (default: %u)\n"), BUFFER_SIZE);
#ifdef _CLIENT_BUILD_ // client only parameters
        MSG(0, _T("  -n number  Number of requests to perform. (default: 1, 0 - infinite)\n"));
        MSG(0, _T("  -d ticks   Delay in ms between server requests. (default: 0)\n"));
        MSG(0, _T("  -i id      Unique client id\n"));
        MSG(0, _T("  -w time    Working time in seconds. (default: 0)\n"));
#endif
        return ERROR_INVALID_PARAMETER;
    }

    LPCTSTR PStr(int type) const throw()
    {
        return (type == SOCK_DGRAM) ? _T("UDP") : _T("TCP");
    }
    LPCTSTR FStr(int family) const throw()
    {
        switch (family)
        {
        case AF_INET:
            return _T("AF_INET");
        case AF_INET6:
            return _T("AF_INET6");
        default:
            return _T("AF_UNSPEC");
        }
    }


    // overridables

    bool GetIniFile(CPath& path) throw()
    {
        TCHAR buf[_MAX_PATH] = {0};
        if (::GetModuleFileName(NULL, buf, _MAX_PATH))
        {
            path = buf;
            path.RenameExtension(_T(".ini"));
            return !!path.FileExists();
        }
        return false;
    }

    int ReadConfig(void) throw()
    {
        return ERROR_SUCCESS;
    }

    int ParseArgs(int argc, LPTSTR argv[])
    {
        T* pT = static_cast<T*>(this);
        DBG_UNREFERENCED_LOCAL_VARIABLE(pT);

        for (int i = 1; i < argc; ++i)
        {
            TCHAR c = argv[i][0];

            if (c != _T('-') && c != _T('/')) continue;

            switch (_totlower(argv[i++][1]))
            {
            case 'f':
                if (i < argc)
                {
                    if (_tcsnicmp(argv[i], _T("4"), 1) == 0)
                    {
                        m_family = AF_INET; break; 
                    }
                    else if (_tcsnicmp(argv[i], _T("6"), 1) == 0)
                    {
                        m_family = AF_INET6; break;
                    }
                }
                return DisplayHelp(argv[0]);

            case 't':
                if (i < argc)
                {
                    if (_tcsnicmp(argv[i], _T("TCP"), 3) == 0)
                    {
                        m_socktype = SOCK_STREAM; break;
                    }
                    else if (_tcsnicmp(argv[i], _T("UDP"), 3) == 0)
                    {
                        m_socktype = SOCK_DGRAM; break;
                    }
                }
                return DisplayHelp(argv[0]);

            case 'p':
                if (i < argc && argv[i][0] != _T('-') && argv[i][0] != _T('/'))
                {
                    m_port = argv[i]; break;
                }
                return DisplayHelp(argv[0]);

            case 's':
                if (i < argc && argv[i][0] != _T('-') && argv[i][0] != _T('/'))
                {
                    m_addr = argv[i]; break;
                }
                return DisplayHelp(argv[0]);

            case 'b':
                if (i < argc)
                {
                    int n = _tstoi(argv[i]);
                    if (errno == 0)
                    {
                        m_buffsize = n; break;
                    }
                }
                return DisplayHelp(argv[0]);

#ifdef _CLIENT_BUILD_ // client only parameters
            case 'd':
                if (i < argc)
                {
                    int n = _tstoi(argv[i]);
                    if (errno == 0)
                    {
                        pT->m_delaytik = n; break;
                    }
                }
                return DisplayHelp(argv[0]);

            case 'i':
                if (i < argc)
                {
                    int n = _tstoi(argv[i]);
                    if (errno == 0)
                    {
                        pT->m_clientid = n; break;
                    }
                }
                return DisplayHelp(argv[0]);

            case 'n':
                if (i < argc)
                {
                    int n = _tstoi(argv[i]);
                    if (errno == 0)
                    {
                        pT->m_requests = n; break;
                    }
                }
                return DisplayHelp(argv[0]);

            case 'w':
                if (i < argc)
                {
                    int n = _tstoi(argv[i]);
                    if (errno == 0)
                    {
                        pT->m_worktime = n; break;
                    }
                }
                return DisplayHelp(argv[0]);
#endif // _CLIENT_BUILD_
            case '?':
                return DisplayHelp(argv[0]);

            default:
                break;
            } // switch

        } // for(i)

        return ERROR_SUCCESS;
    }


protected:
    bool m_bConnected;

    CString m_addr;
    CString m_port;

    int m_family;
    int m_socktype;
    int m_buffsize;

private:

};



//////////////////////////////////////////////////////////////////////////
// Helper classes

template<class T>
class CFactorySingleton
{
protected:
    CFactorySingleton() {}
    ~CFactorySingleton() {}

public:
    static T& Instance()
    {
#if (_MSC_VER >= 1900)
        static T m_inst;

        return m_inst;
#else
    #error 'magic statics (§6.7 [stmt.dcl] p4) supported in Visual C++ 14.0
#endif
    }

};


class CLog
{
public:
    static LPCTSTR Error(int err, CString& str) throw()
    {
        LPTSTR buf = NULL;
        DWORD lcid = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        DWORD flag = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER;
        if (::FormatMessage(flag, NULL, (DWORD)err, lcid, (LPTSTR)&buf, 0, NULL))
        {
            str = buf;
            str.Remove(_T('\n'));
            str.Remove(_T('\r'));
        }

        if (buf) { ::LocalFree(buf); }

        return str;
    }

    static int Error(LPCTSTR msg, int err) throw()
    {
        CString s;
        ERR(_T("%s error code=%i : \"%s\"\n\n"), msg, err, Error(err, s));
        return err;
    }
};


//////////////////////////////////////////////////////////////////////////
// Ctrl+C handler helper class

class CCtrlHandler
{
public:
    CCtrlHandler()
        : m_pHR(NULL)
    {
        // causes the calling process to ignore CTRL+C input
        ::SetConsoleCtrlHandler(m_pHR, TRUE);
    }
    CCtrlHandler(PHANDLER_ROUTINE handler, bool bWarn = true)
        : m_pHR(handler)
    {
        ::SetConsoleCtrlHandler(m_pHR, TRUE);

        if (bWarn)
        {
            MSG(0, _T("\n** Press Ctrl+C to terminate\n\n"));
        }
    }
    virtual ~CCtrlHandler()
    {
        ::SetConsoleCtrlHandler(m_pHR, FALSE);
    }

private:
    PHANDLER_ROUTINE m_pHR;
};


//////////////////////////////////////////////////////////////////////////
// 
// @WARNING: reading of aligned field is atomic. synchronization is not required.
// see Intel 325462, 8.1.1 Guaranteed Atomic Operations

class CQuit
{
public:
    bool yes(void) const throw() { return ms_bQuit; }
    bool run(void) const throw() { return !yes(); }
    // @WARNING: writing from the only source..
    static void set(void) throw() { ms_bQuit = true; }

private:
    static bool ms_bQuit;
};

__declspec(selectany) bool CQuit::ms_bQuit = false;



#ifdef _CLIENT_BUILD_


#ifndef _CHRONO_
#include <chrono>
#endif

#ifndef _RANDOM_
#include <random>
#endif


//////////////////////////////////////////////////////////////////////////
// generates uniformly distributed random integer values in the range [0-1023]
//

class RandomGenerator
{
    static const int M = KEY_MAXIMUM;
public:
    RandomGenerator()
        : distribution(0, M)
        , generator((int)std::chrono::system_clock::now().time_since_epoch().count())
    {
    }

    int number(void) throw()
    {
        return distribution(generator);
    }
    int min(void) const
    {
        return distribution.min();
    }
    int max(void) const
    {
        return distribution.min();
    }

private:
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution;
};


#endif // _CLIENT_BUILD_


#endif // __APPHELP_H__

