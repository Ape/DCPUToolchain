/**

    File:       osutil.c

    Project:    DCPU-16 Toolchain
    Component:  LibDCPU

    Authors:    James Rhodes

    Description:    Defines functions for interacting with the
            local operating system (such as getting
            directory names, etc.)

**/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <windows.h>
#else
#include <libgen.h>
#include <unistd.h>
// #include <sys/time.h> is correct for Linux.  If #include <time.h> is
// required for Mac, please add the appropriate #ifdef APPLE instead of
// changing this include.
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <assert.h>
#include "osutil.h"
#include "config.h"

bstring osutil_arg0 = NULL;
#ifdef _WIN32
char* dirname(char* path)
{
    // FIXME: This assumes the resulting path will always
    // be shorter than the original (which it should be
    // given that we're only returning a component of it, right?)
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    _splitpath(path, drive, dir, NULL, NULL);
    strcpy(path, drive);
    strcpy(path + strlen(path), dir);
    return path;
}
#endif

///
/// Returns the directory name component of the specified path.
///
/// Returns the directory component of the specified path in a
/// cross-platform manner.
///
/// @param path The path to retrieve the directory name of.
/// @return The directory name component.
///
bstring osutil_dirname(bstring path)
{
    bstring bpath;
    char* cpath;
    char* opath;

    cpath = bstr2cstr(path, '0');
    // needs to return in case of dirname implementation not changing parameter
    // in particular this doesn't work on OS X check $ man 3 dirname
    opath = dirname(cpath);
    bpath = bfromcstr(opath);
    bcstrfree(cpath);

    return bpath;
}

///
/// Sets the specified file handler to binary mode.
///
/// On some platforms (Windows), file descriptors are opened in text
/// mode by default.  In particular, on these platforms, standard
/// input, output and error are all text mode by default which causes
/// issues when outputting emulator bytecode to these descriptors.
/// This function sets a file descriptor into binary mode in a
/// cross-platform manner.
///
/// @param fd The file descriptor to switch into binary mode.
///
void osutil_makebinary(FILE* fd)
{
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
}

///
/// Stores the application path in a global context.
///
/// Stores the application path (arg0) in a global context so that
/// libraries can retrieve it later using osutil_getarg0.  All applications
/// using a DCPU-16 Toolchain library should invoke this method after
/// parsing their arguments.  If this value is not set, some libraries
/// may be unable to resolve their required runtime components.
///
/// @param arg0 The string containing argument 0.
///
void osutil_setarg0(freed_bstring arg0)
{
    if (osutil_arg0 != NULL)
        bdestroy(osutil_arg0);
    osutil_arg0 = bstrcpy(arg0.ref);
    bautodestroy(arg0);
}

///
/// Retrieves the current path to the application.
///
/// Retrieves the previously stored path to the application and
/// returns a copy of the internal string.
///
/// @return A copy of the current path to the application.  The result
///     must be freed manually.
///
bstring osutil_getarg0()
{
    assert(osutil_arg0 != NULL);

    return bstrcpy(osutil_arg0);
}

///
/// Retrieves the current directory that the application executable is located in.
///
/// Retrieves the previously stored path to the application, resolves the directory
/// using os_dirname and returns a copy of the directory the application is contained in.
///
/// @return The path of the directory containing the application.  The result must be freed manually.
///
bstring osutil_getarg0path()
{
    bstring arg0 = osutil_getarg0();
    bstring path = osutil_dirname(arg0);
#ifdef WIN32
    // Special handling for Windows where the path
    // can be blank if argument 0 is just the executable
    // name.
    if (biseqcstr(path, ""))
    {
        bdestroy(path);
        bdestroy(arg0);
        arg0 = bfromcstr(".\\");
        bconcat(arg0, osutil_getarg0());
        path = osutil_dirname(arg0);
    }
#endif
    bdestroy(arg0);
    return path;
}

///
/// @brief Retrieves the directory that contains toolchain modules.
///
/// Retrieves the directory that contains toolchain modules based on the existance of the
/// TOOLCHAIN_MODULES environment variable or the default modules folder.
///
/// If this is a deployed build of the toolchain, the default module directory is based on the
/// installation path of the toolchain.
///
/// If this is a non-deployed build of the toolchain (the default), the default module directory is
/// the "modules/" folder relative to the location of the applcation.  This requires osutil_setarg0
/// to have been previously called.
///
/// @return The path of the directory holding toolchain modules.  The result must be freed manually.
///
bstring osutil_getmodulepath()
{
    // FIXME: This function should return NULL
    //    if the path does not exist or is
    //    not a directory.
    bstring tmp;
    int result;
    struct stat buffer;
    char* env = getenv("TOOLCHAIN_MODULES");
    if (env == NULL)
    {
#ifdef DCPU_CONFIG_HAS_MODULE_PATH
        tmp = bfromcstr(DCPU_CONFIG_MODULE_PATH);
#else
        tmp = osutil_getarg0path();
#ifdef _WIN32
        bcatcstr(tmp, "\\modules");
#else
        bcatcstr(tmp, "/modules");
#endif
#endif
    }
    else
        tmp = bfromcstr(env);

    // Locate the correct path.
    result = stat((const char*)tmp->data, &buffer);
    if (result == 0 && (buffer.st_mode & S_IFDIR) != 0)
        return tmp;
    binsertch(tmp, 0, 1, '.');
    result = stat((const char*)tmp->data, &buffer);
    if (result == 0 && (buffer.st_mode & S_IFDIR) != 0)
        return tmp;
    bdestroy(tmp);
    return NULL;
}

///
/// @brief Retrieves the directory that contains toolchain kernels.
///
/// Retrieves the directory that contains toolchain kernels or NULL if this is not a deployed
/// build of the toolchain.
///
/// When this is a deployed build of the toolchain, the kernels directory is based on the
/// installation path of the toolchain.
///
/// @return The path of the directory holding toolchain kernels.  The result must be freed manually.
///
bstring osutil_getkernelpath()
{
    bstring tmp;
    int result;
    struct stat buffer;
    char* env = getenv("TOOLCHAIN_KERNELS");
    if (env == NULL)
    {
#ifdef DCPU_CONFIG_HAS_KERNEL_PATH
        tmp = bfromcstr(DCPU_CONFIG_KERNEL_PATH);
#else
        tmp = osutil_getarg0path();
#ifdef _WIN32
        bcatcstr(tmp, "\\kernel");
#else
        bcatcstr(tmp, "/kernel");
#endif
#endif
    }
    else
        tmp = bfromcstr(env);
    
    // Locate the correct path.
    result = stat((const char*)tmp->data, &buffer);
    if (result == 0 && (buffer.st_mode & S_IFDIR) != 0)
        return tmp;
    binsertch(tmp, 0, 1, '.');
    result = stat((const char*)tmp->data, &buffer);
    if (result == 0 && (buffer.st_mode & S_IFDIR) != 0)
        return tmp;
    bdestroy(tmp);
    return NULL;
}

///
/// @brief Retrieves the directory that contains toolchain standard libraries.
///
/// Retrieves the directory that contains toolchain standard libraries or NULL if this is not a deployed
/// build of the toolchain.
///
/// When this is a deployed build of the toolchain, the standard libraries directory is based on the
/// installation path of the toolchain.
///
/// @return The path of the directory holding toolchain standard libraries.  The result must be freed manually.
///
bstring osutil_getstdlibpath()
{
    bstring tmp;
    int result;
    struct stat buffer;
    char* env = getenv("TOOLCHAIN_STDLIBS");
    if (env == NULL)
    {
#ifdef DCPU_CONFIG_HAS_STDLIB_PATH
        tmp = bfromcstr(DCPU_CONFIG_STDLIB_PATH);
#else
        tmp = osutil_getarg0path();
#ifdef _WIN32
        bcatcstr(tmp, "\\kernel");
#else
        bcatcstr(tmp, "/kernel");
#endif
#endif
    }
    else
        tmp = bfromcstr(env);
    
    // Locate the correct path.
    result = stat((const char*)tmp->data, &buffer);
    if (result == 0 && (buffer.st_mode & S_IFDIR) != 0)
        return tmp;
    binsertch(tmp, 0, 1, '.');
    result = stat((const char*)tmp->data, &buffer);
    if (result == 0 && (buffer.st_mode & S_IFDIR) != 0)
        return tmp;
    bdestroy(tmp);
    return NULL;
}

///
/// @brief Returns the default kernel to use when linking.
///
const char* osutil_getkerneldefault()
{
#ifdef DCPU_CONFIG_HAS_KERNEL_DEFAULT
    return DCPU_CONFIG_KERNEL_DEFAULT;
#else
    return "stubsys";
#endif
}

///
/// Performs gettimeofday() functionality in a cross-platform manner.
///
/// @param timeval The ostimeval structure to place the result in.
/// @param unused Unused parameter.
///
/// @return Whether the call succeeded.
///
int osutil_gettimeofday(struct ostimeval* tv, void* unused)
{
#ifdef WIN32
    // Sourced from http://stackoverflow.com/questions/2494356/how-to-use-gettimeofday-or-something-equivalent-with-visual-studio-c-2008.
    FILETIME ft;
    int64_t tmpres = 0;
    GetSystemTimeAsFileTime(&ft);

    tmpres = ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    /*converting file time to unix epoch*/
    tmpres /= 10;  /*convert into microseconds*/
    tmpres -= 11644473600000000;
    tv->tv_sec = (__int32)(tmpres * 0.000001);
    tv->tv_usec = (tmpres % 1000000);
    return 0;
#else
    struct timeval t;
    int res = gettimeofday(&t, NULL);
    tv->tv_sec = t.tv_sec;
    tv->tv_usec = t.tv_usec;
    return res;
#endif
}

///
/// Performs usleep() functionality in a cross-platform manner.
///
/// @param milliseconds The number of milliseconds to sleep.
///
void osutil_usleep(int milliseconds)
{
#ifdef WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds);
#endif
}
