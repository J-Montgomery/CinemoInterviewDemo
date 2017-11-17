
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
        test_write     = 2,
        test_read      = 4
    };

    #define f_access(file, mode) _access((file), (mode))

    #define INITIAL_SYS_PATH_LEN MAX_PATH

/*
 * pthreads shim API
 */
    #define pthread_mutex_t              CRITICAL_SECTION
    #define pthread_cond_t               CONDITION_VARIABLE

    #define pthread_t                    HANDLE
    #define pthread_attr_t               LPSECURITY_ATTRIBUTES

    #define pthread_mutex_init(mutex, attr) InitializeCriticalSection((mutex))
    #define pthread_mutex_destroy(mutex)    DeleteCriticalSection((mutex))

    #define pthread_mutex_lock(mutex)       EnterCriticalSection((mutex))
    #define pthread_mutex_unlock(mutex)     LeaveCriticalSection((mutex))

    #define pthread_cond_init(cv, attr)     InitializeConditionVariable((cv))
    #define pthread_cond_destroy(cv)     

    #define pthread_cond_signal(cv)         WakeConditionVariable((cv))
    #define pthread_cond_wait(cv, mutex)    SleepConditionVariableCS((cv), (mutex), INFINITE)

    #define pthread_create(tid, attr, f, arg) *tid = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f, arg, 0, NULL)


    int getNumCPUs(void) {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo); // This call can only count up to 32 cores
                                 // GetNativeSystemInfo would count up to 64 if available, but adds call complexity
        return sysinfo.dwNumberOfProcessors;
    }

/*****************************************************************************************
 * POSIX Defines
 ****************************************************************************************/
#else /* Hope this is reasonably posix-compliant compiler. If not, good luck */
    #include <pthread.h>
    #include <unistd.h>
    #include <limits.h> /* Let's hope this includes PATH_MAX and NAME_MAX, but it probably doesn't on most systems */

    #define getCwd getcwd

    enum access_modes {
        test_existence = F_OK, // 0
        test_write     = W_OK, // 2
        test_read      = R_OK  // 4
    };

    #define f_access(file, mode) access((file), (mode))
    

    #if defined(PATH_MAX)
        #define INITIAL_SYS_PATH_LEN PATH_MAX
    #else
        #define INITIAL_SYS_PATH_LEN 255
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