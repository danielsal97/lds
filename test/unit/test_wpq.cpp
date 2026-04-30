/*
    WPQ - Thread Safe Priority Queue - Tests
*/
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "wpq.hpp"

using namespace hrd41;
using namespace std::chrono_literals;

// ---- helpers ---------------------------------------------------------------

static int s_passed = 0;
static int s_failed = 0;

#define TEST(name) std::cout << "[TEST] " << name << " ... "
#define PASS() do { std::cout << "PASS\n"; ++s_passed; } while(0)
#define FAIL(msg) do { std::cout << "FAIL (" << msg << ")\n"; ++s_failed; } while(0)

// ---- single-threaded -------------------------------------------------------

void TestPushPop()
{
    TEST("Push / Pop basic");
    WPQ<int> pq;
    pq.Push(*(new int(3)));   // using lvalue as signature requires T&
    int a = 1, b = 2, c = 3;
    WPQ<int> q;
    q.Push(a);
    q.Push(b);
    q.Push(c);
    // std::less -> max-heap, so pop order should be 3, 2, 1
    if (q.Pop() == 3 && q.Pop() == 2 && q.Pop() == 1) PASS();
    else FAIL("wrong pop order");
}

void TestSize()
{
    TEST("Size");
    WPQ<int> q;
    int x = 10, y = 20;
    assert(q.Size() == 0);
    q.Push(x);
    assert(q.Size() == 1);
    q.Push(y);
    assert(q.Size() == 2);
    q.Pop();
    if (q.Size() == 1) PASS();
    else FAIL("wrong size after pop");
}

void TestIsEmpty()
{
    TEST("IsEmpty");
    WPQ<int> q;
    assert(q.IsEmpty());
    int v = 5;
    q.Push(v);
    assert(!q.IsEmpty());
    q.Pop();
    if (q.IsEmpty()) PASS();
    else FAIL("should be empty after popping last element");
}

void TestCustomCompare()
{
    TEST("Custom compare (min-heap)");
    WPQ<int, std::vector<int>, std::greater<int>> q;
    int a = 3, b = 1, c = 2;
    q.Push(a);
    q.Push(b);
    q.Push(c);
    // std::greater -> min-heap, pop order: 1, 2, 3
    if (q.Pop() == 1 && q.Pop() == 2 && q.Pop() == 3) PASS();
    else FAIL("wrong pop order for min-heap");
}

// ---- multi-threaded --------------------------------------------------------

// Multiple producers push, single consumer pops all — verify count
void TestConcurrentPushPop()
{
    TEST("Concurrent push (4 threads) / pop");
    WPQ<int> q;
    const int N = 25; // items per producer thread
    const int THREADS = 4;

    auto producer = [&](int base) {
        for (int i = 0; i < N; ++i) {
            int* v = new int(base + i);
            q.Push(*v);
        }
    };

    std::vector<std::thread> producers;
    for (int t = 0; t < THREADS; ++t)
        producers.emplace_back(producer, t * N);
    for (auto& t : producers)
        t.join();

    int count = 0;
    while (!q.IsEmpty()) { q.Pop(); ++count; }

    if (count == N * THREADS) PASS();
    else FAIL("expected " + std::to_string(N * THREADS) +
              " items, got " + std::to_string(count));
}

// Pop blocks until Push provides an element
void TestPopBlocks()
{
    TEST("Pop blocks until Push");
    WPQ<int> q;
    int result = 0;
    bool popped = false;

    std::thread consumer([&]() {
        result = q.Pop();   // must block here
        popped = true;
    });

    std::this_thread::sleep_for(50ms); // give consumer time to block
    assert(!popped);                   // should still be blocked

    int val = 42;
    q.Push(val);
    consumer.join();

    if (popped && result == 42) PASS();
    else FAIL("pop did not block or returned wrong value");
}

// Producer pushes after a delay; consumer should wait
void TestProducerConsumerOrdering()
{
    TEST("Producer / consumer ordering");
    WPQ<int> q;
    std::vector<int> results;
    std::mutex results_mutex;

    // Consumer runs first, blocks until all items arrive
    std::thread consumer([&]() {
        for (int i = 0; i < 5; ++i) {
            int v = q.Pop();
            std::lock_guard<std::mutex> lk(results_mutex);
            results.push_back(v);
        }
    });

    std::this_thread::sleep_for(30ms);
    int vals[] = {3, 1, 4, 1, 5};
    for (int& v : vals) q.Push(v);

    consumer.join();

    // Max-heap: sorted descending
    std::vector<int> expected = {5, 4, 3, 1, 1};
    if (results == expected) PASS();
    else FAIL("unexpected pop order");
}

// ---- summary ---------------------------------------------------------------

int main()
{
    std::cout << "=== WPQ Tests ===\n\n";

    // single-threaded
    TestPushPop();
    TestSize();
    TestIsEmpty();
    TestCustomCompare();

    // multi-threaded
    TestConcurrentPushPop();
    TestPopBlocks();
    TestProducerConsumerOrdering();

    std::cout << "\n=== Results: " << s_passed << " passed, "
              << s_failed << " failed ===\n";

    return s_failed == 0 ? 0 : 1;
}
