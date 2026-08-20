/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and permission notice shall be included in all
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

#include "FindWaterGoal.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/BlockState.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../controller/MovementController.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// 对齐 vanilla net.minecraft.world.entity.ai.goal.TryFindWaterGoal（1.21.11）。
// vanilla TryFindWaterGoal 语义：
//   - canUse: mob.onGround() && !level.getFluidState(mob.blockPosition()).is(WATER)
//     （仅当实体站在地面且脚下非水时尝试找水）
//   - start: 在实体周围 ±2 格（x±2, y-2..y, z±2）搜水源，找到则 moveControl.setWantedPosition
//     （短距离移动，不调 navigator 寻路）
//   - 不覆盖 canContinueToUse（默认 false）：start 后立即 stop，下一 tick 重新评估 canUse
//   - 构造不 setFlags：不占用任何 GoalFlag（不抢占 Move flag，不与 MeleeAttackGoal 等冲突）
//
// 此前 Cubium FindWaterGoal 严重偏离 vanilla：
//   1. 搜索范围 16 格（vanilla 2 格）——GameTest 并行环境下相邻测试 force 加载的区块暴露
//      worldgen 地下含水层水源，海豚 16 格内搜到结构外 worldgen 水；
//   2. 用 navigator.tryMoveTo 长距离寻路（vanilla moveControl.setWantedPosition 短距离）；
//   3. 占用 GoalFlag::Move（vanilla 不占任何 flag）——抢占 Move flag 致 MeleeAttackGoal
//      无法执行，海豚受击后不反击玩家（寻路去 worldgen 水而非攻击玩家），dolphin_retaliates
//      集成测试全量并行下确定性超时（单跑 PASSED，因单跑只 force footprint 区块，16 格内
//      未加载区块被跳过搜不到水；并行下相邻测试加载区块暴露水）。详见 memory:
//      findwater-goal-16range-blocks-meleeattack-parallel。
// 重写对齐 vanilla 修复上述三点偏差。
FindWaterGoal::FindWaterGoal(CreatureEntity* creature)
    : Goal() // 对齐 vanilla TryFindWaterGoal：不 setFlags，不占用任何 GoalFlag
    , m_creature(creature)
{
    MC_ASSERT(creature != nullptr);
}

bool FindWaterGoal::shouldExecute()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 对齐 vanilla TryFindWaterGoal.canUse：
    //   return this.mob.onGround() && !this.mob.level().getFluidState(this.mob.blockPosition()).is(WATER);
    // 仅当实体站在地面（onGround）且脚下非水时才尝试找水。onGround 守卫确保实体不在空中/水中
    // 漂浮时无效触发；脚下非水守卫确保已在水中时不再找水。
    if (!m_creature->onGround()) {
        return false;
    }

    // 脚下方块非水（对齐 vanilla getFluidState(blockPosition()).is(WATER) 取反）
    IWorld* world = m_creature->world();
    if (world == nullptr) {
        return false;
    }
    BlockPos feetPos(static_cast<i32>(std::floor(m_creature->x())),
        static_cast<i32>(std::floor(m_creature->y())),
        static_cast<i32>(std::floor(m_creature->z())));
    if (world->isWaterAt(feetPos.x, feetPos.y, feetPos.z)) {
        return false;
    }

    return true;
}

bool FindWaterGoal::shouldContinueExecuting()
{
    // 对齐 vanilla TryFindWaterGoal：不覆盖 canContinueToUse，默认返回 false。
    // startExecuting 一次性设置 moveControl 目标后立即 stop，下一 tick 重新 shouldExecute 评估。
    // 这避免长时间占用 goal 槽位，也与 vanilla 一次性触发语义一致。
    return false;
}

void FindWaterGoal::startExecuting()
{
    if (m_creature == nullptr) {
        return;
    }

    IWorld* world = m_creature->world();
    if (world == nullptr) {
        return;
    }

    // 对齐 vanilla TryFindWaterGoal.start：在实体周围 ±2 格搜水源。
    //   BlockPos.betweenClosed(floor(x-2), floor(y-2), floor(z-2), floor(x+2), blockY, floor(z+2))
    // 即 x∈[x-2,x+2], y∈[y-2, blockY], z∈[z-2,z+2]（y 上界为脚部 blockY）。
    // 找到首个水源即 setWantedPosition 短距离移动过去（不调 navigator 寻路）。
    const i32 minX = static_cast<i32>(std::floor(m_creature->x())) - 2;
    const i32 maxX = static_cast<i32>(std::floor(m_creature->x())) + 2;
    const i32 minY = static_cast<i32>(std::floor(m_creature->y())) - 2;
    const i32 maxY = static_cast<i32>(std::floor(m_creature->y())); // 脚部 blockY
    const i32 minZ = static_cast<i32>(std::floor(m_creature->z())) - 2;
    const i32 maxZ = static_cast<i32>(std::floor(m_creature->z())) + 2;

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                if (world->isWaterAt(x, y, z)) {
                    // 对齐 vanilla moveControl.setWantedPosition(x, y, z, 1.0)。
                    // 短距离移动（≤2 格），moveController 直接朝目标移动，不经 navigator 寻路，
                    // 不抢占 Move flag，不与 MeleeAttackGoal 等冲突。
                    auto* mob = dynamic_cast<MobEntity*>(m_creature);
                    if (mob != nullptr && mob->moveController() != nullptr) {
                        mob->moveController()->setMoveTo(
                            static_cast<f64>(x) + 0.5, static_cast<f64>(y), static_cast<f64>(z) + 0.5, 1.0);
                    }
                    return; // 找到首个水源即返回（对齐 vanilla break + setWantedPosition）
                }
            }
        }
    }
}

void FindWaterGoal::resetTask()
{
    // 对齐 vanilla TryFindWaterGoal：无 stop 覆盖（默认空）。shouldContinueExecuting 恒 false，
    // start 后立即触发 resetTask，此处无需清理（moveController 目标由下一次 setMoveTo 覆盖）。
}

void FindWaterGoal::tick()
{
    // 对齐 vanilla TryFindWaterGoal：无 tick 覆盖。shouldContinueExecuting 恒 false 故 tick 不会被调用。
}

} // namespace mc::entity::ai::goal
