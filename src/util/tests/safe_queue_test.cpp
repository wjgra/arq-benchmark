#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <ranges>
#include <thread>

#include "util/logging.hpp"
#include "util/safe_queue.hpp"

constexpr size_t items_to_add_per_thread = 200;
constexpr size_t num_threads = 10;

static void add_items_to_queue(util::SafeQueue<int>& safe_queue)
{
    std::array<std::thread, num_threads> threads;

    for (size_t thread_idx = 0; thread_idx < threads.size(); ++thread_idx) {
        auto add_items = [&safe_queue, thread_idx]() {
            for (size_t offset = 0; offset < items_to_add_per_thread; ++offset) {
                safe_queue.push(thread_idx * items_to_add_per_thread + offset);
            }
        };
        threads[thread_idx] = std::thread(add_items);
    }

    for (auto& th : threads) {
        th.join();
    }
}

TEST_CASE("SafeQueue add items", "[util]")
{
    util::SafeQueue<int> safe_queue;

    REQUIRE(safe_queue.empty());
    REQUIRE(safe_queue.size() == 0);

    add_items_to_queue(safe_queue);

    REQUIRE_FALSE(safe_queue.empty());
    REQUIRE(safe_queue.size() == items_to_add_per_thread * num_threads);
}

TEST_CASE("SafeQueue remove items", "[util]")
{
    util::SafeQueue<int> safe_queue;

    REQUIRE(safe_queue.empty());
    REQUIRE(safe_queue.size() == 0);

    std::array<std::thread, num_threads> threads;

    // Try to remove items from empty queue
    for (auto& th : threads) {
        auto remove_items = [&safe_queue]() {
            for ([[maybe_unused]] const auto i : std::views::iota(0u, items_to_add_per_thread)) {
                auto top = safe_queue.try_pop();
                REQUIRE(top == std::nullopt);
            }
        };
        th = std::thread(remove_items);
    }

    for (auto& th : threads) {
        th.join();
    }

    REQUIRE(safe_queue.empty());
    REQUIRE(safe_queue.size() == 0);

    // Add items, then remove and check queue is empty

    add_items_to_queue(safe_queue);

    for (auto& th : threads) {
        auto remove_items = [&safe_queue]() {
            for ([[maybe_unused]] const auto i : std::views::iota(0u, items_to_add_per_thread)) {
                auto top = safe_queue.try_pop();
                REQUIRE_FALSE(top == std::nullopt);
            }
        };
        th = std::thread(remove_items);
    }

    for (auto& th : threads) {
        th.join();
    }

    REQUIRE(safe_queue.empty());
    REQUIRE(safe_queue.size() == 0);
}

TEST_CASE("SafeQueue add and remove items", "[util]")
{
    util::SafeQueue<int> safe_queue;

    REQUIRE(safe_queue.empty());
    REQUIRE(safe_queue.size() == 0);

    // Add items in one thread, pop them in another

    auto add_items = [&safe_queue]() {
        for (const auto i : std::views::iota(0u, items_to_add_per_thread)) {
            safe_queue.push(i);
        }
    };

    auto remove_items = [&safe_queue]() {
        for (const int i : std::views::iota(0u, items_to_add_per_thread)) {
            auto top = safe_queue.pop_wait();
            REQUIRE(top == i);
        }
    };

    auto add_thread = std::thread(add_items);
    auto remove_thread = std::thread(remove_items);
    add_thread.join();
    remove_thread.join();

    REQUIRE(safe_queue.empty());
    REQUIRE(safe_queue.size() == 0);

    // Add items in multiple threads whilst popping them in multiple threads
    std::array<std::thread, num_threads> add_threads;

    for (size_t thread_idx = 0; thread_idx < add_threads.size(); ++thread_idx) {
        auto add_items = [&safe_queue, thread_idx]() {
            for (size_t offset = 0; offset < items_to_add_per_thread; ++offset) {
                safe_queue.push(thread_idx * items_to_add_per_thread + offset);
            }
        };
        add_threads[thread_idx] = std::thread(add_items);
    }

    // Keep track of which elements have been popped
    std::array<std::atomic<bool>, num_threads * items_to_add_per_thread> element_popped{};

    for (auto& elt : element_popped) {
        elt = false;
    }

    std::array<std::thread, num_threads> remove_threads;

    for (auto& th : remove_threads) {
        auto remove_items = [&safe_queue, &element_popped]() {
            for (size_t offset = 0; offset < items_to_add_per_thread; ++offset) {
                auto top = safe_queue.pop_wait();
                REQUIRE_FALSE(element_popped[top]); // Item should not already have been removed
                element_popped[top] = true;
            }
        };
        th = std::thread(remove_items);
    }

    // Unfortunately concat_view doesn't seem to be supported...
    for (auto& th : add_threads) {
        th.join();
    }
    for (auto& th : remove_threads) {
        th.join();
    }

    // Check all elements added to the queue have also been popped from the queue
    for (const auto& elt : element_popped) {
        REQUIRE(elt);
    }
}
