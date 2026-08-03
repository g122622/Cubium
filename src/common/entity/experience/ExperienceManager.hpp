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
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include <functional>
#include <utility>

namespace mc {

// 前向声明
class Player;

namespace entity {
namespace experience {

/**
 * @brief 经验管理器
 *
 * 管理玩家的经验值、等级和升级逻辑。
 *
 * 经验公式：
 * - 等级 0-14: 每级需要 7 + level * 2 点经验
 * - 等级 15-29: 每级需要 37 + (level - 15) * 5 点经验
 * - 等级 30+: 每级需要 112 + (level - 30) * 9 点经验
 */
class ExperienceManager {
public:
    /**
     * @brief 构造函数
     * @param player 关联的玩家
     */
    explicit ExperienceManager(Player& player);

    // ========== 经验操作 ==========

    /**
     * @brief 添加经验值
     *
     * 添加指定数量的经验点，自动处理升级。
     *
     * @param amount 经验值数量
     */
    void addExperience(i32 amount);

    /**
     * @brief 消耗经验值
     *
     * 消耗指定数量的经验点，用于附魔等操作。
     * 如果经验不足，不会消耗任何经验。
     *
     * @param amount 要消耗的经验值
     * @return 是否成功消耗
     */
    [[nodiscard]] bool consumeExperience(i32 amount);

    /**
     * @brief 消耗经验等级
     *
     * 消耗指定数量的等级，用于附魔。
     *
     * @param levels 要消耗的等级数
     * @return 是否成功消耗
     */
    [[nodiscard]] bool consumeLevels(i32 levels);

    /**
     * @brief 设置经验状态
     *
     * 同时设置等级、进度和总经验。
     *
     * @param level 等级
     * @param progress 当前等级进度 (0.0 - 1.0)
     * @param totalExperience 总经验值
     */
    void setExperience(i32 level, f32 progress, i32 totalExperience);

    /**
     * @brief 设置等级
     *
     * 直接设置等级，不改变进度和总经验。
     *
     * @param level 目标等级
     */
    void setLevel(i32 level);

    /**
     * @brief 添加等级
     *
     * 添加指定数量的等级，进度保持不变。
     * 可以传负数来降低等级。
     *
     * @param levels 要添加的等级数
     */
    void addLevels(i32 levels);

    /**
     * @brief 重置经验
     *
     * 将所有经验清零。
     */
    void reset();

    // ========== 查询 ==========

    /**
     * @brief 获取当前等级
     */
    [[nodiscard]] i32 getLevel() const noexcept { return m_level; }

    /**
     * @brief 获取当前等级进度
     *
     * @return 0.0 - 1.0 之间的进度值
     */
    [[nodiscard]] f32 getProgress() const noexcept { return m_progress; }

    /**
     * @brief 获取累计总经验值
     */
    [[nodiscard]] i32 getTotalExperience() const noexcept { return m_totalExperience; }

    /**
     * @brief 获取当前等级填满进度条所需的经验值
     *
     * @return 当前等级的进度条容量
     */
    [[nodiscard]] i32 getExperienceForNextLevel() const noexcept;

    /**
     * @brief 计算达到指定等级所需的总经验值
     *
     * @param level 目标等级
     * @return 所需的总经验值
     */
    [[nodiscard]] static i32 getExperienceForLevel(i32 level) noexcept;

    /**
     * @brief 根据总经验值计算等级
     *
     * @param totalExperience 总经验值
     * @return 对应的等级
     */
    [[nodiscard]] static i32 getLevelFromExperience(i32 totalExperience) noexcept;

    /**
     * @brief 计算指定等级的进度条容量
     *
     * 这是从当前等级升级到下一等级所需的经验值。
     *
     * 等级 0-14: 7 + level * 2 (范围: 7-35)
     * 等级 15-29: 37 + (level - 15) * 5 (范围: 37-107)
     * 等级 30+: 112 + (level - 30) * 9 (范围: 112-382)
     *
     * @param level 等级
     * @return 进度条容量
     */
    [[nodiscard]] static i32 calculateBarCapacity(i32 level) noexcept;

    // ========== 附魔相关 ==========

    /**
     * @brief 获取附魔随机种子
     *
     * 每次附魔后更新，用于随机化附魔选项。
     *
     * @return 当前附魔种子
     */
    [[nodiscard]] i32 getXpSeed() const noexcept { return m_xpSeed; }

    /**
     * @brief 设置附魔随机种子
     *
     * 用于从存档数据恢复种子值。如果种子为 0，则下次附魔时会自动生成随机种子。
     *
     * @param seed 种子值
     */
    void setXpSeed(i32 seed) noexcept { m_xpSeed = seed; }

    /**
     * @brief 重置附魔随机种子
     *
     * 在附魔后调用，生成新的随机种子。
     *
     * @param rng 随机数生成器
     */
    void resetXpSeed(math::Random& rng);

    /**
     * @brief 附魔后处理
     *
     * 消耗指定等级并重置种子。
     *
     * @param levels 要消耗的等级
     * @param rng 随机数生成器
     * @return 是否成功
     */
    [[nodiscard]] bool onEnchant(i32 levels, math::Random& rng);

    // ========== 死亡掉落 ==========

    /**
     * @brief 计算死亡时掉落的经验值
     *
     * 计算公式: min(level * 7, 100)
     *
     * @return 掉落的经验值
     */
    [[nodiscard]] i32 calculateDeathDropXp() const noexcept;

    // ========== 同步 ==========

    /**
     * @brief 标记为需要同步
     */
    void markDirty() noexcept { m_dirty = true; }

    /**
     * @brief 检查是否需要同步
     */
    [[nodiscard]] bool isDirty() const noexcept { return m_dirty; }

    /**
     * @brief 清除同步标记
     */
    void clearDirty() noexcept { m_dirty = false; }

    // ========== 回调 ==========

    /**
     * @brief 等级变化回调类型
     */
    using LevelChangeCallback = std::function<void(i32 oldLevel, i32 newLevel)>;

    /**
     * @brief 设置等级变化回调
     *
     * 当等级变化时触发，可用于播放升级音效等。
     *
     * @param callback 回调函数
     */
    void setLevelChangeCallback(LevelChangeCallback callback)
    {
        MC_ASSERT_RELEASE(!m_levelChangeCallback);
        m_levelChangeCallback = std::move(callback);
    }

    /**
     * @brief 经验变化回调类型
     */
    using ExperienceChangeCallback = std::function<void(i32 totalXp)>;

    /**
     * @brief 设置经验变化回调
     *
     * 当总经验变化时触发，可用于同步到客户端。
     *
     * @param callback 回调函数
     */
    void setExperienceChangeCallback(ExperienceChangeCallback callback)
    {
        MC_ASSERT_RELEASE(!m_experienceChangeCallback);
        m_experienceChangeCallback = std::move(callback);
    }

private:
    /**
     * @brief 更新进度条和总经验
     *
     * 内部方法，在经验变化后调用以确保状态一致。
     */
    void _updateProgress();

    /**
     * @brief 处理升级
     *
     * 当进度超过1.0时升级。
     */
    void _handleLevelUp();

    /**
     * @brief 处理降级
     *
     * 当进度低于0时降级。
     */
    void _handleLevelDown();

    /**
     * @brief 验证并修复状态
     *
     * 确保等级、进度和总经验的一致性。
     */
    void _validateState();

    Player& m_player;

    i32 m_level = 0;           // 当前等级
    f32 m_progress = 0.0f;     // 当前等级进度 (0.0 - 1.0)
    i32 m_totalExperience = 0; // 累计总经验值
    i32 m_xpSeed = 0;          // 附魔随机种子

    bool m_dirty = false; // 是否需要同步到客户端

    LevelChangeCallback m_levelChangeCallback;
    ExperienceChangeCallback m_experienceChangeCallback;

    // 上次播放升级音效的 tick，用于确保两次音效间隔至少 100 tick
    u32 m_lastXpSoundTick = 0;
};

} // namespace experience
} // namespace entity
} // namespace mc
