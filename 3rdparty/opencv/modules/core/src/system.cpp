/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2015, Itseez Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"
#include <iostream>

namespace cv {

static Mutex* __initialization_mutex = NULL;
Mutex& getInitializationMutex()
{
    if (__initialization_mutex == NULL)
        __initialization_mutex = new Mutex();
    return *__initialization_mutex;
}
// force initialization (single-threaded environment)
Mutex* __initialization_mutex_initializer = &getInitializationMutex();

} // namespace cv

#ifdef _MSC_VER
# if _MSC_VER >= 1700
#  pragma warning(disable:4447) // Disable warning 'main' signature found without threading model
# endif
#endif

#if defined ANDROID || defined __linux__ || defined __FreeBSD__
#  include <unistd.h>
#  include <fcntl.h>
#  include <elf.h>
#if defined ANDROID || defined __linux__
#  include <linux/auxvec.h>
#endif
#endif

#if defined WIN32 || defined _WIN32 || defined WINCE
#ifndef _WIN32_WINNT           // This is needed for the declaration of TryEnterCriticalSection in winbase.h with Visual Studio 2005 (and older?)
  #define _WIN32_WINNT 0x0400  // http://msdn.microsoft.com/en-us/library/ms686857(VS.85).aspx
#endif
#include <windows.h>
#if (_WIN32_WINNT >= 0x0602)
  #include <synchapi.h>
#endif
#undef small
#undef min
#undef max
#undef abs
#include <tchar.h>
#if defined _MSC_VER
  #if _MSC_VER >= 1400
    #include <intrin.h>
  #elif defined _M_IX86
    static void __cpuid(int* cpuid_data, int)
    {
        __asm
        {
            push ebx
            push edi
            mov edi, cpuid_data
            mov eax, 1
            cpuid
            mov [edi], eax
            mov [edi + 4], ebx
            mov [edi + 8], ecx
            mov [edi + 12], edx
            pop edi
            pop ebx
        }
    }
    static void __cpuidex(int* cpuid_data, int, int)
    {
        __asm
        {
            push edi
            mov edi, cpuid_data
            mov eax, 7
            mov ecx, 0
            cpuid
            mov [edi], eax
            mov [edi + 4], ebx
            mov [edi + 8], ecx
            mov [edi + 12], edx
            pop edi
        }
    }
  #endif
#endif

#ifdef WINRT
#include <wrl/client.h>
#ifndef __cplusplus_winrt
#include <windows.storage.h>
#pragma comment(lib, "runtimeobject.lib")
#endif

std::wstring GetTempPathWinRT()
{
#ifdef __cplusplus_winrt
    return std::wstring(Windows::Storage::ApplicationData::Current->TemporaryFolder->Path->Data());
#else
    Microsoft::WRL::ComPtr<ABI::Windows::Storage::IApplicationDataStatics> appdataFactory;
    Microsoft::WRL::ComPtr<ABI::Windows::Storage::IApplicationData> appdataRef;
    Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageFolder> storagefolderRef;
    Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageItem> storageitemRef;
    HSTRING str;
    HSTRING_HEADER hstrHead;
    std::wstring wstr;
    if (FAILED(WindowsCreateStringReference(RuntimeClass_Windows_Storage_ApplicationData,
                                            (UINT32)wcslen(RuntimeClass_Windows_Storage_ApplicationData), &hstrHead, &str)))
        return wstr;
    if (FAILED(RoGetActivationFactory(str, IID_PPV_ARGS(appdataFactory.ReleaseAndGetAddressOf()))))
        return wstr;
    if (FAILED(appdataFactory->get_Current(appdataRef.ReleaseAndGetAddressOf())))
        return wstr;
    if (FAILED(appdataRef->get_TemporaryFolder(storagefolderRef.ReleaseAndGetAddressOf())))
        return wstr;
    if (FAILED(storagefolderRef.As(&storageitemRef)))
        return wstr;
    str = NULL;
    if (FAILED(storageitemRef->get_Path(&str)))
        return wstr;
    wstr = WindowsGetStringRawBuffer(str, NULL);
    WindowsDeleteString(str);
    return wstr;
#endif
}

std::wstring GetTempFileNameWinRT(std::wstring prefix)
{
    wchar_t guidStr[40];
    GUID g;
    CoCreateGuid(&g);
    wchar_t* mask = L"%08x_%04x_%04x_%02x%02x_%02x%02x%02x%02x%02x%02x";
    swprintf(&guidStr[0], sizeof(guidStr)/sizeof(wchar_t), mask,
             g.Data1, g.Data2, g.Data3, UINT(g.Data4[0]), UINT(g.Data4[1]),
             UINT(g.Data4[2]), UINT(g.Data4[3]), UINT(g.Data4[4]),
             UINT(g.Data4[5]), UINT(g.Data4[6]), UINT(g.Data4[7]));

    return prefix.append(std::wstring(guidStr));
}

#endif
#else
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#if defined __MACH__ && defined __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#endif

#ifdef _OPENMP
#include "omp.h"
#endif

#include <stdarg.h>

#if defined __linux__ || defined __APPLE__ || defined __EMSCRIPTEN__ || defined __FreeBSD__
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#if defined ANDROID
#include <sys/sysconf.h>
#endif
#endif

#ifdef ANDROID
# include <android/log.h>
#endif

namespace cv
{

Exception::Exception() { code = 0; line = 0; }

Exception::Exception(int _code, const String& _err, const String& _func, const String& _file, int _line)
: code(_code), err(_err), func(_func), file(_file), line(_line)
{
    formatMessage();
}

Exception::~Exception() throw() {}

/*!
 \return the error description and the context as a text string.
 */
const char* Exception::what() const throw() { return msg.c_str(); }

void Exception::formatMessage()
{
    if( func.size() > 0 )
        msg = format("%s:%d: error: (%d) %s in function %s\n", file.c_str(), line, code, err.c_str(), func.c_str());
    else
        msg = format("%s:%d: error: (%d) %s\n", file.c_str(), line, code, err.c_str());
}

static const char* g_hwFeatureNames[CV_HARDWARE_MAX_FEATURE] = { NULL };

static const char* getHWFeatureName(int id)
{
    return (id < CV_HARDWARE_MAX_FEATURE) ? g_hwFeatureNames[id] : NULL;
}
static const char* getHWFeatureNameSafe(int id)
{
    const char* name = getHWFeatureName(id);
    return name ? name : "Unknown feature";
}

struct HWFeatures
{
    enum { MAX_FEATURE = CV_HARDWARE_MAX_FEATURE };

    HWFeatures(bool run_initialize = false)
    {
        memset( have, 0, sizeof(have[0]) * MAX_FEATURE );
        if (run_initialize)
            initialize();
    }

    static void initializeNames()
    {
        for (int i = 0; i < CV_HARDWARE_MAX_FEATURE; i++)
        {
            g_hwFeatureNames[i] = 0;
        }
        g_hwFeatureNames[CPU_MMX] = "MMX";
        g_hwFeatureNames[CPU_SSE] = "SSE";
        g_hwFeatureNames[CPU_SSE2] = "SSE2";
        g_hwFeatureNames[CPU_SSE3] = "SSE3";
        g_hwFeatureNames[CPU_SSSE3] = "SSSE3";
        g_hwFeatureNames[CPU_SSE4_1] = "SSE4.1";
        g_hwFeatureNames[CPU_SSE4_2] = "SSE4.2";
        g_hwFeatureNames[CPU_POPCNT] = "POPCNT";
        g_hwFeatureNames[CPU_FP16] = "FP16";
        g_hwFeatureNames[CPU_AVX] = "AVX";
        g_hwFeatureNames[CPU_AVX2] = "AVX2";
        g_hwFeatureNames[CPU_FMA3] = "FMA3";

        g_hwFeatureNames[CPU_AVX_512F] = "AVX512F";
        g_hwFeatureNames[CPU_AVX_512BW] = "AVX512BW";
        g_hwFeatureNames[CPU_AVX_512CD] = "AVX512CD";
        g_hwFeatureNames[CPU_AVX_512DQ] = "AVX512DQ";
        g_hwFeatureNames[CPU_AVX_512ER] = "AVX512ER";
        g_hwFeatureNames[CPU_AVX_512IFMA512] = "AVX512IFMA";
        g_hwFeatureNames[CPU_AVX_512PF] = "AVX512PF";
        g_hwFeatureNames[CPU_AVX_512VBMI] = "AVX512VBMI";
        g_hwFeatureNames[CPU_AVX_512VL] = "AVX512VL";

        g_hwFeatureNames[CPU_NEON] = "NEON";
    }

    void initialize(void)
    {
#ifndef WINRT
        if (getenv("OPENCV_DUMP_CONFIG"))
        {
            fprintf(stderr, "\nOpenCV build configuration is:\n%s\n",
                cv::getBuildInformation().c_str());
        }
#endif

        initializeNames();

        int cpuid_data[4] = { 0, 0, 0, 0 };
        int cpuid_data_ex[4] = { 0, 0, 0, 0 };

    #if defined _MSC_VER && (defined _M_IX86 || defined _M_X64)
    #define OPENCV_HAVE_X86_CPUID 1
        __cpuid(cpuid_data, 1);
    #elif defined __GNUC__ && (defined __i386__ || defined __x86_64__)
    #define OPENCV_HAVE_X86_CPUID 1
        #ifdef __x86_64__
        asm __volatile__
        (
         "movl $1, %%eax\n\t"
         "cpuid\n\t"
         :[eax]"=a"(cpuid_data[0]),[ebx]"=b"(cpuid_data[1]),[ecx]"=c"(cpuid_data[2]),[edx]"=d"(cpuid_data[3])
         :
         : "cc"
        );
        #else
        asm volatile
        (
         "pushl %%ebx\n\t"
         "movl $1,%%eax\n\t"
         "cpuid\n\t"
         "popl %%ebx\n\t"
         : "=a"(cpuid_data[0]), "=c"(cpuid_data[2]), "=d"(cpuid_data[3])
         :
         : "cc"
        );
        #endif
    #endif

    #ifdef OPENCV_HAVE_X86_CPUID
        int x86_family = (cpuid_data[0] >> 8) & 15;
        if( x86_family >= 6 )
        {
            have[CV_CPU_MMX]    = (cpuid_data[3] & (1<<23)) != 0;
            have[CV_CPU_SSE]    = (cpuid_data[3] & (1<<25)) != 0;
            have[CV_CPU_SSE2]   = (cpuid_data[3] & (1<<26)) != 0;
            have[CV_CPU_SSE3]   = (cpuid_data[2] & (1<<0)) != 0;
            have[CV_CPU_SSSE3]  = (cpuid_data[2] & (1<<9)) != 0;
            have[CV_CPU_FMA3]   = (cpuid_data[2] & (1<<12)) != 0;
            have[CV_CPU_SSE4_1] = (cpuid_data[2] & (1<<19)) != 0;
            have[CV_CPU_SSE4_2] = (cpuid_data[2] & (1<<20)) != 0;
            have[CV_CPU_POPCNT] = (cpuid_data[2] & (1<<23)) != 0;
            have[CV_CPU_AVX]    = (cpuid_data[2] & (1<<28)) != 0;
            have[CV_CPU_FP16]   = (cpuid_data[2] & (1<<29)) != 0;

            // make the second call to the cpuid command in order to get
            // information about extended features like AVX2
        #if defined _MSC_VER && (defined _M_IX86 || defined _M_X64)
        #define OPENCV_HAVE_X86_CPUID_EX 1
            __cpuidex(cpuid_data_ex, 7, 0);
        #elif defined __GNUC__ && (defined __i386__ || defined __x86_64__)
        #define OPENCV_HAVE_X86_CPUID_EX 1
            #ifdef __x86_64__
            asm __volatile__
            (
             "movl $7, %%eax\n\t"
             "movl $0, %%ecx\n\t"
             "cpuid\n\t"
             :[eax]"=a"(cpuid_data_ex[0]),[ebx]"=b"(cpuid_data_ex[1]),[ecx]"=c"(cpuid_data_ex[2]),[edx]"=d"(cpuid_data_ex[3])
             :
             : "cc"
            );
            #else
            asm volatile
            (
             "pushl %%ebx\n\t"
             "movl $7,%%eax\n\t"
             "movl $0,%%ecx\n\t"
             "cpuid\n\t"
             "movl %%ebx, %0\n\t"
             "popl %%ebx\n\t"
             : "=r"(cpuid_data_ex[1]), "=c"(cpuid_data_ex[2])
             :
             : "cc"
            );
            #endif
        #endif

        #ifdef OPENCV_HAVE_X86_CPUID_EX
            have[CV_CPU_AVX2]   = (cpuid_data_ex[1] & (1<<5)) != 0;

            have[CV_CPU_AVX_512F]       = (cpuid_data_ex[1] & (1<<16)) != 0;
            have[CV_CPU_AVX_512DQ]      = (cpuid_data_ex[1] & (1<<17)) != 0;
            have[CV_CPU_AVX_512IFMA512] = (cpuid_data_ex[1] & (1<<21)) != 0;
            have[CV_CPU_AVX_512PF]      = (cpuid_data_ex[1] & (1<<26)) != 0;
            have[CV_CPU_AVX_512ER]      = (cpuid_data_ex[1] & (1<<27)) != 0;
            have[CV_CPU_AVX_512CD]      = (cpuid_data_ex[1] & (1<<28)) != 0;
            have[CV_CPU_AVX_512BW]      = (cpuid_data_ex[1] & (1<<30)) != 0;
            have[CV_CPU_AVX_512VL]      = (cpuid_data_ex[1] & (1<<31)) != 0;
            have[CV_CPU_AVX_512VBMI]    = (cpuid_data_ex[2] & (1<<1)) != 0;
        #else
            CV_UNUSED(cpuid_data_ex);
        #endif

            bool have_AVX_OS_support = true;
            bool have_AVX512_OS_support = true;
            if (!(cpuid_data[2] & (1<<27)))
                have_AVX_OS_support = false; // OS uses XSAVE_XRSTORE and CPU support AVX
            else
            {
                int xcr0 = 0;
            #ifdef _XCR_XFEATURE_ENABLED_MASK // requires immintrin.h
                xcr0 = (int)_xgetbv(_XCR_XFEATURE_ENABLED_MASK);
            #elif defined __GNUC__ && (defined __i386__ || defined __x86_64__)
                __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
            #endif
                if ((xcr0 & 0x6) != 0x6)
                    have_AVX_OS_support = false; // YMM registers
                if ((xcr0 & 0xe6) != 0xe6)
                    have_AVX512_OS_support = false; // ZMM registers
            }

            if (!have_AVX_OS_support)
            {
                have[CV_CPU_AVX] = false;
                have[CV_CPU_FP16] = false;
                have[CV_CPU_AVX2] = false;
                have[CV_CPU_FMA3] = false;
            }
            if (!have_AVX_OS_support || !have_AVX512_OS_support)
            {
                have[CV_CPU_AVX_512F] = false;
                have[CV_CPU_AVX_512BW] = false;
                have[CV_CPU_AVX_512CD] = false;
                have[CV_CPU_AVX_512DQ] = false;
                have[CV_CPU_AVX_512ER] = false;
                have[CV_CPU_AVX_512IFMA512] = false;
                have[CV_CPU_AVX_512PF] = false;
                have[CV_CPU_AVX_512VBMI] = false;
                have[CV_CPU_AVX_512VL] = false;
            }
        }
    #else
        CV_UNUSED(cpuid_data);
        CV_UNUSED(cpuid_data_ex);
    #endif // OPENCV_HAVE_X86_CPUID

    #if defined ANDROID || defined __linux__
    #ifdef __aarch64__
        have[CV_CPU_NEON] = true;
        have[CV_CPU_FP16] = true;
    #elif defined __arm__
        int cpufile = open("/proc/self/auxv", O_RDONLY);

        if (cpufile >= 0)
        {
            Elf32_auxv_t auxv;
            const size_t size_auxv_t = sizeof(auxv);

            while ((size_t)read(cpufile, &auxv, size_auxv_t) == size_auxv_t)
            {
                if (auxv.a_type == AT_HWCAP)
                {
                    have[CV_CPU_NEON] = (auxv.a_un.a_val & 4096) != 0;
                    have[CV_CPU_FP16] = (auxv.a_un.a_val & 2) != 0;
                    break;
                }
            }

            close(cpufile);
        }
    #endif
    #elif (defined __clang__ || defined __APPLE__)
    #if (defined __ARM_NEON__ || (defined __ARM_NEON && defined __aarch64__))
        have[CV_CPU_NEON] = true;
    #endif
    #if (defined __ARM_FP  && (((__ARM_FP & 0x2) != 0) && defined __ARM_NEON__))
        have[CV_CPU_FP16] = true;
    #endif
    #endif

        int baseline_features[] = { CV_CPU_BASELINE_FEATURES };
        if (!checkFeatures(baseline_features, sizeof(baseline_features) / sizeof(baseline_features[0])))
        {
            fprintf(stderr, "\n"
                    "******************************************************************\n"
                    "* FATAL ERROR:                                                   *\n"
                    "* This OpenCV build doesn't support current CPU/HW configuration *\n"
                    "*                                                                *\n"
                    "* Use OPENCV_DUMP_CONFIG=1 environment variable for details      *\n"
                    "******************************************************************\n");
            fprintf(stderr, "\nRequired baseline features:\n");
            checkFeatures(baseline_features, sizeof(baseline_features) / sizeof(baseline_features[0]), true);
            CV_ErrorNoReturn(cv::Error::StsAssert, "Missing support for required CPU baseline features. Check OpenCV build configuration and required CPU/HW setup.");
        }

        readSettings(baseline_features, sizeof(baseline_features) / sizeof(baseline_features[0]));
    }

    bool checkFeatures(const int* features, int count, bool dump = false)
    {
        bool result = true;
        for (int i = 0; i < count; i++)
        {
            int feature = features[i];
            if (feature)
            {
                if (have[feature])
                {
                    if (dump) fprintf(stderr, "%s - OK\n", getHWFeatureNameSafe(feature));
                }
                else
                {
                    result = false;
                    if (dump) fprintf(stderr, "%s - NOT AVAILABLE\n", getHWFeatureNameSafe(feature));
                }
            }
        }
        return result;
    }

    static inline bool isSymbolSeparator(char c)
    {
        return c == ',' || c == ';' || c == '-';
    }

    void readSettings(const int* baseline_features, int baseline_count)
    {
        bool dump = true;
        const char* disabled_features =
#ifndef WINRT
                getenv("OPENCV_CPU_DISABLE");
#else
                NULL;
#endif
        if (disabled_features && disabled_features[0] != 0)
        {
            const char* start = disabled_features;
            for (;;)
            {
                while (start[0] != 0 && isSymbolSeparator(start[0]))
                {
                    start++;
                }
                if (start[0] == 0)
                    break;
                const char* end = start;
                while (end[0] != 0 && !isSymbolSeparator(end[0]))
                {
                    end++;
                }
                if (end == start)
                    continue;
                cv::String feature(start, end);
                start = end;

                CV_Assert(feature.size() > 0);

                bool found = false;
                for (int i = 0; i < CV_HARDWARE_MAX_FEATURE; i++)
                {
                    if (!g_hwFeatureNames[i]) continue;
                    size_t len = strlen(g_hwFeatureNames[i]);
                    if (len != feature.size()) continue;
                    if (feature.compare(g_hwFeatureNames[i]) == 0)
                    {
                        bool isBaseline = false;
                        for (int k = 0; k < baseline_count; k++)
                        {
                            if (baseline_features[k] == i)
                            {
                                isBaseline = true;
                                break;
                            }
                        }
                        if (isBaseline)
                        {
                            if (dump) fprintf(stderr, "OPENCV: Trying to disable baseline CPU feature: '%s'. This has very limited effect, because code optimizations for this feature are executed unconditionally in the most cases.\n", getHWFeatureNameSafe(i));
                        }
                        if (!have[i])
                        {
                            if (dump) fprintf(stderr, "OPENCV: Trying to disable unavailable CPU feature on the current platform: '%s'.\n", getHWFeatureNameSafe(i));
                        }
                        have[i] = false;

                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    if (dump) fprintf(stderr, "OPENCV: Trying to disable unknown CPU feature: '%s'.\n", feature.c_str());
                }
            }
        }
    }

    bool have[MAX_FEATURE+1];
};

static HWFeatures  featuresEnabled(true), featuresDisabled = HWFeatures(false);
static HWFeatures* currentFeatures = &featuresEnabled;

bool checkHardwareSupport(int feature)
{
    CV_DbgAssert( 0 <= feature && feature <= CV_HARDWARE_MAX_FEATURE );
    return currentFeatures->have[feature];
}


volatile bool useOptimizedFlag = true;

void setUseOptimized( bool flag )
{
    useOptimizedFlag = flag;
    currentFeatures = flag ? &featuresEnabled : &featuresDisabled;

    ipp::setUseIPP(flag);
#ifdef HAVE_OPENCL
    ocl::setUseOpenCL(flag);
#endif
#ifdef HAVE_TEGRA_OPTIMIZATION
    ::tegra::setUseTegra(flag);
#endif
}

bool useOptimized(void)
{
    return useOptimizedFlag;
}

int64 getTickCount(void)
{
#if defined WIN32 || defined _WIN32 || defined WINCE
    LARGE_INTEGER counter;
    QueryPerformanceCounter( &counter );
    return (int64)counter.QuadPart;
#elif defined __linux || defined __linux__
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (int64)tp.tv_sec*1000000000 + tp.tv_nsec;
#elif defined __MACH__ && defined __APPLE__
    return (int64)mach_absolute_time();
#else
    struct timeval tv;
    struct timezone tz;
    gettimeofday( &tv, &tz );
    return (int64)tv.tv_sec*1000000 + tv.tv_usec;
#endif
}

double getTickFrequency(void)
{
#if defined WIN32 || defined _WIN32 || defined WINCE
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (double)freq.QuadPart;
#elif defined __linux || defined __linux__
    return 1e9;
#elif defined __MACH__ && defined __APPLE__
    static double freq = 0;
    if( freq == 0 )
    {
        mach_timebase_info_data_t sTimebaseInfo;
        mach_timebase_info(&sTimebaseInfo);
        freq = sTimebaseInfo.denom*1e9/sTimebaseInfo.numer;
    }
    return freq;
#else
    return 1e6;
#endif
}

#if defined __GNUC__ && (defined __i386__ || defined __x86_64__ || defined __ppc__)
#if defined(__i386__)

int64 getCPUTickCount(void)
{
    int64 x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__x86_64__)

int64 getCPUTickCount(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return (int64)lo | ((int64)hi << 32);
}

#elif defined(__ppc__)

int64 getCPUTickCount(void)
{
    int64 result = 0;
    unsigned upper, lower, tmp;
    __asm__ volatile(
                     "0:                  \n"
                     "\tmftbu   %0           \n"
                     "\tmftb    %1           \n"
                     "\tmftbu   %2           \n"
                     "\tcmpw    %2,%0        \n"
                     "\tbne     0b         \n"
                     : "=r"(upper),"=r"(lower),"=r"(tmp)
                     );
    return lower | ((int64)upper << 32);
}

#else

#error "RDTSC not defined"

#endif

#elif defined _MSC_VER && defined WIN32 && defined _M_IX86

int64 getCPUTickCount(void)
{
    __asm _emit 0x0f;
    __asm _emit 0x31;
}

#else

//#ifdef HAVE_IPP
//int64 getCPUTickCount(void)
//{
//    return ippGetCpuClocks();
//}
//#else
int64 getCPUTickCount(void)
{
    return getTickCount();
}
//#endif

#endif

const String& getBuildInformation()
{
    static String build_info =
#include "version_string.inc"
    ;
    return build_info;
}

String format( const char* fmt, ... )
{
    AutoBuffer<char, 1024> buf;

    for ( ; ; )
    {
        va_list va;
        va_start(va, fmt);
        int bsize = static_cast<int>(buf.size());
#if defined _MSC_VER && __cplusplus < 201103L
        int len = _vsnprintf_s((char *)buf, bsize, _TRUNCATE, fmt, va);
#else
        int len = vsnprintf((char *)buf, bsize, fmt, va);
#endif
        va_end(va);

        if (len < 0 || len >= bsize)
        {
            buf.resize(std::max(bsize << 1, len + 1));
            continue;
        }
        buf[bsize - 1] = 0;
        return String((char *)buf, len);
    }
}

String tempfile( const char* suffix )
{
    String fname;
#ifndef WINRT
    const char *temp_dir = getenv("OPENCV_TEMP_PATH");
#endif

#if defined WIN32 || defined _WIN32
#ifdef WINRT
    RoInitialize(RO_INIT_MULTITHREADED);
    std::wstring temp_dir = GetTempPathWinRT();

    std::wstring temp_file = GetTempFileNameWinRT(L"ocv");
    if (temp_file.empty())
        return String();

    temp_file = temp_dir.append(std::wstring(L"\\")).append(temp_file);
    DeleteFileW(temp_file.c_str());

    char aname[MAX_PATH];
    size_t copied = wcstombs(aname, temp_file.c_str(), MAX_PATH);
    CV_Assert((copied != MAX_PATH) && (copied != (size_t)-1));
    fname = String(aname);
    RoUninitialize();
#else
    char temp_dir2[MAX_PATH] = { 0 };
    char temp_file[MAX_PATH] = { 0 };

    if (temp_dir == 0 || temp_dir[0] == 0)
    {
        ::GetTempPathA(sizeof(temp_dir2), temp_dir2);
        temp_dir = temp_dir2;
    }
    if(0 == ::GetTempFileNameA(temp_dir, "ocv", 0, temp_file))
        return String();

    DeleteFileA(temp_file);

    fname = temp_file;
#endif
# else
#  ifdef ANDROID
    //char defaultTemplate[] = "/mnt/sdcard/__opencv_temp.XXXXXX";
    char defaultTemplate[] = "/data/local/tmp/__opencv_temp.XXXXXX";
#  else
    char defaultTemplate[] = "/tmp/__opencv_temp.XXXXXX";
#  endif

    if (temp_dir == 0 || temp_dir[0] == 0)
        fname = defaultTemplate;
    else
    {
        fname = temp_dir;
        char ech = fname[fname.size() - 1];
        if(ech != '/' && ech != '\\')
            fname = fname + "/";
        fname = fname + "__opencv_temp.XXXXXX";
    }

    const int fd = mkstemp((char*)fname.c_str());
    if (fd == -1) return String();

    close(fd);
    remove(fname.c_str());
# endif

    if (suffix)
    {
        if (suffix[0] != '.')
            return fname + "." + suffix;
        else
            return fname + suffix;
    }
    return fname;
}

static ErrorCallback customErrorCallback = 0;
static void* customErrorCallbackData = 0;
static bool breakOnError = false;

bool setBreakOnError(bool value)
{
    bool prevVal = breakOnError;
    breakOnError = value;
    return prevVal;
}

static bool cv_snprintf(char* buf, int len, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
#if defined _MSC_VER && __cplusplus < 201103L
    int res = _vsnprintf_s((char *)buf, len - 1, _TRUNCATE, fmt, va);
    va_end(va);
    buf[len - 1] = 0;
    return res >= 0 && res < len - 1;
#else
    int res = vsnprintf((char *)buf, len, fmt, va);
    va_end(va);
    return res >= 0 && res < len;
#endif
}

void error( const Exception& exc )
{
    if (customErrorCallback != 0)
        customErrorCallback(exc.code, exc.func.c_str(), exc.err.c_str(),
                            exc.file.c_str(), exc.line, customErrorCallbackData);
    else
    {
        const char* errorStr = cvErrorStr(exc.code);
        char buf[1 << 12];

        cv_snprintf(buf, sizeof(buf),
            "OpenCV Error: %s (%s) in %s, file %s, line %d",
            errorStr, exc.err.c_str(), exc.func.size() > 0 ?
            exc.func.c_str() : "unknown function", exc.file.c_str(), exc.line);
        fprintf( stderr, "%s\n", buf );
        fflush( stderr );
#  ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR, "cv::error()", "%s", buf);
#  endif
    }

    if(breakOnError)
    {
        static volatile int* p = 0;
        *p = 0;
    }

    throw exc;
}

void error(int _code, const String& _err, const char* _func, const char* _file, int _line)
{
    error(cv::Exception(_code, _err, _func, _file, _line));
}


ErrorCallback
redirectError( ErrorCallback errCallback, void* userdata, void** prevUserdata)
{
    if( prevUserdata )
        *prevUserdata = customErrorCallbackData;

    ErrorCallback prevCallback = customErrorCallback;

    customErrorCallback     = errCallback;
    customErrorCallbackData = userdata;

    return prevCallback;
}

}

CV_IMPL int cvCheckHardwareSupport(int feature)
{
    CV_DbgAssert( 0 <= feature && feature <= CV_HARDWARE_MAX_FEATURE );
    return cv::currentFeatures->have[feature];
}

CV_IMPL int cvUseOptimized( int flag )
{
    int prevMode = cv::useOptimizedFlag;
    cv::setUseOptimized( flag != 0 );
    return prevMode;
}

CV_IMPL int64  cvGetTickCount(void)
{
    return cv::getTickCount();
}

CV_IMPL double cvGetTickFrequency(void)
{
    return cv::getTickFrequency()*1e-6;
}

CV_IMPL CvErrorCallback
cvRedirectError( CvErrorCallback errCallback, void* userdata, void** prevUserdata)
{
    return cv::redirectError(errCallback, userdata, prevUserdata);
}

CV_IMPL int cvNulDevReport( int, const char*, const char*,
                            const char*, int, void* )
{
    return 0;
}

CV_IMPL int cvStdErrReport( int, const char*, const char*,
                            const char*, int, void* )
{
    return 0;
}

CV_IMPL int cvGuiBoxReport( int, const char*, const char*,
                            const char*, int, void* )
{
    return 0;
}

CV_IMPL int cvGetErrInfo( const char**, const char**, const char**, int* )
{
    return 0;
}


CV_IMPL const char* cvErrorStr( int status )
{
    static char buf[256];

    switch (status)
    {
    case CV_StsOk :                  return "No Error";
    case CV_StsBackTrace :           return "Backtrace";
    case CV_StsError :               return "Unspecified error";
    case CV_StsInternal :            return "Internal error";
    case CV_StsNoMem :               return "Insufficient memory";
    case CV_StsBadArg :              return "Bad argument";
    case CV_StsNoConv :              return "Iterations do not converge";
    case CV_StsAutoTrace :           return "Autotrace call";
    case CV_StsBadSize :             return "Incorrect size of input array";
    case CV_StsNullPtr :             return "Null pointer";
    case CV_StsDivByZero :           return "Division by zero occurred";
    case CV_BadStep :                return "Image step is wrong";
    case CV_StsInplaceNotSupported : return "Inplace operation is not supported";
    case CV_StsObjectNotFound :      return "Requested object was not found";
    case CV_BadDepth :               return "Input image depth is not supported by function";
    case CV_StsUnmatchedFormats :    return "Formats of input arguments do not match";
    case CV_StsUnmatchedSizes :      return "Sizes of input arguments do not match";
    case CV_StsOutOfRange :          return "One of arguments\' values is out of range";
    case CV_StsUnsupportedFormat :   return "Unsupported format or combination of formats";
    case CV_BadCOI :                 return "Input COI is not supported";
    case CV_BadNumChannels :         return "Bad number of channels";
    case CV_StsBadFlag :             return "Bad flag (parameter or structure field)";
    case CV_StsBadPoint :            return "Bad parameter of type CvPoint";
    case CV_StsBadMask :             return "Bad type of mask argument";
    case CV_StsParseError :          return "Parsing error";
    case CV_StsNotImplemented :      return "The function/feature is not implemented";
    case CV_StsBadMemBlock :         return "Memory block has been corrupted";
    case CV_StsAssert :              return "Assertion failed";
    case CV_GpuNotSupported :        return "No CUDA support";
    case CV_GpuApiCallError :        return "Gpu API call";
    case CV_OpenGlNotSupported :     return "No OpenGL support";
    case CV_OpenGlApiCallError :     return "OpenGL API call";
    };

    sprintf(buf, "Unknown %s code %d", status >= 0 ? "status":"error", status);
    return buf;
}

CV_IMPL int cvGetErrMode(void)
{
    return 0;
}

CV_IMPL int cvSetErrMode(int)
{
    return 0;
}

CV_IMPL int cvGetErrStatus(void)
{
    return 0;
}

CV_IMPL void cvSetErrStatus(int)
{
}


CV_IMPL void cvError( int code, const char* func_name,
                      const char* err_msg,
                      const char* file_name, int line )
{
    cv::error(cv::Exception(code, err_msg, func_name, file_name, line));
}

/* function, which converts int to int */
CV_IMPL int
cvErrorFromIppStatus( int status )
{
    switch (status)
    {
    case CV_BADSIZE_ERR:               return CV_StsBadSize;
    case CV_BADMEMBLOCK_ERR:           return CV_StsBadMemBlock;
    case CV_NULLPTR_ERR:               return CV_StsNullPtr;
    case CV_DIV_BY_ZERO_ERR:           return CV_StsDivByZero;
    case CV_BADSTEP_ERR:               return CV_BadStep;
    case CV_OUTOFMEM_ERR:              return CV_StsNoMem;
    case CV_BADARG_ERR:                return CV_StsBadArg;
    case CV_NOTDEFINED_ERR:            return CV_StsError;
    case CV_INPLACE_NOT_SUPPORTED_ERR: return CV_StsInplaceNotSupported;
    case CV_NOTFOUND_ERR:              return CV_StsObjectNotFound;
    case CV_BADCONVERGENCE_ERR:        return CV_StsNoConv;
    case CV_BADDEPTH_ERR:              return CV_BadDepth;
    case CV_UNMATCHED_FORMATS_ERR:     return CV_StsUnmatchedFormats;
    case CV_UNSUPPORTED_COI_ERR:       return CV_BadCOI;
    case CV_UNSUPPORTED_CHANNELS_ERR:  return CV_BadNumChannels;
    case CV_BADFLAG_ERR:               return CV_StsBadFlag;
    case CV_BADRANGE_ERR:              return CV_StsBadArg;
    case CV_BADCOEF_ERR:               return CV_StsBadArg;
    case CV_BADFACTOR_ERR:             return CV_StsBadArg;
    case CV_BADPOINT_ERR:              return CV_StsBadPoint;

    default:
      return CV_StsError;
    }
}

namespace cv {
bool __termination = false;
}

namespace cv
{

#if defined WIN32 || defined _WIN32 || defined WINCE

struct Mutex::Impl
{
    Impl()
    {
#if (_WIN32_WINNT >= 0x0600)
        ::InitializeCriticalSectionEx(&cs, 1000, 0);
#else
        ::InitializeCriticalSection(&cs);
#endif
        refcount = 1;
    }
    ~Impl() { DeleteCriticalSection(&cs); }

    void lock() { EnterCriticalSection(&cs); }
    bool trylock() { return TryEnterCriticalSection(&cs) != 0; }
    void unlock() { LeaveCriticalSection(&cs); }

    CRITICAL_SECTION cs;
    int refcount;
};

#else

struct Mutex::Impl
{
    Impl()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mt, &attr);
        pthread_mutexattr_destroy(&attr);

        refcount = 1;
    }
    ~Impl() { pthread_mutex_destroy(&mt); }

    void lock() { pthread_mutex_lock(&mt); }
    bool trylock() { return pthread_mutex_trylock(&mt) == 0; }
    void unlock() { pthread_mutex_unlock(&mt); }

    pthread_mutex_t mt;
    int refcount;
};

#endif

Mutex::Mutex()
{
    impl = new Mutex::Impl;
}

Mutex::~Mutex()
{
    if( CV_XADD(&impl->refcount, -1) == 1 )
        delete impl;
    impl = 0;
}

Mutex::Mutex(const Mutex& m)
{
    impl = m.impl;
    CV_XADD(&impl->refcount, 1);
}

Mutex& Mutex::operator = (const Mutex& m)
{
    CV_XADD(&m.impl->refcount, 1);
    if( CV_XADD(&impl->refcount, -1) == 1 )
        delete impl;
    impl = m.impl;
    return *this;
}

void Mutex::lock() { impl->lock(); }
void Mutex::unlock() { impl->unlock(); }
bool Mutex::trylock() { return impl->trylock(); }


//////////////////////////////// thread-local storage ////////////////////////////////

#ifdef WIN32
#ifdef _MSC_VER
#pragma warning(disable:4505) // unreferenced local function has been removed
#endif
#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#endif
#endif

// TLS platform abstraction layer
class TlsAbstraction
{
public:
    TlsAbstraction();
    ~TlsAbstraction();
    void* GetData() const;
    void  SetData(void *pData);

private:
#ifdef WIN32
#ifndef WINRT
    DWORD tlsKey;
#endif
#else // WIN32
    pthread_key_t  tlsKey;
#endif
};

#ifdef WIN32
#ifdef WINRT
static __declspec( thread ) void* tlsData = NULL; // using C++11 thread attribute for local thread data
TlsAbstraction::TlsAbstraction() {}
TlsAbstraction::~TlsAbstraction() {}
void* TlsAbstraction::GetData() const
{
    return tlsData;
}
void  TlsAbstraction::SetData(void *pData)
{
    tlsData = pData;
}
#else //WINRT
TlsAbstraction::TlsAbstraction()
{
    tlsKey = TlsAlloc();
    CV_Assert(tlsKey != TLS_OUT_OF_INDEXES);
}
TlsAbstraction::~TlsAbstraction()
{
    TlsFree(tlsKey);
}
void* TlsAbstraction::GetData() const
{
    return TlsGetValue(tlsKey);
}
void  TlsAbstraction::SetData(void *pData)
{
    CV_Assert(TlsSetValue(tlsKey, pData) == TRUE);
}
#endif
#else // WIN32
TlsAbstraction::TlsAbstraction()
{
    CV_Assert(pthread_key_create(&tlsKey, NULL) == 0);
}
TlsAbstraction::~TlsAbstraction()
{
    CV_Assert(pthread_key_delete(tlsKey) == 0);
}
void* TlsAbstraction::GetData() const
{
    return pthread_getspecific(tlsKey);
}
void  TlsAbstraction::SetData(void *pData)
{
    CV_Assert(pthread_setspecific(tlsKey, pData) == 0);
}
#endif

// Per-thread data structure
struct ThreadData
{
    ThreadData()
    {
        idx = 0;
        slots.reserve(32);
    }

    std::vector<void*> slots; // Data array for a thread
    size_t idx;               // Thread index in TLS storage. This is not OS thread ID!
};

// Main TLS storage class
class TlsStorage
{
public:
    TlsStorage()
    {
        tlsSlots.reserve(32);
        threads.reserve(32);
    }
    ~TlsStorage()
    {
        for(size_t i = 0; i < threads.size(); i++)
        {
            if(threads[i])
            {
                /* Current architecture doesn't allow proper global objects release, so this check can cause crashes

                // Check if all slots were properly cleared
                for(size_t j = 0; j < threads[i]->slots.size(); j++)
                {
                    CV_Assert(threads[i]->slots[j] == 0);
                }
                */
                delete threads[i];
            }
        }
        threads.clear();
    }

    void releaseThread()
    {
        AutoLock guard(mtxGlobalAccess);
        ThreadData *pTD = (ThreadData*)tls.GetData();
        for(size_t i = 0; i < threads.size(); i++)
        {
            if(pTD == threads[i])
            {
                threads[i] = 0;
                break;
            }
        }
        tls.SetData(0);
        delete pTD;
    }

    // Reserve TLS storage index
    size_t reserveSlot()
    {
        AutoLock guard(mtxGlobalAccess);

        // Find unused slots
        for(size_t slot = 0; slot < tlsSlots.size(); slot++)
        {
            if(!tlsSlots[slot])
            {
                tlsSlots[slot] = 1;
                return slot;
            }
        }

        // Create new slot
        tlsSlots.push_back(1);
        return (tlsSlots.size()-1);
    }

    // Release TLS storage index and pass associated data to caller
    void releaseSlot(size_t slotIdx, std::vector<void*> &dataVec, bool keepSlot = false)
    {
        AutoLock guard(mtxGlobalAccess);
        CV_Assert(tlsSlots.size() > slotIdx);

        for(size_t i = 0; i < threads.size(); i++)
        {
            if(threads[i])
            {
                std::vector<void*>& thread_slots = threads[i]->slots;
                if (thread_slots.size() > slotIdx && thread_slots[slotIdx])
                {
                    dataVec.push_back(thread_slots[slotIdx]);
                    thread_slots[slotIdx] = NULL;
                }
            }
        }

        if (!keepSlot)
            tlsSlots[slotIdx] = 0;
    }

    // Get data by TLS storage index
    void* getData(size_t slotIdx) const
    {
        CV_Assert(tlsSlots.size() > slotIdx);

        ThreadData* threadData = (ThreadData*)tls.GetData();
        if(threadData && threadData->slots.size() > slotIdx)
            return threadData->slots[slotIdx];

        return NULL;
    }

    // Gather data from threads by TLS storage index
    void gather(size_t slotIdx, std::vector<void*> &dataVec)
    {
        AutoLock guard(mtxGlobalAccess);
        CV_Assert(tlsSlots.size() > slotIdx);

        for(size_t i = 0; i < threads.size(); i++)
        {
            if(threads[i])
            {
                std::vector<void*>& thread_slots = threads[i]->slots;
                if (thread_slots.size() > slotIdx && thread_slots[slotIdx])
                    dataVec.push_back(thread_slots[slotIdx]);
            }
        }
    }

    // Set data to storage index
    void setData(size_t slotIdx, void* pData)
    {
        CV_Assert(tlsSlots.size() > slotIdx && pData != NULL);

        ThreadData* threadData = (ThreadData*)tls.GetData();
        if(!threadData)
        {
            threadData = new ThreadData;
            tls.SetData((void*)threadData);
            {
                AutoLock guard(mtxGlobalAccess);
                threadData->idx = threads.size();
                threads.push_back(threadData);
            }
        }

        if(slotIdx >= threadData->slots.size())
        {
            AutoLock guard(mtxGlobalAccess);
            while(slotIdx >= threadData->slots.size())
                threadData->slots.push_back(NULL);
        }
        threadData->slots[slotIdx] = pData;
    }

private:
    TlsAbstraction tls; // TLS abstraction layer instance

    Mutex  mtxGlobalAccess;           // Shared objects operation guard
    std::vector<int> tlsSlots;        // TLS keys state
    std::vector<ThreadData*> threads; // Array for all allocated data. Thread data pointers are placed here to allow data cleanup
};

// Create global TLS storage object
static TlsStorage &getTlsStorage()
{
    CV_SINGLETON_LAZY_INIT_REF(TlsStorage, new TlsStorage())
}

TLSDataContainer::TLSDataContainer()
{
    key_ = (int)getTlsStorage().reserveSlot(); // Reserve key from TLS storage
}

TLSDataContainer::~TLSDataContainer()
{
    CV_Assert(key_ == -1); // Key must be released in child object
}

void TLSDataContainer::gatherData(std::vector<void*> &data) const
{
    getTlsStorage().gather(key_, data);
}

void TLSDataContainer::release()
{
    std::vector<void*> data;
    data.reserve(32);
    getTlsStorage().releaseSlot(key_, data); // Release key and get stored data for proper destruction
    key_ = -1;
    for(size_t i = 0; i < data.size(); i++)  // Delete all associated data
        deleteDataInstance(data[i]);
}

void TLSDataContainer::cleanup()
{
    std::vector<void*> data;
    data.reserve(32);
    getTlsStorage().releaseSlot(key_, data, true); // Extract stored data with removal from TLS tables
    for(size_t i = 0; i < data.size(); i++)  // Delete all associated data
        deleteDataInstance(data[i]);
}

void* TLSDataContainer::getData() const
{
    void* pData = getTlsStorage().getData(key_); // Check if data was already allocated
    if(!pData)
    {
        // Create new data instance and save it to TLS storage
        pData = createDataInstance();
        getTlsStorage().setData(key_, pData);
    }
    return pData;
}

TLSData<CoreTLSData>& getCoreTlsData()
{
    CV_SINGLETON_LAZY_INIT_REF(TLSData<CoreTLSData>, new TLSData<CoreTLSData>())
}

#if defined CVAPI_EXPORTS && defined WIN32 && !defined WINCE
#ifdef WINRT
    #pragma warning(disable:4447) // Disable warning 'main' signature found without threading model
#endif

extern "C"
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID lpReserved);

extern "C"
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_THREAD_DETACH || fdwReason == DLL_PROCESS_DETACH)
    {
        if (lpReserved != NULL) // called after ExitProcess() call
        {
            cv::__termination = true;
        }
        else
        {
            // Not allowed to free resources if lpReserved is non-null
            // http://msdn.microsoft.com/en-us/library/windows/desktop/ms682583.aspx
            cv::deleteThreadAllocData();
            cv::getTlsStorage().releaseThread();
        }
    }
    return TRUE;
}
#endif

#ifdef CV_COLLECT_IMPL_DATA
ImplCollector& getImplData()
{
    CV_SINGLETON_LAZY_INIT_REF(ImplCollector, new ImplCollector())
}

void setImpl(int flags)
{
    cv::AutoLock lock(getImplData().mutex);

    getImplData().implFlags = flags;
    getImplData().implCode.clear();
    getImplData().implFun.clear();
}

void addImpl(int flag, const char* func)
{
    cv::AutoLock lock(getImplData().mutex);

    getImplData().implFlags |= flag;
    if(func) // use lazy collection if name was not specified
    {
        size_t index = getImplData().implCode.size();
        if(!index || (getImplData().implCode[index-1] != flag || getImplData().implFun[index-1].compare(func))) // avoid duplicates
        {
            getImplData().implCode.push_back(flag);
            getImplData().implFun.push_back(func);
        }
    }
}

int getImpl(std::vector<int> &impl, std::vector<String> &funName)
{
    cv::AutoLock lock(getImplData().mutex);

    impl    = getImplData().implCode;
    funName = getImplData().implFun;
    return getImplData().implFlags; // return actual flags for lazy collection
}

bool useCollection()
{
    return getImplData().useCollection;
}

void setUseCollection(bool flag)
{
    cv::AutoLock lock(getImplData().mutex);

    getImplData().useCollection = flag;
}
#endif

namespace instr
{
bool useInstrumentation()
{
#ifdef ENABLE_INSTRUMENTATION
    return getInstrumentStruct().useInstr;
#else
    return false;
#endif
}

void setUseInstrumentation(bool flag)
{
#ifdef ENABLE_INSTRUMENTATION
    getInstrumentStruct().useInstr = flag;
#else
    CV_UNUSED(flag);
#endif
}

InstrNode* getTrace()
{
#ifdef ENABLE_INSTRUMENTATION
    return &getInstrumentStruct().rootNode;
#else
    return NULL;
#endif
}

void resetTrace()
{
#ifdef ENABLE_INSTRUMENTATION
    getInstrumentStruct().rootNode.removeChilds();
    getInstrumentTLSStruct().pCurrentNode = &getInstrumentStruct().rootNode;
#endif
}

void setFlags(FLAGS modeFlags)
{
#ifdef ENABLE_INSTRUMENTATION
    getInstrumentStruct().flags = modeFlags;
#else
    CV_UNUSED(modeFlags);
#endif
}
FLAGS getFlags()
{
#ifdef ENABLE_INSTRUMENTATION
    return (FLAGS)getInstrumentStruct().flags;
#else
    return (FLAGS)0;
#endif
}

NodeData::NodeData(const char* funName, const char* fileName, int lineNum, void* retAddress, bool alwaysExpand, cv::instr::TYPE instrType, cv::instr::IMPL implType)
{
    m_funName       = funName;
    m_instrType     = instrType;
    m_implType      = implType;
    m_fileName      = fileName;
    m_lineNum       = lineNum;
    m_retAddress    = retAddress;
    m_alwaysExpand  = alwaysExpand;

    m_threads    = 1;
    m_counter    = 0;
    m_ticksTotal = 0;

    m_funError  = false;
}
NodeData::NodeData(NodeData &ref)
{
    *this = ref;
}
NodeData& NodeData::operator=(const NodeData &right)
{
    this->m_funName      = right.m_funName;
    this->m_instrType    = right.m_instrType;
    this->m_implType     = right.m_implType;
    this->m_fileName     = right.m_fileName;
    this->m_lineNum      = right.m_lineNum;
    this->m_retAddress   = right.m_retAddress;
    this->m_alwaysExpand = right.m_alwaysExpand;

    this->m_threads     = right.m_threads;
    this->m_counter     = right.m_counter;
    this->m_ticksTotal  = right.m_ticksTotal;

    this->m_funError    = right.m_funError;

    return *this;
}
NodeData::~NodeData()
{
}
bool operator==(const NodeData& left, const NodeData& right)
{
    if(left.m_lineNum == right.m_lineNum && left.m_funName == right.m_funName && left.m_fileName == right.m_fileName)
    {
        if(left.m_retAddress == right.m_retAddress || !(cv::instr::getFlags()&cv::instr::FLAGS_EXPAND_SAME_NAMES || left.m_alwaysExpand))
            return true;
    }
    return false;
}

#ifdef ENABLE_INSTRUMENTATION
InstrStruct& getInstrumentStruct()
{
    static InstrStruct instr;
    return instr;
}

InstrTLSStruct& getInstrumentTLSStruct()
{
    return *getInstrumentStruct().tlsStruct.get();
}

InstrNode* getCurrentNode()
{
    return getInstrumentTLSStruct().pCurrentNode;
}

IntrumentationRegion::IntrumentationRegion(const char* funName, const char* fileName, int lineNum, void *retAddress, bool alwaysExpand, TYPE instrType, IMPL implType)
{
    m_disabled    = false;
    m_regionTicks = 0;

    InstrStruct *pStruct = &getInstrumentStruct();
    if(pStruct->useInstr)
    {
        InstrTLSStruct *pTLS = &getInstrumentTLSStruct();

        // Disable in case of failure
        if(!pTLS->pCurrentNode)
        {
            m_disabled = true;
            return;
        }

        int depth = pTLS->pCurrentNode->getDepth();
        if(pStruct->maxDepth && pStruct->maxDepth <= depth)
        {
            m_disabled = true;
            return;
        }

        NodeData payload(funName, fileName, lineNum, retAddress, alwaysExpand, instrType, implType);
        Node<NodeData>* pChild = NULL;

        if(pStruct->flags&FLAGS_MAPPING)
        {
            // Critical section
            cv::AutoLock guard(pStruct->mutexCreate); // Guard from concurrent child creation
            pChild = pTLS->pCurrentNode->findChild(payload);
            if(!pChild)
            {
                pChild = new Node<NodeData>(payload);
                pTLS->pCurrentNode->addChild(pChild);
            }
        }
        else
        {
            pChild = pTLS->pCurrentNode->findChild(payload);
            if(!pChild)
            {
                m_disabled = true;
                return;
            }
        }
        pTLS->pCurrentNode = pChild;

        m_regionTicks = getTickCount();
    }
}

IntrumentationRegion::~IntrumentationRegion()
{
    InstrStruct *pStruct = &getInstrumentStruct();
    if(pStruct->useInstr)
    {
        if(!m_disabled)
        {
            InstrTLSStruct *pTLS = &getInstrumentTLSStruct();

            if (pTLS->pCurrentNode->m_payload.m_implType == cv::instr::IMPL_OPENCL &&
                (pTLS->pCurrentNode->m_payload.m_instrType == cv::instr::TYPE_FUN ||
                    pTLS->pCurrentNode->m_payload.m_instrType == cv::instr::TYPE_WRAPPER))
            {
                cv::ocl::finish(); // TODO Support "async" OpenCL instrumentation
            }

            uint64 ticks = (getTickCount() - m_regionTicks);
            {
                cv::AutoLock guard(pStruct->mutexCount); // Concurrent ticks accumulation
                pTLS->pCurrentNode->m_payload.m_counter++;
                pTLS->pCurrentNode->m_payload.m_ticksTotal += ticks;
                pTLS->pCurrentNode->m_payload.m_tls.get()->m_ticksTotal += ticks;
            }

            pTLS->pCurrentNode = pTLS->pCurrentNode->m_pParent;
        }
    }
}
#endif
}

namespace ipp
{

#ifdef HAVE_IPP
struct IPPInitSingleton
{
public:
    IPPInitSingleton()
    {
        useIPP       = true;
        ippStatus    = 0;
        funcname     = NULL;
        filename     = NULL;
        linen        = 0;
        ippFeatures  = 0;

        const char* pIppEnv = getenv("OPENCV_IPP");
        cv::String env = pIppEnv;
        if(env.size())
        {
            if(env == "disabled")
            {
                std::cerr << "WARNING: IPP was disabled by OPENCV_IPP environment variable" << std::endl;
                useIPP = false;
            }
#if IPP_VERSION_X100 >= 900
            else if(env == "sse")
                ippFeatures = ippCPUID_SSE;
            else if(env == "sse2")
                ippFeatures = ippCPUID_SSE2;
            else if(env == "sse3")
                ippFeatures = ippCPUID_SSE3;
            else if(env == "ssse3")
                ippFeatures = ippCPUID_SSSE3;
            else if(env == "sse41")
                ippFeatures = ippCPUID_SSE41;
            else if(env == "sse42")
                ippFeatures = ippCPUID_SSE42;
            else if(env == "avx")
                ippFeatures = ippCPUID_AVX;
            else if(env == "avx2")
                ippFeatures = ippCPUID_AVX2;
#endif
            else
                std::cerr << "ERROR: Improper value of OPENCV_IPP: " << env.c_str() << std::endl;
        }

        IPP_INITIALIZER(ippFeatures)
        ippFeatures = ippGetEnabledCpuFeatures();
    }

    bool useIPP;

    int         ippStatus; // 0 - all is ok, -1 - IPP functions failed
    const char *funcname;
    const char *filename;
    int         linen;
    Ipp64u      ippFeatures;
};

static IPPInitSingleton& getIPPSingleton()
{
    CV_SINGLETON_LAZY_INIT_REF(IPPInitSingleton, new IPPInitSingleton())
}
#endif

#if OPENCV_ABI_COMPATIBILITY > 300
unsigned long long getIppFeatures()
#else
int getIppFeatures()
#endif
{
#ifdef HAVE_IPP
#if OPENCV_ABI_COMPATIBILITY > 300
    return getIPPSingleton().ippFeatures;
#else
    return (int)getIPPSingleton().ippFeatures;
#endif
#else
    return 0;
#endif
}

void setIppStatus(int status, const char * const _funcname, const char * const _filename, int _line)
{
#ifdef HAVE_IPP
    getIPPSingleton().ippStatus = status;
    getIPPSingleton().funcname = _funcname;
    getIPPSingleton().filename = _filename;
    getIPPSingleton().linen = _line;
#else
    CV_UNUSED(status); CV_UNUSED(_funcname); CV_UNUSED(_filename); CV_UNUSED(_line);
#endif
}

int getIppStatus()
{
#ifdef HAVE_IPP
    return getIPPSingleton().ippStatus;
#else
    return 0;
#endif
}

String getIppErrorLocation()
{
#ifdef HAVE_IPP
    return format("%s:%d %s", getIPPSingleton().filename ? getIPPSingleton().filename : "", getIPPSingleton().linen, getIPPSingleton().funcname ? getIPPSingleton().funcname : "");
#else
    return String();
#endif
}

bool useIPP()
{
#ifdef HAVE_IPP
    CoreTLSData* data = getCoreTlsData().get();
    if(data->useIPP < 0)
    {
        data->useIPP = getIPPSingleton().useIPP;
    }
    return (data->useIPP > 0);
#else
    return false;
#endif
}

void setUseIPP(bool flag)
{
    CoreTLSData* data = getCoreTlsData().get();
#ifdef HAVE_IPP
    data->useIPP = (getIPPSingleton().useIPP)?flag:false;
#else
    (void)flag;
    data->useIPP = false;
#endif
}

} // namespace ipp

} // namespace cv

#ifdef HAVE_TEGRA_OPTIMIZATION

namespace tegra {

bool useTegra()
{
    cv::CoreTLSData* data = cv::getCoreTlsData().get();

    if (data->useTegra < 0)
    {
        const char* pTegraEnv = getenv("OPENCV_TEGRA");
        if (pTegraEnv && (cv::String(pTegraEnv) == "disabled"))
            data->useTegra = false;
        else
            data->useTegra = true;
    }

    return (data->useTegra > 0);
}

void setUseTegra(bool flag)
{
    cv::CoreTLSData* data = cv::getCoreTlsData().get();
    data->useTegra = flag;
}

} // namespace tegra

#endif

/* End of file. */
