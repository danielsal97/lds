// ============================================================================
// wpq.hpp (Weighted Priority Queue)
// What:  Thread-safe priority queue with blocking Pop()
// Why:   ThreadPool needs thread-safe command queue where Pop() blocks if
//        empty (worker thread sleeps until command arrives)
// How:   Push(element) adds item and notifies waiting threads; Pop() blocks
//        on condition_variable until queue has items
// ============================================================================

#ifndef __ILRD_WPQ_HPP__
#define __ILRD_WPQ_HPP__

#include <condition_variable> // std::condetional_variable
#include <mutex>              // std::mutex , std::lock_guard
#include <queue>              // std::piority_queue

namespace hrd41
{

template <typename T, typename Container = std::vector<T>,
          typename Compare = std::less<typename Container::value_type>>
class WPQ
{
public:
  explicit WPQ(const Compare& compare = Compare(),
               const Container& cont = Container());
  ~WPQ() = default;
  WPQ(const WPQ& other_) = delete;
  WPQ& operator=(const WPQ& other_) = delete;

  // blocking
  T Pop();
  void Push(T& element);
  size_t Size() const;
  bool IsEmpty() const;

private:
  std::priority_queue<T, Container, Compare> m_pq;
  std::condition_variable m_cv;
  mutable std::mutex m_mutex;
};


// ----------------- Ctor ---------------------------------
template <typename T, typename Container, typename Compare>
WPQ<T, Container, Compare>::WPQ(const Compare& compare_,
                                const Container& container_) 
    : m_pq(compare_, container_), m_mutex()
{
    
}

// ----------------- Push ---------------------------------
template <typename T, typename Container, typename Compare>
void WPQ<T, Container, Compare>::Push(T &element_)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_pq.push(element_);
    m_cv.notify_one();
    
}
// ----------------- Pop ----------------------------------
template <typename T, typename Container, typename Compare>
T WPQ<T, Container, Compare>::Pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]{return !m_pq.empty();});
    T ret = m_pq.top();
    m_pq.pop();
    return ret;
}

// ----------------- Size ---------------------------------
template <typename T, typename Container, typename Compare>
size_t WPQ<T, Container, Compare>::Size() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pq.size();
}

// ----------------- IsEmpty ------------------------------
template <typename T, typename Container, typename Compare>
bool WPQ<T, Container, Compare>::IsEmpty() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pq.empty();
}

} // namespace hrd41

#endif // __ILRD_WPQ_HPP__

