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

namespace mc {

class IWorld;
class BlockPos;

namespace entity::combat {

/**
 * @brief 区域难度实例
 *
 * 对应 Minecraft 原版的 DifficultyInstance 类。
 * 封装了位置感知的难度计算，考虑了基础难度、世界运行时间、
 * 区块居住时间和月相等因素。
 *
 * 核心概念：
 * - effectiveDifficulty（有效难度）：综合所有因子计算出的难度值
 * - specialMultiplier（特殊乘数）：0.0~1.0 之间的值，
 *   当 effectiveDifficulty < 2.0 时为 0，> 4.0 时为 1.0，中间线性插值。
 *   用于调节怪物装备生成概率和附魔概率。
 */
class DifficultyInstance {
public:
    /**
     * @brief 构造区域难度实例
     *
     * @param baseDifficulty 基础难度等级
     * @param worldTime 世界运行时间（ticks）
     * @param chunkInhabitedTime 区块被玩家居住的时间（ticks）
     * @param moonPhaseFactor 月相因子（0.0~1.0）
     */
    DifficultyInstance(Difficulty baseDifficulty, i64 worldTime, i64 chunkInhabitedTime, f32 moonPhaseFactor);

    /**
     * @brief 在指定位置创建区域难度实例
     *
     * 对应 MC 原版 ServerLevel.getCurrentDifficultyAt(BlockPos)。
     * 从世界和时间信息中自动提取所有参数，包括世界运行时间、
     * 区块居住时间和月相因子。
     *
     * @param world 世界引用
     * @param pos 方块位置（用于获取区块居住时间）
     * @return 区域难度实例
     */
    [[nodiscard]] static DifficultyInstance at(const IWorld& world, const BlockPos& pos);

    /**
     * @brief 简化构造，仅基于基础难度
     *
     * 不考虑世界时间、区块居住时间和月相，
     * 有效难度直接使用基础难度的固定倍率。
     * 适用于不需要精确位置感知的场景（如测试、命令生成等）。
     *
     * @param baseDifficulty 基础难度等级
     */
    explicit DifficultyInstance(Difficulty baseDifficulty);

    /**
     * @brief 获取基础难度等级
     */
    [[nodiscard]] Difficulty getDifficulty() const { return m_baseDifficulty; }

    /**
     * @brief 获取有效难度
     *
     * 有效难度综合了基础难度、世界运行时间、区块居住时间和月相因子。
     * 取值范围取决于难度等级：
     * - Peaceful: 0.0
     * - Easy: 0.0 ~ 1.5 (典型)
     * - Normal: 0.0 ~ 4.0 (典型)
     * - Hard: 0.0 ~ 6.75 (典型)
     */
    [[nodiscard]] f32 getEffectiveDifficulty() const { return m_effectiveDifficulty; }

    /**
     * @brief 获取特殊乘数
     *
     * 特殊乘数用于调节怪物装备生成概率和附魔概率：
     * - effectiveDifficulty < 2.0: 返回 0.0（不生成装备）
     * - effectiveDifficulty > 4.0: 返回 1.0（最大概率）
     * - 中间: 线性插值 0.0 ~ 1.0
     *
     * Minecraft 原版中的用途：
     * - 护甲生成概率: 0.15 * specialMultiplier
     * - 武器附魔概率: 0.25 * specialMultiplier
     * - 护甲附魔概率: 0.5 * specialMultiplier
     * - 拾取物品概率: 0.55 * specialMultiplier
     * - 破门能力概率: specialMultiplier * 0.1
     */
    [[nodiscard]] f32 getSpecialMultiplier() const;

    /**
     * @brief 检查是否为困难难度
     *
     * 当有效难度 >= 3.0 时返回 true
     */
    [[nodiscard]] bool isHard() const;

    /**
     * @brief 检查有效难度是否高于指定值
     *
     * @param threshold 阈值
     * @return 如果有效难度 > threshold 返回 true
     */
    [[nodiscard]] bool isHarderThan(f32 threshold) const;

private:
    /**
     * @brief 计算有效难度
     *
     * 对应 Minecraft 原版 DifficultyInstance.calculateDifficulty()
     *
     * @param baseDifficulty 基础难度等级
     * @param worldTime 世界运行时间（ticks）
     * @param chunkInhabitedTime 区块居住时间（ticks）
     * @param moonPhaseFactor 月相因子
     * @return 有效难度值
     */
    [[nodiscard]] static f32 calculateDifficulty(
        Difficulty baseDifficulty, i64 worldTime, i64 chunkInhabitedTime, f32 moonPhaseFactor);

    Difficulty m_baseDifficulty;
    f32 m_effectiveDifficulty;

    // 常量 - 对应 Minecraft 原版 DifficultyInstance
    static constexpr f32 DIFFICULTY_TIME_GLOBAL_OFFSET = -72000.0f;
    static constexpr f32 MAX_DIFFICULTY_TIME_GLOBAL = 1440000.0f;
    static constexpr f32 MAX_DIFFICULTY_TIME_LOCAL = 3600000.0f;
};

} // namespace entity::combat
} // namespace mc
