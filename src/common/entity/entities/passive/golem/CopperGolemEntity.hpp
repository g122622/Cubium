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

#include "CopperGolemTypes.hpp"
#include "GolemEntity.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/interfaces/ContainerUser.hpp"
#include "common/entity/interfaces/IShearable.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace mc {

// Forward declarations
class IWorld;
class LivingEntity;

namespace test {
class CopperGolemEntityTestAccessor; // 测试访问器，声明为 friend 以访问 private 成员
} // namespace test

/**
 * @brief 铜傀儡实体（MC 1.21.11）
 *
 * 铜傀儡是一种友好型傀儡，由玩家用斧头敲击铜傀儡雕像（Unaffected 等级）生成。
 *
 * 特性：
 * - 友好：不主动攻击任何生物
 * - 持久化：生成后标记为 PersistenceRequired，不会自然消失
 * - 氧化：随时间氧化到下一等级（Unaffected → Exposed → Weathered → Oxidized）
 * - 转雕像：达到 Oxidized 等级后，有概率在空气中转化为 Oxidized 铜傀儡雕像
 * - 可剪切：可用剪刀剪下天线槽（Saddle 槽）中的物品（如罂粟花）
 * - 蜜脾涂蜡：可用蜜脾阻止氧化
 * - 斧头刮削：可用斧头刮削回上一氧化等级
 * - 物品运输：在铜箱子与普通箱子/陷阱箱之间运输物品（TransportItemsBetweenContainersGoal）
 *
 * 与 MC 原版的差异：
 * - MC 原版使用 Brain 系统（CopperGolemAi）实现物品运输行为，本项目使用 GoalSelector
 *   实现，通过 TransportItemsBetweenContainersGoal 复刻相同行为。
 * - 氧化与转雕像逻辑完整实现（对应 MC 1.21.11 CopperGolem.updateWeathering/turnToStatue）。
 *
 * 天线槽设计（对应 MC 1.21.11 CopperGolem.EQUIPMENT_SLOT_ANTENNA）：
 * - 铜傀儡的"天线槽"复用 EquipmentSlot::Saddle（与 MC 原版一致）。
 * - 天线物品并非独立物品，而是罂粟花（minecraft:poppy），由铁傀儡的
 *   OfferFlowerGoal 赠予铜傀儡并装备到 Saddle 槽。
 * - 剪切时取出 Saddle 槽物品并掉落，由 ItemTags::SHEARABLE_FROM_COPPER_GOLEM 判断可剪性。
 * - 转雕像时通过 MobEntity::dropPreservedEquipment() 自动掉落 Saddle 槽物品
 *   （需先 setGuaranteedDrop(Saddle) 标记保留，由 OfferFlowerGoal 调用）。
 *
 * ContainerUser 接口（对应 MC 1.21.11 CopperGolem implements ContainerUser）：
 * - hasContainerOpen(BlockPos)：检查 m_openedChestPos 是否匹配（含双箱另一半场景）
 * - getContainerInteractionRange()：返回 3.0（MC 1.21.11 CopperGolem.getContainerInteractionRange）
 * - getLivingEntity()：返回 this（CopperGolemEntity IS-A LivingEntity）
 * - setOpenedChestPos / clearOpenedChestPos：由 TransportItemsBetweenContainersGoal 调用
 *
 * 参考: net.minecraft.world.entity.animal.golem.CopperGolem (MC 1.21.11)
 */
class CopperGolemEntity : public GolemEntity, public entity::IShearable, public entity::ContainerUser {
public:
    /**
     * @brief 铜傀儡的天线装备槽
     *
     * 对应 MC 1.21.11 CopperGolem.EQUIPMENT_SLOT_ANTENNA = EquipmentSlot.SADDLE。
     * 铜傀儡头顶"天线"实际是 Saddle 槽中持有的物品（罂粟花），
     * 由铁傀儡 OfferFlowerGoal 赠予，可被剪刀剪下。
     */
    static constexpr EquipmentSlot EQUIPMENT_SLOT_ANTENNA = EquipmentSlot::Saddle;

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    explicit CopperGolemEntity(EntityInstanceId id);

    ~CopperGolemEntity() override = default;

    // 禁止拷贝
    CopperGolemEntity(const CopperGolemEntity&) = delete;
    CopperGolemEntity& operator=(const CopperGolemEntity&) = delete;

    // 允许移动
    CopperGolemEntity(CopperGolemEntity&&) = delete;
    CopperGolemEntity& operator=(CopperGolemEntity&&) = delete;

    /**
     * @brief 创建铜傀儡实体
     * @param world 世界实例
     * @return 新的铜傀儡实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 氧化状态 ==========

    /**
     * @brief 获取氧化等级
     * @return 当前氧化等级
     *
     * 对应 MC Java: CopperGolem.getWeatherState()
     */
    [[nodiscard]] entity::CopperGolemWeatherState getWeatherState() const noexcept { return m_weatherState; }

    /**
     * @brief 设置氧化等级
     * @param state 新的氧化等级
     *
     * 对应 MC Java: CopperGolem.setWeatherState(WeatherState)
     */
    void setWeatherState(entity::CopperGolemWeatherState state) noexcept { m_weatherState = state; }

    // ========== 行为状态 ==========

    /**
     * @brief 获取行为状态（动画状态机标识）
     * @return 当前行为状态
     *
     * 对应 MC Java: CopperGolem.getState()
     */
    [[nodiscard]] entity::CopperGolemState getBehaviorState() const noexcept { return m_behaviorState; }

    /**
     * @brief 设置行为状态
     * @param state 新的行为状态
     *
     * 对应 MC Java: CopperGolem.setState(CopperGolemState)
     */
    void setBehaviorState(entity::CopperGolemState state) noexcept { m_behaviorState = state; }

    // ========== 生成 ==========

    /**
     * @brief 由雕像转化生成时调用
     *
     * 设置氧化等级为 Unaffected 并播放生成音效。
     * 对应 MC Java: CopperGolem.spawn(WeatherState.UNAFFECTED)
     *
     * @param weatherState 初始氧化等级
     */
    void spawnFromStatue(entity::CopperGolemWeatherState weatherState);

    /**
     * @brief 播放生成音效
     *
     * 对应 MC Java: CopperGolem.playSpawnSound()
     */
    void playSpawnSound();

    // ========== ContainerUser 接口实现 ==========

    /**
     * @brief 检查当前是否打开了指定位置的容器
     *
     * 对应 MC 1.21.11 CopperGolem.hasContainerOpen(ContainerOpenersCounter, BlockPos)：
     * - 若 m_openedChestPos 为空返回 false
     * - 若 m_openedChestPos == pos 返回 true
     * - 否则获取 m_openedChestPos 处的 BlockState，若方块是 ChestBlock 且
     *   CHEST_TYPE != Single（双箱），计算连通位置（getConnectedDirection + offset）
     *   并比较是否等于 pos
     *
     * @param pos 待检查的容器位置
     * @return 如果当前打开此容器（或其双箱另一半）返回 true
     */
    [[nodiscard]] bool hasContainerOpen(const BlockPos& pos) const override;

    /**
     * @brief 获取容器交互范围
     *
     * 对应 MC 1.21.11 CopperGolem.getContainerInteractionRange() = 3.0
     *
     * @return 交互半径（3.0 方块）
     */
    [[nodiscard]] f64 getContainerInteractionRange() const override { return CONTAINER_INTERACTION_RANGE; }

    /**
     * @brief 获取实现此接口的 LivingEntity 指针
     *
     * CopperGolemEntity IS-A LivingEntity（通过 GolemEntity → CreatureEntity → MobEntity → LivingEntity），
     * 直接返回 this。
     */
    [[nodiscard]] LivingEntity* getLivingEntity() override;
    [[nodiscard]] const LivingEntity* getLivingEntity() const override;

    /**
     * @brief 设置当前打开的箱子位置
     *
     * 对应 MC 1.21.11 CopperGolem.setOpenedChestPos(BlockPos)。
     * 由 TransportItemsBetweenContainersGoal 在到达容器后（ticks==1）调用。
     *
     * @param pos 打开的箱子位置
     */
    void setOpenedChestPos(const BlockPos& pos) { m_openedChestPos = pos; }

    /**
     * @brief 清除当前打开的箱子位置
     *
     * 对应 MC 1.21.11 CopperGolem.clearOpenedChestPos()。
     * 由 TransportItemsBetweenContainersGoal 在交互结束（ticks==60）或寻路中调用。
     */
    void clearOpenedChestPos() { m_openedChestPos.reset(); }

    // ========== IShearable 接口实现 ==========

    /**
     * @brief 检查是否可以被剪切
     * @return 活着且天线槽（Saddle）物品属于 SHEARABLE_FROM_COPPER_GOLEM 标签时可剪
     *
     * 对应 MC Java: CopperGolem.readyForShearing()
     *   return this.isAlive() && this.getItemBySlot(EQUIPMENT_SLOT_ANTENNA).is(ItemTags.SHEARABLE_FROM_COPPER_GOLEM)
     */
    [[nodiscard]] bool isShearable() const override;

    /**
     * @brief 剪切铜傀儡
     *
     * 对应 MC Java: CopperGolem.shear(ServerLevel, SoundSource, ItemStack)
     * - 播放 COPPER_GOLEM_SHEAR 音效
     * - 取出天线槽（Saddle）物品并清空槽位
     * - 返回掉落物列表（由 ShearsItem 统一调用 ItemDropHelper 在世界生成 ItemEntity）
     *
     * @param player 执行剪切的玩家（可为 nullptr）
     * @return 获得的物品列表（天线槽中的物品）
     */
    std::vector<ItemStack> shear(Player* player = nullptr) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     *
     * MC 1.21.11 CopperGolem 默认尺寸：宽 0.49，高 0.98
     * 眼睛高度按比例取约 0.45（接近高度的一半）。
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.45f; }

    /**
     * @brief 获取实体宽度
     * MC 1.21.11 CopperGolem.createAttributes() 中的尺寸为 0.49f。
     */
    [[nodiscard]] f32 width() const override { return 0.49f; }

    /**
     * @brief 获取实体高度
     * MC 1.21.11 CopperGolem.createAttributes() 中的尺寸为 0.98f。
     */
    [[nodiscard]] f32 height() const override { return 0.98f; }

    // ========== 声音 ==========

    /**
     * @brief 获取受伤声音
     *
     * 根据氧化等级返回对应的 hurt 声音（基础/锈蚀/氧化）。
     * 对应 MC Java: CopperGolem.getHurtSound() -> CopperGolemOxidationLevels.getOxidationLevel(...).hurtSound()
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     *
     * 根据氧化等级返回对应的 death 声音。
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 播放脚步声
     *
     * 根据氧化等级返回对应的 step 声音。
     * 对应 MC Java: CopperGolem.playStepSound()
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

    // ========== 生命周期 ==========

    /**
     * @brief 每tick更新
     *
     * 服务端：更新氧化状态（updateWeathering）
     */
    void tick() override;

    // ========== NBT 序列化 ==========
    /**
     * @brief 写入额外 NBT 数据
     *
     * 对应 MC Java: CopperGolem.addAdditionalSaveData
     * 持久化字段：next_weather_age (i64)、weather_state (string)
     * 注意：behaviorState 为运行时动画状态，不持久化（与 MC 一致）
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 读取额外 NBT 数据
     *
     * 对应 MC Java: CopperGolem.readAdditionalSaveData
     * 缺失字段时使用默认值：next_weather_age=-1（未设置），weather_state=unaffected
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 获取环境音效
     *
     * 铜傀儡无环境音，对齐原版 AbstractGolem.getAmbientSound 返回 null，
     * 避免默认拼接出不存在的 entity.copper_golem.ambient。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

private:
    // ========== 私有方法 ==========

    /**
     * @brief 更新氧化状态
     *
     * 对应 MC Java: CopperGolem.updateWeathering(ServerLevel, RandomSource, long)
     *
     * 氧化逻辑：
     * - m_nextWeatheringTick == -2 表示已涂蜡，不氧化
     * - m_nextWeatheringTick == -1 表示需要初始化下一次氧化时间
     * - 否则：达到时间后氧化到下一等级，达到 Oxidized 后有概率转化为雕像
     */
    void updateWeathering(i64 currentGameTime);

    /**
     * @brief 检查是否可以转化为雕像
     *
     * 对应 MC Java: CopperGolem.canTurnToStatue(Level)
     * 条件：脚下位置为空气，且随机数 <= 0.0058F
     */
    [[nodiscard]] bool canTurnToStatue() const;

    /**
     * @brief 转化为氧化铜傀儡雕像
     *
     * 对应 MC Java: CopperGolem.turnToStatue(ServerLevel)
     * - 在当前位置放置 oxidized_copper_golem_statue（随机姿态、当前朝向）
     * - 创建方块实体并保存自定义名称
     * - 丢弃保存的装备（如有）
     * - 移除实体
     * - 播放变雕像音效
     * - 处理拴绳掉落
     */
    void turnToStatue();

    // ========== 私有成员 ==========

    /// 当前氧化等级（默认 Unaffected）
    entity::CopperGolemWeatherState m_weatherState = entity::CopperGolemWeatherState::Unaffected;

    /// 当前行为状态（默认 Idle）
    entity::CopperGolemState m_behaviorState = entity::CopperGolemState::Idle;

    /// 下次氧化 tick（-2 表示已涂蜡不氧化，-1 表示需初始化，>=0 表示具体 tick）
    i64 m_nextWeatheringTick = -1;

    /// 当前打开的箱子位置（对应 MC CopperGolem.openedChestPos，transient 不持久化）
    std::optional<BlockPos> m_openedChestPos;

    // 常量（对应 MC 1.21.11 CopperGolem 中的私有常量）
    static constexpr i64 IGNORE_WEATHERING_TICK = -2;     ///< 已涂蜡标记
    static constexpr i64 UNSET_WEATHERING_TICK = -1;      ///< 未设置标记
    static constexpr i32 WEATHERING_TICK_FROM = 504000;   ///< 氧化最小 tick
    static constexpr i32 WEATHERING_TICK_TO = 552000;     ///< 氧化最大 tick
    static constexpr f32 TURN_TO_STATUE_CHANCE = 0.0058F; ///< 转雕像概率

    /// 容器交互范围（对应 MC CopperGolem.getContainerInteractionRange = 3.0）
    static constexpr f64 CONTAINER_INTERACTION_RANGE = 3.0; ///< 转雕像概率

    friend class test::CopperGolemEntityTestAccessor;
};

} // namespace mc
