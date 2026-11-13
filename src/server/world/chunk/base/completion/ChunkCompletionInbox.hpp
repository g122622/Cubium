#pragma once

#include "server/world/chunk/base/completion/ChunkCompletionQueue.hpp"

namespace mc::server {

/**
 * @brief 可由后台回调独立保活的完成接收端。
 *
 * 服务关闭后拒绝新结果。回调只持有此对象，不访问已销毁的生命周期服务。
 * 接收者在主线程按入口快照和预算推进，关闭与后台投递共用互斥锁。
 */
template <typename T>
class ChunkCompletionInbox {
public:
    /** @brief 后台提交一个拥有全部数据的完成项。 */
    void push(T item)
    {
        std::lock_guard lock(m_mutex);
        if (m_accepting) {
            m_pending.push_back(std::move(item));
        }
    }

    /** @brief 主线程接收一批入口处已存在的结果。 */
    template <typename Callback>
    void drain(std::chrono::milliseconds budget, Callback&& callback)
    {
        drainChunkCompletions(m_pending, m_mutex, budget, std::forward<Callback>(callback));
    }

    /** @brief 查询是否仍有等待主线程接收的结果。 */
    bool pending() const
    {
        std::lock_guard lock(m_mutex);
        return !m_pending.empty();
    }

    /** @brief 关闭接收端；后台回调可以在宿主销毁后安全结束。 */
    void close()
    {
        std::lock_guard lock(m_mutex);
        m_accepting = false;
        m_pending.clear();
    }

private:
    mutable std::mutex m_mutex;
    std::deque<T> m_pending;
    bool m_accepting = true;
};

} // namespace mc::server
