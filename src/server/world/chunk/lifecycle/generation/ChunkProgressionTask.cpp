/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING LIMITATION TO THE NOTICE BELOW.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING LIMITATION TO THE NOTICE BELOW.
 *
 * Github: https://github.com/guoyi22
 */

#include "server/world/chunk/lifecycle/generation/ChunkProgressionTask.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "server/world/chunk/base/pipeline/ChunkPyramid.hpp"
#include "server/world/chunk/base/pipeline/ChunkStatus.hpp"
#include "server/world/chunk/base/pipeline/ChunkStep.hpp"
#include "server/world/chunk/lifecycle/generation/ChunkGenerationScheduler.hpp"
#include "server/world/chunk/lifecycle/generation/ChunkStepExecutor.hpp"
#include "server/world/chunk/lifecycle/generation/base/StaticChunkCache2D.hpp"
#include "server/world/chunk/lifecycle/state/ChunkHolder.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server {

using mc::world::chunk::ChunkPrimer;
using mc::world::chunk::ChunkPyramid;
using mc::world::chunk::ChunkStatus;
using mc::world::chunk::ChunkStep;
namespace ChunkStatuses = mc::world::chunk::ChunkStatuses;

// ============================================================================
// 构造
// ============================================================================

ChunkProgressionTask::ChunkProgressionTask(ChunkGenerationScheduler& scheduler,
    ChunkStepExecutor& steps,
    std::function<void(world::chunk::ChunkHolder&, world::chunk::ChunkPrimer&)> publishFull,
    std::shared_ptr<mc::world::chunk::ChunkHolder> holder,
    ChunkCoord x,
    ChunkCoord z,
    const ChunkStatus& toStatus,
    StaticChunkCache2D<std::shared_ptr<mc::world::chunk::ChunkHolder>> neighbours)
    : m_scheduler(scheduler)
    , m_steps(steps)
    , m_publishFull(std::move(publishFull))
    , m_holder(std::move(holder))
    , m_x(x)
    , m_z(z)
    , m_toStatus(&toStatus)
    , m_neighbours(std::move(neighbours))

{}

// ============================================================================
// ITask 接口
// ============================================================================

bool ChunkProgressionTask::execute(const std::atomic<bool>& abortSignal)
{
    {
        // 与票据降级串行决定启动或取消，避免检查之后又开始执行已取消的阶段。
        auto lock = m_scheduler.schedulingLockArea().lock(m_x, m_z, 0);
        if (abortSignal.load(std::memory_order::acquire)) {
            return false;
        }
        m_started = true;
    }

    // EMPTY 走存档加载/新建空 Primer 路径，不调用 IChunkGenerator
    if (*m_toStatus == ChunkStatuses::EMPTY) {
        return executeEmptyLoad(abortSignal);
    }

    return executeStatusStep(abortSignal);
}

void ChunkProgressionTask::onCancel()
{
    // 执行器确认本任务退出后按身份结清；期间新增的需求仍保留，由调度器重新评估。
    if (m_holder != nullptr) {
        m_scheduler.cancelGeneration(*m_holder, this);
    }
}

std::string ChunkProgressionTask::description() const
{
    return "ChunkProgressionTask(x=" + std::to_string(m_x) + ", z=" + std::to_string(m_z) +
        ", toStatus=" + m_toStatus->name() + ")";
}

// ============================================================================
// executeEmptyLoad：EMPTY 状态（从存档加载或新建空 Primer）
// ============================================================================

bool ChunkProgressionTask::executeEmptyLoad(const std::atomic<bool>& abortSignal)
{
    if (abortSignal.load(std::memory_order::acquire)) {
        return false;
    }

    MC_ASSERT_RELEASE(m_holder != nullptr);
    auto& holder = *m_holder;
    MC_ASSERT_RELEASE(holder.sourceState() == mc::world::chunk::ChunkHolder::SourceState::StorageMissing);

    auto primer = std::make_unique<ChunkPrimer>(m_x, m_z);
    holder.setCurrentChunk(std::move(primer));

    // 推进 EMPTY 状态（primer 构造时已是 EMPTY，这里推进 holder.currentGenStatus）
    m_scheduler.onChunkGenComplete(holder, ChunkStatuses::EMPTY, *this);
    return true;
}

// ============================================================================
// executeStatusStep：普通状态推进
// ============================================================================

bool ChunkProgressionTask::executeStatusStep(const std::atomic<bool>& abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
        "ChunkProgressionTask::executeStatusStep",
        "x",
        m_x,
        "z",
        m_z,
        "targetStatus",
        m_toStatus->name(),
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(m_x, m_z).toId())](::perfetto::EventContext ctx) {
            flow(ctx);
        });

    if (abortSignal.load(std::memory_order::acquire)) {
        return false;
    }

    if (m_holder == nullptr) {
        spdlog::warn("[ChunkProgressionTask] m_holder is null for ({}, {}) at status {}", m_x, m_z, m_toStatus->name());
        return false;
    }
    mc::world::chunk::ChunkHolder& holder = *m_holder;

    ChunkPrimer* primer = holder.getCurrentChunk();
    if (primer == nullptr) {
        spdlog::warn("[ChunkProgressionTask] currentChunk is null for ({}, {}) at status {} (cancelled={})",
            m_x,
            m_z,
            m_toStatus->name(),
            abortSignal.load(std::memory_order::acquire));
        return false;
    }

    // 构建 WorldGenRegion：从 StaticChunkCache2D<shared_ptr<ChunkHolder>> 提取各 holder 的可变 ChunkPrimer。
    // m_neighbours 持有邻居 holder 的 shared_ptr，保证 executeStatusStep 期间邻居 holder 及其 m_currentChunk
    // 不被主线程 requestChunkUnload 销毁（消除裸 ChunkPrimer* 的 use-after-free 竞态，对齐 Moonrise
    // StaticCache2D<GenerationChunkHolder> 强引用语义）。从存活的 holder 取裸 ChunkPrimer* 填充 WorldGenRegion，
    // WorldGenRegion 只在本次调用内存活，期间 holder 因 shared_ptr 保活。
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& step = pyramid.getStepTo(*m_toStatus);
    const i32 radius = m_neighbours.radius();
    const i32 diameter = m_neighbours.diameter();

    std::vector<IChunk*> chunks;
    chunks.reserve(static_cast<size_t>(diameter) * static_cast<size_t>(diameter));
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkProgressionTask::executeStatusStep::buildWorldGenRegion");
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                const std::shared_ptr<mc::world::chunk::ChunkHolder>& neighbourHolder =
                    m_neighbours.get(m_x + dx, m_z + dz);
                // holder 由 shared_ptr 保活（构造缓存时已断言非空）；primer 由 checkNeighbour 保证就绪。
                // 中心 holder 的 primer 上面已取并判空；邻居 holder 的 primer 由 buildNeighbourCache 断言非空。
                ChunkPrimer* neighbourPrimer = neighbourHolder->getCurrentChunk();
                chunks.push_back(neighbourPrimer);
            }
        }
    }

    WorldGenRegion region(m_x, m_z, step, std::move(chunks), m_steps.dimension());
    m_steps.configureRegion(region);

    // 执行生成步骤（调用 IChunkGenerator 的对应方法）
    m_steps.execute(*primer, *m_toStatus, region);

    if (abortSignal.load(std::memory_order::acquire)) {
        return false;
    }

    // 推进 primer 状态（对齐 _doGenerateChunkToTargetStatus 的 setPersistedStatus/setChunkStatus）
    primer->setPersistedStatus(*m_toStatus);
    primer->setChunkStatus(*m_toStatus);

    // FULL 完成时：转换为 ChunkData 存入内存缓存（含实体生成、后处理）
    // FULL 发布回调 内部调用 primer.toChunkData()（非破坏性，返回 shared_ptr 共享同一份
    // ChunkData）+ ChunkRegistry::publish（共享所有权）+ spawnEntitiesFromChunkGeneration + _postProcessChunk。
    // 不释放 primer：FULL 后 currentChunk（primer）仍存活供邻居引用（STRUCTURE_REFERENCES/
    // LIGHT 等状态可能并发读取已 FULL 的邻居），直到 holder 卸载（isSafeToUnload）才随 holder 析构。
    if (*m_toStatus == ChunkStatuses::FULL) {
        m_publishFull(holder, *primer);
        // 不调用 holder.releaseCurrentChunk()：primer 保留 m_currentChunk，邻居 getChunkIfPresentUnchecked
        // 仍可返回有效指针。ChunkData 已通过 shared_ptr 与 m_chunks 共享所有权。
    }

    // 通知调度器完成
    m_scheduler.onChunkGenComplete(holder, *m_toStatus, *this);
    return true;
}

} // namespace mc::server
