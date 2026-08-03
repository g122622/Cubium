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
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"

#include <functional>
#include <string>

namespace mc {

// 前向声明
class CreatureEntity;
class BlockTag;
class BlockState;

namespace entity::ai::goal {

/**
 * @brief 避开方块目标
 *
 * 使生物主动远离指定标签的方块。当生物检测到附近有排斥方块时，
 * 会计算远离该方块的位置并导航过去。
 *
 * 对应 MC 1.21.11 的 SetWalkTargetAwayFrom.pos(NEAREST_REPELLENT) 行为。
 * 在原版中，此行为通过 Brain/Sensor 系统实现（如 PiglinSpecificSensor /
 * HoglinSpecificSensor 缓存 NEAREST_REPELLENT 记忆模块），当前项目使用
 * Goal 系统实现等效逻辑。
 *
 * 使用示例：
 * @code
 * // 疣猪兽避开 HOGLIN_REPELLENTS 方块
 * m_goalSelector.addGoal(5,
 *     std::make_unique<AvoidBlockGoal>(
 *         this, BlockTags::HOGLIN_REPELLENTS(), 1.0, 8, 4));
 *
 * // 猪灵避开 PIGLIN_REPELLENTS 方块（含额外验证）
 * m_goalSelector.addGoal(4,
 *     std::make_unique<AvoidBlockGoal>(
 *         this, BlockTags::PIGLIN_REPELLENTS(), 1.0, 8, 4,
 *         [](const BlockState& state) {
 *             // 未点燃的灵魂营火不排斥猪灵
 *             if (state.is(block_registry::NetherBlocks::SOUL_CAMPFIRE)) {
 *                 return blocks::CampfireBlock::isLitCampfire(state);
 *             }
 *             return true;
 *         }));
 * @endcode
 */
class AvoidBlockGoal : public Goal {
public:
    /**
     * @brief 方块验证函数类型
     *
     * 给定一个匹配标签的 BlockState，返回 true 表示该方块确实应该被避开，
     * 返回 false 表示虽然匹配标签但不应被避开（例如未点燃的灵魂营火）。
     */
    using BlockValidator = std::function<bool(const BlockState&)>;

    /**
     * @brief 构造函数
     *
     * @param creature 生物实体
     * @param tag 要避开的方块标签引用
     * @param speed 逃跑速度倍率
     * @param horizontalRange 水平搜索范围（方块数，对应 MC REPELLENT_DETECTION_RANGE_HORIZONTAL）
     * @param verticalRange 垂直搜索范围（方块数，对应 MC REPELLENT_DETECTION_RANGE_VERTICAL）
     */
    AvoidBlockGoal(CreatureEntity* creature, const BlockTag& tag, f64 speed, i32 horizontalRange, i32 verticalRange);

    /**
     * @brief 构造函数（带额外验证函数）
     *
     * @param creature 生物实体
     * @param tag 要避开的方块标签引用
     * @param speed 逃跑速度倍率
     * @param horizontalRange 水平搜索范围
     * @param verticalRange 垂直搜索范围
     * @param validator 方块验证函数，用于排除虽然匹配标签但不应被避开的情况
     */
    AvoidBlockGoal(CreatureEntity* creature,
        const BlockTag& tag,
        f64 speed,
        i32 horizontalRange,
        i32 verticalRange,
        BlockValidator validator);

    ~AvoidBlockGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "AvoidBlockGoal"; }

protected:
    /**
     * @brief 扫描附近区域寻找排斥方块
     *
     * 以生物当前位置为中心，搜索指定范围内的方块是否匹配标签。
     * 找到第一个匹配的排斥方块后，将其位置记录到 m_nearestRepellentPos。
     *
     * @return 是否找到排斥方块
     */
    [[nodiscard]] bool _findNearestRepellent();

    /**
     * @brief 寻找远离排斥方块的位置
     *
     * 使用 RandomPositionGenerator::findRandomTargetBlockAwayFrom 计算远离
     * m_nearestRepellentPos 的位置，并通过验证检查确保位置有效。
     *
     * @return 是否找到有效的逃跑位置
     */
    [[nodiscard]] bool _findEscapePosition();

    /**
     * @brief 验证逃跑位置是否有效
     *
     * 检查逃跑位置比当前位置更远离排斥方块。
     *
     * @param escapePos 逃跑位置
     * @return 如果逃跑位置有效返回 true
     */
    [[nodiscard]] bool _isEscapePositionValid(const Vector3& escapePos) const;

    CreatureEntity* m_creature;

    /** 要避开的方块标签（引用，必须比此 Goal 对象存活更久） */
    const BlockTag& m_tag;

    /** 逃跑速度倍率 */
    f64 m_speed;

    /** 水平搜索范围 */
    i32 m_horizontalRange;

    /** 垂直搜索范围 */
    i32 m_verticalRange;

    /** 可选的方块验证函数 */
    BlockValidator m_validator;

    /** 最近发现的排斥方块位置 */
    BlockPos m_nearestRepellentPos{0, 0, 0};

    /** 逃跑目标坐标 */
    f64 m_escapeX = 0.0;
    f64 m_escapeY = 0.0;
    f64 m_escapeZ = 0.0;

    /** 逃跑位置搜索范围（对应 MC SetWalkTargetAwayFrom 的水平搜索范围） */
    static constexpr i32 ESCAPE_HORIZONTAL_RANGE = 16;
    /** 逃跑位置搜索范围（对应 MC SetWalkTargetAwayFrom 的垂直搜索范围） */
    static constexpr i32 ESCAPE_VERTICAL_RANGE = 7;
};

} // namespace entity::ai::goal
} // namespace mc
