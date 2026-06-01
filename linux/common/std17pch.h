
#pragma once
/*Setup Pre-compile header file*/
// Standard Libraty
#include <algorithm>
#include <assert.h>
#include <array>
#include <bitset>
#include <cassert>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
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
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <math.h>
#include <type_traits>
#include <typeindex>
#include <signal.h>
#include <iostream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mutex>
#include <chrono>
#include <thread>    // get info
#include <unistd.h>  // get process id
#include <sys/syscall.h>
// queue -> async log
#include <condition_variable>
#include <atomic>

#include <dlfcn.h>  // dloader

#include <cstdint>
// Unix socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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