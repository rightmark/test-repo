
#pragma once



template<class T, class Log = CLog>
class ATL_NO_VTABLE CTransmitBase : public CStatManager<STAT_INTERVAL>
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

class CTransmitTcpSimple
    : public CTransmitBase<CTransmitTcpSimple>
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

            int cbRead = recv(s, buf, sizeof(buf), 0);
            if (cbRead == SOCKET_ERROR)
            {
                if ((err = ::WSAGetLastError()) != WSAEWOULDBLOCK)
                {
                    DisplayError(_T("recv() failed."), err);
                }
                break;
            }

            bytes += cbRead;

            if (cbRead == 0)
            {
                MSG(1, _T("recv() returned zero. Client closed connection.\n"));
                break;
            }

            ReadDataStat(cbRead);

            LPSOCKADDR from = NULL;

            err = s.NameInfo(&from, hostname, _countof(hostname));
            if (err != NO_ERROR)
            {
                DisplayError(_T("CSocketAsync.NameInfo() failed."), err);
                _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
            }

            LPDATABOXT box = (LPDATABOXT)buf;

            while (DATABOX_SIZE_READ <= cbRead && box->pkg.anchor == ANCHOR)
            {
                ULONG result = 0;
                USHORT key = htons(box->pkg.keyval);

                if (key <= KEY_MAXIMUM)
                {
                    MSG(1, _T("recv bytes=%i, key=%u, port=%u, from %s\n"), bytes, key, ntohs(SS_PORT(from)), hostname);

                    result = GetResult(key);
                }

                box->pkg.result = ntohl(result);

                int cbSent = send(s, box->buffer, DATABOX_SIZE_SEND, 0);
                if (cbSent == SOCKET_ERROR)
                {
                    DisplayError(_T("send() failed."));
                }
                else
                {
                    SentDataStat(cbSent);
                }

                ++box; cbRead -= DATABOX_SIZE_READ;
            }

            if (cbRead > 0)
            {
                CString str(box->buffer, cbRead);
                MSG(1, _T("recv bytes=%i, port=%u: %s, from %s\n"), cbRead, ntohs(SS_PORT(from)), (LPCTSTR)str, hostname);

                int cbSent = send(s, box->buffer, cbRead, 0);
                if (cbSent == SOCKET_ERROR)
                {
                    DisplayError(_T("send() failed."));
                }
                else
                {
                    SentDataStat(cbSent);
                }

            }

        } // for()

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

class CTransmitUdpSimple
    : public CTransmitBase<CTransmitUdpSimple>
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

        ReadDataStat(bytes);

        int err = ::GetNameInfo((LPSOCKADDR)&from, fromlen, hostname, _countof(hostname), NULL, 0, NI_NUMERICHOST);
        if (err != NO_ERROR)
        {
            DisplayError(_T("GetNameInfo() failed."), err);
            _tcscpy_s(hostname, _countof(hostname), UNKNOWN_NAME);
        }

        if (DATABOX_SIZE_READ == bytes && ((LPDATABOXT)buf)->pkg.anchor == ANCHOR)
        {
            DATABOXT& box = *(LPDATABOXT)buf;
            ULONG result = 0;
            USHORT key = htons(box.pkg.keyval);

            if (key <= KEY_MAXIMUM)
            {
                MSG(1, _T("recv bytes=%i, key=%u, port=%u, from %s\n"), bytes, key, ntohs(SS_PORT(&from)), hostname);

                result = GetResult(key);
            }

            box.pkg.result = ntohl(result);
            bytes = DATABOX_SIZE_SEND;
        }
        else
        {
            CString str(buf, bytes);
            MSG(1, _T("recv bytes=%i, port=%u: %s, from %s\n"), bytes, ntohs(SS_PORT(&from)), (LPCTSTR)str, hostname);
        }

        bytes = sendto(s, buf, bytes, 0, (LPSOCKADDR)&from, fromlen);
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

// @TODO: km 20160907 - write code..
class CTransmitTcpBuffered
{
public:

private:
};



typedef CTransmitTcpSimple CTransmitTcp;
typedef CTransmitUdpSimple CTransmitUdp;
