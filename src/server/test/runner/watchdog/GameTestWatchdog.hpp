#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace mc::test {

/**
 * @brief 独立线程监控实际时间期限；主线程阻塞时仍能调用超时处理器。
 *
 * 只共享期限和诊断文字，不读取世界、实例或报告器。析构会唤醒并等待线程退出。
 */
class GameTestWatchdog final {
public:
    GameTestWatchdog(std::chrono::steady_clock::time_point deadline,
        std::string phase,
        std::function<void(const std::string&)> onTimeout)
        : m_deadline(deadline)
        , m_phase(std::move(phase))
        , m_onTimeout(std::move(onTimeout))
        , m_thread([this] { _watch(); })
    {}

    ~GameTestWatchdog()
    {
        std::string expiredPhase;
        {
            std::lock_guard lock(m_mutex);
            if (!m_stopped && std::chrono::steady_clock::now() >= m_deadline) {
                expiredPhase = m_phase;
            }
            m_stopped = true;
        }
        m_changed.notify_one();
        m_thread.join();
        if (!expiredPhase.empty()) {
            m_onTimeout(expiredPhase);
        }
    }

    GameTestWatchdog(const GameTestWatchdog&) = delete;
    GameTestWatchdog& operator=(const GameTestWatchdog&) = delete;

    /** @brief 主线程发布下一阶段的绝对期限，通知监控线程重新计时。 */
    void arm(std::chrono::steady_clock::time_point deadline, std::string phase)
    {
        std::string expiredPhase;
        {
            std::lock_guard lock(m_mutex);
            if (m_stopped) {
                return;
            }
            // 监控线程可能尚未获调度；已经超过的期限不能被下一阶段覆盖。
            if (std::chrono::steady_clock::now() >= m_deadline) {
                m_stopped = true;
                expiredPhase = m_phase;
            } else {
                m_deadline = deadline;
                m_phase = std::move(phase);
                ++m_revision;
            }
        }
        m_changed.notify_one();
        if (!expiredPhase.empty()) {
            m_onTimeout(expiredPhase);
        }
    }

private:
    void _watch()
    {
        std::unique_lock lock(m_mutex);
        while (!m_stopped) {
            const auto revision = m_revision;
            const auto deadline = m_deadline;
            if (m_changed.wait_until(
                    lock, deadline, [this, revision] { return m_stopped || m_revision != revision; })) {
                continue;
            }
            const auto phase = m_phase;
            m_stopped = true;
            lock.unlock();
            m_onTimeout(phase);
            return;
        }
    }

    std::mutex m_mutex;
    std::condition_variable m_changed;
    std::chrono::steady_clock::time_point m_deadline;
    std::string m_phase;
    std::function<void(const std::string&)> m_onTimeout;
    bool m_stopped = false;
    std::size_t m_revision = 0;
    std::thread m_thread;
};

} // namespace mc::test
