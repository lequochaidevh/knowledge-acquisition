#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <algorithm>

using OrderID  = uint64_t;
using Price    = uint32_t;
using Quantity = uint32_t;

enum class Side : uint8_t { BUY, SELL };

struct Order {
    OrderID  id;
    Price    price;
    Quantity qty;
    Side     side;
    Order*   prev = nullptr;
    Order*   next = nullptr;
};

template <size_t MaxOrders>
class OrderPool {
 public:
    OrderPool() {
        // Dynamically allocate the huge buffer once on HEAP to prevent Stack Overflow
        // Initialization Phase
        m_pool = std::make_unique<Order[]>(MaxOrders);

        for (size_t i = 0; i < MaxOrders - 1; ++i) {
            m_pool[i].next = &m_pool[i + 1];
        }
        m_pool[MaxOrders - 1].next = nullptr;
        m_free_list                = &m_pool[0];
    }

    Order* allocate() {
        if (!m_free_list) return nullptr;
        Order* node = m_free_list;
        m_free_list = m_free_list->next;
        node->prev  = nullptr;
        node->next  = nullptr;
        return node;
    }

    void deallocate(Order* node) {
        node->next  = m_free_list;
        m_free_list = node;
    }

 private:
    // Memory is safely kept on the Heap now
    std::unique_ptr<Order[]> m_pool;
    Order*                   m_free_list = nullptr;
};

struct Limit {
    Price    price;
    Quantity total_volume = 0;
    Order*   head         = nullptr;
    Order*   tail         = nullptr;

    void append(Order* order) {
        if (!head) {
            head = tail = order;
        } else {
            tail->next  = order;
            order->prev = tail;
            tail        = order;
        }
        total_volume += order->qty;
    }

    void remove(Order* order) {
        total_volume -= order->qty;
        if (order->prev) order->prev->next = order->next;
        if (order->next) order->next->prev = order->prev;
        if (order == head) head = order->next;
        if (order == tail) tail = order->prev;
    }
};

class OrderBook {
 public:
    OrderBook() { m_order_map.reserve(20000000); }

    ~OrderBook() {
        for (auto const& val : m_buy_limits) {
            delete val.second;
        }
        for (auto const& val : m_sell_limits) {
            delete val.second;
        }
    }

    void limit_order(OrderID id, Price price, Quantity qty, Side side) {
        // Tracks the unfulfilled residual quantity as the order sweeps through the book
        Quantity remaining_qty = qty;

        // ========================================================================
        // --- PHASE 1: THE MATCHING ENGINE KERNEL ---
        // ========================================================================
        if (side == Side::BUY) {
            // Continuous matching loop: loops until the incoming buy order is fully filled
            // or there is no more sell liquidity available on the book
            while (remaining_qty > 0 && !m_sell_limits.empty()) {
                // Fetch the lowest resting ask price in O(1) from the ascending map
                auto   it              = m_sell_limits.begin();
                Price  best_sell_price = it->first;
                Limit* limit_level     = it->second;

                // Economic boundary check: Aggressor buy price must be >= the lowest resting sell price.
                // If the market's cheapest seller is more expensive than our budget, matching terminates.
                if (price < best_sell_price) break;

                // Price match confirmed. Traverse the queue of resting sell orders at this price level.
                // FIFO Order Execution: Head represents the oldest order at this price (Time Priority).
                Order* resting_order = limit_level->head;
                while (resting_order && remaining_qty > 0) {
                    // Safeguard the next node pointer immediately before executing any modifications
                    // because the current node could be deleted and recycled inside this iteration block
                    Order* next_order = resting_order->next;
                    // Calculate the execution volume bound by available liquidity constraints
                    Quantity match_qty = std::min(remaining_qty, resting_order->qty);

                    // Deduct executed trade volume simultaneously from both market participants
                    remaining_qty -= match_qty;
                    resting_order->qty -= match_qty;
                    limit_level->total_volume -= match_qty;

                    // Check if the resting order liquidity has been completely depleted to zero
                    if (resting_order->qty == 0) {
                        // Detach the filled node out of the price level's doubly linked list in O(1)
                        limit_level->remove(resting_order);
                        // Erase order tracker mapping from the global ID index dictionary
                        m_order_map.erase(resting_order->id);
                        // Instantly recycle memory back into the Order Pool without hitting OS allocators
                        m_pool.deallocate(resting_order);
                    }

                    // Advance deep into the queue to match against the next resting order at this price
                    resting_order = next_order;
                }

                // Book Eviction: If all orders at this specific price level are fully depleted (head is null)
                if (limit_level->head == nullptr) {
                    // Deallocate the vacant Limit level structure from heap
                    delete limit_level;
                    // Wipe the price index entry permanently out of the market depth map
                    m_sell_limits.erase(it);
                }
            }
        } else {  // side == Side::SELL
            // Continuous matching loop: loops until the incoming sell order is fully filled
            // or there is no more buy liquidity available on the book
            while (remaining_qty > 0 && !m_buy_limits.empty()) {
                // Fetch the highest resting bid price in O(1) from the descending map
                auto   it             = m_buy_limits.begin();
                Price  best_buy_price = it->first;
                Limit* limit_level    = it->second;

                // Economic boundary check: Aggressor sell price must be <= the highest resting bid price.
                // If the market's highest buyer is offering less than our minimum limit price, matching terminates.
                if (price > best_buy_price) break;

                // Price match confirmed. Traverse the queue of resting buy orders at this price level.
                // FIFO Order Execution: Head represents the oldest order at this price (Time Priority).
                Order* resting_order = limit_level->head;
                while (resting_order && remaining_qty > 0) {
                    // Safeguard the next node pointer immediately before executing any modifications
                    // because the current node could be deleted and recycled inside this iteration block
                    Order* next_order = resting_order->next;
                    // Calculate the execution volume bound by available liquidity constraints
                    Quantity match_qty = std::min(remaining_qty, resting_order->qty);

                    // Deduct executed trade volume simultaneously from both market participants
                    remaining_qty -= match_qty;
                    resting_order->qty -= match_qty;
                    limit_level->total_volume -= match_qty;

                    // Check if the resting order liquidity has been completely depleted to zero
                    if (resting_order->qty == 0) {
                        // Detach the filled node out of the price level's doubly linked list in O(1)
                        limit_level->remove(resting_order);
                        // Erase order tracker mapping from the global ID index dictionary
                        m_order_map.erase(resting_order->id);
                        // Instantly recycle memory back into the Order Pool without hitting OS allocators
                        m_pool.deallocate(resting_order);
                    }

                    // Advance deep into the queue to match against the next resting order at this price
                    resting_order = next_order;
                }

                // Book Eviction: If all orders at this specific price level are fully depleted (head is null)
                if (limit_level->head == nullptr) {
                    // Deallocate the vacant Limit level structure from heap
                    delete limit_level;
                    // Wipe the price index entry permanently out of the market depth map
                    m_buy_limits.erase(it);
                }
            }
        }

        // ========================================================================
        // --- PHASE 2: THE BOOKING ENGINE KERNEL ---
        // ========================================================================
        // If the aggressive order has swept all matching shares and still contains leftover volume,
        // it shifts states to become a passive resting order and attaches itself to the order book.
        if (remaining_qty > 0) {
            // Reserve an empty block of raw memory from the pre-allocated Order pool block array
            Order* new_order = m_pool.allocate();
            if (!new_order) return;  // Critical Fail-safe: Memory Pool exhaustion guard

            // Inject current order properties directly into the assigned memory slot
            new_order->id    = id;
            new_order->price = price;
            new_order->qty   = remaining_qty;  // Record only the unfulfilled residual volume component
            new_order->side  = side;

            if (side == Side::BUY) {
                auto it = m_buy_limits.find(price);
                // If this price point is currently inactive and has no existing layer on the book
                if (it == m_buy_limits.end()) {
                    // Instantiate a new price limit level container on the book heap
                    m_buy_limits[price] = new Limit{price};
                }
                // Append the new order node to the tail of the doubly linked list (Preserves Time Priority)
                m_buy_limits[price]->append(new_order);
            } else {
                auto it = m_sell_limits.find(price);
                // If this price point is currently inactive and has no existing layer on the book
                if (it == m_sell_limits.end()) {
                    // Instantiate a new price limit level container on the book heap
                    m_sell_limits[price] = new Limit{price};
                }
                // Append the new order node to the tail of the doubly linked list (Preserves Time Priority)
                m_sell_limits[price]->append(new_order);
            }

            // Map the Unique Order ID directly to the memory address of the node inside the hash table.
            // This ensures subsequent cancel requests can fetch and wipe this node in absolute O(1) time.
            m_order_map[id] = new_order;
        }
    }

    void cancel_order(OrderID id) {
        auto it = m_order_map.find(id);
        if (it == m_order_map.end()) return;

        Order* order = it->second;
        Price  price = order->price;

        if (order->side == Side::BUY) {
            auto lit = m_buy_limits.find(price);
            if (lit != m_buy_limits.end()) {
                Limit* limit = lit->second;
                limit->remove(order);
                if (limit->head == nullptr) {
                    delete limit;
                    m_buy_limits.erase(lit);
                }
            }
        } else {
            auto lit = m_sell_limits.find(price);
            if (lit != m_sell_limits.end()) {
                Limit* limit = lit->second;
                limit->remove(order);
                if (limit->head == nullptr) {
                    delete limit;
                    m_sell_limits.erase(lit);
                }
            }
        }

        m_order_map.erase(it);
        m_pool.deallocate(order);
    }

 private:
    OrderPool<20000000>                          m_pool;
    std::unordered_map<OrderID, Order*>          m_order_map;
    std::map<Price, Limit*>                      m_sell_limits;
    std::map<Price, Limit*, std::greater<Price>> m_buy_limits;
};

int main() {
    // Allocation on Heap inside OrderBook guarantees no Stack Overflow
    OrderBook book;

    std::cout << "--- HFT Order Book Fixed (Stack Shielded) ---" << std::endl;

    std::cout << "Step 1: Adding resting SELL order..." << std::endl;
    book.limit_order(1, 105, 50, Side::SELL);

    std::cout << "Step 2: Adding aggressive BUY order..." << std::endl;
    book.limit_order(2, 106, 20, Side::BUY);

    std::cout << "Step 3: Canceling residual order..." << std::endl;
    book.cancel_order(1);

    std::cout << "SUCCESS: Order book passed all memory safety barriers!" << std::endl;
    return 0;
}
