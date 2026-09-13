#include "PoolAllocator.h"

#include <iostream>
#include <string>

int main() {
    // Initialize Cache with a maximum capacity of 3 elements, managed via the pre-allocated Pool
    MemoryFriendlyLRUCache<int, std::string, 3> cache;

    std::cout << "--- Inserting initial 3 elements ---\n";
    cache.put(1, "Data_1");
    cache.put(2, "Data_2");
    cache.put(3, "Data_3");

    // Access element 1 to mark it as "Most Recently Used"
    auto val = cache.get(1);
    if (val) std::cout << "Get Key 1: " << *val << "\n";  // Output: Data_1

    std::cout << "\n--- Inserting 4th element (Exceeding capacity = 3) ---\n";
    // Since Key 1 was just accessed, Key 2 becomes the oldest -> Key 2 will be evicted from the pool memory
    cache.put(4, "Data_4");

    // Check if Key 2 still exists
    if (!cache.get(2).has_value()) {
        std::cout << "Key 2 has been successfully evicted from the Cache!\n";
    }

    // Verify Key 1 is still preserved
    if (cache.get(1).has_value()) {
        std::cout << "Key 1 is still safe inside the Cache.\n";
    }

    return 0;
}
