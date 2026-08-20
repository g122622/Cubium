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
#include "../../../../resource/ResourceLocation.hpp"
#include "AnimalEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/interfaces/BoostHelper.hpp"
#include "common/entity/interfaces/IEquipable.hpp"
#include "common/entity/interfaces/IRideable.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class Player;
class DamageSource;

/**
 * @brief 猪实体
 *
 * 最基础的被动动物，可被骑乘（使用鞍）。
 * 实现 IRideable 接口以支持骑乘功能。
 * 实现 IEquipable 接口以支持鞍装备。
 */
class PigEntity : public AnimalEntity, public entity::IRideable, public entity::IEquipable {
public:
    using LivingEntity::getEquipment;
    using LivingEntity::setEquipment;

    PigEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~PigEntity() override = default;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 玩家交互 ==========

    /**
     * @brief 玩家右键交互（对齐 Java 1.21.11 Pig.mobInteract）
     *
     * 玩家右键配鞍的猪时骑上它：
     *   条件：手持非繁殖食物 + 已配鞍 + 无乘客 + 玩家未蹲下 → 服务端 player.startRiding(*this) + Success。
     * 否则委托父类 AnimalEntity::interactMob（喂食/繁殖）；父类返 Pass 时，若手持鞍则返 Pass，
     * 由 Player::interactOn 第4步调 SaddleItem::itemInteractionForEntity 装鞍。
     *
     * 此前 Cubium PigEntity 无 interactMob override（落入 AnimalEntity::interactMob 仅处理喂食），
     * 空手右键配鞍猪返 Pass 不骑乘（对齐缺陷）。本次补全实体侧骑乘入口（与 StriderEntity 同构）。
     *
     * 参考: net.minecraft.world.entity.animal.pig.Pig#mobInteract(Player, InteractionHand)
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    // ========== IRideable 接口实现 ==========

    // 鞍状态存储（对齐 Java 1.21.11 Mob.isSaddled() 语义：鞍状态独立于同步数据，
    // 由 SaddleItem::itemInteractionForEntity 调 setSaddle(true) 置位）。
    // 注意：Cubium BoostHelper 走 EntityDataManager 同步参数，但 PigEntity 构造期未 init
    // BoostHelper（m_initialized=false），故 BoostHelper.getSaddled() 恒返 false，
    // hasSaddle() 读它会致装鞍后仍判无鞍、骑乘分支永不命中。改用独立 bool 成员（与
    // StriderEntity::m_saddled 同构）保证装鞍状态正确读写。
    [[nodiscard]] bool hasSaddle() const override { return m_saddled; }

    void setSaddle(bool saddle) override { m_saddled = saddle; }

    void onPlayerStartRiding(Player* player) override;

    void onPlayerStopRiding(Player* player) override;

    /**
     * @brief 获取骑乘速度
     * @return 基础速度 * 0.225F
     */
    [[nodiscard]] f32 getSteeringSpeed() const override;

    bool boost() override;

    [[nodiscard]] i32 getBoostTime() const override { return m_boostHelper.getBoostTime(); }

    void setBoostTime(i32 time) override { m_boostHelper.setBoostTime(time); }

    /**
     * @brief 检查是否可以被控制方向
     * 猪需要玩家手持胡萝卜钓竿才能被控制
     */
    [[nodiscard]] bool canBeSteered() const override;

    /**
     * @brief 执行骑乘移动逻辑
     */
    void travelTowards(const Vector3& travelVec) override;

    /**
     * @brief 处理移动
     * 重写以使用 IRideable::ride()
     */
    void travel(const Vector3& travelVec) override;

    // ========== IEquipable 接口实现 ==========

    /**
     * @brief 获取装备槽数量
     * 猪只有一个鞍槽
     */
    [[nodiscard]] i32 getEquipmentSlotCount() const override { return 1; }

    /**
     * @brief 获取指定槽位的装备
     * @param slot 槽位索引 (0 = 鞍槽)
     */
    [[nodiscard]] ItemStack getEquipment(i32 slot) const override;

    /**
     * @brief 设置指定槽位的装备
     * @param slot 槽位索引 (0 = 鞍槽)
     */
    void setEquipment(i32 slot, const ItemStack& item) override;

    /**
     * @brief 检查是否可以装备指定物品
     * 猪只能装备鞍
     */
    [[nodiscard]] bool canEquip(const ItemStack& item, i32 slot) const override;

    // ========== 雷击 ==========

    /**
     * @brief 被闪电击中时的回调
     *
     * 猪被闪电击中会转化为僵尸猪灵（和平难度下不转化）。
     */
    void onStruckByLightning() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    void tick() override;

    /**
     * @brief 死亡时掉落鞍
     */
    void die(DamageSource& cause) override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.9f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.9f; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.4f * height(); }

private:
    BoostHelper m_boostHelper; ///< 加速辅助器
    bool m_saddled = false;    ///< 鞍状态（独立存储，对齐 Java Mob.isSaddled 装备槽语义）

    // 常量
    static constexpr f32 PIG_SPEED = 0.25f;           // 基础移动速度
    static constexpr f32 MOUNTED_SPEED_MULT = 0.225f; // 骑乘速度乘数
};

} // namespace mc
