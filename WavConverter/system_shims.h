
#ifndef SYSTEM_SHIMS_H_
#define SYSTEM_SHIMS_H_


/*****************************************************************************************
 * MSVC Defines
 ****************************************************************************************/
#if defined(_WIN32) || defined(_MSC_VER) /* Then we're building with MSVC */

    #include <windows.h>
    #include <direct.h> 
    #include <io.h>
    #define getCwd _getcwd

    enum access_modes {
        test_existence = 0,
        test_write = 2,
        test_read = 4
    };

    #define f_access(file, mode) _access((file), (mode))
/*
 * pthreads shim API
 */
    #define MUTEX_T CRITICAL_SECTION
    #define COND_T CONDITION_VARIABLE

    #define TID_T HANDLE
    #define THREAD_ATTR_T LPSECURITY_ATTRIBUTES

    #define mutex_init(mutex)    InitializeCriticalSection((mutex))
    #define mutex_destroy(mutex) DeleteCriticalSection((mutex))

    #define cond_init(cv)        InitializeConditionVariable((cv))
    #define cond_destroy(cv)     

    #define mutex_lock(mutex)    EnterCriticalSection((mutex))
    #define mutex_unlock(mutex)  LeaveCriticalSection((mutex))

    #define cond_signal(cv)      WakeConditionVariable((cv))
    #define cond_wait(cv, mutex) SleepConditionVariableCS((cv), (mutex), INFINITE)

    #define create_thread(tid, f, arg) tid = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f, arg, 0, NULL)


    #define INITIAL_SYS_PATH_LEN MAX_PATH
    #define INITIAL_SYS_NAME_LEN 255

    int getNumCPUs(void) {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo); // This call can only count up to 32 cores. 
                                 // GetNativeSystemInfo would count up to 64 if available, 
                                 // but adds call complexity
        return sysinfo.dwNumberOfProcessors;
    }


/*****************************************************************************************
 * POSIX Defines
 ****************************************************************************************/
#else /* With any luck this is reasonably posix-compliant compiler. If not... */
    #include <pthread.h>
    #include <unistd.h>
    #include <limits.h> /* Let's hope this includes PATH_MAX and NAME_MAX, but it probably doesn't on most systems */

    #define getCwd getcwd

    enum access_modes {
        test_exist = F_OK, // 0
        test_write = W_OK, // 2
        test_read = R_OK   // 4
    };

    #define f_access(file, mode) access((x), (mode))
    
/*
 * POSIX Threading API
 */
    
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

 /*
 * Workarounds for practical non-compliance to POSIX 
 */
    #if defined(PATH_MAX)
        #define INITIAL_SYS_PATH_LEN PATH_MAX
    #else
        #define INITIAL_SYS_PATH_LEN 255
    #endif

    #if defined(NAME_MAX)
        #define INITIAL_SYS_NAME_LEN NAME_MAX
    #else
        #define INITIAL_SYS_NAME_LEN 255
    #endif


    
    int getNumCPUs(void) {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }

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