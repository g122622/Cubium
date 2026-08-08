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

#include "AbstractHorseEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 僵尸马实体
 *
 * 稀有的亡灵马，只能通过命令或刷怪蛋生成。
 *
 * 特性：
 * - 可骑乘：可直接骑乘，无需驯服
 * - 不死生物：免疫溺水、中毒
 * - 阳光燃烧：MC 原版中僵尸马在 BURN_IN_DAYLIGHT 标签中，会在阳光下燃烧
 * - 防护槽位为身体（EquipmentSlot::Chest），即马铠槽位提供阳光防护
 * - 不繁殖：无法繁殖
 * - 稀有：只能通过命令生成
 */
class ZombieHorseEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    ZombieHorseEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~ZombieHorseEntity() override = default;

    // 禁止拷贝
    ZombieHorseEntity(const ZombieHorseEntity&) = delete;
    ZombieHorseEntity& operator=(const ZombieHorseEntity&) = delete;

    // 允许移动
    ZombieHorseEntity(ZombieHorseEntity&&) = delete;
    ZombieHorseEntity& operator=(ZombieHorseEntity&&) = delete;

    /**
     * @brief 创建僵尸马实体
     * @param world 世界实例
     * @return 新的僵尸马实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 骑乘系统 ==========

    /**
     * @brief 检查玩家是否可以骑乘
     * 僵尸马不需要驯服即可骑乘
     */
    [[nodiscard]] bool canBeRiddenBy(Player* player) const;

    // ========== 繁殖系统 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 僵尸马不能繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override
    {
        (void)itemStack;
        return false;
    }

    /**
     * @brief 生成幼体
     * 僵尸马不能繁殖
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override
    {
        (void)partner;
        return nullptr;
    }

    // ========== 不死生物特性 ==========

    /**
     * @brief 是否免疫溺水
     */
    [[nodiscard]] bool canBreatheUnderwater() const override { return true; }

    /**
     * @brief 僵尸马不能吃草
     */
    [[nodiscard]] bool canEatGrass() const override { return false; }

    /**
     * @brief 阳光防护装备槽位
     *
     * 僵尸马使用身体槽位（EquipmentSlot::Chest）作为阳光防护，
     * 对应 MC 原版中 ZombieHorse.sunProtectionSlot() 返回 EquipmentSlot.BODY。
     * 这意味着马铠槽位中的物品可以替代僵尸马承受阳光下的耐久损耗。
     */
    [[nodiscard]] EquipmentSlot sunProtectionSlot() const override { return EquipmentSlot::Chest; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.6f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 僵尸马没有特殊状态
};

} // namespace mc
