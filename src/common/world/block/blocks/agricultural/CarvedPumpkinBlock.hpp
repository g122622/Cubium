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
#include "common/util/Direction.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/blocks/base/HorizontalBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"

namespace mc {

class IWorld;
class BlockItemUseContext;
class Entity;

namespace entity {
enum class CopperGolemWeatherState : u8;
}

namespace blockpattern {
class BlockPatternMatch;
}

namespace blocks {

/**
 * @brief 刻过的南瓜方块
 *
 * 有 FACING 属性的南瓜，可以用于制作雪傀儡、铁傀儡和铜傀儡。
 * 傀儡生成的核心逻辑在此类中实现，JackOLanternBlock 复用此类的静态方法。
 *
 * 三种傀儡模式：
 * - 雪傀儡：南瓜 + 雪块 + 雪块（垂直 3 格）
 * - 铁傀儡：南瓜 + T 形铁块结构（十字手臂 + 中央身体）
 * - 铜傀儡：南瓜 + 任意铜块（垂直 2 格，铜块替换为铜箱子）
 *
 * 优先级：雪傀儡 > 铁傀儡 > 铜傀儡
 */
class CarvedPumpkinBlock : public HorizontalBlock {
public:
    explicit CarvedPumpkinBlock(const BlockProperties& properties);

    ~CarvedPumpkinBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 方块添加 ==========

    /**
     * @brief 方块添加到世界时调用
     *
     * 检测是否可以生成雪傀儡或铁傀儡。
     * 仅在方块类型实际改变时触发（防止 FACING 属性变化时重复触发）。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    // ========== 傀儡生成静态方法 ==========

    /**
     * @brief 尝试在指定位置生成傀儡
     *
     * 检测雪傀儡、铁傀儡和铜傀儡三种模式。如果匹配，移除方块并生成对应实体。
     * 此方法为静态方法，供 CarvedPumpkinBlock 和 JackOLanternBlock 共用。
     *
     * 优先级：
     * 1. 雪傀儡（垂直 3 格：南瓜 + 雪块 + 雪块）
     * 2. 铁傀儡（T 形铁块结构）
     * 3. 铜傀儡（垂直 2 格：南瓜 + 铜块，铜块替换为铜箱子）
     *
     * @param world 世界引用
     * @param pos 南瓜/南瓜灯位置
     * @return 是否成功生成傀儡
     */
    static bool trySpawnGolem(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方块状态是否为南瓜类型（可作为傀儡头部）
     *
     * 刻过的南瓜和南瓜灯都可以作为傀儡的头部方块。
     *
     * @param state 方块状态
     * @return 是否为南瓜类型
     */
    [[nodiscard]] static bool isPumpkinHead(const BlockState* state);

    /**
     * @brief 检查指定位置是否可以生成傀儡（仅检查身体部分）
     *
     * 公共 API，允许外部查询某位置是否满足任意一种傀儡模式（雪/铁/铜）的身体部分。
     * 头部由调用方负责提供（南瓜或南瓜灯）。
     *
     * 与 trySpawnGolem 的区别：
     * - canSpawnGolem 仅检测身体模式，不消耗方块也不生成实体
     * - trySpawnGolem 检测完整模式（含头部）并实际生成傀儡
     *
     * @param world 世界引用（只读访问）
     * @param pos 南瓜头部位置（身体在该位置下方）
     * @return 是否匹配任意一种傀儡身体模式
     */
    [[nodiscard]] static bool canSpawnGolem(IWorld& world, const BlockPos& pos);

    /**
     * @brief 清除匹配结果覆盖的全部模式方块（width × height，depth=0）
     *
     * 将每个模式格子设为空气并播放方块破坏粒子（levelEvent 2001）。
     * 凋灵召唤也复用此方法。
     *
     * @param world 世界引用
     * @param match 模式匹配结果
     */
    static void clearPatternBlocks(IWorld& world, const blockpattern::BlockPatternMatch& match);

    /**
     * @brief 通知匹配结果覆盖的全部模式方块的邻居
     *
     * 对每个模式格子调用 updateNeighborsAt(pos, AIR)。
     * 凋灵召唤也复用此方法。
     *
     * @param world 世界引用
     * @param match 模式匹配结果
     */
    static void updatePatternBlocks(IWorld& world, const blockpattern::BlockPatternMatch& match);

    /**
     * @brief 对实体周围指定半径内的玩家触发 SUMMONED_ENTITY 进度
     *
     * @param world 世界引用
     * @param entity 被召唤的实体
     * @param radius 触发半径（傀儡 5 格，凋灵 50 格）
     */
    static void triggerSummonedEntity(IWorld& world, Entity& entity, f32 radius);

private:
    /**
     * @brief 从铜块状态推导铜傀儡氧化等级
     *
     * @param copperState 铜块状态
     * @return 对应的铜傀儡氧化等级
     */
    [[nodiscard]] static entity::CopperGolemWeatherState getWeatherStateFromCopperBlock(const BlockState& copperState);
};

} // namespace blocks
} // namespace mc
