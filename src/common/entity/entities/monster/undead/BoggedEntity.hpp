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

#include "AbstractSkeletonEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/interfaces/IShearable.hpp"

#include <memory>
#include <vector>

namespace mc {

// 前向声明
namespace entity {
class ArrowEntity;
}

/**
 * @brief 沼骸实体
 *
 * 沼骸是骷髅的变种，生成于沼泽/红树林沼泽（代替 30% 骷髅生成）及试炼密室远程型
 * 试炼刷怪笼，主要特征（对应原版 MC 1.21.11 Bogged）：
 * - 使用弓箭远程攻击，射出的箭矢附带中毒效果（对应原版 getArrow 注入 POISON 100 ticks）
 * - 作为亡灵骷髅变种会在阳光下燃烧（继承基类 shouldBurnInDaylight 默认 true，不 override）
 * - 生命值 16（普通骷髅为 20）
 * - 攻击间隔比普通骷髅慢：困难 50 ticks / 非困难 70 ticks（普通骷髅 20/40）
 * - 可被剪刀剪去头上的蘑菇（实现 IShearable，对齐 Java Bogged.mobInteract 剪菇分支）
 *
 * 参考 MC Java: net.minecraft.world.entity.monster.skeleton.Bogged
 */
class BoggedEntity : public AbstractSkeletonEntity, public entity::IShearable {
public:
    BoggedEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~BoggedEntity() override = default;

    BoggedEntity(const BoggedEntity&) = delete;
    BoggedEntity& operator=(const BoggedEntity&) = delete;
    BoggedEntity(BoggedEntity&&) = delete;
    BoggedEntity& operator=(BoggedEntity&&) = delete;

    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /// 中毒效果持续时间（ticks），100 ticks = 5 秒，对应原版 MobEffects.POISON, 100
    static constexpr i32 POISON_DURATION_TICKS = 100;
    /// 沼骸生命值（普通骷髅为 20），对应原版 createAttributes().add(MAX_HEALTH, 16.0)
    static constexpr f32 BOGGED_MAX_HEALTH = 16.0f;
    /// 被剪刀剪去蘑菇后掉落的蘑菇数量（对齐 wiki：对沼骸使用剪刀掉落 2 个随机颜色蘑菇）
    static constexpr i32 SHEAR_MUSHROOM_COUNT = 2;

    // ========== 蘑菇剪取状态（对齐 Java Bogged.isSheared/setSheared + DATA_SHEARED） ==========

    /**
     * @brief 是否已被剪去蘑菇
     * @return 已剪返回 true（剪过后头部不再有蘑菇，不可再剪）
     *
     * 对应原版 Bogged.isSheared()（entityData.get(DATA_SHEARED)）。
     * 持久化到 NBT "sheared" 字段（对齐 addAdditionalSaveData/readAdditionalSaveData）。
     */
    [[nodiscard]] bool isSheared() const { return m_sheared; }

    /**
     * @brief 设置蘑菇剪取状态
     * @param sheared 是否已剪
     *
     * 对应原版 Bogged.setSheared(boolean)（entityData.set(DATA_SHEARED, ...)）。
     */
    void setSheared(bool sheared) { m_sheared = sheared; }

    // ========== IShearable 接口实现 ==========

    /**
     * @brief 是否可被剪刀剪蘑菇
     * @return 未剪过且存活返回 true
     *
     * 对应原版 Bogged.readyForShearing()：!isSheared() && isAlive()。
     * ShearsItem::itemInteractionForEntity 调此判定（对齐 Java 在 mobInteract 内
     * 检测 itemstack.is(SHEARS) && readyForShearing() 的等价路径）。
     */
    [[nodiscard]] bool isShearable() const override;

    /**
     * @brief 执行剪蘑菇
     * @param player 执行剪菇的玩家（可能为 nullptr）
     * @return 剪下的蘑菇物品列表（2 个随机颜色蘑菇）
     *
     * 对应原版 Bogged.shear()：播放 BOGGED_SHEAR 音效 + spawnShearedMushrooms
     * （从 BOGGED_SHEAR 战利品表掉落 2 个随机红/棕蘑菇）+ setSheared(true)。
     * 战利品表等价为固定 2 个蘑菇，颜色随机（红蘑菇/棕蘑菇各 50% 概率，对齐 wiki
     * "掉落 2 个随机颜色的蘑菇"）。
     */
    std::vector<ItemStack> shear(Player* player = nullptr) override;

protected:
    void registerAttributes() override;

    /**
     * @brief 为射出的箭矢附加中毒效果
     *
     * 重写基类 customizeArrow 钩子，attackEntityWithRangedAttack 发射箭矢前调用。
     * 对应原版 Bogged.getArrow()：arrow.addEffect(MobEffectInstance(POISON, 100))，
     * 箭矢命中生物时由 ArrowEntity::onEntityHit 施加 5 秒中毒 I。
     */
    void customizeArrow(entity::ArrowEntity& arrow) override;

    /**
     * @brief 困难难度最小攻击间隔（50 ticks，比普通骷髅 20 慢）
     *
     * 对应原版 Bogged.getHardAttackInterval() = 50。
     */
    [[nodiscard]] i32 getHardAttackInterval() const override { return INCREASED_HARD_ATTACK_INTERVAL; }

    /**
     * @brief 非困难难度最小攻击间隔（70 ticks，比普通骷髅 40 慢）
     *
     * 对应原版 Bogged.getAttackInterval() = 70。
     */
    [[nodiscard]] i32 getAttackInterval() const override { return INCREASED_NORMAL_ATTACK_INTERVAL; }

private:
    bool m_sheared = false; ///< 蘑菇是否已被剪去（对齐 Java DATA_SHEARED）
};

} // namespace mc
