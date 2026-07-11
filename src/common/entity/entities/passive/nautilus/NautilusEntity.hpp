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

#include "AbstractNautilusEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

// 前向声明
class AnimalEntity;
class DamageSource;
class ItemStack;
class Player;

/**
 * @brief 鹦鹉螺实体（活体）
 *
 * 对应 MC 1.21.11 net.minecraft.world.entity.animal.nautilus.Nautilus。
 *
 * 与 ZombieNautilusEntity 的区别：
 * - 可繁殖（spawnBaby 返回有效幼体）
 * - 音效有幼体/水下/陆地变体
 * - 移动速度 1.0（僵尸鹦鹉螺为 1.1）
 * - 不燃烧（非亡灵）
 * - 空气供应：水下恢复 300，陆地以 -20 为阈值承受干涸伤害
 *
 * 驯服物品（对应 ItemTags.NAUTILUS_TAMING_ITEMS）：鳕鱼、鲑鱼、河豚、热带鱼、
 * 鳕鱼桶、鲑鱼桶、河豚桶、热带鱼桶（桶装食物使用后返还水桶）。
 * 食物（对应 ItemTags.NAUTILUS_FOOD）：鳕鱼、鲑鱼、河豚、热带鱼。
 */
class NautilusEntity : public AbstractNautilusEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    explicit NautilusEntity(EntityId id);

    ~NautilusEntity() override = default;

    // 禁止拷贝
    NautilusEntity(const NautilusEntity&) = delete;
    NautilusEntity& operator=(const NautilusEntity&) = delete;

    // 禁止移动
    NautilusEntity(NautilusEntity&&) = delete;
    NautilusEntity& operator=(NautilusEntity&&) = delete;

    /**
     * @brief 工厂方法
     * @param world 世界指针（未使用，实体 ID 由 EntityManager 分配）
     * @return 新创建的 NautilusEntity 实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 繁殖系统 ==========

    /**
     * @brief 生成幼体
     *
     * 对应 MC 1.21.11 Nautilus.getBreedOffspring()：
     * 若父体已驯服，幼体继承主人和驯服状态。
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 物品判断 ==========

    /**
     * @brief 检查物品是否为驯服物品
     *
     * 对应 MC 1.21.11 Nautilus.isFood() 未驯服分支：
     * item.is(ItemTags.NAUTILUS_TAMING_ITEMS)
     * 简化实现：直接检查物品 ID
     */
    [[nodiscard]] bool isTamingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否为鹦鹉螺食物
     *
     * 对应 MC 1.21.11 Nautilus.isFood() 已驯服分支：
     * item.is(ItemTags.NAUTILUS_FOOD)
     * 简化实现：直接检查物品 ID
     */
    [[nodiscard]] bool isNautilusFood(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否为繁殖物品
     *
     * 对应 MC 1.21.11 Nautilus.isFood()：根据驯服状态返回驯服物品或食物
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    // ========== 音效 ==========

    /**
     * @brief 环境音效
     *
     * 4 路分支：幼体/成体 × 水下/陆地
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 受伤音效
     *
     * 4 路分支：幼体/成体 × 水下/陆地
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 死亡音效
     *
     * 4 路分支：幼体/成体 × 水下/陆地
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 冲刺音效
     *
     * 2 路分支：水下/陆地
     */
    [[nodiscard]] std::optional<ResourceLocation> getDashSound() const override;

    /**
     * @brief 冲刺就绪音效
     *
     * 2 路分支：水下/陆地
     */
    [[nodiscard]] std::optional<ResourceLocation> getDashReadySound() const override;

    /**
     * @brief 进食音效
     *
     * 2 路分支：幼体/成体
     */
    [[nodiscard]] std::optional<ResourceLocation> getEatSound() const override;

    // ========== 空气供应 ==========

    /**
     * @brief 更新空气供应
     *
     * 对应 MC 1.21.11 Nautilus.handleAirSupply()：
     * - 在水中：空气恢复到 300
     * - 不在水中：空气 -1，到 -20 时清零并承受 2 点干涸伤害
     */
    void updateAirSupply() override;

protected:
    /**
     * @brief 注册 AI 目标
     *
     * 调用父类方法，并在其中添加繁殖目标。
     * NautilusEntity 额外添加 BreedGoal（仅活体鹦鹉螺可繁殖）。
     */
    void registerGoals() override;

    /**
     * @brief 注册属性
     *
     * 鹦鹉螺（活体）属性：
     * - MAX_HEALTH: 15.0（继承自父类）
     * - MOVEMENT_SPEED: 1.0（继承自父类）
     */
    void registerAttributes() override;
};

} // namespace mc
