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

/**
 * @file SculkShriekerHelper.hpp
 * @brief 幽匿尖啸体服务端逻辑辅助类
 *
 * 实现 SculkShriekerBlock 的服务端专属逻辑，包括：
 * - tryShriek(): 尖啸触发条件检查（玩家解析、附近监守者检查、警告等级递增）
 * - tryRespond(): 尖啸结束后的响应（播放警告声音、应用黑暗效果、尝试召唤监守者）
 * - _canRespond(): 检查 CAN_SUMMON + 非和平 + 游戏规则
 * - _trySummonWarden(): 监守者召唤尝试
 * - _hasNearbyWarden(): 附近监守者检测
 * - _playWardenReplySound(): 警告等级对应声音
 * - _applyDarknessAround(): 黑暗效果应用
 *
 * 这些逻辑依赖 ServerWorld（玩家查找、实体搜索、效果应用等），
 * 因此不能放在 mc_common 层的 SculkShriekerBlock 中。
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {

class Entity;
class IWorld;
class Player;

namespace blocks {
class SculkShriekerBlock;
}

namespace server {

class ServerWorld;

/**
 * @brief 幽匿尖啸体服务端逻辑辅助类
 *
 * 所有方法均为静态方法，提供 SculkShriekerBlock 需要的服务端逻辑。
 * 在 ServerWorld 的 tick 流程中由 SculkVibrationManager 或方块 tick 调用。
 */
class SculkShriekerHelper {
public:
    /**
     * @brief 尝试激活幽匿尖啸体
     *
     * 检查流程：
     * 1. 尖啸体不能处于 SHRIEKING 状态
     * 2. 尝试将触发实体解析为玩家（直接玩家、载具乘客、投射物主人、物品主人）
     * 3. 如果 _canRespond() 为 true，调用 _tryWarn() 递增警告等级
     * 4. 警告等级递增成功或 _canRespond() 为 false 时，执行 shriek() 播放效果
     *
     * @param world 服务端世界
     * @param pos 方块位置
     * @param sourceEntity 触发实体（可为nullptr，如振动触发）
     */
    static void tryShriek(ServerWorld& world, const BlockPos& pos, const Entity* sourceEntity);

    /**
     * @brief 尖啸结束后的响应逻辑
     *
     * 1. 检查 _canRespond()（CAN_SUMMON + 非和平 + 游戏规则）
     * 2. 检查警告等级 > 0
     * 3. 尝试召唤监守者（警告等级 >= 4），失败则播放警告声音
     * 4. 对附近玩家应用黑暗效果
     *
     * @param world 服务端世界
     * @param pos 方块位置
     */
    static void tryRespond(ServerWorld& world, const BlockPos& pos);

    /**
     * @brief 处理方块实体的尖啸结束标志
     *
     * 在 SculkVibrationManager::tickAll() 中调用，检测方块的
     * shriekingFinished 标志并执行响应逻辑。
     *
     * @param world 服务端世界
     * @param pos 方块位置
     */
    static void checkShriekingFinished(ServerWorld& world, const BlockPos& pos);

    /**
     * @brief 将触发实体解析为玩家
     *
     * 按优先级链式判断：
     * 1. 实体本身就是玩家 -> 直接返回
     * 2. 实体的控制乘客是玩家（骑乘载具场景）-> 返回控制乘客
     * 3. 实体是投射物且发射者是玩家 -> 返回发射者
     * 4. 实体是掉落物品且所有者是玩家 -> 返回所有者
     *
     * @param world 世界引用（用于通过 EntityInstanceId/UUID 查找实体）
     * @param entity 触发实体
     */
    static Player* tryGetPlayer(IWorld& world, const Entity* entity);

    // ========== 常量 ==========

    /// 尖啸体警告等级对应的声音
    static constexpr const char* WARDEN_SOUND_BY_LEVEL[5] = {
        nullptr,                                  // level 0: no sound
        "minecraft:entity.warden.nearby_close",   // level 1
        "minecraft:entity.warden.nearby_closer",  // level 2
        "minecraft:entity.warden.nearby_closest", // level 3
        "minecraft:entity.warden.listening_angry" // level 4 (warden about to spawn)
    };

    /// 检查附近监守者的搜索半径（格）
    static constexpr f32 WARDEN_SEARCH_RADIUS = 48.0f;

    /// 查找附近玩家的搜索半径（格）
    static constexpr f32 PLAYER_SEARCH_RADIUS = 16.0f;

    /// 黑暗效果的应用半径（格）
    static constexpr f32 DARKNESS_RADIUS = 40.0f;

    /// 黑暗效果持续时间（tick）
    static constexpr i32 DARKNESS_DURATION = 260;

    /// 黑暗效果应用冷却（tick）
    static constexpr i32 DARKNESS_COOLDOWN = 200;

    /// 监守者生成尝试次数
    static constexpr i32 SUMMON_ATTEMPTS = 20;

    /// 监守者生成水平偏移范围
    static constexpr i32 SUMMON_HORIZONTAL_RANGE = 5;

    /// 监守者生成垂直偏移范围
    static constexpr i32 SUMMON_VERTICAL_RANGE = 6;

private:
    /**
     * @brief 检查尖啸体是否可以响应（召唤监守者的前置条件）
     *
     * 条件：CAN_SUMMON=true、非和平难度、游戏规则允许监守者生成
     */
    static bool _canRespond(ServerWorld& world, const BlockPos& pos);

    /**
     * @brief 尝试召唤监守者
     *
     * 在尖啸体附近 +/-5 水平、+6 垂直范围内尝试找到有效生成位置。
     * 需要监守者实体类型已注册且可生成。
     *
     * @return 是否成功召唤
     */
    static bool _trySummonWarden(ServerWorld& world, const BlockPos& pos);

    /**
     * @brief 播放监守者回应声音
     *
     * 根据警告等级播放不同声音，在尖啸体附近随机偏移位置播放。
     */
    static void _playWardenReplySound(ServerWorld& world, const BlockPos& pos, i32 warningLevel);

    /**
     * @brief 检查附近是否有监守者
     *
     * 在 48x48x48 范围内搜索是否存在监守者实体。
     */
    static bool _hasNearbyWarden(ServerWorld& world, const BlockPos& pos);

    /**
     * @brief 对附近玩家应用黑暗效果
     *
     * 对半径 40 格内的玩家应用黑暗效果（260 tick 持续时间）。
     */
    static void _applyDarknessAround(ServerWorld& world, const BlockPos& pos);

    /**
     * @brief 尝试递增附近玩家的警告等级
     *
     * 查找附近 16 格内的玩家，检查冷却，找到最高警告等级的追踪器递增，
     * 并同步所有附近玩家的警告等级。
     *
     * @return 警告等级是否成功递增
     */
    static bool _tryWarn(ServerWorld& world, const BlockPos& pos);
};

} // namespace server
} // namespace mc
