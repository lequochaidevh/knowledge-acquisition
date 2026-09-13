#include <iostream>
#include <array>
#include <string_view>

// Microcontroller-friendly flat node layout
struct FlatNode {
    int16_t left  = 0;
    int16_t right = 0;
};

// Custom minimal compile-time vector compatible with C++17
template <typename T, size_t Capacity>
struct CompileTimeVector {
    T                  data[Capacity] = {};
    size_t             sz             = 0;
    constexpr void     push_back(const T& val) { data[sz++] = val; }
    constexpr size_t   size() const { return sz; }
    constexpr T&       operator[](size_t idx) { return data[idx]; }
    constexpr const T& operator[](size_t idx) const { return data[idx]; }
};

struct TempNode {
    char    ch          = '\0';
    size_t  freq        = 0;
    int16_t left_child  = -1;
    int16_t right_child = -1;
    bool    is_active   = true;
};

// Temporary structure to record paths at compile time
struct BitPath {
    uint32_t bits = 0;
    size_t   len  = 0;
};

class CompileTimeHuffman {
 public:
    std::array<FlatNode, 128>  tree             = {};
    size_t                     tree_size        = 0;
    std::array<uint8_t, 65000> compressed_bytes = {};
    size_t                     compressed_size  = 0;
    size_t                     original_length  = 0;

    // Compile-time helper to find the bit sequence for a character in the final flat tree
    constexpr BitPath find_path(int16_t node_idx, char target, uint32_t current_bits, size_t depth) const {
        const auto& node = tree[node_idx];

        // Check Left Child
        if (node.left <= 0) {  // It's a leaf
            if (static_cast<char>(-node.left) == target) {
                return {(current_bits << 1) | 0, depth + 1};
            }
        } else {  // It's an internal node
            auto path = find_path(node.left, target, (current_bits << 1) | 0, depth + 1);
            if (path.len > 0) return path;
        }

        // Check Right Child
        if (node.right <= 0) {  // It's a leaf
            if (static_cast<char>(-node.right) == target) {
                return {(current_bits << 1) | 1, depth + 1};
            }
        } else {  // It's an internal node
            auto path = find_path(node.right, target, (current_bits << 1) | 1, depth + 1);
            if (path.len > 0) return path;
        }

        return {0, 0};  // Not found in this branch
    }

    constexpr CompileTimeHuffman(std::string_view input) {
        original_length = input.length();
        if (input.empty()) return;

        size_t freq_map[256] = {0};
        for (size_t i = 0; i < input.length(); ++i) {
            freq_map[static_cast<uint8_t>(input[i])]++;
        }

        CompileTimeVector<TempNode, 256> nodes;
        for (int i = 0; i < 256; ++i) {
            if (freq_map[i] > 0) {
                TempNode leaf;
                leaf.ch          = static_cast<char>(i);
                leaf.freq        = freq_map[i];
                leaf.left_child  = -1;
                leaf.right_child = -1;
                leaf.is_active   = true;
                nodes.push_back(leaf);
            }
        }

        while (true) {
            int first = -1, second = -1;
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (!nodes[i].is_active) continue;
                if (first == -1 || nodes[i].freq < nodes[first].freq) {
                    second = first;
                    first  = i;
                } else if (second == -1 || nodes[i].freq < nodes[second].freq) {
                    second = i;
                }
            }
            if (second == -1) break;

            TempNode parent;
            parent.ch               = '\0';
            parent.freq             = nodes[first].freq + nodes[second].freq;
            parent.left_child       = static_cast<int16_t>(first);
            parent.right_child      = static_cast<int16_t>(second);
            parent.is_active        = true;
            nodes[first].is_active  = false;
            nodes[second].is_active = false;
            nodes.push_back(parent);
        }

        size_t                          root_idx = nodes.size() - 1;
        CompileTimeVector<int16_t, 256> queue;
        queue.push_back(static_cast<int16_t>(root_idx));

        int16_t old_to_new_map[256] = {0};
        for (size_t i = 0; i < 256; ++i) old_to_new_map[i] = -1;

        size_t flat_idx          = 0;
        old_to_new_map[root_idx] = 0;

        for (size_t q = 0; q < queue.size(); ++q) {
            int16_t     curr = queue[q];
            const auto& n    = nodes[curr];
            if (n.ch != '\0') continue;

            FlatNode fn;
            if (nodes[n.left_child].ch != '\0') {
                fn.left = -static_cast<int16_t>(static_cast<uint8_t>(nodes[n.left_child].ch));
            } else {
                flat_idx++;
                old_to_new_map[n.left_child] = static_cast<int16_t>(flat_idx);
                queue.push_back(n.left_child);
                fn.left = static_cast<int16_t>(flat_idx);
            }

            if (nodes[n.right_child].ch != '\0') {
                fn.right = -static_cast<int16_t>(static_cast<uint8_t>(nodes[n.right_child].ch));
            } else {
                flat_idx++;
                old_to_new_map[n.right_child] = static_cast<int16_t>(flat_idx);
                queue.push_back(n.right_child);
                fn.right = static_cast<int16_t>(flat_idx);
            }
            tree[tree_size++] = fn;
        }

        // --- ACTUAL BIT PACKING SYSTEM (Runs at compile-time) ---
        size_t  current_byte_idx  = 0;
        uint8_t current_bit_shift = 0;

        for (size_t i = 0; i < input.length(); ++i) {
            BitPath path = find_path(0, input[i], 0, 0);

            // Push individual bits matching MSB-first orientation
            for (size_t b = 0; b < path.len; ++b) {
                bool bit = (path.bits >> (path.len - 1 - b)) & 1;
                if (bit) {
                    compressed_bytes[current_byte_idx] |= (1 << (7 - current_bit_shift));
                }
                current_bit_shift++;
                if (current_bit_shift == 8) {
                    current_bit_shift = 0;
                    current_byte_idx++;
                }
            }
        }
        compressed_size = current_byte_idx + (current_bit_shift > 0 ? 1 : 0);
    }

    void printLog() const {
        std::cout << "--- CompileTimeHuffman Summary ---\n";
        std::cout << "Original Size:   " << original_length << " bytes\n";
        std::cout << "Compressed Size: " << compressed_size << " bytes\n";
        std::cout << "Flat Tree Size:  " << tree_size << " internal nodes\n";
        std::cout << "----------------------------------\n";
    }
};

// Runtime parser function
std::string decompressEmbeddedData(const CompileTimeHuffman& huff) {
    if (huff.original_length == 0) return "";

    std::string result;
    result.reserve(huff.original_length);

    size_t  characters_decompressed = 0;
    int16_t curr_node_idx           = 0;

    struct BitReader {
        const uint8_t* bytes;
        size_t         byte_idx = 0;
        uint8_t        bit_idx  = 0;

        bool read_bit() {
            bool bit = (bytes[byte_idx] >> (7 - bit_idx)) & 1;
            bit_idx++;
            if (bit_idx == 8) {
                bit_idx = 0;
                byte_idx++;
            }
            return bit;
        }
    };

    BitReader reader{huff.compressed_bytes.data(), 0, 0};

    while (characters_decompressed < huff.original_length) {
        bool            bit  = reader.read_bit();
        const FlatNode& node = huff.tree[curr_node_idx];

        int16_t next_step = (bit == 0) ? node.left : node.right;

        if (next_step <= 0) {
            result.push_back(static_cast<char>(-next_step));
            characters_decompressed++;
            curr_node_idx = 0;
        } else {
            curr_node_idx = next_step;
        }
    }

    return result;
}

int main() {
    constexpr CompileTimeHuffman embeddedStorage(
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parserdad ladsklfjaslk function called \n"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compiledad ladsklfjaslk time log!"
        "Run time parserdad ladsklfjaslk function called \n"
        "Run time parserdad ladsklfjaslk function called \n"
        "Run time parserdad ladsklfjaslk function called \n"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compiledad ladsklfjaslk time log!"
        "Run time parserdad ladsklfjaslk function called \n"
        "Run time parserdad ladsklfjaslk function called \n"
        "Run time parserdad ladsklfjaslk function called \n"
        "Run time parserdad ladsklfjaslk function called \n"
        "huffman compiledad ladsklfjaslk time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser asdfk2 function called \n"
        "Run time parser asdfk2 function called \n"
        "Run time parser asdfk2 function called \n"
        "huffman compile asdfk2 time log!"
        "huffman compile asdfk2 time log!"
        "huffman compile asdfk2 time log!"
        "huffman compile asdfk2 time log!"
        "huffman compile asdfk2 time log!"
        "huffman compile asdfk2 time log!"
        "huffman compile asdfk2 time log!"
        "huffman compile asdfk2 time log!"
        "huffman compile asdfk2 time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile 134 009 time log!"
        "huffman compile 134 009 time log!"
        "huffman compile 134 009 time log!"
        "huffman compile 134 009 time log!"
        "huffman compile 134 009 time log!"
        "huffman compile 134 009 time log!"
        "huffman compile 134 009 time log!"
        "Run time parser 134 009 function called \n"
        "Run time parser 134 009 function called \n"
        "Run time parser 134 009 function called \n"
        "huffman compile 134 009 time log!"
        "huffman compile 134 009 time log!"
        "Run time parser 134 009 function called \n"
        "Run time parser 134 009 function called \n"
        "Run time parser 134 009 function called \n"
        "huffman compile 134 009 time log!"
        "huffman compile 134 009 time log!"
        "Run time parser 134 009 function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"

        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "Run time parser fuasdg  ntion called \n"
        "huffman compile tiasdg  me log!"
        "huffman compile tiasdg  me log!"
        "huffman compile tiasdg  me log!"
        "Run time parser fuasdg  ntion called \n"
        "Run time parser fuasdg  ntion called \n"
        "Run time psa arserdadasdg   ladsklfjaslk function called \n"
        "huffman compiledadasdg   ladsklfjaslk time log!"
        "huffman cosa mpiledadasdg   ladsklfjaslk time log!"
        "Run time parser fuasdg  ntion called \n"
        "huffman cosa mpdsfasaf iledadasdg   ladsklfjaslk time log!"
        "Run time parser fuasdg  ntion called \n"
        "huffman cosa mpdsfasaf iledadasdg   ladsklfjaslk time log!"
        "Run time parser function called \n"
        "huffman cosa mpdsfasaf iledadasdg   ladsklfjaslk time log!"
        "huffman compile time log!"
        "huffman cosa mpdsfasaf ile 13asdg  4 009 time log!"
        "Run time parser function called \n"
        "Run time psa ardsfasaf ser fuasdg  ntion called \n"
        "Run time parser function called \n"
        "Run time psa ardsfasaf ser asasdg  dfasfunction called \n"
        "huffman compile time log!"
        "Run time psa ardsfasaf ser function called \n"
        "huffman compile time log!"
        "huffman compdsfasaf ile time log!"
        "Run time parser function called \n"
        "Run time parser function called \n"
        "huffman compile time log!"
        "huffman compile time log!"
        "Run time parser function called \n"
        "Run time parserdad ladsklfjaslk function called \n");

    // 1. PRINT CLEAN LOG TO CONSOLE
    embeddedStorage.printLog();

    // 2. PARSE AT RUNTIME
    std::string recoveredText = decompressEmbeddedData(embeddedStorage);
    std::cout << "Runtime Parser Output: " << recoveredText << "\n";

    return 0;
}