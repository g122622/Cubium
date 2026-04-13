#include "TickManager.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../fluid/FluidRegistry.hpp"
#include "../../IWorld.hpp"
#include "common/perfetto/TraceEvents.hpp"

namespace mc::world::tick {

// ============================================================================
// 构造和析构
// ============================================================================

TickManager::TickManager(IWorld& world)
    : m_world(world) {

    // 创建方块tick列表
    // 过滤器：无过滤器（在tick回调中检查空气方块）
    m_blockTicks = std::make_unique<ServerTickList<Block>>(
        world,
        // 过滤器：无过滤
        [](Block&) -> bool { return false; },
        // 序列化：Block -> ResourceLocation
        [](Block& block) -> const ResourceLocation& {
            return block.blockLocation();
        },
        // 反序列化：ResourceLocation -> Block*
        [](const ResourceLocation& id) -> Block* {
            return BlockRegistry::instance().getBlock(id);
        },
        // tick回调：执行方块tick
        [](IWorld& w, const BlockPos& pos, Block& block) {
            // 获取方块状态并执行tick
            const BlockState* state = w.getBlockState(pos.x, pos.y, pos.z);
            if (state != nullptr && !block.isAir(*state)) {
                BlockState* mutableState = const_cast<BlockState*>(state);
                block.tick(w, pos, *mutableState);
            }
        }
    );

    // 创建流体tick列表
    // 过滤器：忽略空流体
    m_fluidTicks = std::make_unique<ServerTickList<fluid::Fluid>>(
        world,
        [](fluid::Fluid& fluid) -> bool {
            return fluid.isEmpty();
        },
        // 序列化：Fluid -> ResourceLocation
        [](fluid::Fluid& fluid) -> const ResourceLocation& {
            return fluid.fluidLocation();
        },
        // 反序列化：ResourceLocation -> Fluid*
        [](const ResourceLocation& id) -> fluid::Fluid* {
            return fluid::FluidRegistry::instance().getFluid(id);
        },
        // tick回调：执行流体tick
        [](IWorld& w, const BlockPos& pos, fluid::Fluid& fluid) {
            // 获取流体状态并执行tick
            const fluid::FluidState* state = w.getFluidState(pos.x, pos.y, pos.z);
            if (state != nullptr && !state->isEmpty()) {
                // 流体tick需要修改状态
                fluid::FluidState mutableState = *state;
                fluid.tick(w, pos, mutableState);
            }
        }
    );
}

TickManager::~TickManager() = default;

// ============================================================================
// 方块tick调度
// ============================================================================

void TickManager::scheduleBlockTick(const BlockPos& pos, Block& block, i32 delay) {
    m_blockTicks->scheduleTick(pos, block, delay);
}

void TickManager::scheduleBlockTick(const BlockPos& pos, Block& block, i32 delay, TickPriority priority) {
    m_blockTicks->scheduleTick(pos, block, delay, priority);
}

bool TickManager::isBlockTickScheduled(const BlockPos& pos, Block& block) const {
    return m_blockTicks->isTickScheduled(pos, block);
}

bool TickManager::isBlockTickPending(const BlockPos& pos, Block& block) const {
    return m_blockTicks->isTickPending(pos, block);
}

bool TickManager::cancelBlockTick(const BlockPos& pos, Block& block) {
    return m_blockTicks->cancelTick(pos, block);
}

// ============================================================================
// 流体tick调度
// ============================================================================

void TickManager::scheduleFluidTick(const BlockPos& pos, fluid::Fluid& fluid, i32 delay) {
    m_fluidTicks->scheduleTick(pos, fluid, delay);
}

void TickManager::scheduleFluidTick(const BlockPos& pos, fluid::Fluid& fluid, i32 delay, TickPriority priority) {
    m_fluidTicks->scheduleTick(pos, fluid, delay, priority);
}

bool TickManager::isFluidTickScheduled(const BlockPos& pos, fluid::Fluid& fluid) const {
    return m_fluidTicks->isTickScheduled(pos, fluid);
}

bool TickManager::isFluidTickPending(const BlockPos& pos, fluid::Fluid& fluid) const {
    return m_fluidTicks->isTickPending(pos, fluid);
}

bool TickManager::cancelFluidTick(const BlockPos& pos, fluid::Fluid& fluid) {
    return m_fluidTicks->cancelTick(pos, fluid);
}

// ============================================================================
// 执行tick
// ============================================================================

void TickManager::tick(u64 currentTick) {
    MC_TRACE_EVENT("server.tick", "TickManager::tick",
        "currentTick", currentTick);

    // 设置当前tick用于调度计算
    m_blockTicks->setCurrentTick(currentTick);
    m_fluidTicks->setCurrentTick(currentTick);

    // 先执行方块计划刻
    {
        MC_TRACE_EVENT("server.tick", "BlockTicks");
        m_blockTicks->tick(currentTick);
    }

    // 再执行流体计划刻
    {
        MC_TRACE_EVENT("server.tick", "FluidTicks");
        m_fluidTicks->tick(currentTick);
    }
}

// ============================================================================
// 区块序列化
// ============================================================================

std::vector<ScheduledTick<Block>> TickManager::getPendingBlockTicks(
    i32 chunkX, i32 chunkZ, bool remove) {

    return m_blockTicks->getPendingTicks(chunkX, chunkZ, remove, true);
}

std::vector<ScheduledTick<fluid::Fluid>> TickManager::getPendingFluidTicks(
    i32 chunkX, i32 chunkZ, bool remove) {

    return m_fluidTicks->getPendingTicks(chunkX, chunkZ, remove, true);
}

// ============================================================================
// 统计
// ============================================================================

size_t TickManager::pendingBlockTickCount() const {
    return m_blockTicks->pendingCount();
}

size_t TickManager::pendingFluidTickCount() const {
    return m_fluidTicks->pendingCount();
}

size_t TickManager::executedBlockTickCount() const {
    return m_blockTicks->executedThisTickCount();
}

size_t TickManager::executedFluidTickCount() const {
    return m_fluidTicks->executedThisTickCount();
}

} // namespace mc::world::tick
