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

#include "common/command/ICommandSource.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/interfaces/IAngerable.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <optional>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

namespace scoreboard {
class Team;
}

class LivingEntity;

/**
 * @brief 可驯服实体基类
 *
 * 支持被玩家驯服的动物实体基类。
 * 狼、猫、鹦鹉等可驯服动物继承此类。
 *
 * 特性：
 * - 驯服状态（是否被驯服）
 * - 主人ID（驯服后的玩家）
 * - 坐下/站起
 * - 愤怒系统（攻击目标追踪）
 * - 跟随主人行为
 */
class TameableEntity : public AnimalEntity, public entity::IAngerable {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    TameableEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~TameableEntity() override = default;

    // 禁止拷贝
    TameableEntity(const TameableEntity&) = delete;
    TameableEntity& operator=(const TameableEntity&) = delete;

    // 允许移动
    TameableEntity(TameableEntity&&) = delete;
    TameableEntity& operator=(TameableEntity&&) = delete;

    // ========== 驯服系统 ==========

    /**
     * @brief 检查是否已被驯服
     * @return 如果已被驯服返回true
     *
     * 通过 DataParameter 从 EntityDataManager 读取 DATA_FLAGS_PARAM 的 bit2（对齐 vanilla
     * TamableAnimal.DATA_FLAGS_ID & 4）。服务端调用 setTamed 修改后自动经元数据同步到客户端。
     * 客户端 ClientEntity::setWolfTamed 镜像此状态（读同一 Byte 字段解 bit2）。
     */
    [[nodiscard]] bool isTamed() const { return (m_dataManager.get<i8>(DATA_FLAGS_PARAM) & 0x04) != 0; }

    /**
     * @brief 设置驯服状态
     * @param tamed 是否驯服
     *
     * 驯服后会更新实体的AI行为
     */
    virtual void setTamed(bool tamed);

    /**
     * @brief 检查物品是否可用于驯服
     * @param itemStack 物品堆
     * @return 如果物品可用于驯服返回true
     *
     * 子类应重写此方法以定义特定的驯服物品：
     * - 狼：骨头
     * - 猫：生鳕鱼、生鲑鱼
     * - 鹦鹉：种子
     *
     * 默认返回 false（不可驯服）。
     */
    [[nodiscard]] virtual bool isTameItem(const ItemStack& itemStack) const
    {
        MC_UNUSED(itemStack);
        return false;
    }

    /**
     * @brief 获取主人 UUID
     * @return 主人的 profile UUID，如果没有主人返回空
     *
     * 对齐 vanilla TamableAnimal.getOwnerReference()（1.21.11 为 Optional<EntityReference>，
     * 内部即 UUID）。getOwner() 据此 UUID 在世界中查找主人玩家实体。
     */
    [[nodiscard]] std::optional<Uuid> getOwnerId() const { return m_ownerId; }

    /**
     * @brief 设置主人 UUID
     * @param ownerId 主人的 profile UUID
     *
     * 同时把 owner 写入 DATA_OWNERUUID_PARAM 同步到客户端（对齐 vanilla
     * TamableAnimal.DATA_OWNERUUID_ID，wire = OptionalLivingEntityRef）。
     */
    void setOwnerId(Uuid ownerId);

    /**
     * @brief 清除主人
     */
    void clearOwner();

    /**
     * @brief 检查指定玩家是否是主人
     * @param playerId 玩家的 profile UUID
     * @return 如果是主人返回true
     */
    [[nodiscard]] bool isOwner(Uuid playerId) const { return m_ownerId.has_value() && m_ownerId.value() == playerId; }

    /**
     * @brief 获取驯服/坐下标志数据参数 ID
     *
     * 返回 DATA_FLAGS_PARAM 的 id（vanilla TamableAnimal.DATA_FLAGS_ID，Byte 类型）。
     * 客户端 ClientEntity 据此 id 读取 Byte 并解 bit2=tame / bit0=sitting 镜像状态。
     * 名字保留 getTamedParamId 以兼容现有调用方（语义为「flags 字段 id」）。
     */
    [[nodiscard]] static u16 getTamedParamId() { return DATA_FLAGS_PARAM.id(); }

    /**
     * @brief 获取主人 UUID 数据参数 ID
     *
     * 返回 DATA_OWNERUUID_PARAM 的 id（vanilla TamableAnimal.DATA_OWNERUUID_ID，
     * OptionalLivingEntityRef 类型）。测试/诊断用。
     */
    [[nodiscard]] static u16 getOwnerUuidParamId() { return DATA_OWNERUUID_PARAM.id(); }

    /**
     * @brief 获取主人实体
     *
     * 通过主人ID在世界中查找玩家实体。
     * @return 主人实体指针，如果未找到或无主人返回nullptr
     */
    [[nodiscard]] Player* getOwner() const;

    // ========== 队伍系统 ==========

    /**
     * @brief 获取驯服动物所在的队伍
     *
     * 已驯服的动物继承主人的队伍。未驯服的动物不在任何队伍中。
     *
     * @return 队伍指针，如果不在任何队伍返回 nullptr
     */
    [[nodiscard]] scoreboard::Team* getTeam() override;
    [[nodiscard]] const scoreboard::Team* getTeam() const override;

    // ========== 攻击判定 ==========

    /**
     * @brief 判断此驯服动物是否想要攻击指定目标
     *
     * 默认实现返回 true（允许攻击所有目标）。
     * 子类可重写此方法来限制攻击目标，例如狼不会攻击苦力怕和恶魂。
     *
     * @param target 待攻击的目标实体
     * @param owner 此驯服动物的主人（可能为 nullptr）
     * @return true 如果此驯服动物愿意攻击该目标
     */
    [[nodiscard]] virtual bool wantsToAttack(const LivingEntity& target, const LivingEntity* owner) const;

    // ========== 坐下/站起 ==========

    /**
     * @brief 检查是否坐下
     * @return 如果坐下返回true
     */
    [[nodiscard]] bool isSitting() const { return m_sitting; }

    /**
     * @brief 设置坐下状态
     * @param sitting 是否坐下
     */
    void setSitting(bool sitting);

    /**
     * @brief 切换坐下状态
     */
    void toggleSitting() { setSitting(!m_sitting); }

    // ========== IAngerable 接口实现 ==========

    void setAttackTarget(LivingEntity* target) override;
    [[nodiscard]] LivingEntity* getAttackTarget() const override
    {
        return const_cast<TameableEntity*>(this)->MobEntity::attackTarget();
    }
    void setRevengeTarget(LivingEntity* target) override;
    [[nodiscard]] LivingEntity* getRevengeTarget() const override;
    [[nodiscard]] i32 getRevengeTimer() const override { return m_revengeTimer; }
    [[nodiscard]] bool isAngry() const override { return m_angerTime > 0; }
    void setAngry(bool angry) override;
    [[nodiscard]] i32 getAngerTime() const override { return m_angerTime; }
    void setAngerTime(i32 time) override { m_angerTime = time; }

    // ========== 生命周期 ==========

    void tick() override;

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    /**
     * @brief 注册 AI 目标
     *
     * 子类应调用此方法来注册驯服动物的基础行为：
     * - SwimGoal (优先级 0)
     * - SitGoal (优先级 1, 驯服后)
     * - BreedGoal (优先级 2)
     * - FollowOwnerGoal (优先级 3, 驯服后)
     * - TemptGoal (优先级 4)
     * - FollowParentGoal (优先级 5)
     * - WaterAvoidingRandomWalkingGoal (优先级 6)
     * - LookAtGoal (优先级 7)
     * - LookRandomlyGoal (优先级 8)
     */
    void registerGoals() override;

    /**
     * @brief 注册属性
     *
     * 注册驯服动物的基础属性。
     */
    void registerAttributes() override;

    /**
     * @brief 注册同步数据参数
     *
     * 注册 DATA_TAMED_PARAM（驯服状态）到 EntityDataManager。
     * 客户端通过 getTamedParamId() 读取此参数 ID 并在元数据同步时
     * 调用 setWolfTamed 更新客户端镜像状态。
     *
     * 注意：由于 C++ 虚函数在基类构造函数中不会派发到派生类，
     * TameableEntity 构造函数必须显式调用此方法（参考 WolfEntity 模式）。
     */
    void registerData() override;

    /**
     * @brief 更新愤怒状态
     */
    void updateAnger() override;

    /**
     * @brief 当驯服状态改变时调用
     * @param tamed 是否驯服
     *
     * 子类可重写此方法来处理驯服状态变化的副作用
     */
    virtual void onTamed(bool tamed) { MC_UNUSED(tamed); }

private:
    // ========== 数据同步 ==========
    /**
     * @brief 驯服/坐下标志同步参数（Byte）
     *
     * 对应 MC 1.21.11 TamableAnimal.DATA_FLAGS_ID：bit2(mask 0x4)=tame，bit0(mask 0x1)=sitting。
     * 由 setTamed/setSitting 写入对应位，由 EntityTracker 自动广播到所有观察者客户端。
     * 客户端 ClientEntity::syncMetadataFromDataManager 读取此 Byte 并解 bit2/bit0 镜像状态。
     */
    static entity::DataParameter<i8> DATA_FLAGS_PARAM;

    /**
     * @brief 主人 UUID 同步参数（OptionalLivingEntityRef）
     *
     * 对应 MC 1.21.11 TamableAnimal.DATA_OWNERUUID_ID（Optional<EntityReference<LivingEntity>>，
     * wire = OptionalLivingEntityRef = 1 byte present + 16 字节大端 UUID）。
     * 由 setOwnerId/clearOwner 写入。
     */
    static entity::DataParameter<entity::OptionalUuidValue> DATA_OWNERUUID_PARAM;

protected:
    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

private:
    // 驯服状态
    bool m_sitting = false;
    std::optional<Uuid> m_ownerId;

    // 愤怒系统（m_attackTarget 使用 MobEntity::m_attackTarget，不重复声明）
    i32 m_angerTime = 0;
    i32 m_revengeTimer = 0;
    std::optional<u64> m_revengeTargetId;

    // 常量
    static constexpr i32 MAX_ANGER_TIME = 600; // 30秒
};

} // namespace mc
