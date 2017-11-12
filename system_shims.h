
#ifndef SYSTEM_SHIMS_H_
#define SYSTEM_SHIMS_H_


/*****************************************************************************************
 * MSVC Defines
 ****************************************************************************************/
#if defined(_MSC_VER) /* Then we're building with MSVC */
    #include <direct.h> 
    #define getCwd _getcwd

    #include <windows.h>
    int getNumCPUs(void) {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo); // This call can only count up to 32 cores. 
                                 // GetNativeSystemInfo would count up to 64 if available, 
                                 // but adds call complexity
        return sysinfo.dwNumberOfProcessors;
    }

/*
 * MSVC Threading API: http://66.165.136.43/dictionary/pthread_cond_wait-entry.php
 */
    #define MUTEX_T CRITICAL_SECTION
    #define COND_T CONDITION_VARIABLE

    #define TID_T HANDLE
    #define THREAD_ATTR_T LPSECURITY_ATTRIBUTES

    #define mutex_init(mutex)    InitializeCriticalSection((mutex))
    #define mutex_destroy(mutex) DeleteCriticalSection((mutex))

    #define cond_init(cv)        InitializeConditionVariable((cv))
    #define cond_destroy(cv)     ()

    #define mutex_lock(mutex)    EnterCriticalSection((mutex))
    #define mutex_unlock(mutex)  LeaveCriticalSection((mutex))

    #define cond_signal(cv)      WakeConditionVariable((cv))
    #define cond_wait(cv, mutex) SleepConditionVariableCS((cv), (mutex), INFINITE)

    #define create_thread(tid, f, arg) tid = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f, arg, 0, NULL)

/*****************************************************************************************
 * POSIX Defines
 ****************************************************************************************/
#else /* Hope this is a POSIX compatible compiler */
    #include <unistd.h> /* Needed for getcwd */
    #define getCwd getcwd

    int getNumCPUs(void) {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }

/*
 * POSIX Threading API
 */
    #include <pthread.h>
    #define MUTEX_T pthread_mutex_t
    #define COND_T pthread_cond_t

    #define TID_T pthread_t
    #define THREAD_ATTR_T pthread_attr_t

    #define mutex_init(mutex)    pthread_mutex_init((mutex), NULL)
    #define mutex_destroy(mutex) pthread_mutex_destroy((mutex))

    #define cond_init(cv)        pthread_cond_init((cv), NULL)
    #define cond_destroy(cv)     pthread_cond_destroy((cv))

    #define mutex_lock(mutex)    pthread_mutex_lock((mutex))
    #define mutex_unlock(mutex)  pthread_mutex_unlock((mutex))

    #define cond_signal(cv)      pthread_cond_signal((cv))
    #define cond_wait(cv, mutex) pthread_cond_wait((cv), (mutex))

    #define create_thread(tid, f, arg) pthread_create(&(tid), NULL, f, arg)


#endif

/*****************************************************************************************
 * Defines that really should be in the standard library
 ****************************************************************************************/
#ifndef MAX
    #define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
    #define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif








#endif /* SYSTEM_SHIMS_H_ */