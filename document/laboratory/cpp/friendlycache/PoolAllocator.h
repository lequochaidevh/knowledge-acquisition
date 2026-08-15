#pragma once
#include <cstddef>
#include <new>
#include <utility>
#include <vector>
#include <stdexcept>
#include <list>
#include <unordered_map>
#include <optional>
#include <memory>
#include <iostream>

/**
 * @brief A memory-friendly allocator that provides fixed-size block pools.
 * @tparam T The element type initially declared.
 * @tparam BlockCount The maximum number of elements the pool can sustain.
 */
template <typename T, std::size_t BlockCount = 1024>
class PoolAllocator {
 private:
    struct Node {
        Node* next;
    };

    // Containers like std::list rebind this allocator to internal nodes which are larger than T.
    // We compute the maximum size required between T and Node to prevent memory corruption/overflow.
    static constexpr std::size_t BlockSize = (sizeof(T) > sizeof(Node)) ? sizeof(T) : sizeof(Node);
    static constexpr std::size_t NodeAlign = (alignof(T) > alignof(Node)) ? alignof(T) : alignof(Node);

    struct alignas(NodeAlign) MemoryBlock {
        char storage[BlockSize];
    };

    // Shared pointer keeps the pre-allocated pool alive across internal STL container rebinds
    std::shared_ptr<std::vector<MemoryBlock>> m_pool;
    Node**                                    m_free_list;

    void init_pool() {
        m_pool      = std::make_shared<std::vector<MemoryBlock>>(BlockCount);
        m_free_list = new Node*(nullptr);
        for (std::size_t i = 0; i < BlockCount; ++i) {
            Node* current_node = reinterpret_cast<Node*>(&((*m_pool)[i]));
            current_node->next = *m_free_list;
            *m_free_list       = current_node;
        }
    }

 public:
    using value_type = T;

    template <typename U>
    struct rebind {
        using other = PoolAllocator<U, BlockCount>;
    };

    // Default Constructor
    PoolAllocator() { init_pool(); }

    // Destructor clears the free list pointer root only when the last proxy allocator goes out of scope
    ~PoolAllocator() {
        if (m_pool.use_count() == 1) {
            delete m_free_list;
        }
    }

    // Copy Constructor required by STL containers
    PoolAllocator(const PoolAllocator& other) noexcept : m_pool(other.m_pool), m_free_list(other.m_free_list) {}

    // Conversion Constructor handling cross-assignment for rebound allocators
    template <typename U>
    PoolAllocator(const PoolAllocator<U, BlockCount>& other) noexcept {
        m_pool      = reinterpret_cast<const std::shared_ptr<std::vector<MemoryBlock>>&>(other._get_pool());
        m_free_list = reinterpret_cast<Node**>(other._get_free_list_ptr());
    }

    // Move Operations
    PoolAllocator(PoolAllocator&&) noexcept = default;
    PoolAllocator& operator=(PoolAllocator&&) noexcept = default;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    // Internal accessors utilized by the conversion constructor
    const auto& _get_pool() const { return m_pool; }
    void*       _get_free_list_ptr() const { return m_free_list; }

    [[nodiscard]] T* allocate(std::size_t n) {
        // std::list c++ function key word - call at m_list.emplace_front(key, value);
        // Dynamic continuous structures like the bucket array in std::unordered_map
        // request n > 1 blocks; fallback safely to standard heap allocation.

        std::cout << "allocate \n";
        if (n > 1) {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
        if (!(*m_free_list)) {
            throw std::bad_alloc();
        }

        Node* node   = *m_free_list;
        *m_free_list = (*m_free_list)->next;
        return reinterpret_cast<T*>(node);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        // std::list c++ function key word - call at m_list.pop_back();

        std::cout << "deallocate \n";
        if (!p) return;
        if (n > 1) {
            ::operator delete(p);
            return;
        }
        Node* node   = reinterpret_cast<Node*>(p);
        node->next   = *m_free_list;
        *m_free_list = node;
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        // std::list c++ key - this->allocate call / C++20 will remove
        std::cout << "construct \n";
        ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p) noexcept {
        p->~U();
    }

    template <typename U>
    bool operator==(const PoolAllocator<U, BlockCount>&) const noexcept {
        return true;
    }

    template <typename U>
    bool operator!=(const PoolAllocator<U, BlockCount>&) const noexcept {
        return false;
    }
};

/**
 * @brief An Least Recently Used Cache using isolated memory allocation policies to mitigate fragmentation.
 */
template <typename Key, typename Value, std::size_t Capacity = 1024>
class MemoryFriendlyLRUCache {
 private:
    using CacheItem = std::pair<Key, Value>;

    // Configured pool allocators for list elements and map indexing elements
    using ListAllocator = PoolAllocator<CacheItem, Capacity>;
    // template< typename T, typename Allocator = std::allocator<T> > class list;
    using CacheList    = std::list<CacheItem, ListAllocator>;
    using ListIterator = typename CacheList::iterator;

    using MapItem      = std::pair<const Key, ListIterator>;
    using MapAllocator = PoolAllocator<MapItem, Capacity>;
    using CacheMap     = std::unordered_map<Key, ListIterator, std::hash<Key>, std::equal_to<Key>, MapAllocator>;

    CacheList   m_list;
    CacheMap    m_map;
    std::size_t m_capacity;

 public:
    explicit MemoryFriendlyLRUCache() : m_capacity(Capacity) {}

    std::optional<Value> get(const Key& key) {
        auto it = m_map.find(key);
        if (it == m_map.end()) {
            return std::nullopt;
        }

        // Bubbles node to front via pointer swapping (no reallocation occurs)
        m_list.splice(m_list.begin(), m_list, it->second);
        return it->second->second;
    }

    void put(const Key& key, const Value& value) {
        auto it = m_map.find(key);

        if (it != m_map.end()) {
            // Key matches: update value contents and refresh its pool position
            it->second->second = value;
            m_list.splice(m_list.begin(), m_list, it->second);
            return;
        }

        // Evict the least recently used element from the back of the list if capacity overflows
        if (m_list.size() >= m_capacity) {
            auto last = m_list.back();
            m_map.erase(last.first);
            m_list.pop_back();
        }

        // Populate new values straight into the front pool node
        m_list.emplace_front(key, value);
        m_map[key] = m_list.begin();
    }

    [[nodiscard]] std::size_t size() const noexcept { return m_list.size(); }
};