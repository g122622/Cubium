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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/chunk/data/IChunk.hpp"

#include <memory>
#include <optional>
#include <string>

namespace mc {

// 前向声明
class IWorld;
class ItemStack;
class DamageSource;

namespace entity::ai {
class Goal;
} // namespace entity::ai

/**
 * @brief 快乐恶魂实体
 *
 * 1.21 新增的被动飞行生物，由干燥恶魂方块在水中孵化（幼年个体）或通过幼体成长获得。
 * 成体会自动远离地面悬浮巡游，可载多名乘客。
 *
 * 参考：net.minecraft.world.entity.animal.happyghast.HappyGhast
 *
 * @note 本项目无 Brain 系统，vanilla HappyGhastAi 的 IDLE 活动改以 GoalSelector 的
 *       GhastHappyFloatGoal（幼体漂浮）+ GhastRandomFlyGoal（成年随机飞行）实现，
 *       与恶魂共用同一套飞行移动控制器与飞行物理（travelFlying）。
 * @note 已实现：挽具（HARNESS）装备槽语义（右键装备、剪刀剪下、必须装备挽具才能骑乘）、
 *       多人载客上限 4、挽具驱动的载客/离座音效、静止超时（serverStillTimeout，
 *       玩家不再停在其上方时禁止载客驾驶）与降水回血。
 * @note 已实现：成年/幼年切换时的移动控制与目标集合切换（setChild → ageBoundaryReached，
 *       幼体用 FlyingMoveControl(180, true) 且无目标、成体恢复恶魂式飞行控制器与目标集合）。
 * @note 未实现（TODO）：
 *       - 挽具颜色同步给客户端渲染（客户端已停止维护，服务端只保存挽具物品本身）。
 */
class HappyGhastEntity : public AnimalEntity {
public:
    /**
     * @brief 幼体缩放（对齐 vanilla HappyGhast.BABY_SCALE）
     *
     * vanilla 幼体尺寸为成体的 0.2375 倍（远小于常规动物的 0.5 倍）。
     */
    static constexpr f32 HAPPY_GHAST_BABY_SCALE = 0.2375f;

    /**
     * @brief 构造快乐恶魂
     * @param id 实体 ID
     * @param registry ECS 注册表
     */
    HappyGhastEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~HappyGhastEntity() override = default;

    // 禁止拷贝
    HappyGhastEntity(const HappyGhastEntity&) = delete;
    HappyGhastEntity& operator=(const HappyGhastEntity&) = delete;

    // 允许移动
    HappyGhastEntity(HappyGhastEntity&&) = delete;
    HappyGhastEntity& operator=(HappyGhastEntity&&) = delete;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    // ========== 年龄 / 尺寸 ==========

    /// 幼体缩放比例（对齐 vanilla HappyGhast.BABY_SCALE = 0.2375）
    [[nodiscard]] f32 babyScale() const override
    {
        return HAPPY_GHAST_BABY_SCALE;
    }

    /// 幼体基础宽度（缩放由 babyScale 承担，故此处返回成体宽度）
    [[nodiscard]] f32 getBaseWidth() const override;
    /// 幼体基础高度（缩放由 babyScale 承担，故此处返回成体高度）
    [[nodiscard]] f32 getBaseHeight() const override;

    // ========== 繁殖 ==========

    /// 繁殖/食物判定：HAPPY_GHAST_FOOD 标签（雪球），对齐 vanilla HappyGhast.isFood
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    /// 快乐恶魂不能繁殖（对齐 vanilla HappyGhast.canFallInLove = false 与 getBreedOffspring 的产卵语义）
    [[nodiscard]] bool canBreed() const override;

    /**
     * @brief 生成幼体
     *
     * @note vanilla 繁殖产出的仍是快乐恶魂幼体，但实际孵化路径为干燥恶魂方块；
     *       此处返回幼体以满足 BreedGoal 契约。
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 飞行物理 ==========

    /**
     * @brief 飞行移动（对齐 vanilla LivingEntity.travelFlying，加速 0.02、阻力 0.91/0.8/0.5）
     */
    void travel(f32 strafing, f32 vertical, f32 forward) override;

    // ========== 受伤 ==========

    /// 对齐 vanilla HappyGhast.checkFallDamage：空实现，飞行生物免疫摔落伤害。
    void handleFallDamage(f32 distance, f32 damageMultiplier) override;

    // ========== 声音 ==========

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /// 音量：幼体 1.0，成体 4.0（对齐 vanilla HappyGhast.getSoundVolume）
    [[nodiscard]] f32 getSoundVolume() const override;

    // ========== 挽具 / 骑乘 ==========

    /// 是否已装备挽具（对齐 vanilla HappyGhast.isWearingBodyArmor 的挽具语义）
    [[nodiscard]] bool hasHarness() const;

    /// 挽具物品堆（未装备时为空堆），供掉落/剪下与测试断言使用
    [[nodiscard]] const ItemStack& harnessStack() const;

    /**
     * @brief 乘客上限 4（对齐 vanilla HappyGhast.canAddPassenger：passengers.size() < 4）
     */
    [[nodiscard]] i32 getMaxPassengers() const override
    {
        return 4;
    }

    /**
     * @brief 能否被玩家骑乘（对齐 vanilla HappyGhast.getControllingPassenger 的挽具前提）
     *
     * vanilla 的玩家骑乘入口是 mobInteract 中 isWearingBodyArmor() 的上车分支
     * （HappyGhast.java:278-280），而非 Entity.canRide 的载体检查；对应到 Cubium 即为
     * 本虚方法：只有装备了挽具的成体才承载玩家。幼体与未装备挽具的成体不可被骑。
     */
    [[nodiscard]] bool canBeRidden(const Entity& vehicle) const override;

    // ========== 静止超时 ==========

    /**
     * @brief 是否处于静止超时（对齐 vanilla HappyGhast.isOnStillTimeout 背后的
     *        serverStillTimeout > 0）
     *
     * 静止超时期间快乐恶魂拒绝乘客的操控（vanilla 把 getControllingPassenger 置空），
     * 由 tick 在检测到"上方无玩家"时累计、检测到玩家时重置。
     */
    [[nodiscard]] bool isOnStillTimeout() const
    {
        return m_serverStillTimeout > 0;
    }

    /// 静止超时剩余 tick 数（测试与调试断言用）
    [[nodiscard]] i32 serverStillTimeout() const
    {
        return m_serverStillTimeout;
    }

    /// 测试入口：直接设置静止超时（触发 0 → 正数时的位置快照下发）。
    void setServerStillTimeoutForTest(i32 ticks)
    {
        _setServerStillTimeout(ticks);
    }

    /// 控制乘客：vanilla 仅在"装备挽具 + 非静止超时 + 首名乘客是玩家"时返回该玩家，
    /// 否则回退基类（HappyGhast.java:328-332）。静止超时时乘客无法驾驶。
    [[nodiscard]] EntityInstanceId getControllingPassenger() const override;

    /**
     * @brief 玩家（或其坐骑的根载具）是否停在快乐恶魂上方
     *
     * 对齐 vanilla HappyGhast.scanPlayerAboveGhast:532-546：取包围盒向两侧各外扩 1 格、
     * 向上延伸到"顶面 + 高度/2"的区域，区域内有任意非观察者玩家的根载具（且该根载具
     * 不是快乐恶魂本身）即返回 true。
     */
    [[nodiscard]] bool scanPlayerAbove() const;

    // ========== 刻更新 ==========

    /**
     * @brief 每 tick：维护静止超时与降水回血（对齐 vanilla HappyGhast.tick:401-421 /
     *        aiStep:423-431 的 continuousHeal）
     */
    void tick() override;

public:
    /// 幼体 → 成体或成体 → 幼体切换时重建移动控制与目标集合（对齐 vanilla ageBoundaryReached）
    void setChild(bool child) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 交互 ==========

    /**
     * @brief 右键交互（对齐 vanilla HappyGhast.mobInteract:266-285）
     *
     * 幼体：交基类（喂食雪球加速成长）。
     * 成体：手持物品时先走物品的 itemInteractionForEntity，可执行则结束（装备挽具）；
     *       已装备挽具且玩家未潜行时让玩家骑乘；否则交基类。
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    // ========== 装备槽语义 ==========

    /// 挽具可右键装备到 Body 槽（对齐 vanilla Equippable.harness 的 equipOnInteract +
    /// allowedEntities=CAN_EQUIP_HARNESS；实体侧等价判定见 canEquipItemOnInteract 注释）
    [[nodiscard]] bool canEquipItemOnInteract(const ItemStack& stack, EquipmentSlot slot) const override;

    /// Body 槽（挽具）被剪下时使用 HARNESS_UNEQUIP（vanilla Equippable.harness 的 shearingSound）
    [[nodiscard]] std::optional<ResourceLocation> shearingSoundForSlot(EquipmentSlot slot) const override;

    /// Body 槽被清空（挽具被剪下）时清空 home 位置（对齐 vanilla HappyGhast.removePassenger 的 clearHome）
    void onSlotCleared(EquipmentSlot slot) override;

    // ========== 乘客 ==========

    /// 首个乘客上船时播放 HARNESS_GOGGLES_DOWN 并按上方是否有玩家设定静止超时
    /// （对齐 vanilla HappyGhast.addPassenger:293-307）
    void onPassengerAdded(Entity& passenger) override;

    /// 最后一个乘客离开时播放 HARNESS_GOGGLES_UP 并清空 home；任何乘客下车都把
    /// 静止超时设为 10（对齐 vanilla HappyGhast.removePassenger:309-320）
    void onPassengerRemoved(Entity& passenger) override;

private:
    /**
     * @brief 设置静止超时（对齐 vanilla HappyGhast.setServerStillTimeout:77-85）
     */
    void _setServerStillTimeout(i32 ticks);

    /**
     * @brief 静止超时时快乐恶魂"原地停住"（对齐 vanilla GhastMoveControl 在
     *        shouldBeStopped 为真时调用 stopInPlace）
     *
     * 注意：Cubium 的 GoalSelector 仍会驱动飞行控制器，故这里把速度清零并清空导航，
     * 等价于 vanilla 的 stopInPlace。
     */
    void _stopInPlace();

    /**
     * @brief 降水/云层回血（对齐 vanilla HappyGhast.continuousHeal:446-453）
     *
     * 处于云层中或所处位置正在降水时每 20 tick 回 1 点，否则每 600 tick 回 1 点。
     */
    void _continuousHeal();

    /// 静止超时计数（对齐 vanilla HappyGhast.serverStillTimeout）
    i32 m_serverStillTimeout = 0;

    /// 上一帧是否为静止超时（用于检测"刚进入静止"以执行一次停住）
    bool m_wasOnStillTimeout = false;
};

} // namespace mc
