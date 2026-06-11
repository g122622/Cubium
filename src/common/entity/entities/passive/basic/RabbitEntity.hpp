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
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include <memory>
#include <random>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 兔子实体
 *
 * 小型被动动物，有多种变种。
 *
 * 特性：
 * - 8种皮肤：棕色、白色、黑白斑点、黑色、金色、椒盐色、杀手兔、吐司兔
 * - 快速移动和跳跃
 * - 繁殖：胡萝卜、金胡萝卜、蒲公英
 * - 幼体：小兔子
 * - 杀手兔：敌对变种（彩蛋）
 * - 吐司兔：特殊命名彩蛋
 */
class RabbitEntity : public AnimalEntity {
public:
    /**
     * @brief 兔子皮肤类型
     */
    enum class RabbitType : u8 {
        Brown = 0,         // 棕色兔子
        White = 1,         // 白色兔子
        Black = 2,         // 黑色兔子
        WhiteSpotted = 3,  // 黑白斑点兔子
        Gold = 4,          // 金色兔子
        SaltAndPepper = 5, // 椒盐色兔子
        Toast = 6,         // 吐司兔（命名彩蛋）
        Killer = 99        // 杀手兔（敌对）
    };

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    RabbitEntity(EntityId id);
    ~RabbitEntity() override = default;

    // 禁止拷贝
    RabbitEntity(const RabbitEntity&) = delete;
    RabbitEntity& operator=(const RabbitEntity&) = delete;

    // 允许移动
    RabbitEntity(RabbitEntity&&) = delete;
    RabbitEntity& operator=(RabbitEntity&&) = delete;

    /**
     * @brief 创建兔子实体
     * @param world 世界实例
     * @return 新的兔子实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 皮肤类型 ==========

    /**
     * @brief 获取皮肤类型
     */
    [[nodiscard]] RabbitType getRabbitType() const { return m_rabbitType; }

    /**
     * @brief 设置皮肤类型
     */
    void setRabbitType(RabbitType type) { m_rabbitType = type; }

    /**
     * @brief 随机设置皮肤类型（基于群系）
     */
    void setRandomRabbitType();

    /**
     * @brief 根据群系获取默认的兔子类型
     *
     * 参考 MC 1.21.11 Rabbit.getRandomRabbitVariant：
     * - 雪地群系：80% 白色，20% 白色斑点
     * - 沙漠群系：100% 金色
     * - 其他群系：50% 棕色，40% 椒盐色，10% 黑色
     *
     * @return 基于当前位置群系的兔子类型
     */
    [[nodiscard]] RabbitType getDefaultRabbitTypeForBiome() const;

    /**
     * @brief 是否是杀手兔
     */
    [[nodiscard]] bool isKillerRabbit() const { return m_rabbitType == RabbitType::Killer; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 设置跳跃状态
     */
    void setJumping(bool jumping) override;

    /**
     * @brief 获取声音类别
     */
    [[nodiscard]] sound::SoundCategory getSoundCategory() const override;

    /**
     * @brief 播放攻击声音
     */
    void playAttackSound(LivingEntity& target) override;

    // ========== 移动 ==========

    /**
     * @brief 获取跳跃力量
     * 兔子有更强的跳跃能力
     */
    [[nodiscard]] f32 getJumpPower() const { return m_jumpPower; }

    /**
     * @brief 设置跳跃力量
     */
    void setJumpPower(f32 power) { m_jumpPower = power; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.2f : 0.35f; }

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.4f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.5f; }

private:
    // 皮肤类型
    RabbitType m_rabbitType = RabbitType::Brown;

    // 跳跃力量
    f32 m_jumpPower = 0.5f;

    // 跳跃计时器
    i32 m_jumpTimer = 0;

    // 跳跃方向
    f32 m_jumpDirectionX = 0.0f;
    f32 m_jumpDirectionZ = 0.0f;
};

} // namespace mc
