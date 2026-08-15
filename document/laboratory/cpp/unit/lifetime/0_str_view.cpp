#include <iostream>
#include <unordered_map>
#include <string_view>
#include <string>
#include <vector>

// Global map using string_view as a key.
// DANGER: string_view does not own the string data, it only points to it!
std::unordered_map<std::string_view, int> my_map;

// Inserts an entry into the map using a string_view key
void test_crash(std::string_view dynamic_name) { my_map[dynamic_name] = 10; }

// Searches for a key and prints the result safely
void find_and_print(std::string_view key) {
    auto it = my_map.find(key);
    if (it != my_map.end()) {
        std::cout << "Found '" << it->first << "': " << it->second << "\n";
    } else {
        std::cout << "Key not found!\n";
    }
}

// Accesses the map using operator[].
// If a dead/dangling string_view is passed, it triggers Undefined Behavior during hashing.
void access_dangling_pointer(std::string_view key) {
    std::cout << "--- Accessing map using operator[] ---\n";

    if (my_map.empty()) {
        std::cout << "Map is empty!\n";
        return;
    }
    std::cout << key << " Ptr to address : " << (const void*)key.data() << "\n";

    // operator[] will compute the hash of the key.
    // If the key's memory is deallocated, this reads junk memory or crashes.
    auto num = my_map[key];
    std::cout << "Num value retrieved: " << num << "\n\n";
}

int main() {
    // =========================================================================
    // TEST CASE 1: SAFE USAGE (String Literal)
    // =========================================================================
    std::cout << "===========================================\n";
    std::cout << "TEST 1: String Literal (100% Safe)\n";
    std::cout << "===========================================\n";
    {}
    std::string hihi = "HIHI";

    // "HIHI" is a string literal stored in the static memory area. It lives forever.
    test_crash(hihi);
    find_and_print(hihi);
    access_dangling_pointer(hihi);
#if 0
    // =========================================================================
    // TEST CASE 2: UNDEFINED BEHAVIOR (Dangling Pointer via Local Scope)
    // =========================================================================
    std::cout << "===========================================\n";
    std::cout << "TEST 2: Dynamic String Stack-Use-After-Scope\n";
    std::cout << "===========================================\n";

    {
        // Allocate a dynamic string on the heap inside this local scope {}
        std::string name_on_heap = "User_" + std::to_string(123);

        // string_view inside the map now points to name_on_heap's memory
        test_crash(name_on_heap);

        // Safe here: name_on_heap is still alive
        std::cout << "Before string destruction: ";
        find_and_print("User_123");
    }
    // CRITICAL POINT: name_on_heap goes out of scope and is DESTROYED here!
    // The string_view inside my_map is now pointing to a dead memory address.

    std::cout << "After string destruction (AddressSanitizer will catch this):\n";
    find_and_print("User_123");
    access_dangling_pointer("User_123");

    // =========================================================================
    // TEST CASE 3: FORCED MEMORY CORRUPTION (Heap Overwriting)
    // =========================================================================
    std::cout << "===========================================\n";
    std::cout << "TEST 3: Memory Thrashing & Overwriting\n";
    std::cout << "===========================================\n";

    {
        std::string name_on_heap = "User_" + std::to_string(123);
        test_crash(name_on_heap);
    }  // name_on_heap is destroyed here

    find_and_print("User_123");

    std::cout << "--- Allocating junk strings to overwrite freed heap memory ---\n";
    // Create heavy heap allocations to explicitly overwrite the recently freed memory area
    for (int i = 0; i < 10005; ++i) {
        std::string junk = "Overwrite_Junk_Memory_Pool_" + std::to_string(i);
    }

    std::cout << "After heap memory has been overwritten:\n";
    find_and_print("User_123");
    access_dangling_pointer("User_123");

    // Verify that the safe key from Test 1 is still unaffected
    find_and_print(hihi);

    // =========================================================================
    // TEST CASE 4: EXPLICIT DANGLING KEY ACCESS (Guaranteed Crash with ASan)
    // =========================================================================
    std::cout << "===========================================\n";
    std::cout << "TEST 4: Passing a Dead string_view directly into operator[]\n";
    std::cout << "===========================================\n";
#endif
    // Create a ghost string_view outside the block scope
    std::string_view phantom_key;

    {
        // Use a long string (> 15 chars) to completely bypass Short String Optimization (SSO)
        // This guarantees a real Heap allocation
        std::string long_name = "User_123_Must_Force_Read_Into_This_To_Trigger_ASan_Crash";
        test_crash(long_name);

        // Store the raw heap address into our ghost key before the source string dies
        phantom_key = long_name;
    }  // long_name is completely wiped out from the Heap here

    std::cout << "--- Corrupting the heap aggressively ---\n";
    std::vector<std::string> junk2;
    for (int i = 0; i < 1000000; ++i) {
        junk2.push_back("TRASH_TRASH_TRASH_TRASH_TRASH_TRASH_TRASH_" + std::to_string(i));
    }

    // Forcefully pass the dead memory address into the map's operator[]
    // ASan will detect `heap-use-after-free` or `stack-use-after-scope` immediately!
    std::cout << "Attempting to use the phantom key...\n";
    access_dangling_pointer(phantom_key);
    // std::string_view hihi = "HIHI";
    access_dangling_pointer(hihi);
    return 0;
}

// g++ -std=c++17 str_view.cpp -o test -fsanitize=address && ./test
// remove line 118 and 126 and rebuild and test
// change if 0 to 1 and test
