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

#include "../../../../core/Types.hpp"
#include "AbstractHorseEntity.hpp"
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
 * - 不燃烧：僵尸马不会在阳光下燃烧
 * - 不繁殖：无法繁殖
 * - 稀有：只能通过命令生成
 *
 * 参考 MC 1.16.5 ZombieHorseEntity
 */
class ZombieHorseEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    ZombieHorseEntity(EntityId id);
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
    static std::unique_ptr<Entity> create(IWorld* world);

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
     * @brief 是否应该燃烧（阳光）
     * 僵尸马不会燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const { return false; }

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
