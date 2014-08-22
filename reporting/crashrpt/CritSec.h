
/*! \file  CritSec.h
*  \brief  Critical section wrapper classes. 
	Code of CCritSec and CAutoLock classes is taken from DirectShow base classes and modified in some way.
*/

#ifndef _CRITSEC_H
#define _CRITSEC_H

/*!
\class CCritSec 
\brief
	wrapper for whatever critical section we have
*/
class CCritSec 
{
public:

    CCritSec() 
    {
        InitializeCriticalSection(&m_CritSec);
    };

    ~CCritSec() 
    {
        DeleteCriticalSection(&m_CritSec);
    }

    void Lock() 
    {
        EnterCriticalSection(&m_CritSec);
    };

    void Unlock() 
    {
        LeaveCriticalSection(&m_CritSec);
    };

	// make copy constructor and assignment operator inaccessible
private:
	CCritSec(const CCritSec &refCritSec);
	CCritSec &operator=(const CCritSec &refCritSec);

	CRITICAL_SECTION m_CritSec;
};

/*!
\class CAutoLock 
\brief
	locks a critical section, and unlocks it automatically when the lock goes out of scope
*/
class CAutoLock 
{

public:
    CAutoLock(__in CCritSec * plock)
    {
        m_pLock = plock;
        m_pLock->Lock();
    };

    ~CAutoLock() 
    {
        m_pLock->Unlock();
    };

protected:

	CCritSec * m_pLock;

private:    

	// make copy constructor and assignment operator inaccessible

	CAutoLock(const CAutoLock &refAutoLock);
	CAutoLock &operator=(const CAutoLock &refAutoLock);
};


#endif  //_CRITSEC_H
