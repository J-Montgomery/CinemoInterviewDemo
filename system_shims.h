
#ifndef SYSTEM_SHIMS_H_
#define SYSTEM_SHIMS_H_
#if defined(_MSC_VER) /* Then we're building with MSVC */
    #include <direct.h> 
    #define getCwd __getcwd

    #include <windows.h>
    int getNumCPUs(void) {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo); // This call can only count up to 32 cores. 
                                 // GetNativeSystemInfo would count up to 64 if available, 
                                 // but adds call complexity
        return sysinfo.dwNumberOfProcessors;
    }

#else /* Hope this is a POSIX compatible compiler */
    #include <unistd.h> /* Needed for getcwd */
    #define getCwd getcwd

    int getNumCPUs(void) {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }
    
#endif

/*! Gets the current working directory from the filesystem in a semi-portable way 
 *  on both Windows and Linux systems 
 */
// char *getCwd(char *buf, size_t size);


#ifndef MAX
    #define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
    #define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif








#endif /* SYSTEM_SHIMS_H_ */