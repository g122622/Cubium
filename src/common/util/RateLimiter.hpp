/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>

namespace mc::util {

/**
 * @brief 限流器类，用于限制函数的调用频率
 */
class RateLimiter {
public:
    /**
     * @brief 构造函数
     * @param max_calls_per_sec 每秒内最大调用次数
     */
    explicit RateLimiter(size_t max_calls_per_sec)
        : max_calls_per_sec_(max_calls_per_sec)
        , last_reset_time_(std::chrono::steady_clock::now())
        , call_count_(0)
    {}

    /**
     * @brief 尝试获取调用许可
     * @return true 如果允许调用，false 如果被限流
     */
    bool tryAcquire()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_reset_time_).count();

        if (elapsed >= 1) {
            // 重置计数器
            last_reset_time_ = now;
            call_count_ = 0;
        }

        if (call_count_ < max_calls_per_sec_) {
            ++call_count_;
            return true;
        }

        return false;
    }

    /**
     * @brief 获取剩余可调用次数
     * @return 当前时间窗口内剩余可调用次数
     */
    size_t remaining_calls() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_reset_time_).count();

        if (elapsed >= 1) {
            return max_calls_per_sec_;
        }

        return (call_count_ < max_calls_per_sec_) ? (max_calls_per_sec_ - call_count_) : 0;
    }

    /**
     * @brief 重置限流器
     * @param max_calls_per_sec 新的每秒最大调用次数（可选）
     */
    void reset(size_t max_calls_per_sec = 0)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (max_calls_per_sec > 0) {
            max_calls_per_sec_ = max_calls_per_sec;
        }
        last_reset_time_ = std::chrono::steady_clock::now();
        call_count_ = 0;
    }

private:
    size_t max_calls_per_sec_;
    std::chrono::steady_clock::time_point last_reset_time_;
    size_t call_count_;
    mutable std::mutex mutex_;
};

/**
 * @brief 创建带限流的函数包装器
 * @param func 要限流的函数
 * @param max_calls_per_sec 每秒内最大调用次数
 * @return 限流后的函数包装器
 */
template <typename Func>
auto make_rate_limited(Func&& func, size_t max_calls_per_sec)
{
    auto limiter = std::make_shared<RateLimiter>(max_calls_per_sec);

    return [limiter, func = std::forward<Func>(func)](auto&&... args) -> std::invoke_result_t<Func, decltype(args)...> {
        using ReturnType = std::invoke_result_t<Func, decltype(args)...>;

        if (limiter->tryAcquire()) {
            if constexpr (std::is_void_v<ReturnType>) {
                func(std::forward<decltype(args)>(args)...);
            } else {
                return func(std::forward<decltype(args)>(args)...);
            }
        } else {
            // 被限流时的处理，可以抛出异常或返回默认值
            throw std::runtime_error("Rate limit exceeded");
        }
    };
}

/**
 * @brief 创建带限流的函数包装器（无返回值版本）
 * @param func 要限流的函数
 * @param max_calls_per_sec 每秒内最大调用次数
 * @param on_limit_exceeded 限流时的回调函数
 * @return 限流后的函数包装器
 */
template <typename Func, typename OnLimit>
auto make_rate_limited_with_callback(Func&& func, size_t max_calls_per_sec, OnLimit&& on_limit)
{
    auto limiter = std::make_shared<RateLimiter>(max_calls_per_sec);

    return [limiter, func = std::forward<Func>(func), on_limit = std::forward<OnLimit>(on_limit)](auto&&... args) {
        if (limiter->tryAcquire()) {
            func(std::forward<decltype(args)>(args)...);
        } else {
            on_limit();
        }
    };
}

} // namespace mc::util

// // example.cpp - 使用示例
// #include "rate_limiter.hpp"
// #include <iostream>
// #include <thread>

// void test_function(int id) {
//     std::cout << "Function called with id: " << id
//               << " at " << std::chrono::system_clock::now().time_since_epoch().count()
//               << std::endl;
// }

// int add(int a, int b) {
//     return a + b;
// }

// int main() {
//     using namespace mc::util::log;

//     // 示例1：直接使用限流器
//     RateLimiter limiter(5); // 每秒最多5次调用

//     for (int i = 0; i < 10; ++i) {
//         if (limiter.tryAcquire()) {
//             std::cout << "Call " << i << " allowed" << std::endl;
//         } else {
//             std::cout << "Call " << i << " blocked" << std::endl;
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }

//     std::cout << "\n--- 等待1秒重置 ---\n" << std::endl;
//     std::this_thread::sleep_for(std::chrono::seconds(1));

//     // 示例2：使用包装器限流函数
//     auto limited_test = make_rate_limited(test_function, 3);

//     for (int i = 0; i < 10; ++i) {
//         try {
//             limited_test(i);
//         } catch (const std::runtime_error& e) {
//             std::cout << "Call " << i << ": " << e.what() << std::endl;
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(200));
//     }

//     std::cout << "\n--- 带返回值的函数限流 ---\n" << std::endl;

//     auto limited_add = make_rate_limited(add, 2);

//     for (int i = 0; i < 5; ++i) {
//         try {
//             int result = limited_add(i, i * 2);
//             std::cout << "add(" << i << ", " << i * 2 << ") = " << result << std::endl;
//         } catch (const std::runtime_error& e) {
//             std::cout << "Call " << i << ": " << e.what() << std::endl;
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(300));
//     }

//     std::cout << "\n--- 带回调的限流函数 ---\n" << std::endl;

//     auto limited_with_callback = make_rate_limited_with_callback(
//         test_function, 2, []() {
//             std::cout << "Rate limit exceeded, call dropped!" << std::endl;
//         }
//     );

//     for (int i = 0; i < 10; ++i) {
//         limited_with_callback(i);
//         std::this_thread::sleep_for(std::chrono::milliseconds(150));
//     }

//     return 0;
// }
