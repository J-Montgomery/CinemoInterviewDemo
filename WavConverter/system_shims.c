
#include "system_shims.h"

/*****************************************************************************************
* MSVC Defines
****************************************************************************************/
#if defined(_WIN32) || defined(_MSC_VER) /* MSYS and MSVC can both call windows functions */


#include <windows.h>
int getNumCPUs(void) {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo); // This call can only count up to 32 cores. 
                             // GetNativeSystemInfo would count up to 64 if available, 
                             // but adds call complexity
    return sysinfo.dwNumberOfProcessors;
}

long int getSystemPathMax(char *filename) {
    return INITIAL_SYS_PATH_LEN; // We don't want to ever exceed this or things get complicated
}

long int getSystemNameMax(char *filename) {
    DWORD lpNameMax = 0;
    GetVolumeInformation(NULL, NULL, 0, NULL, &lpNameMax, NULL, NULL, 0);
    return lpNameMax;
}


/*****************************************************************************************
* POSIX Defines
****************************************************************************************/
#else /* Hope this is a POSIX-ish compiler */
#include <unistd.h> /* Needed for getcwd */
#define getCwd getcwd

int getNumCPUs(void) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

long int getSystemNameMax(char *filename) {
    return pathconf(filename, _PC_NAME_MAX);
}

#endif