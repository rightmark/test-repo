
#pragma once



template <class T, class Log = CLog>
class ATL_NO_VTABLE CTransmitBase
    : public CStatManager<STAT_INTERVAL>
    , public CQuit
{
public:
    int ReadData(CSocketAsync& s) throw()
    {
        T* pT = static_cast<T*>(this);
        return pT->ReadData(s);
    }
    int SendData(CSocketAsync& s) throw()
    {
        T* pT = static_cast<T*>(this);
        return pT->SendData(s);
    }

    int DisplayError(LPCTSTR msg, int err = ::WSAGetLastError()) throw()
    {
        return Log::Error(msg, err);
    }

protected:
    // helpers
    void ReadDataStat(int cb) throw()
    {
        ReadMoreData(cb);
    }

    void SentDataStat(int cb) throw()
    {
        SentMoreData(cb);
    }

private:
};

class CTransmitTcp
    : public CTransmitBase<CTransmitTcp>
    , public CTaskProxy
{
public:
    int ReadData(CSocketAsync& s) throw()
    {
        char buf[BUFFER_SIZE] = {0};
        TCHAR hostname[MAX_ADDRSTRLEN];

        int bytes = 0;
        int err = ERROR_SUCCESS;

        for (;;)
        {
            // TCP does NOT maintain message boundaries, we may recv() the client's data grouped differently than it was sent.
            // Since all this server does is echo the data it receives back to the client, we don't need to concern ourselves
            // about message boundaries. But it does mean that the message data we print for a particular recv() below may
            // contain more or less data than was contained in a particular client send().
            //

            int cb = recv(s, buf, sizeof(buf), 0);
            if (cb == SOCKET_ERROR)
            {
                if ((err = ::WSAGetLastError()) != WSAEWOULDBLOCK)
                {
                    DisplayError(_T("recv() failed."), err);
                }
                break;
            }

            bytes += cb;

            if (cb == 0)
            {
                MSG(1, _T("recv() returned zero. Client closed connection.\n"));
                break;
            }
            else
            {
                ReadDataStat(cb);
            }

            LPSOCKADDR from = NULL;

            err = s.NameInfo(&from, hostname, sizeof(hostname));
            if (err != NO_ERROR)
            {
                DisplayError(_T("CSocketAsync.NameInfo() failed."), err);
                _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
            }

            CString str(buf, cb);
            MSG(1, _T("recv bytes=%i, port=%u: %s, from %s\n"), cb, ntohs(SS_PORT(from)), (LPCTSTR)str, hostname);

#if 0
            cb = send(s, buf, cb, 0);
            if (cb == SOCKET_ERROR)
            {
                DisplayError(_T("send() failed."));
            }
            else
            {
                SentDataStat(cb);
            }
#endif
        } // for()

        bytes = send(s, buf, bytes, 0);
        if (bytes == SOCKET_ERROR)
        {
            DisplayError(_T("send() failed."));
        }
        else
        {
            SentDataStat(bytes);
        }

        // @TODO: km 20160810 - write code..
        return bytes;
    }
    int SendData(CSocketAsync& /*s*/) throw()
    {
#if 0
        char buf[] = "TCP server response";
        int len = (int)strlen(buf) + 1;

        int bytes = send(s, buf, len, 0);
        if (bytes == SOCKET_ERROR)
        {
            DisplayError(_T("send() failed."));
        }
        else
        {
            SentDataStat(bytes);
        }
        // @TODO: km 20160810 - write code..
        return bytes;
#else
        return 0;
#endif
    }
};

class CTransmitUdp
    : public CTransmitBase<CTransmitUdp>
    , public CTaskProxy
{
public:
    int ReadData(CSocketAsync& s) throw()
    {
        char buf[BUFFER_SIZE] = {0};
        TCHAR hostname[MAX_ADDRSTRLEN];

        SOCKADDR_STORAGE from = {0};
        int fromlen = sizeof(from);

        // Since UDP maintains message boundaries, the amount of data we get from a recvfrom()
        // should match exactly the amount of data the client sent in the corresponding sendto().
        //
        int bytes = recvfrom(s, buf, sizeof(buf), 0, (LPSOCKADDR)&from, &fromlen); //WSARecvFrom();
        if (bytes == SOCKET_ERROR)
        {
            DisplayError(_T("recvfrom() failed."));
            return bytes;
        }
        if (bytes == 0)
        {
            // Actually, this should never happen on an unconnected socket..
            MSG(1, _T("recvfrom() returned zero, aborting\n"));
            return bytes;
        }
        else
        {
            ReadDataStat(bytes);
        }

        int err = ::GetNameInfo((LPSOCKADDR)&from, fromlen, hostname, sizeof(hostname), NULL, 0, NI_NUMERICHOST);
        if (err != NO_ERROR)
        {
            DisplayError(_T("GetNameInfo() failed."), err);
            _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
        }

        CString str(buf, bytes);
        MSG(1, _T("recv bytes=%i, port=%u: %s, from %s\n"), bytes, ntohs(SS_PORT(&from)), (LPCTSTR)str, hostname);
        //MSG(_T("Echoing same data back to client\n"));

#if 1 // @KLUDGE: km 20160822 - testing..
        {
            static short val = 0;

            UINT result = GetResult(++val & 1023);
            MSG(0, _T("random=%u, average=%u\n"), val, result);
        }
#endif

        bytes = sendto(s, buf, bytes, 0, (LPSOCKADDR)&from, fromlen); //WSASendTo();
        if (bytes == SOCKET_ERROR)
        {
            DisplayError(_T("sendto() failed."), err);
        }
        else
        {
            SentDataStat(bytes);
        }

        return bytes;
    }
    int SendData(CSocketAsync& /*s*/) throw()
    {
        // @TODO: km 20160810 - write code..
        return 0;
    }
};

