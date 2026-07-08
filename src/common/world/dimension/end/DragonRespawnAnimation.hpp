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

#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <vector>

namespace mc {

// 前向声明
class IWorld;
class EndDragonFight;
class EndDragonFightResident; // 内部辅助，见 EndDragonFight.cpp

namespace entity {
class EnderCrystalEntity;
} // namespace entity

/**
 * @brief 末影龙重生动画阶段
 *
 * 描述玩家通过末影水晶触发的龙重生序列的 5 个阶段。
 * 每个阶段实现自己的 tick() 逻辑，由 EndDragonFight 每 tick 调用。
 *
 * 阶段时序（对齐 MC 1.21.11 DragonRespawnAnimation）：
 *
 * | 阶段 | 持续时间 | 主要行为 |
 * |------|----------|----------|
 * | START | 1 tick | 将所有重生水晶光束指向 (0, 128, 0)，立即切换到 PREPARING_TO_SUMMON_PILLARS |
 * | PREPARING_TO_SUMMON_PILLARS | 100 tick | 在 tick 0/50/51/52/95+ 播放音效事件 3001；100 tick 后切换到
 * SUMMONING_PILLARS | | SUMMONING_PILLARS | 400 tick (10柱×40) | 每 40 tick 处理一根柱子：第 0 tick 切光束到柱顶，第 39
 * tick 移除柱区方块+爆炸+重新生成柱子+水晶；所有柱子处理完后切换到 SUMMONING_DRAGON | | SUMMONING_DRAGON | 100 tick |
 * tick 0 切光束到 (0, 128, 0)；<5 每 tick 播放 3001；>=80 每 tick 播放 3001；100 tick 时爆炸所有水晶+discard+切换到 END
 * | | END | 0 tick | 空操作，EndDragonFight.setRespawnStage(END) 会创建新龙并清除重生状态 |
 *
 * 与 MC 原版的差异：
 * - MC 使用 enum + abstract tick()，Cubium 使用 enum class + 命名空间内静态 tick 函数
 * - MC 通过 `level.explode(null, x, y, z, 5.0F, BLOCK)` 触发爆炸，
 *   Cubium 通过 `IWorld::createExplosion(pos, 5.0f, ExplosionMode::Break, false, nullptr)`
 * - MC 通过 `level.removeBlock(pos, false)` 移除方块，
 *   Cubium 通过 `IWorld::setBlockState(pos, airState, 3)`
 */
enum class DragonRespawnAnimation : u8 {
    START,                       ///< 起始阶段：设置光束指向 (0, 128, 0)
    PREPARING_TO_SUMMON_PILLARS, ///< 准备召唤柱子：播放音效 100 tick
    SUMMONING_PILLARS,           ///< 召唤柱子：每 40 tick 处理一根柱子
    SUMMONING_DRAGON,            ///< 召唤龙：100 tick 后爆炸水晶并切换到 END
    END                          ///< 结束：空操作，由 EndDragonFight 处理龙生成
};

/**
 * @brief 龙重生阶段 tick 函数命名空间
 *
 * 每个 DragonRespawnAnimation 枚举值对应一个本命名空间内的 tick 函数。
 * EndDragonFight::tick() 在 respawnStage != END 时调用对应的 tick 函数。
 *
 * 所有 tick 函数共享相同的签名：
 * @param world 末地世界
 * @param fight 末影龙战斗管理器
 * @param crystals 重生水晶列表（玩家放置在出口传送门四周的 4 个水晶）
 * @param time 当前阶段已持续的 tick 数
 * @param portalLocation 出口传送门位置（MC 原版参数，Cubium 当前未使用，保留接口）
 */
namespace dragon_respawn {

/**
 * @brief START 阶段 tick
 *
 * 将所有重生水晶的光束指向 (0, 128, 0)，然后立即切换到 PREPARING_TO_SUMMON_PILLARS。
 */
void tickStart(IWorld& world,
    EndDragonFight& fight,
    std::vector<entity::EnderCrystalEntity*>& crystals,
    i32 time,
    const BlockPos& portalLocation);

/**
 * @brief PREPARING_TO_SUMMON_PILLARS 阶段 tick
 *
 * 在 tick 0/50/51/52/95+ 播放事件 3001（末影人低吼音效）。
 * 100 tick 后切换到 SUMMONING_PILLARS。
 */
void tickPreparingToSummonPillars(IWorld& world,
    EndDragonFight& fight,
    std::vector<entity::EnderCrystalEntity*>& crystals,
    i32 time,
    const BlockPos& portalLocation);

/**
 * @brief SUMMONING_PILLARS 阶段 tick
 *
 * 每 40 tick 处理一根柱子：
 * - tick % 40 == 0：将所有水晶光束指向当前柱子顶部
 * - tick % 40 == 39：移除柱区方块、爆炸、重新生成柱子（含末影水晶）
 * 所有柱子处理完后（10 根 × 40 = 400 tick）切换到 SUMMONING_DRAGON。
 */
void tickSummoningPillars(IWorld& world,
    EndDragonFight& fight,
    std::vector<entity::EnderCrystalEntity*>& crystals,
    i32 time,
    const BlockPos& portalLocation);

/**
 * @brief SUMMONING_DRAGON 阶段 tick
 *
 * - tick 0：将所有水晶光束指向 (0, 128, 0)
 * - tick < 5：每 tick 播放事件 3001
 * - tick >= 80：每 tick 播放事件 3001
 * - tick >= 100：切换到 END，重置柱顶水晶（resetSpikeCrystals），
 *               爆炸所有重生水晶并 discard
 */
void tickSummoningDragon(IWorld& world,
    EndDragonFight& fight,
    std::vector<entity::EnderCrystalEntity*>& crystals,
    i32 time,
    const BlockPos& portalLocation);

/**
 * @brief END 阶段 tick（空操作）
 *
 * EndDragonFight.setRespawnStage(END) 会创建新龙并清除重生状态，
 * 此 tick 函数不需要执行任何操作。
 */
void tickEnd(IWorld& world,
    EndDragonFight& fight,
    std::vector<entity::EnderCrystalEntity*>& crystals,
    i32 time,
    const BlockPos& portalLocation);

} // namespace dragon_respawn

} // namespace mc
