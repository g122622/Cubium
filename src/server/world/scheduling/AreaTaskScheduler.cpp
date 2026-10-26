#include "AreaTaskScheduler.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include <algorithm>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::server {

struct AreaTaskScheduler::State : std::enable_shared_from_this<State> {
    enum class Phase { Waiting, Scheduled, Running, Finished };

    struct Task {
        u64 id;
        util::TaskPriority priority;
        std::unique_ptr<util::ITask> body;
        util::TaskCallback callback;
        std::shared_ptr<std::atomic<bool>> abortSignal;
        std::vector<u64> keys;
        size_t waiting = 0;
        u64 generation = 0;
        Phase phase = Phase::Waiting;
    };

    using TaskPtr = std::shared_ptr<Task>;

    struct Order {
        bool operator()(const TaskPtr& a, const TaskPtr& b) const
        {
            if (a->priority != b->priority) {
                return a->priority < b->priority;
            }
            return a->id < b->id;
        }
    };

    struct Position {
        std::set<TaskPtr, Order> tasks;
        TaskPtr running;
    };

    struct Dispatch {
        TaskPtr task;
        u64 generation;
    };

    // 提交后尚未运行的任务可能被更高优先级任务撤回资格。
    // generation 使旧执行凭证失效；它出队时立即返回，不等待区域，也不触碰任务体。
    class DispatchTask final : public util::ITask {
    public:
        DispatchTask(std::shared_ptr<State> state, Dispatch dispatch)
            : m_state(std::move(state))
            , m_dispatch(std::move(dispatch))
        {}

        bool execute(const std::atomic<bool>& abortSignal) override
        {
            m_claimed = m_state->_claim(m_dispatch);
            return m_claimed && m_dispatch.task->body->execute(abortSignal);
        }

        void onCancel() override
        {
            if (!m_claimed) {
                m_claimed = m_state->_claim(m_dispatch);
            }
            if (m_claimed) {
                _cancelBody(m_dispatch.task);
            }
        }

        util::TaskType type() const override
        {
            return m_dispatch.task->body->type();
        }

        std::string description() const override
        {
            return m_dispatch.task->body->description();
        }

        bool claimed() const
        {
            return m_claimed;
        }

    private:
        std::shared_ptr<State> m_state;
        Dispatch m_dispatch;
        bool m_claimed = false;
    };

    explicit State(util::UniversalWorkerPool* pool)
        : executor(pool)
    {}

    static void _cancelBody(const TaskPtr& task)
    {
        try {
            task->body->onCancel();
        }
        catch (const std::exception& e) {
            spdlog::error("[AreaTaskScheduler] Cancellation threw: {}", e.what());
        }
        catch (...) {
            spdlog::error("[AreaTaskScheduler] Cancellation threw an unknown exception");
        }
    }

    static void _callback(const TaskPtr& task, bool success)
    {
        if (!task->callback) {
            return;
        }
        try {
            task->callback(success, task->body.get());
        }
        catch (const std::exception& e) {
            spdlog::error("[AreaTaskScheduler] Callback threw: {}", e.what());
        }
        catch (...) {
            spdlog::error("[AreaTaskScheduler] Callback threw an unknown exception");
        }
    }

    void _scheduleLocked(const TaskPtr& task, std::vector<Dispatch>& ready)
    {
        if (!stopped && task->waiting == 0 && task->phase == Phase::Waiting) {
            task->phase = Phase::Scheduled;
            ready.push_back({task, ++task->generation});
        }
    }

    void _addLocked(const TaskPtr& task, std::vector<Dispatch>& ready)
    {
        task->waiting = task->keys.size();
        for (const u64 key : task->keys) {
            auto& position = positions[key];
            const auto previous =
                position.running ? position.running : (position.tasks.empty() ? nullptr : *position.tasks.begin());
            position.tasks.insert(task);
            if (position.running || *position.tasks.begin() != task) {
                continue;
            }
            --task->waiting;
            if (previous && previous->waiting++ == 0) {
                previous->phase = Phase::Waiting;
                ++previous->generation;
            }
        }
        _scheduleLocked(task, ready);
    }

    void _removeLocked(const TaskPtr& task, std::vector<Dispatch>& ready)
    {
        for (const u64 key : task->keys) {
            const auto it = positions.find(key);
            MC_ASSERT_RELEASE(it != positions.end());
            auto& position = it->second;
            const auto previous = position.running ? position.running : *position.tasks.begin();
            MC_ASSERT_RELEASE(position.tasks.erase(task) == 1);
            if (position.running == task) {
                position.running.reset();
            }
            if (position.tasks.empty()) {
                positions.erase(it);
            } else if (previous == task) {
                const auto& next = *position.tasks.begin();
                MC_ASSERT_RELEASE(next->waiting > 0);
                --next->waiting;
                _scheduleLocked(next, ready);
            }
        }
    }

    bool _claim(const Dispatch& dispatch)
    {
        std::lock_guard lock(mutex);
        const auto& task = dispatch.task;
        if (task->phase != Phase::Scheduled || task->generation != dispatch.generation) {
            return false;
        }
        MC_ASSERT_RELEASE(task->waiting == 0);
        task->phase = Phase::Running;
        for (const u64 key : task->keys) {
            auto& position = positions.at(key);
            MC_ASSERT_RELEASE(!position.running && *position.tasks.begin() == task);
            position.running = task;
        }
        return true;
    }

    void _dispatch(std::vector<Dispatch> ready)
    {
        for (const auto& dispatch : ready) {
            util::UniversalWorkerPool* pool;
            {
                std::lock_guard lock(mutex);
                if (dispatch.task->phase != Phase::Scheduled || dispatch.task->generation != dispatch.generation) {
                    continue;
                }
                pool = executor;
            }
            if (pool == nullptr || !pool->isRunning()) {
                _cancelPending(true, 0);
                return;
            }
            auto self = shared_from_this();
            pool->submit(
                std::make_unique<DispatchTask>(self, dispatch),
                [self, dispatch](bool success, util::ITask* raw) {
                    if (raw == nullptr) {
                        // 执行器与提交并发关闭：一次性结清等待者，避免逐任务递归失败。
                        self->_cancelPending(true, 0);
                    } else if (static_cast<DispatchTask*>(raw)->claimed()) {
                        self->_finish(dispatch.task, success);
                    }
                },
                dispatch.task->priority,
                dispatch.task->abortSignal);
        }
    }

    void _finish(const TaskPtr& task, bool success)
    {
        // 回调可能继续提交本区域任务，必须在锁外调用；当前任务仍占据区域至回调完成。
        _callback(task, success);
        std::vector<Dispatch> ready;
        {
            std::lock_guard lock(mutex);
            MC_ASSERT_RELEASE(task->phase == Phase::Running);
            _removeLocked(task, ready);
            task->phase = Phase::Finished;
            tasks.erase(task->id);
            MC_ASSERT_RELEASE(outstanding > 0);
            --outstanding;
            completion.notify_all();
        }
        _dispatch(std::move(ready));
    }

    bool _cancelPending(bool stop, u64 taskId)
    {
        std::vector<TaskPtr> cancelled;
        std::vector<Dispatch> ready;
        {
            std::lock_guard lock(mutex);
            stopped |= stop;
            for (auto it = tasks.begin(); it != tasks.end();) {
                const auto& task = it->second;
                const bool selected = stop ||
                    (taskId != 0 ? taskId == task->id
                                 : task->abortSignal && task->abortSignal->load(std::memory_order::acquire));
                if (!selected || task->phase == Phase::Running) {
                    ++it;
                    continue;
                }
                _removeLocked(task, ready);
                task->phase = Phase::Finished;
                if (task->abortSignal) {
                    task->abortSignal->store(true, std::memory_order::release);
                }
                cancelled.push_back(task);
                it = tasks.erase(it);
            }
        }
        for (const auto& task : cancelled) {
            _cancelBody(task);
            _callback(task, false);
            std::lock_guard lock(mutex);
            MC_ASSERT_RELEASE(outstanding > 0);
            --outstanding;
            completion.notify_all();
        }
        _dispatch(std::move(ready));
        return !cancelled.empty();
    }

    std::mutex mutex;
    std::condition_variable completion;
    util::UniversalWorkerPool* executor;
    bool stopped = false;
    u64 nextId = 1;
    size_t outstanding = 0;
    std::unordered_map<u64, TaskPtr> tasks;
    std::unordered_map<u64, Position> positions;
};

AreaTaskScheduler::AreaTaskScheduler(util::UniversalWorkerPool* executor)
    : m_state(std::make_shared<State>(executor))
{}

AreaTaskScheduler::~AreaTaskScheduler()
{
    shutdown();
}

void AreaTaskScheduler::setExecutor(util::UniversalWorkerPool* executor)
{
    std::lock_guard lock(m_state->mutex);
    MC_ASSERT_RELEASE(m_state->executor == executor || m_state->outstanding == 0);
    m_state->executor = executor;
}

u64 AreaTaskScheduler::submit(std::unique_ptr<util::ITask> body,
    util::TaskCallback callback,
    ChunkPos center,
    i32 radius,
    util::TaskPriority priority,
    std::shared_ptr<std::atomic<bool>> abortSignal)
{
    MC_ASSERT_RELEASE(body != nullptr);
    auto task = std::make_shared<State::Task>();
    task->body = std::move(body);
    task->callback = std::move(callback);
    task->priority = priority;
    task->abortSignal = std::move(abortSignal);
    const i32 r = std::max(0, radius);
    task->keys.reserve(static_cast<size_t>((2 * r + 1) * (2 * r + 1)));
    for (i32 dz = -r; dz <= r; ++dz) {
        for (i32 dx = -r; dx <= r; ++dx) {
            task->keys.push_back(ChunkPos(center.x + dx, center.z + dz).toId());
        }
    }
    std::vector<State::Dispatch> ready;
    bool accepted;
    {
        std::lock_guard lock(m_state->mutex);
        accepted = !m_state->stopped && m_state->executor != nullptr && m_state->executor->isRunning();
        if (accepted) {
            task->id = m_state->nextId++;
            ++m_state->outstanding;
            m_state->tasks.emplace(task->id, task);
            if (task->abortSignal && task->abortSignal->load(std::memory_order::acquire)) {
                task->keys.clear();
            }
            m_state->_addLocked(task, ready);
        }
    }
    if (!accepted) {
        State::_cancelBody(task);
        State::_callback(task, false);
        return 0;
    }
    m_state->_dispatch(std::move(ready));
    return task->id;
}

bool AreaTaskScheduler::cancel(u64 taskId)
{
    return taskId != 0 && m_state->_cancelPending(false, taskId);
}

void AreaTaskScheduler::pruneCancelledTasks()
{
    m_state->_cancelPending(false, 0);
}

void AreaTaskScheduler::waitForCompletion()
{
    std::unique_lock lock(m_state->mutex);
    m_state->completion.wait(lock, [this] {
        return m_state->outstanding == 0;
    });
}

void AreaTaskScheduler::shutdown()
{
    m_state->_cancelPending(true, 0);
    waitForCompletion();
}

size_t AreaTaskScheduler::pendingTaskCount() const
{
    std::lock_guard lock(m_state->mutex);
    return static_cast<size_t>(std::count_if(m_state->tasks.begin(), m_state->tasks.end(), [](const auto& entry) {
        return entry.second->phase != State::Phase::Running;
    }));
}

size_t AreaTaskScheduler::waitingTaskCount() const
{
    std::lock_guard lock(m_state->mutex);
    return static_cast<size_t>(std::count_if(m_state->tasks.begin(), m_state->tasks.end(), [](const auto& entry) {
        return entry.second->phase == State::Phase::Waiting;
    }));
}

} // namespace mc::server
