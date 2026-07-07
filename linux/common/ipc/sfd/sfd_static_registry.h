#pragma once
#include "std17pch.h"

namespace HarisLinux {

// Struct fields are atomic to support lock-free data synchronization
struct FileDescriptionRefCount {
    std::atomic<int>    fd{-1};
    std::atomic<size_t> count{0};
};

// Static registry to manage reference counts at compile-time without heap allocation
template <size_t MaxSlots = 200>
class StaticFileDescriptionRegistry {
 private:
    // Compile-time fixed-size array allocated in the data segment
    static inline std::array<FileDescriptionRefCount, MaxSlots> _registry{};

    // Fine-grained mutex array to protect read/write operations per slot
    static inline std::array<std::mutex, MaxSlots> _io_mutexes{};

    // Fast hash-index calculation
    static size_t get_slot_index(int fd) { return static_cast<size_t>(fd) % MaxSlots; }

 public:
    // Provides access to the specific mutex bound to the file descriptor
    static std::mutex& get_mutex(int fd) {
        size_t initial_index = get_slot_index(fd);
        for (size_t i = 0; i < MaxSlots; ++i) {
            size_t idx = (initial_index + i) % MaxSlots;
            if (_registry[idx].fd.load(std::memory_order_relaxed) == fd) {
                return _io_mutexes[idx];
            }
        }
        // Fallback to avoid undefined behavior if fd is not found
        static std::mutex fallback_mutex;
        return fallback_mutex;
    }

    // Retains an fd using a fast read scan and an optimized hash-probe allocation
    static void retain(int fd) {
        if (fd < 0) return;

        // --- STEP 1: FAST READ-ONLY SCAN (O(1) in practical cache lines) ---
        // We use a simple hash index to locate the fd instantly if it already exists
        size_t initial_index = get_slot_index(fd);

        for (size_t i = 0; i < MaxSlots; ++i) {
            size_t idx = (initial_index + i) % MaxSlots;

            // RAM -> L3 -> L2 -> L1 -> CPU Core
            int current_fd = _registry[idx].fd.load(std::memory_order_relaxed);

            // If we find the fd already registered, increment and return immediately
            if (current_fd == fd) {
                // Cache Coherence Protocol - MESI
                _registry[idx].count.fetch_add(1, std::memory_order_relaxed);
                return;
            }
            // Optimization: If we hit a completely unused slot (-1), it means this fd
            // has definitely not been registered yet in this neighborhood. Break early!
            if (current_fd == -1) {
                break;
            }
        }

        // --- STEP 2: OPTIMIZED ATOMIC ALLOCATION (No longer loops from 0 to MaxSlots) ---
        // Jump directly back to the calculated hash position to claim the slot
        for (size_t i = 0; i < MaxSlots; ++i) {
            size_t idx  = (initial_index + i) % MaxSlots;
            auto&  slot = _registry[idx];

            int expected_empty = -1;
            // Execute CAS right at the calculated index
            // compare-and-swap (CAS)
            if (slot.fd.compare_exchange_strong(expected_empty, fd, std::memory_order_acq_rel,
                                                std::memory_order_relaxed)) {
                slot.count.store(1, std::memory_order_release);
                return;  // Claimed successfully, exit instantly!
            }

            // If another thread claimed it at the exact same moment, double check the value
            if (expected_empty == fd) {
                slot.count.fetch_add(1, std::memory_order_relaxed);
                return;  // Matches our fd, increment counter and exit instantly!
            }
        }
    }

    // Releases an fd with the same hash-lookup optimization
    // returns true only if the count drops to exactly zero
    static bool release(int fd) {
        if (fd < 0) return false;

        // Scan for the target slot without coarse-grained locking
        size_t initial_index = get_slot_index(fd);

        for (size_t i = 0; i < MaxSlots; ++i) {
            size_t idx  = (initial_index + i) % MaxSlots;
            auto&  slot = _registry[idx];

            if (slot.fd.load(std::memory_order_relaxed) == fd) {
                if (slot.count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    slot.fd.store(-1, std::memory_order_release);
                    return true;  // Signals that the caller must trigger system close()
                }
                return false;
            }
        }
        return false;
    }

    // Lock-free check for the current count, utilizing optimized memory ordering
    static size_t get_count(int fd) {
        for (const auto& slot : _registry) {
            if (slot.fd.load(std::memory_order_relaxed) == fd) {
                return slot.count.load(std::memory_order_relaxed);
            }
        }
        return 0;
    }
};

}  // namespace HarisLinux
