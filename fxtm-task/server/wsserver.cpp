// wsserver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define _SERVER_BUILD_

#include "apphelper.h"

using namespace std;
using namespace WSA;

#include "Task.h"
#include "Server.h"



// CExeModule

class CExeModule
    : public IExeModuleImpl<CExeModule, CLog>
    , public CStatManager<STAT_INTERVAL>
    , public CQuit
{
    static const DWORD WATCHER_WAIT = 5000; // ms
    static const DWORD WORKER_WAIT  = 50;

public:
    CExeModule()
        : m_bCircular(false)
        , m_MaxConnect(MAXIMUM_WAIT_OBJECTS)
        , m_pServer(NULL)
    {
        m_bWorker = (++ms_instcnt > 0);
    }


    // IExeModule
    int Connect(void) throw()
    {
        int err = ERROR_SUCCESS;

        if (!m_bConnected)
        {
            try
            {
                switch (m_socktype)
                {
                case SOCK_STREAM:
                    m_pServer = static_cast<IServer*>(new CServerTcp);
                    break;
                case SOCK_DGRAM:
                    m_pServer = static_cast<IServer*>(new CServerUdp);
                    break;
                default:
                    return ERROR_UNSUPPORTED_TYPE;
                    break;
                }

                CAddrInfo ai(m_port, m_socktype, m_family);

                err = m_pServer->Connect(ai);
                if (err == ERROR_SUCCESS)
                {
                    m_bConnected = true;
                }
            }
            catch (int e)
            {
                err = DisplayError(_T("Connect() failed."), e);
            }
            catch (std::bad_alloc& e)
            {
                ERR(_T("bad_alloc caught: %s\n"), (LPCTSTR)CString(e.what()));
                err = ERROR_OUTOFMEMORY;
            }
        }
        return err;
    }

    int Disconnect(void) throw()
    {
        if (m_bConnected)
        {
            DisplayData(true); // display statistics..

            if (m_pServer->Disconnect() == ERROR_SUCCESS)
            {
                m_bConnected = false;
            }
#ifdef _DEBUG
            ::Sleep(100);
#endif
        }
        if (m_pServer) { m_pServer->Destroy(); m_pServer = NULL; }

        return ERROR_SUCCESS;
    }

    int Run(void) throw()
    {
        // Set Ctrl+C, Ctrl+Break handler
        CCtrlHandler ctrl(_CtrlHandler);

        int err = m_pServer->Run();
        if (err == ERROR_SUCCESS)
        {
            while (CQuit::run())
            {
                DisplayData(false); // display statistics..

                ::Sleep(WORKER_WAIT);

                ++ms_tickcnt; // I'm alive..
            }
        }
        return err;
    }

    long InstanceCount(void) const throw() { return ms_instcnt; }

    bool IsWorker(void) const throw()
    {
#ifdef _DEBUG // @TODO: km 20160818 - testing..
        return true;
#else
        return m_bWorker;
#endif
    }
    bool BackupTaskState(void) throw()
    {
        CPath path;
        bool bRet = false;

        if (GetIniFile(path))
        {
            CString temp;
            bRet = true;

            // get storage name
            if (GetConfigString(temp, path, _T("storage"), STORAGE_NAME))
            {
                path.RemoveFileSpec();
                path.Append(temp);

                if (path.FileExists())
                {
                    temp = (LPCTSTR)path; // store existing
                    path.RenameExtension(_T(".bak"));
                    bRet = (::MoveFileEx(temp, path, MOVEFILE_REPLACE_EXISTING) != 0);
                } 
            }
        }
        return bRet;
    }
    bool CreateWorkerProcess(PROCESS_INFORMATION& pi) throw()
    {
        STARTUPINFO si = {0};
        si.cb = sizeof(si);
        // Create worker process.
        return (::CreateProcess(NULL, ::GetCommandLine(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) != 0);
        // @TODO: km 20160824 - parse the command line and run dedicated processes for each protocol (family).
        // surviving assistance scheme must be reworked for multiple workers..
    }
    bool WatchWorkerProcess(PROCESS_INFORMATION& pi) throw()
    {
        // Set Ctrl+C, Ctrl+Break handler
        CCtrlHandler ctrl(_CtrlHandler, false);

        LONG watchtickcnt = ms_tickcnt; // remember worker process ticks

        while (::WaitForSingleObject(pi.hProcess, WATCHER_WAIT) != WAIT_OBJECT_0)
        {
            DWORD code = 0;
            if (!::GetExitCodeProcess(pi.hProcess, &code) || STILL_ACTIVE != code)
            {
                break;
            }

            if (ms_tickcnt != watchtickcnt)
            {
                watchtickcnt = ms_tickcnt;

                MSG(2, _T("Watcher: Worker alive...\n"));
            }
            else if (CQuit::run())
            {
                // terminate worker process and run another one..
                BOOL b = ::TerminateProcess(pi.hProcess, ERROR_GEN_FAILURE);
                DBG_UNREFERENCED_LOCAL_VARIABLE(b); // @TODO: km 20160824 - TBD..

                MSG(0, _T("Watcher: Worker seems dead. New one is spawned.\n\n"));
                ERR(_T("** Worker respawned.\n"));
            }
        }
        ::CloseHandle(pi.hThread);
        ::CloseHandle(pi.hProcess);

        return CQuit::yes();
    }

public:
    // overridables

    int ReadConfig(void) throw()
    {
        CPath path;

        if (GetIniFile(path))
        {
            int f = ::GetPrivateProfileInt(_T("server"), _T("family"), 0, path);
            m_family = (f == 4)? AF_INET : (f == 6)? AF_INET6 : DEFAULT_FAMILY;

            m_bCircular  = !!::GetPrivateProfileInt(_T("server"), _T("circular"), 0, path);
            m_MaxConnect = ::GetPrivateProfileInt(_T("server"), _T("maxconnect"), m_MaxConnect, path);

            CString buf;

            if (GetConfigString(buf, path, _T("ipaddress"))) { m_addr = buf; }

            if (GetConfigString(buf, path, _T("port"))) { m_port = buf; }

            if (GetConfigString(buf, path, _T("protocol")))
            {
                if (buf.CompareNoCase(_T("TCP")) == 0)
                {
                    m_socktype = SOCK_STREAM;
                }
                else if (buf.CompareNoCase(_T("UDP")) == 0)
                {
                    m_socktype = SOCK_DGRAM;
                }
            }

            // get storage name and interval
            CTask& task = CFactorySingleton<CTask>::Instance();

            UINT interval = ::GetPrivateProfileInt(_T("server"), _T("interval"), 0, path);

            if (interval > 0) { task.SetInterval(interval); }

            if (GetConfigString(buf, path, _T("storage"), STORAGE_NAME))
            {
                path.RemoveFileSpec();
                path.Append(buf);

                task.LoadState(path);
            }
        }

        return ERROR_SUCCESS;
    }

protected:
    // helper methods
    bool GetConfigString(CString& buf, LPCTSTR path, LPCTSTR key, LPCTSTR dflt = 0, LPCTSTR app = _T("server")) throw()
    {
        DWORD cch, len = 16;

        do 
        {
            len *= 2;
            cch = ::GetPrivateProfileString(app, key, dflt, buf.GetBuffer(len), len, path);
        } while (cch == (len - 2));

        if (cch > 0) { buf.ReleaseBuffer(); }

        return (cch > 0);
    }

    static BOOL WINAPI _CtrlHandler(DWORD /*type*/)
    {
        // @TRICKY: reset instance counter to zero..
        if (InterlockedCompareExchange(&ms_instcnt, 0, ms_instcnt) > 0)
        {
            // to avoid ICP race condition
            MSG(0, _T("\nStop signaled. Terminating...\n"));
        }
        CQuit::set();

        return TRUE;
    }


protected:

private:
    bool m_bWorker;
    bool m_bCircular;   // the oldest message in the queue should be eliminated in order to accommodate
                        // the newly arrived message. (UDP server)
    UINT m_MaxConnect;  // maximum concurrent connections (TCP server)

    IServer* m_pServer;

    static long volatile ms_instcnt;
    static long volatile ms_tickcnt; // worker process ticks
};


#pragma data_seg(".share")

long volatile CExeModule::ms_instcnt = -1;
long volatile CExeModule::ms_tickcnt = -1; // worker process ticks

#pragma data_seg() // with no parameters resets the segment to .data 
#pragma comment(linker, "/section:.share,rws")


CExeModule _Module;     // reserved name, DO NOT change


//////////////////////////////////////////////////////////////////////////
// main entry


int _tmain(int argc, LPTSTR argv[])
{
    int nRet = 0;

    if (_Module.IsWorker())
    {
        MSG(0, _T("Greetings from Worker! [%i]\n\n"), _Module.InstanceCount());
    }
    else
    {
        // check for help request or errors..
        nRet = _Module.LoadPreferences(argc, argv);
        if (nRet == ERROR_SUCCESS)
        {
            _Module.BackupTaskState();

            PROCESS_INFORMATION pi = { 0 };

            do
            {
                if (!_Module.CreateWorkerProcess(pi)) { nRet = -1; break; }

            } while (!_Module.WatchWorkerProcess(pi));
        }

        return nRet;
    }

    try
    {
        nRet = _Module.LoadPreferences(argc, argv);
        if (nRet == ERROR_SUCCESS)
        {
            CWsaInitialize _;

            if (_Module.Connect() == ERROR_SUCCESS)
            {
                nRet = _Module.Run();
            }
            _Module.Disconnect();
        }
    }
    catch (int e)
    {
        nRet = CLog::Error(_T("Server failed."), e);
    }
    catch (std::exception& e)
    {
        ERR(_T("Exception caught: %s\n"), (LPCTSTR)CString(e.what()));
    }
    catch (...)
    {
        ERR(_T("Exception caught: unspecified.\n"));
    }

#ifdef _DEBUG
    MSG(0, _T("[%i] worker bye... \n"), _Module.InstanceCount());
    ::Sleep(3000);
#endif
    return nRet;
}
