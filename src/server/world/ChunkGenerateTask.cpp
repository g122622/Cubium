#include "ChunkGenerateTask.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <fmt/format.h>

namespace mc::server {

ChunkGenerateTask::ChunkGenerateTask(ChunkCoord x, ChunkCoord z,
                                       const ChunkStatus& targetStatus,
                                       GeneratorFunc generator)
    : m_x(x)
    , m_z(z)
    , m_targetStatus(&targetStatus)
    , m_generator(std::move(generator))
{
}

bool ChunkGenerateTask::execute(const std::atomic<bool>& cancelSignal)
{
    // 检查取消
    if (cancelSignal.load(std::memory_order_acquire)) {
        return false;
    }

    // 追踪事件
    MC_TRACE_EVENT("world.chunk_gen", "ChunkGenerateTask::execute",
        "pos", fmt::format("({}, {})", m_x, m_z),
        "status", m_targetStatus->name());

    try {
        // 创建区块中间态
        m_result = std::make_unique<ChunkPrimer>(m_x, m_z);

        // 执行生成
        if (m_generator) {
            m_generator(*m_result, *m_targetStatus, cancelSignal);
        }

        // 再次检查取消
        if (cancelSignal.load(std::memory_order_acquire)) {
            m_result.reset();
            return false;
        }

        m_success = true;
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[ChunkGenerateTask] Exception generating chunk ({}, {}): {}",
                      m_x, m_z, e.what());
        m_result.reset();
        return false;
    } catch (...) {
        spdlog::error("[ChunkGenerateTask] Unknown exception generating chunk ({}, {})",
                      m_x, m_z);
        m_result.reset();
        return false;
    }
}

void ChunkGenerateTask::onCancel()
{
    m_result.reset();
}

std::string ChunkGenerateTask::description() const
{
    return fmt::format("ChunkGenerate({}, {}, {})", m_x, m_z, m_targetStatus->name());
}

} // namespace mc::server
