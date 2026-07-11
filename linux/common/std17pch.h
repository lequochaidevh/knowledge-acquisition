
#pragma once
/*Setup Pre-compile header file*/
// Standard Libraty
#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstring>  // For strrchr
#include <fstream>
#include <functional>
#include <istream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <type_traits>
#include <typeindex>
#include <iostream>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>  // get info
#include <utility>
#include <atomic>
#include <iomanip>
#include <condition_variable>  // queue -> async log

// C libs
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>  // Required for getpid() on Linux
#include <dlfcn.h>   // dloader
#include <cstdint>
#include <poll.h>
#include <errno.h>
// Unix socket
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <netinet/in.h>
// Linux System
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>  // Required for SYS_gettid on Linux embedded/ubuntu

template <typename T>
using Shared = std::shared_ptr<T>;
template <typename T>
using Unique = std::unique_ptr<T>;

// Build share library
#if ON_HARIS_ENGINE
#if DYNAMIC_BUILD
#ifdef _MSC_VER
#define HARIS_API __declspec(dllexport)
#else
#define HARIS_API __attribute__((visibility("default")))
#endif
#else  // Linux
#define HARIS_API
#endif
#else
#if DYNAMIC_IMPORT
#ifdef _MSC_VER
#define HARIS_API __declspec(dllimport)
#else
#define HARIS_API
#endif
#else  // Linux
#define HARIS_API
#endif
#endif

#if defined(__MSVC__)
#define BREAK_POINT __debugbreak()
#else
#define BREAK_POINT raise(SIGTRAP)
#endif

// Static assert when compile C2607
#if defined(__clang__) || defined(__gcc__)
#define HARIS_STATIC_ASSERT _Static_assert
#else
#define HARIS_STATIC_ASSERT static_assert
#endif

// Static cast
#define HARIS_BASE_CLASS_ASSERT(baseClass, derivedClass, message) \
    HARIS_STATIC_ASSERT(std::is_base_of<baseClass, derivedClass>::value &&message)

// Inline function
#if defined(__clang__) || defined(_gcc__)
#define HARIS_FORCE_INLINE __attribute__((always_inline)) inline
#define HARIS_NOINLINE     __attribute__((noinline))
#elif defined(_MSC_VER)
#define HARIS_FORCE_INLINE __forceinline
#define HARIS_NOINLINE     __declspec(noinline)
#else
#define HARIS_FORCE_INLINE inline
#define HARIS_NOINLINE
#endif

#define HARIS_PLATFORM_LINUX 1

// Free memory
#define HARIS_FREE_MEMORY(memory) \
    if (memory != nullptr) {      \
        delete memory;            \
        memory = nullptr;         \
    }

#define BIND_EVENT_FUNCTION(function) \
    [this](auto &... args) -> decltype(auto) { return this->function(std::forward<decltype(args)>(args)...); }

#define INVALID_ID 0

#ifdef HARIS_BUILD_DEBUG
#define HARIS_ENABLE_ASSERTS
#define HARIS_ENABLE_LOGGER
#endif