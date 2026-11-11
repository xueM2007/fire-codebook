#pragma once
#include <atomic>

// 最小自旋锁：同学的代码里用它保护任务队列
struct SpinLock {
    std::atomic<bool> flag{false};
    void lock() {
        while (flag.exchange(true, std::memory_order_acquire)) {
        }
    }
    void unlock() { flag.store(false, std::memory_order_release); }
};
