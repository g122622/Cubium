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

#include "TickManager.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/fluid/FluidRegistry.hpp"

using namespace mc::trace;

namespace mc::world::tick {

// ============================================================================
// 构造和析构
// ============================================================================

TickManager::TickManager(IWorld& world)
    : m_world(world)
{

    // 创建方块tick列表
    // 过滤器：无过滤器（在tick回调中检查空气方块）
    m_blockTicks = std::make_unique<ServerTickList<Block>>(
        world,
        // 过滤器：无过滤
        [](const Block&) -> bool { return false; },
        // 序列化：Block -> ResourceLocation
        [](const Block& block) -> const ResourceLocation& { return block.blockLocation(); },
        // 反序列化：ResourceLocation -> Block*
        [](const ResourceLocation& id) -> Block* { return BlockRegistry::instance().getBlock(id); },
        // tick回调：执行方块tick
        // 必须检查当前位置的方块是否是调度时的目标方块
        [](IWorld& w, const BlockPos& pos, const Block& block) {
            const BlockState* state = w.getBlockState(pos);
            if (state != nullptr && &state->getBlock() == &block) {
                // 当前位置的方块与调度时的目标方块匹配，执行tick
                // Block::tick 需要非const的BlockState&来通过with()修改状态，
                // 但BlockState本身是不可变的（with()返回新状态），此处const_cast是安全的
                BlockState* mutableState = const_cast<BlockState*>(state);
                const_cast<Block&>(block).tick(w, pos, *mutableState, w.getRandom());

                // 派发自定义方块组件回调 - onTick
                auto& blockCompReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
                std::string typeId = block.blockLocation().toString();
                if (blockCompReg.hasTickCallback(typeId)) {
                    mc::mod::bedrock::addon::BlockComponentTickEvent event;
                    event.blockTypeId = typeId;
                    event.blockX = pos.x;
                    event.blockY = pos.y;
                    event.blockZ = pos.z;
                    event.dimensionId = w.dimension();
                    blockCompReg.dispatchTick(typeId, event);
                }
            }
            // 如果方块不匹配，说明方块已改变，跳过此次tick
        });

    // 创建流体tick列表
    // 过滤器：忽略空流体
    m_fluidTicks = std::make_unique<ServerTickList<fluid::Fluid>>(
        world,
        [](const fluid::Fluid& fluid) -> bool { return fluid.isEmpty(); },
        // 序列化：Fluid -> ResourceLocation
        [](const fluid::Fluid& fluid) -> const ResourceLocation& { return fluid.fluidLocation(); },
        // 反序列化：ResourceLocation -> Fluid*
        [](const ResourceLocation& id) -> fluid::Fluid* { return fluid::FluidRegistry::instance().getFluid(id); },
        // tick回调：执行流体tick
        // 必须检查当前位置的流体是否是调度时的目标流体
        [](IWorld& w, const BlockPos& pos, const fluid::Fluid& fluid) {
            const fluid::FluidState* state = w.getFluidState(pos);
            if (state != nullptr && !state->isEmpty()) {
                // 检查当前位置的流体是否是调度时的目标流体
                if (&state->getFluid() == &fluid) {
                    // 流体匹配，执行tick
                    fluid::FluidState mutableState = *state;
                    const_cast<fluid::Fluid&>(fluid).tick(w, pos, mutableState);
                }
                // 如果流体不匹配，说明流体已改变，跳过此次tick
            }
        });
}

TickManager::~TickManager() noexcept = default;

// ============================================================================
// 方块tick调度
// ============================================================================

void TickManager::scheduleBlockTick(const BlockPos& pos, const Block& block, i32 delay)
{
    m_blockTicks->scheduleTick(pos, block, delay);
}

void TickManager::scheduleBlockTick(const BlockPos& pos, const Block& block, i32 delay, TickPriority priority)
{
    m_blockTicks->scheduleTick(pos, block, delay, priority);
}

bool TickManager::isBlockTickScheduled(const BlockPos& pos, const Block& block) const
{
    return m_blockTicks->isTickScheduled(pos, block);
}

bool TickManager::isBlockTickPending(const BlockPos& pos, const Block& block) const
{
    return m_blockTicks->isTickPending(pos, block);
}

bool TickManager::cancelBlockTick(const BlockPos& pos, const Block& block)
{
    return m_blockTicks->cancelTick(pos, block);
}

// ============================================================================
// 流体tick调度
// ============================================================================

void TickManager::scheduleFluidTick(const BlockPos& pos, const fluid::Fluid& fluid, i32 delay)
{
    m_fluidTicks->scheduleTick(pos, fluid, delay);
}

void TickManager::scheduleFluidTick(const BlockPos& pos, const fluid::Fluid& fluid, i32 delay, TickPriority priority)
{
    m_fluidTicks->scheduleTick(pos, fluid, delay, priority);
}

bool TickManager::isFluidTickScheduled(const BlockPos& pos, const fluid::Fluid& fluid) const
{
    return m_fluidTicks->isTickScheduled(pos, fluid);
}

bool TickManager::isFluidTickPending(const BlockPos& pos, const fluid::Fluid& fluid) const
{
    return m_fluidTicks->isTickPending(pos, fluid);
}

bool TickManager::cancelFluidTick(const BlockPos& pos, const fluid::Fluid& fluid)
{
    return m_fluidTicks->cancelTick(pos, fluid);
}

// ============================================================================
// 执行tick
// ============================================================================

void TickManager::tick(u64 currentTick)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "TickManager::tick", "currentTick", currentTick);

    // 设置当前tick用于调度计算
    m_blockTicks->setCurrentTick(currentTick);
    m_fluidTicks->setCurrentTick(currentTick);

    // 先执行方块计划刻
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "BlockTicks");
        m_blockTicks->tick(currentTick);
    }

    // 再执行流体计划刻
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "FluidTicks");
        m_fluidTicks->tick(currentTick);
    }
}

// ============================================================================
// 区块序列化
// ============================================================================

std::vector<ScheduledTick<Block>> TickManager::getPendingBlockTicks(i32 chunkX, i32 chunkZ, bool remove)
{

    return m_blockTicks->getPendingTicks(chunkX, chunkZ, remove, true);
}

std::vector<ScheduledTick<fluid::Fluid>> TickManager::getPendingFluidTicks(i32 chunkX, i32 chunkZ, bool remove)
{

    return m_fluidTicks->getPendingTicks(chunkX, chunkZ, remove, true);
}

// ============================================================================
// 统计
// ============================================================================

size_t TickManager::pendingBlockTickCount() const
{
    return m_blockTicks->pendingCount();
}

size_t TickManager::pendingFluidTickCount() const
{
    return m_fluidTicks->pendingCount();
}

size_t TickManager::executedBlockTickCount() const
{
    return m_blockTicks->executedThisTickCount();
}

size_t TickManager::executedFluidTickCount() const
{
    return m_fluidTicks->executedThisTickCount();
}

} // namespace mc::world::tick
