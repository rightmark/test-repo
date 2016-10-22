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
        , m_connects(1)
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
            ERR(_T("connections: %u, requests: %u, delay ticks: %u, working time: %u\n\n"), m_connects, m_requests, m_delaytik, m_worktime);


            m_ConnSocket.resize(m_connects);
            m_evConnects.resize(m_connects);

            for (UINT conn = 0; conn < m_connects; ++conn)
            {
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

                    MSG(2, _T("Attempting to connect to: %s\n"), hostname);

                    if (socket.Connect(pai))
                    {
                        m_ConnSocket[conn] = socket;
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
                if (m_ConnSocket[conn].NameInfo(psa, hostname, _countof(hostname)) != NO_ERROR)
                {
                    _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
                }
                MSG(0, _T("Connected to %s, port %u, (%s, %s)\n"), hostname, ntohs(SS_PORT(psa)), PStr(pai->ai_socktype), FStr(pai->ai_family));

                // local address and port the system picked
                SOCKADDR_STORAGE addr;
                int addrlen = sizeof(addr);

                if (getsockname(m_ConnSocket[conn], (PSOCKADDR)&addr, &addrlen) != SOCKET_ERROR)
                {
                    LPCTSTR p = InetNtop(pai->ai_family, INETADDR_ADDRESS((PSOCKADDR)&addr), hostname, _countof(hostname));
                    MSG(0, _T("Using local address %s, port %u\n"), p ? p : UNKNOWN_NAME, ntohs(SS_PORT(&addr)));
                }

            } // for (conn)

            m_bConnected = true;
        }
        return ERROR_SUCCESS;
    }

    int Disconnect(void) throw()
    {
        if (m_bConnected)
        {
            DisplayData(true); // display statistics..

            for (auto& it : m_ConnSocket) { it.Close(); }
            for (auto& it : m_evConnects) { it.Close(); }

            m_ConnSocket.clear();
            m_evConnects.clear();

            m_bConnected = false;
        }
        ERR(_T("\nTerminated: %s\n"), (LPCTSTR)CTime::GetCurrentTime().Format(_T("%X")));

        return ERROR_SUCCESS;
    }

    int Run(void) throw()
    {
        // Set Ctrl+C, Ctrl+Break handler
        CCtrlHandler ctrl(_CtrlHandler);

        LONG events = (FD_READ | FD_WRITE | FD_CLOSE);

        for (UINT conn = 0; conn < m_connects; ++conn)
        {
            CSocketEvent ev;
            if (!ev.Create())
            {
                return DisplayError(_T("CSocketEvent.Create() failed."));
            }
            m_evConnects[conn] = ev;

            if (!m_ConnSocket[conn].SelectEvents(m_evConnects[conn], events))
            {
                return DisplayError(_T("CSocketAsync.SelectEvents() failed."));
            }

        } // for()

        DWORD requestcnt = 0;
        DWORD requestmax = m_connects * m_requests;
        DWORD sockopened = m_connects;
        DWORD timeoutcnt = 0;
        DWORD timeoutmax = CLIENT_TIMEOUT / CLIENT_WAIT;

        UINT64 tick = 0;
        vector<USHORT> lastkey(m_connects, 0); // last random key sent..

        if (m_worktime > 0)
        {
            requestmax = (UINT_MAX - 1); // "infinite"
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
            if (sockopened == 0) break; // job done

            for (DWORD offs = 0; offs < m_connects; offs += WSA_MAXIMUM_WAIT_EVENTS)
            {
                if (CQuit::yes()) break;

                LPCWSAEVENT pev = (LPCWSAEVENT)m_evConnects.data();

                DWORD cnt = min((DWORD)(m_evConnects.size() - offs), (DWORD)WSA_MAXIMUM_WAIT_EVENTS);
                DWORD idx = ::WSAWaitForMultipleEvents(cnt, pev, FALSE, CLIENT_WAIT, FALSE);

                if (idx == WSA_WAIT_FAILED)
                {
                    return DisplayError(_T("WSAWaitForMultipleEvents() failed."));
                }
                else if (idx == WSA_WAIT_TIMEOUT)
                {
                    if (++timeoutcnt < timeoutmax) continue;

                    return ERROR_TIMEOUT;
                }
                timeoutcnt = 0; // reset

                // one socket fired event
                idx = idx + offs - WSA_WAIT_EVENT_0;

                WSANETWORKEVENTS ne = {0};
                if (!m_ConnSocket[idx].EnumEvents(m_evConnects[idx], &ne))
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

                    if (ReadData(m_ConnSocket[idx], box.buffer, len) == SOCKET_ERROR)
                        return SOCKET_ERROR;

                    if (DATABOX_SIZE_READ != len || box.pkg.anchor != ANCHOR)
                        continue; // skip unsupported data

                    if (lastkey[idx] != htons(box.pkg.keyval))
                    {
                        ERR(_T("Key mismatch: sent %u, received %u\n"), lastkey[idx], htons(box.pkg.keyval));
                    }

                    if (m_delaytik > 0) { ::Sleep(m_delaytik); }

                    if (requestcnt >= requestmax)
                    {
                        MSG(0, _T("Request limit (%i) is reached\n"), m_requests);
                        // wait for FD_CLOSE event on success..
                        if (Shutdown(m_ConnSocket[idx])) continue;

                        break; // immediate non-graceful closure
                    }

                    box.pkg.anchor = ANCHOR;
                    box.pkg.keyval = ntohs((USHORT)m_rg.number());

                    len = DATABOX_SIZE_SEND;

                    if (SendData(m_ConnSocket[idx], box.buffer, len) == SOCKET_ERROR)
                    {
                        --sockopened; continue;
                    }
                    ++requestcnt;

                    lastkey[idx] = htons(box.pkg.keyval);
                    MSG(0, _T("Sent %i bytes, key=%u\n"), len, lastkey[idx]);
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

                    if (SendData(m_ConnSocket[idx], box.buffer, len) == SOCKET_ERROR)
                    {
                        --sockopened; continue;
                    }
                    ++requestcnt;

                    lastkey[idx] = htons(box.pkg.keyval);
                    MSG(0, _T("Sent %i bytes, key=%u\n"), len, lastkey[idx]);
                }
                else if (ne.lNetworkEvents & FD_CLOSE)
                {
                    MSG(2, _T("FD_CLOSE event fired\n"));
                    if (ne.iErrorCode[FD_CLOSE_BIT] != 0)
                    {
                        return DisplayError(_T("FD_CLOSE failed."), ne.iErrorCode[FD_CLOSE_BIT]);
                    }
                    MSG(0, _T("Server closed connection\n"));

                    --sockopened;
                }
                DisplayData(false); // display statistics..

            } // for (offs)

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

            m_connects = ::GetPrivateProfileInt(_T("client"), _T("connections"), m_connects, path);
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

    int ReadData(CSocketAsync& s, char* buf, int& len) throw()
    {
        int bytes = recv(s, buf, len, 0);
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
                int cb = recv(s, buf, len, 0);
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

    int SendData(CSocketAsync& s, const char* buf, int& len) throw()
    {
        int bytes = send(s, buf, len, 0);
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
        bool b = false;
        for (auto& it : m_ConnSocket) { b = Shutdown(it); }

        return b;
    }
    bool Shutdown(CSocketAsync& s) throw()
    {
        char buf[BUFFER_SIZE] = {0};
        int len = sizeof(buf);

        if (m_socktype == SOCK_DGRAM)
        {
            // signal to UDP server we are finishing..
            if (send(s, buf, 0, 0) == SOCKET_ERROR)
            {
                DisplayError(_T("send() failed."));
            }
        }

        if (s.Shutdown(SD_SEND))
        {
            if (m_socktype == SOCK_STREAM)
            {
                // Since TCP does not preserve message boundaries, there may still be more data arriving from the server.
                while (recv(s, buf, len, 0) > 0);

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

    UINT m_connects;    // number of client connections (same address/port)
    UINT m_clientid;
    UINT m_requests;
    UINT m_delaytik;    // delay between requests, ms (client)
    UINT m_worktime;    // total working time, s (client)

private:
    std::vector<CSocketAsync> m_ConnSocket;
    std::vector<CSocketEvent> m_evConnects;

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

            if (_Module.m_bPause)
            {
                PAUSE(_T("\npress a key to exit...\n"));
            }
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

    return nRet;
}

