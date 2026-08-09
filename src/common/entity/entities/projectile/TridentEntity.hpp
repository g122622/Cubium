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

#include "../../../item/core/ItemStack.hpp"
#include "AbstractArrowEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 三叉戟实体
 *
 * 三叉戟是一种特殊的投掷武器，可以被玩家拾取并具有特殊攻击模式。
 */
class TridentEntity : public AbstractArrowEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 构造函数
     */
    explicit TridentEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.5f; }
    [[nodiscard]] f32 height() const override { return 0.5f; }

    void tick() override;

    // ========== 三叉戟属性 ==========

    /**
     * @brief 获取三叉戟物品堆
     */
    [[nodiscard]] ItemStack getItemStack() const;

    /**
     * @brief 获取箭矢对应的物品堆（AbstractArrowEntity 接口实现）
     * @return 三叉戟物品堆副本
     */
    [[nodiscard]] ItemStack getArrowStack() const override;

    /**
     * @brief 设置三叉戟物品堆（同时更新附魔等级）
     */
    void setItemStack(const ItemStack& stack);

    /**
     * @brief 是否在返回中（忠诚附魔）
     */
    [[nodiscard]] bool isReturning() const;

    /**
     * @brief 设置返回状态
     */
    void setReturning(bool returning);

    /**
     * @brief 是否已击中方块（插入方块）
     */
    [[nodiscard]] bool hasHitBlock() const;

    /**
     * @brief 击中方块的坐标
     */
    [[nodiscard]] BlockPos hitBlockPos() const;

    /**
     * @brief 获取忠诚附魔等级
     */
    [[nodiscard]] u8 loyaltyLevel() const;

    /**
     * @brief 设置忠诚附魔等级
     */
    void setLoyaltyLevel(u8 level);

    /**
     * @brief 获取返回计时器
     */
    [[nodiscard]] i32 returningTicks() const;

    /**
     * @brief 设置返回计时器
     */
    void setReturningTicks(i32 ticks);

    /**
     * @brief 获取水中阻力
     */
    [[nodiscard]] f32 getWaterDrag() const override;

    /**
     * @brief 设置生物射出三叉戟的基础伤害
     *
     * 三叉戟不使用弓类附魔（力量/冲击/火焰），因此不重写 applyBowEnchantments。
     * 三叉戟的专属附魔（忠诚/穿刺/引雷/激流）在其他地方处理。
     */
    void setBaseDamageFromMob(f32 power) override;

    /**
     * @brief 玩家拾取三叉戟
     */
    bool onPlayerPickup(Player& player) override;

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

    /**
     * @brief 三叉戟在方块中的tick处理
     */
    void tickInGroundTrident();

    /**
     * @brief 处理返回逻辑
     */
    void _tickReturning();

    /**
     * @brief 检查是否应该返回到射手
     *
     * 旁观者模式玩家和已死亡射手的忠诚三叉戟不应返回，
     * 而是应该在原位掉落物品。
     */
    bool _shouldReturnToThrower();

    // 批次6 子目标2 Step4：m_tridentStack/m_hitBlock/m_returning/m_hitBlockPos/
    // m_loyaltyLevel/m_returningTicks 迁入 ecs::TridentStateComponent。
};

} // namespace entity
} // namespace mc
