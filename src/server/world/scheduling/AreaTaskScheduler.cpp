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
    enum class Phase : u8 { Waiting, Scheduled, Running, Finished };

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
            if (!m_claimed) {
                // 资格已被撤回：任务体由调度器在 _dispatch 中释放。
                return false;
            }
            m_type = m_dispatch.task->body->type();
            m_description = m_dispatch.task->body->description();
            // 任务体抛异常时 worker 池会捕获并转成失败回调，但异常路径会跳过下面的
            // releaseBody()。若此时不释放，任务体将活到 _finish 的断言之后，既不满足
            // 「完成即释放」的时序约定，也让区域锁释放在断言处中断。
            try {
                const bool success = m_dispatch.task->body->execute(abortSignal);
                releaseBody();
                return success;
            }
            catch (...) {
                releaseBody();
                throw;
            }
        }

        void onCancel() override
        {
            if (!m_claimed) {
                // 计算池的取消发生在执行之前（出队时发现已取消）：这里认领区域后立即取消任务体。
                // 此前该分支直接调用 onCancel 而未经 claim，任务体自然也没登记，而执行路径
                // 结束时已经释放过任务体——认领后就是第二次取消一个已释放的任务体（UBSan 空指针）。
                // 认领时登记任务体，完成/取消的释放点因此统一。
                m_claimed = m_state->_claim(m_dispatch);
                if (m_claimed) {
                    _cancelBody(m_dispatch.task);
                    releaseBody();
                }
            }
        }

        /// 任务体释放后，计算池仍会在回调整段查询类型与描述（日志/追踪），
        /// 故执行前先缓存一份，不能转发到已释放的任务体。
        util::TaskType type() const override
        {
            if (m_dispatch.task->body != nullptr) {
                return m_dispatch.task->body->type();
            }
            return m_type;
        }

        std::string description() const override
        {
            if (m_dispatch.task->body != nullptr) {
                return m_dispatch.task->body->description();
            }
            return m_description;
        }

        bool claimed() const
        {
            return m_claimed;
        }

        /// 任务体自身持有的世界数据（光照任务的 provider 保活 5×5 区块等）随任务体存放。
        /// 任务一结束（完成或取消）就经此释放，而不是等调度器的最后一个引用消失：世界关闭时
        /// 服务器侧的 shared_ptr 仍在，若等到那一步再析构任务体，其释放的区块数据惯常先于
        /// 世界析构，且析构时机落到另一条调用链的上下文里。
        /// 注销登记后由本对象在此销毁任务体：唯一延迟到这里的窗口是 execute 之后、回调之前，
        /// 由 shutdown 的补收兜底，因此任务体的销毁时机不会逃出调度器。
        void releaseBody() noexcept
        {
            m_state->unregisterBody(m_dispatch.task);
            m_dispatch.task->body.reset();
        }

    private:
        std::shared_ptr<State> m_state;
        Dispatch m_dispatch;
        bool m_claimed = false;
        /// 执行前从任务体抄下的类型与描述：任务体释放后计算池仍会查询它们做日志与追踪。
        util::TaskType m_type = util::TaskType::Custom;
        std::string m_description;
    };

    explicit State(util::UniversalWorkerPool* pool)
        : executor(pool)
    {}

    /// 仍持有未释放任务体的任务（进行执行或取消收尾的任务）。释放点分散在调度器锁外
    /// （DispatchTask::execute/onCancel、_cancelPending），任务体之前统一在此登记；
    /// 调度器关闭时据此补收，保证所有任务体都在 shutdown 返回前析构。
    void registerBody(const TaskPtr& task)
    {
        std::lock_guard lock(bodiesMutex);
        pendingBodies.insert(task);
    }

    void unregisterBody(const TaskPtr& task) noexcept
    {
        std::lock_guard lock(bodiesMutex);
        pendingBodies.erase(task);
    }

    /// 补收尚未释放的任务体（调度器关闭时调用，此时不允许再有任务结束）。
    void releaseRemainingBodies()
    {
        std::vector<TaskPtr> remaining;
        {
            std::lock_guard lock(bodiesMutex);
            remaining.assign(pendingBodies.begin(), pendingBodies.end());
            pendingBodies.clear();
        }
        for (TaskPtr& task : remaining) {
            task->body.reset();
        }
    }

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
        registerBody(task);
        for (const u64 key : task->keys) {
            auto& position = positions.at(key);
            MC_ASSERT_RELEASE(position.running == nullptr);
            MC_ASSERT_RELEASE(*position.tasks.begin() == task);
            position.running = task;
        }
        return true;
    }

    void _dispatch(std::vector<Dispatch>& ready)
    {
        for (auto& dispatch : ready) {
            util::UniversalWorkerPool* pool;
            bool stillScheduled;
            {
                std::lock_guard lock(mutex);
                stillScheduled =
                    dispatch.task->phase == Phase::Scheduled && dispatch.task->generation == dispatch.generation;
                pool = executor;
            }
            if (!stillScheduled) {
                // 资格已被更高优先级任务撤回。任务体必须在锁外释放：其析构会连带销毁
                // 任务持有的世界数据（如光照任务保活的区块）。
                dispatch.task->body.reset();
                continue;
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
                        self->_finish(dispatch, success);
                    }
                },
                dispatch.task->priority,
                dispatch.task->abortSignal);
        }
    }

    void _finish(const Dispatch& dispatch, bool success)
    {
        const TaskPtr& task = dispatch.task;
        // 回调可能继续提交本区域任务，必须在锁外调用；当前任务仍占据区域至回调完成。
        // 任务体已在 DispatchTask::execute 结束时释放，这里 callback 拿到的是空指针。
        _callback(task, success);
        // 回调是 DispatchTask 的最后一个使用点，随后计算池会销毁它。任务体此时应当已经释放，
        // 否则会随 DispatchTask 一起销毁，时机就逃出调度器（关服时正是这样落到世界析构链上）。
        // 在 worker 线程无法安全抛错（断言只输出、不中止），漏释放由 shutdown 的补收兜底。
        if (task->body != nullptr) {
            spdlog::critical("[AreaTaskScheduler] task {} finished with body still alive", task->id);
        }
        MC_ASSERT_RELEASE_MSG(task->body == nullptr, "AreaTaskScheduler: task body must be released by completion");
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
        _dispatch(ready);
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
        for (auto& task : cancelled) {
            _cancelBody(task);
            _callback(task, false);
            // 任务已结清，立即释放任务体（与运行完成路径同一释放点）。
            task->body.reset();
            std::lock_guard lock(mutex);
            MC_ASSERT_RELEASE(outstanding > 0);
            --outstanding;
            completion.notify_all();
        }
        _dispatch(ready);
        return !cancelled.empty();
    }

    std::mutex mutex;
    std::mutex bodiesMutex; ///< 保护 pendingBodies；与 mutex 分开，避免释放任务体时夹带调度器锁
    std::set<TaskPtr> pendingBodies;
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
    try {
        shutdown();
    }
    catch (...) {
        // 关闭失败时不能继续销毁仍可能被任务访问的世界，保持析构的终止语义。
        spdlog::critical("[AreaTaskScheduler] Shutdown failed during destruction");
        std::terminate();
    }
}

void AreaTaskScheduler::setExecutor(util::UniversalWorkerPool* executor)
{
    std::lock_guard lock(m_state->mutex);
    if (m_state->executor != executor) {
        MC_ASSERT_RELEASE(m_state->outstanding == 0);
    }
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
    m_state->_dispatch(ready);
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
    // 所有任务与回调均已结束：补收任何尚未释放的任务体，使本调度器不再连带持有世界数据。
    m_state->releaseRemainingBodies();
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
