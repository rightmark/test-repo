// wsclient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define _CLIENT_BUILD_

#include "apphelper.h"

using namespace std;
using namespace WSA;

#include "StatManager.h"
#include "Task.h"


// CExeModule

class CExeModule
    : public IExeModuleImpl<CExeModule, CLog>
    , public CStatManager<STAT_INTERVAL>
    , public CQuit
{

    static const DWORD
        CLIENT_TIMEOUT  = 30000,    // ms
        CLOSE_TIMEOUT   = 5000,
        CLIENT_WAIT     = 10;

public:
    CExeModule()
        : m_bPause(false)
        , m_clientid(0)
        , m_requests(1)
        , m_delaytik(0)
        , m_worktime(0)
    {
    }


    // IExeModule
    int Connect(void)
    {

        if (!m_bConnected)
        {
#ifdef _DEBUG
            MSG(0, _T("Greetings from %s Client!\n\n"), PStr(m_socktype));
#endif
            ERR(_T("\nStarted: %s\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));
            ERR(_T("parameters: ip=%s, port=%s (%s, %s)\n"), (LPCTSTR)m_addr, (LPCTSTR)m_port, PStr(m_socktype), FStr(m_family));
            ERR(_T("requests: %u, delay ticks: %u, working time: %u\n\n"), m_requests, m_delaytik, m_worktime);

            CAddrInfo ai(m_addr, m_port, m_socktype, m_family);

            TCHAR hostname[MAX_ADDRSTRLEN];
            PADDRINFOT pai = ai;

            for (; pai != NULL; pai = pai->ai_next)
            {
                CSocketAsync socket;

                if ((pai->ai_family != AF_INET) && (pai->ai_family != AF_INET6)) continue;

                if (!socket.Create(pai))
                {
                    DisplayError(_T("CSocketAsync.Create() failed."));
                    continue;
                }

                if (::GetNameInfo(pai->ai_addr, (int)pai->ai_addrlen, hostname, _countof(hostname), NULL, 0, NI_NUMERICHOST) != NO_ERROR)
                {
                    _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
                }

                MSG(0, _T("Attempting to connect to: %s\n"), hostname);

                if (socket.Connect(pai))
                {
                    m_ConnSocket = socket;
                    break;
                }

                DisplayError(_T("CSocketAsync.Connect() failed."));

            } // for()

            if (pai == NULL)
            {
                ERR(_T("Fatal error: unable to connect to the server.\n"));
                return ERROR_FATAL_APP_EXIT;
            }

            // socket connection details..

            PSOCKADDR psa = NULL;

            // remote address and port
            if (m_ConnSocket.NameInfo(psa, hostname, _countof(hostname)) != NO_ERROR)
            {
                _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
            }
            MSG(0, _T("Connected to %s, port %u, (%s, %s)\n"), hostname, ntohs(SS_PORT(psa)), PStr(pai->ai_socktype), FStr(pai->ai_family));

            // local address and port the system picked
            SOCKADDR_STORAGE addr;
            int addrlen = sizeof(addr);

            if (getsockname(m_ConnSocket, (PSOCKADDR)&addr, &addrlen) != SOCKET_ERROR)
            {
                LPCTSTR p = InetNtop(pai->ai_family, INETADDR_ADDRESS((PSOCKADDR)&addr), hostname, _countof(hostname));
                MSG(0, _T("Using local address %s, port %u\n"), p ? p : UNKNOWN_NAME, ntohs(SS_PORT(&addr)));
            }

            m_bConnected = true;
        }
        return ERROR_SUCCESS;
    }

    int Disconnect(void) throw()
    {
        if (m_bConnected)
        {
            DisplayData(true); // display statistics..

            m_ConnSocket.Close();
            m_ev.Close();

            m_bConnected = false;
        }
        ERR(_T("\nTerminated: %s\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));

        return ERROR_SUCCESS;
    }

    int Run(void) throw()
    {
        // Set Ctrl+C, Ctrl+Break handler
        CCtrlHandler ctrl(_CtrlHandler);

        if (!m_ev.Create())
        {
            return DisplayError(_T("CSocketEvent.Create() failed."));
        }

        if (!m_ConnSocket.SelectEvents(m_ev, FD_READ | FD_WRITE | FD_CLOSE))
        {
            return DisplayError(_T("CSocketAsync.SelectEvents() failed."));
        }

        DWORD timeout = CLIENT_TIMEOUT / CLIENT_WAIT;
        DWORD timeoutcnt = 0;
        DWORD requestcnt = 0;

        USHORT lkey = 0; // last random key sent..
        UINT64 tick = 0;

        if (m_worktime > 0)
        {
            m_requests = (UINT_MAX - 1);// "infinite"
            tick = ::GetTickCount64() + m_worktime * 1000;
        }

        while (CQuit::run())
        {
            if (m_worktime > 0 && tick < ::GetTickCount64())
            {
                MSG(0, _T("Working time (%i s) is elapsed\n"), m_worktime);
                // wait for FD_CLOSE event on success..
                if (!Shutdown()) break; // immediate non-graceful closure
            }

            DWORD idx = ::WSAWaitForMultipleEvents(1, m_ev, FALSE, CLIENT_WAIT, FALSE);

            if (idx == WSA_WAIT_FAILED)
            {
                return DisplayError(_T("WSAWaitForMultipleEvents() failed."));
            }
            else if (idx == WSA_WAIT_TIMEOUT)
            {
                if (++timeoutcnt < timeout) continue;

                return ERROR_TIMEOUT;
            }
            timeoutcnt = 0; // reset

            WSANETWORKEVENTS ne = {0};
            if (!m_ConnSocket.EnumEvents(m_ev, &ne))
            {
                DisplayError(_T("CSocketAsync.EnumEvents() failed."));
                continue;
            }
            else if (ne.lNetworkEvents & FD_READ)
            {
                MSG(2, _T("FD_READ event fired\n"));

                if (ne.iErrorCode[FD_READ_BIT] != 0)
                {
                    return DisplayError(_T("FD_READ failed."), ne.iErrorCode[FD_READ_BIT]);
                }

                DATABOXT box = {0};

                int len = DATABOX_SIZE_READ;

                if (ReadData(box.buffer, len) == SOCKET_ERROR)
                    return SOCKET_ERROR;

                if (DATABOX_SIZE_READ != len || box.pkg.anchor != ANCHOR)
                    continue; // skip unsupported data

                if (lkey != htons(box.pkg.keyval))
                {
                    ERR(_T("Key mismatch: sent %u, received %u\n"), lkey, htons(box.pkg.keyval));
                }

                if (m_delaytik > 0) { ::Sleep(m_delaytik); }

                if (requestcnt >= m_requests)
                {
                    MSG(0, _T("Request limit (%i) is reached\n"), m_requests);
                    // wait for FD_CLOSE event on success..
                    if (Shutdown()) continue;

                    break; // immediate non-graceful closure
                }

                box.pkg.anchor = ANCHOR;
                box.pkg.keyval = ntohs((USHORT)m_rg.number());

                len = DATABOX_SIZE_SEND;

                if (SendData(box.buffer, len) == SOCKET_ERROR)
                    return SOCKET_ERROR;

                ++requestcnt;

                lkey = htons(box.pkg.keyval);
                MSG(0, _T("Sent %i bytes, key=%u\n"), len, lkey);
            }
            else if (ne.lNetworkEvents & FD_WRITE)
            {
                MSG(2, _T("FD_WRITE event fired\n"));

                if (ne.iErrorCode[FD_WRITE_BIT] != 0)
                {
                    return DisplayError(_T("FD_WRITE failed."), ne.iErrorCode[FD_WRITE_BIT]);
                }

                DATABOXT box;
                box.pkg.anchor = ANCHOR;
                box.pkg.keyval = ntohs((USHORT)m_rg.number()); // @WARNING: a new one random value send.

                int len = DATABOX_SIZE_SEND;

                if (SendData(box.buffer, len) == SOCKET_ERROR)
                    return SOCKET_ERROR;

                ++requestcnt;

                lkey = htons(box.pkg.keyval);
                MSG(0, _T("Sent %i bytes, key=%u\n"), len, lkey);
            }
            else if (ne.lNetworkEvents & FD_CLOSE)
            {
                MSG(2, _T("FD_CLOSE event fired\n"));
                if (ne.iErrorCode[FD_CLOSE_BIT] != 0)
                {
                    return DisplayError(_T("FD_CLOSE failed."), ne.iErrorCode[FD_CLOSE_BIT]);
                }
                MSG(0, _T("Server closed connection\n"));

                break;
            }

            DisplayData(false); // display statistics..

        } // while()

        return ERROR_SUCCESS;
    }


public:
    // overridables

    int ReadConfig(void) throw()
    {
        CPath path;

        if (GetIniFile(path))
        {
            int f = ::GetPrivateProfileInt(_T("client"), _T("family"), 0, path);
            m_family = (f == 4) ? AF_INET : (f == 6) ? AF_INET6 : DEFAULT_FAMILY;

            m_bPause = !!::GetPrivateProfileInt(_T("client"), _T("pause"), m_bPause, path);
            m_delaytik = ::GetPrivateProfileInt(_T("client"), _T("delaytick"), m_delaytik, path);
            m_worktime = ::GetPrivateProfileInt(_T("client"), _T("worktime"), m_worktime, path);

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

        }

        return ERROR_SUCCESS;
    }

protected:
    // helper methods

    int ReadData(char* buf, int& len) throw()
    {
        int bytes = recv(m_ConnSocket, buf, len, 0);
        if (bytes == SOCKET_ERROR)
        {
            if (::WSAGetLastError() != WSAEWOULDBLOCK)
            {
                DisplayError(_T("recv() failed."));
                return SOCKET_ERROR;
            }
            return WSAEWOULDBLOCK;
        }
        if (DATABOX_SIZE_READ == bytes && ((LPDATABOXT)buf)->pkg.anchor == ANCHOR)
        {
            DATABOXT& box = *(LPDATABOXT)buf;
            MSG(0, _T("Received: key=%u, result=%u\n"), htons(box.pkg.keyval), htonl(box.pkg.result));
        }
        else
        {
            // read another type of message..
            CString str(buf, bytes);
            for (;;)
            {
                int cb = recv(m_ConnSocket, buf, len, 0);
                if (cb == SOCKET_ERROR)
                {
                    if (::WSAGetLastError() != WSAEWOULDBLOCK)
                    {
                        DisplayError(_T("recv() failed."));
                        return SOCKET_ERROR;
                    }
                    break;
                }
                str.Append(CString(buf, cb));
                bytes += cb;
            }
            len = bytes;

            MSG(0, _T("Received %i bytes: \"%s\"\n"), bytes, (LPCTSTR)str);
        }
        ReadMoreData(bytes); // statistics..

        return ERROR_SUCCESS;
    }

    int SendData(const char* buf, int& len) throw()
    {
        int bytes = send(m_ConnSocket, buf, len, 0);
        if (bytes == SOCKET_ERROR)
        {
            if (::WSAGetLastError() != WSAEWOULDBLOCK)
            {
                DisplayError(_T("send() failed."));
                return SOCKET_ERROR;
            }
            return WSAEWOULDBLOCK;
        }
        len = bytes;

        SentMoreData(bytes); // statistics..

        return ERROR_SUCCESS;
    }

    bool Shutdown(void) throw()
    {
        char buf[BUFFER_SIZE] = {0};
        int len = sizeof(buf);

        if (m_socktype == SOCK_DGRAM)
        {
            // signal to UDP server we are finishing..
            if (send(m_ConnSocket, buf, 0, 0) == SOCKET_ERROR)
            {
                DisplayError(_T("send() failed."));
            }
        }

        if (m_ConnSocket.Shutdown(SD_SEND))
        {
            if (m_socktype == SOCK_STREAM)
            {
                // Since TCP does not preserve message boundaries, there may still be more data arriving from the server.
                while (recv(m_ConnSocket, buf, len, 0) > 0);

                MSG(2, _T("Done sending\n"));

                return true;
            }
        }
        // do not wait for FD_CLOSE
        return false;
    }

    bool GetConfigString(CString& buf, LPCTSTR path, LPCTSTR key, LPCTSTR dflt = 0, LPCTSTR app = _T("client")) throw()
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
        MSG(0, _T("\nStop signaled. Terminating...\n"));
        ERR(_T("User break.\n"));

        CQuit::set();

        return TRUE;
    }


public:
    bool m_bPause;      // "press any key" on exit..

    UINT m_clientid;
    UINT m_requests;
    UINT m_delaytik;    // delay between requests, ms (client)
    UINT m_worktime;    // total working time, s (client)

private:
    CSocketAsync m_ConnSocket;
    CSocketEvent m_ev;

    RandomGenerator m_rg;
};

CExeModule _Module;     // reserved name, DO NOT change


int _tmain(int argc, LPTSTR argv[])
{
    int nRet = 0;

    try
    {

        nRet = _Module.LoadPreferences(argc, argv);
        if (nRet == ERROR_SUCCESS)
        {
            CWsaInitialize _;

            do 
            {
                nRet = _Module.Connect();
                if (nRet == ERROR_SUCCESS)
                {
                    nRet = _Module.Run();
                }
                _Module.Disconnect();

            } while (nRet == ERROR_TIMEOUT);
        }
    }
    catch (int e)
    {
        nRet = CLog::Error(_T("Client failed."), e);
    }
    catch (std::exception& e)
    {
        ERR(_T("Exception caught: %s\n"), (LPCTSTR)CString(e.what()));
    }
    catch (...)
    {
        ERR(_T("Exception caught: unspecified.\n"));
    }

    if (_Module.m_bPause)
    {
        PAUSE(_T("\npress a key to exit...\n"));
    }

    return nRet;
}

