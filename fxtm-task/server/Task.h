#pragma once



class CTask
{
    typedef ATL::CComAutoCriticalSection AutoCriticalSection;
    typedef ATL::CComCritSecLock<AutoCriticalSection> AutoLock;

    typedef std::map<SHORT, UINT> TStorage;

    static const size_t STORAGE_SIZE = (KEY_MAXIMUM + 1);

    static const UINT SAVER_WAIT = 100; // ms

    static const UINT
        INTERVAL_MIN    = 5,
        INTERVAL_MAX    = 50,
        INTERVAL_INI    = 10;


public:
    CTask()
        : m_sqsum(0)
        , m_count(0)
        , m_bQuit(false)
        , m_interval(INTERVAL_INI)
        , m_bRequiresSave(false)
    {}

    ~CTask() throw() { Terminate(); }

#if (_MSC_VER >= 1900)
    CTask(CTask const&) = delete;             // copy ctor
    CTask(CTask&&) = delete;                  // move ctor
    CTask& operator=(CTask const&) = delete;  // copy assign
    CTask& operator=(CTask &&) = delete;      // move assign
#endif

    bool LoadState(LPCTSTR fn) throw()
    {
        bool bRet = false;

        if (m_data.size() == 0)
        {
            m_file = fn;

            CAtlFile f;

            if (SUCCEEDED(f.Create(m_file, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING)))
            {
                UINT buf[STORAGE_SIZE] = {0};
                DWORD cch, len = sizeof(buf);

                if (SUCCEEDED(f.Read(buf, len, cch)) && cch == sizeof(buf))
                {
                    AutoLock _(m_cs);

                    m_sqsum = 0;
                    m_count = 0;

                    for (short i = 0; i < _countof(buf); ++i)
                    {
                        if (buf[i] > 0)
                        {
                            m_count += buf[i];
                            m_sqsum += (i * i) * buf[i];

                            m_data[i] = buf[i];
                        }
                    }
                    ATLTRACE(_T("CTask.LoadState(\"%s\")\n"), (LPCTSTR)m_file);
                }
            }
            bRet = (_beginthreadex(NULL, 0, _ThreadProc, this, 0, NULL) != 0);
        }
        return bRet;
    }
    bool SaveState(void) throw()
    {
        bool bRet = false;

        if (m_data.size() > 0)
        {
            CAtlFile f;

            if (SUCCEEDED(f.Create(m_file, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS)))  // @TODO: TBD..
            {
                Lock();

                TStorage temp = m_data;

                Unlock();

                UINT buf[STORAGE_SIZE] = {0};

                for (TStorage::const_iterator it = temp.begin(); it != temp.end(); ++it)
                {
                    buf[it->first] = it->second;
                }
                bRet = SUCCEEDED(f.Write(buf, sizeof(buf)));

                ATLTRACE(_T("CTask.SaveState(\"%s\")\n"), (LPCTSTR)m_file);
            }
        }
        return bRet;
    }

    // @TODO: km 20160822 - use C++11 parametric return type..
    UINT GetResult(SHORT val) throw()
    {
        UINT result = 0;

        AutoLock _(m_cs);

        TStorage::iterator it = m_data.find(val);
        if (it != m_data.end())
        {
            it->second++; 
        }
        else
        {
            m_data[val] = 1;
        }

        m_sqsum += (val * val);
        if (++m_count)
        {
            result = (UINT)(m_sqsum / m_count);

            m_bRequiresSave = true;
        }

        return result;
    }
    void SetInterval(UINT val) throw()
    {
        m_interval = min(max(val, INTERVAL_MIN), INTERVAL_MAX);
    }
    void Terminate(void) throw()
    {
        m_bQuit = true;
    }


protected:
    // helpers
    void Lock(void) throw() { m_cs.Lock(); }
    void Unlock(void) throw() { m_cs.Unlock(); }

private:
    UINT ThreadProc(void) throw()
    {
        ATLTRACE(_T("Task thread run (%p)\n"), this);

        static UINT64 lasttick = 0;

        while (!m_bQuit)
        {
            ::Sleep(SAVER_WAIT);

            UINT64 tick = ::GetTickCount64();
            if (tick > lasttick)
            {
                lasttick = tick + m_interval;

                if (m_bRequiresSave)
                {
                    m_bRequiresSave = false;

                    if (!SaveState()) break;
                }
            }
        }
        if (m_bRequiresSave) { SaveState(); }

        ATLTRACE(_T("Task thread exited (%p)\n"), this);

        return ERROR_SUCCESS;
    }

    static UINT WINAPI _ThreadProc(void* pv)
    {
        CTask* p = static_cast<CTask*>(pv);
        return p->ThreadProc();
    }

protected:
    UINT64 m_sqsum;
    UINT32 m_count;

    TStorage m_data;
    CString m_file;     // task state storage file name

private:
    bool volatile m_bQuit;
    bool volatile m_bRequiresSave;
    UINT volatile m_interval;   // data saving interval in seconds

    AutoCriticalSection m_cs;
};



class CTaskProxy
{
public:
    CTaskProxy() {}
    ~CTaskProxy() throw() {}

    UINT GetResult(SHORT val) throw()
    {
        return Task.GetResult(val);
    }

    // properties
    __declspec(property(get=get_Task)) CTask& Task;
    CTask& get_Task(void) const { return CFactorySingleton<CTask>::Instance(); }

};
