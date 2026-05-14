#pragma once

#include "common/core/Types.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include <functional>
#include <memory>

namespace mc::server {

/**
 * @brief 区块生成任务
 *
 * 执行区块生成，支持协作取消。
 * 继承自 ITask，提交到 ServerWorkerPool 执行。
 */
class ChunkGenerateTask : public util::ITask {
public:
    /**
     * @brief 生成器函数类型
     *
     * @param chunk 区块中间态
     * @param targetStatus 目标生成阶段
     * @param cancelSignal 取消信号
     */
    using GeneratorFunc =
        std::function<void(ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal)>;

    /**
     * @brief 构造区块生成任务
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 目标生成阶段
     * @param generator 生成器函数
     */
    ChunkGenerateTask(ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus, GeneratorFunc generator);

    ~ChunkGenerateTask() override = default;

    // ITask 接口实现
    bool execute(const std::atomic<bool>& cancelSignal) override;
    void onCancel() override;
    util::TaskType type() const override { return util::TaskType::ChunkGenerate; }
    std::string description() const override;
    const char* traceCategory() const override { return "world.chunk_gen"; }

    /**
     * @brief 获取区块 X 坐标
     */
    [[nodiscard]] ChunkCoord x() const noexcept { return m_x; }

    /**
     * @brief 获取区块 Z 坐标
     */
    [[nodiscard]] ChunkCoord z() const noexcept { return m_z; }

    /**
     * @brief 获取生成的区块结果
     *
     * 调用者获取结果后，此任务不再持有结果。
     *
     * @return 区块中间态，如果生成失败返回 nullptr
     */
    std::unique_ptr<ChunkPrimer> takeResult() { return std::move(m_result); }

    /**
     * @brief 获取目标生成阶段
     */
    [[nodiscard]] const ChunkStatus& targetStatus() const noexcept { return *m_targetStatus; }

private:
    ChunkCoord m_x;
    ChunkCoord m_z;
    const ChunkStatus* m_targetStatus;
    GeneratorFunc m_generator;
    std::unique_ptr<ChunkPrimer> m_result;
    bool m_success = false;
};

} // namespace mc::server
