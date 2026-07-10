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

#include "ChunkProgressionTask.hpp"
#include "ChunkTaskScheduler.hpp"
#include "ServerChunkManager.hpp"
#include "ServerWorld.hpp"
#include "SingleChunkLifecycleManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/gen/ChunkStep.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

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

ChunkProgressionTask::ChunkProgressionTask(ChunkTaskScheduler& scheduler,
    ServerChunkManager& manager,
    std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager> holder,
    ChunkCoord x,
    ChunkCoord z,
    const ChunkStatus& toStatus,
    StaticChunkCache2D<std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager>> neighbours,
    i32 writeRadius)
    : m_scheduler(scheduler)
    , m_manager(manager)
    , m_holder(std::move(holder))
    , m_x(x)
    , m_z(z)
    , m_toStatus(&toStatus)
    , m_neighbours(std::move(neighbours))
    , m_writeRadius(writeRadius)
{}

// ============================================================================
// ITask 接口
// ============================================================================

bool ChunkProgressionTask::execute(const std::atomic<bool>& abortSignal)
{
    if (abortSignal.load(std::memory_order::acquire)) {
        return false;
    }

    // EMPTY 走存档加载/新建空 Primer 路径，不调用 IChunkGenerator
    if (*m_toStatus == ChunkStatuses::EMPTY) {
        return executeEmptyLoad(abortSignal);
    }

    return executeStatusStep(abortSignal);
}

void ChunkProgressionTask::onCancel()
{
    // 任务被取消（abortSignal）：清理依赖图并通知等待者，但不 markFailed。
    // 取消是正常操作（holder 卸载/票据级别下降），不是生成失败——holder 可被重新创建并重新生成。
    // 取消路径不 markFailed，仅清理依赖图。
    //
    // 传递 this 调用任务身份感知的 cancelGeneration(holder, task)：仅当 holder.m_generationTask 仍指向
    // 本任务时才清理。避免旧任务 A 的 onCancel 在新任务 B 已 setGenerationTask(B) 后误清 B 的依赖图
    // （cancelActiveWork 清空 m_generationTask 后，submitRequest 可能已调度 B）。
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

    if (m_holder == nullptr) {
        return false;
    }
    mc::world::chunk::SingleChunkLifecycleManager& holder = *m_holder;

    // 异步存档加载路径（_resolveChunkSourceSync → loadChunkAsyncCallback → _onChunkLoadComplete）
    // 已在主线程 tick() 完成 sourceState 推进：存档命中→Ready(FULL)（不走生成），存档缺失→StorageMissing
    // （_advanceChunkState→_scheduleGeneration 调度本 EMPTY 任务）。因此 executeEmptyLoad 运行时
    // sourceState 应为 StorageMissing。
    //
    // sourceState 守卫：
    // - ResolvingStorage：异步加载尚未完成（不应发生，调度在 _onChunkLoadComplete 之后），返回 false 等待。
    // - LoadedFromStorage/Ready：存档命中已被 _onChunkLoadComplete 处理（存入内存+markLoadedFromStorageReady），
    //   不应进入生成。防御性检查内存缓存，命中则直接完成，避免重复 I/O。
    // - StorageMissing：新建空 Primer，SCLM 接管所有权。
    const auto sourceState = holder.sourceState();
    if (sourceState == mc::world::chunk::SingleChunkLifecycleManager::SourceState::ResolvingStorage) {
        // 异步加载未完成：本任务不应在此状态运行。返回 false，调度器会重试或由 holder 取消。
        return false;
    }

    if (sourceState == mc::world::chunk::SingleChunkLifecycleManager::SourceState::LoadedFromStorage ||
        sourceState == mc::world::chunk::SingleChunkLifecycleManager::SourceState::Ready) {
        // 防御分支：异步路径（_onChunkLoadComplete）已把 ChunkData 存入内存缓存并 markLoadedFromStorageReady(FULL)。
        // 此状态不应进入生成调度（currentGenStatus=FULL >= requestedGenStatus，schedule 不会创建 EMPTY 任务）。
        // 若极端竞态下仍进入，直接完成 FULL（chunk 已在内存），解除调度器等待。
        // ChunkData 不可拷贝（copy ctor deleted），不构造 primer；onChunkGenComplete 仅推进 currentGenStatus（已是
        // FULL）。
        holder.completeStatusTo(ChunkStatuses::FULL);
        holder.markLoadedFromStorageReady();
        m_scheduler.onChunkGenComplete(holder, ChunkStatuses::FULL);
        return true;
    }

    // 存档缺失（StorageMissing）或 Unknown（极端：sourceState 未推进）：新建空 Primer，SCLM 接管所有权
    auto primer = std::make_unique<ChunkPrimer>(m_x, m_z);
    holder.setCurrentChunk(std::move(primer));

    // 推进 EMPTY 状态（primer 构造时已是 EMPTY，这里推进 holder.currentGenStatus）
    m_scheduler.onChunkGenComplete(holder, ChunkStatuses::EMPTY);
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
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(m_x, m_z).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    if (abortSignal.load(std::memory_order::acquire)) {
        return false;
    }

    if (m_holder == nullptr) {
        spdlog::warn("[ChunkProgressionTask] m_holder is null for ({}, {}) at status {}", m_x, m_z, m_toStatus->name());
        return false;
    }
    mc::world::chunk::SingleChunkLifecycleManager& holder = *m_holder;

    ChunkPrimer* primer = holder.getCurrentChunk();
    if (primer == nullptr) {
        spdlog::warn("[ChunkProgressionTask] currentChunk is null for ({}, {}) at status {} (cancelled={})",
            m_x,
            m_z,
            m_toStatus->name(),
            abortSignal.load(std::memory_order::acquire));
        return false;
    }

    // 构建 WorldGenRegion：从 StaticChunkCache2D<shared_ptr<SCLM>> 提取各 holder 的可变 ChunkPrimer。
    // m_neighbours 持有邻居 holder 的 shared_ptr，保证 executeStatusStep 期间邻居 holder 及其 m_currentChunk
    // 不被主线程 unloadChunkSync 销毁（消除裸 ChunkPrimer* 的 use-after-free 竞态，对齐 Moonrise
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
                const std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager>& neighbourHolder =
                    m_neighbours.get(m_x + dx, m_z + dz);
                // holder 由 shared_ptr 保活（构造缓存时已断言非空）；primer 由 checkNeighbour 保证就绪。
                // 中心 holder 的 primer 上面已取并判空；邻居 holder 的 primer 由 buildNeighbourCache 断言非空。
                ChunkPrimer* neighbourPrimer = neighbourHolder->getCurrentChunk();
                chunks.push_back(neighbourPrimer);
            }
        }
    }

    const DimensionId dimId = m_manager.hasWorld() ? m_manager.world()->dimension() : 0;
    WorldGenRegion region(m_x, m_z, step, std::move(chunks), dimId);

    // 设置 WorldGenRegion 的世界信息（种子、tick、难度等）
    if (m_manager.hasWorld()) {
        ServerWorld* world = m_manager.world();
        region.setSeed(world->seed());
        region.setCurrentTick(world->currentTick());
        region.setDayTime(world->dayTime());
        region.setHardcore(world->isHardcore());
        region.setDifficulty(world->difficulty());
    }

    // 执行生成步骤（调用 IChunkGenerator 的对应方法）
    m_manager._executeStepTask(*primer, *m_toStatus, region);

    if (abortSignal.load(std::memory_order::acquire)) {
        return false;
    }

    // 推进 primer 状态（对齐 _doGenerateChunkToTargetStatus 的 setPersistedStatus/setChunkStatus）
    primer->setPersistedStatus(*m_toStatus);
    primer->setChunkStatus(*m_toStatus);

    // FULL 完成时：转换为 ChunkData 存入内存缓存（含实体生成、后处理）
    // _finalizeGeneratedChunkSync 内部调用 primer.toChunkData()（非破坏性，返回 shared_ptr 共享同一份
    // ChunkData）+ _storeChunkInMemorySync（共享所有权）+ spawnEntitiesFromChunkGeneration + _postProcessChunk。
    // 不释放 primer：FULL 后 currentChunk（primer）仍存活供邻居引用（STRUCTURE_REFERENCES/
    // LIGHT 等状态可能并发读取已 FULL 的邻居），直到 holder 卸载（isSafeToUnload）才随 holder 析构。
    if (*m_toStatus == ChunkStatuses::FULL) {
        (void)m_manager._finalizeGeneratedChunkSync(m_x, m_z, *primer);
        // 不调用 holder.releaseCurrentChunk()：primer 保留 m_currentChunk，邻居 getChunkIfPresentUnchecked
        // 仍可返回有效指针。ChunkData 已通过 shared_ptr 与 m_chunks 共享所有权。
    }

    // 通知调度器完成
    m_scheduler.onChunkGenComplete(holder, *m_toStatus);
    return true;
}

} // namespace mc::server
