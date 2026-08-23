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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/ecs/components/HorseAnimationComponent.hpp"
#include "common/entity/ecs/components/HorseAttributeComponent.hpp"
#include "common/entity/ecs/components/HorseBoostComponent.hpp"
#include "common/entity/ecs/components/HorseInventoryComponent.hpp"
#include "common/entity/ecs/components/HorseJumpComponent.hpp"
#include "common/entity/ecs/components/HorseStatusComponent.hpp"
#include "common/entity/ecs/components/HorseTamingComponent.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/interfaces/IEquipable.hpp"
#include "common/entity/interfaces/IJumpingMount.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <memory>
#include <optional>
#include <string>

namespace mc {

// Forward declarations
class Player;
class ItemStack;
class LivingEntity;

/**
 * @brief 马类实体基类
 *
 * 所有马类实体（马、驴、骡、羊驼、骷髅马、僵尸马）的抽象基类。
 * 实现可骑乘、可跳跃、装备栏等通用功能。
 *
 * 【重要】MC 1.16.5 中，AbstractHorseEntity 只实现 IJumpingMount，
 * 不实现 IRideable 接口。马的控制逻辑通过 MobEntity 的乘客系统实现，
 * 而不是像猪/炽足兽那样通过 IRideable::ride() 方法。
 *
 * 参考 MC 1.16.5 AbstractHorseEntity
 */
class AbstractHorseEntity : public AnimalEntity, public entity::IJumpingMount, public entity::IEquipable {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    AbstractHorseEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~AbstractHorseEntity() override = default;

    // 禁止拷贝
    AbstractHorseEntity(const AbstractHorseEntity&) = delete;
    AbstractHorseEntity& operator=(const AbstractHorseEntity&) = delete;

    // 允许移动
    AbstractHorseEntity(AbstractHorseEntity&&) = delete;
    AbstractHorseEntity& operator=(AbstractHorseEntity&&) = delete;

    // ========== IJumpingMount 接口实现 ==========

    void onJump() override;
    [[nodiscard]] i32 getJumpCharge() const override;
    void setJumpCharge(i32 power) override;
    [[nodiscard]] f32 getMaxJumpHeight() const override;
    [[nodiscard]] bool canJump() const override;
    void startJumping(i32 jumpPower) override;
    void stopJumping() override;

    // ========== 骑乘系统 ==========

    /**
     * @brief 检查是否正在被骑乘
     */
    [[nodiscard]] bool isBeingRidden() const;

    /**
     * @brief 检查玩家是否可以骑乘
     * @param player 玩家
     * @return 是否可以骑乘
     */
    [[nodiscard]] bool canBeRiddenBy(Player* player) const;

    /**
     * @brief 获取骑乘者（玩家）
     * @return 骑乘者指针，如果没有则返回 nullptr
     */
    [[nodiscard]] Player* getRider() const { return m_rider; }

    /**
     * @brief 设置骑乘者
     * @param rider 骑乘者
     */
    void setRider(Player* rider) { m_rider = rider; }

    // ========== 驯服系统 ==========

    /**
     * @brief 是否已驯服
     */
    [[nodiscard]] bool isTame() const;

    /**
     * @brief 检查是否有主人（非空UUID）
     */
    [[nodiscard]] bool hasOwner() const;

    /**
     * @brief 设置驯服状态
     * @param tame 是否驯服
     */
    void setTame(bool tame);

    /**
     * @brief 由玩家驯服此马
     *
     * MC 1.16.5: setTamedBy(PlayerEntity player)
     * 设置主人UUID、设为已驯服、触发进度、发送爱心粒子
     *
     * @param player 驯服者
     * @return 是否成功
     */
    bool setTamedBy(Player* player);

    /**
     * @brief 让马愤怒（扬蹄并播放愤怒音效）
     *
     * MC 1.16.5: makeMad()
     * 当驯服失败或被激怒时调用。
     */
    void makeMad();

    /**
     * @brief 检查是否可以执行扬蹄动画
     *
     * MC 1.21.11: canPerformRearing()
     * 羊驼不扬蹄，覆写返回 false。其他马类默认返回 true。
     */
    [[nodiscard]] virtual bool canPerformRearing() const { return true; }

    /**
     * @brief 让马后腿站立（扬蹄）
     *
     * MC 1.21.11: standIfPossible()
     * 条件：canPerformRearing() && (canPassengerSteer() || !isClientSide())
     * 持续 20 tick 后自动恢复。
     */
    void makeHorseRear();

    /**
     * @brief 检查是否正在扬蹄
     * @return 是否正在扬蹄
     */
    [[nodiscard]] bool isRearing() const;

    /**
     * @brief 设置扬蹄状态
     * @param rearing 是否扬蹄
     */
    void setRearing(bool rearing);

    /**
     * @brief 清除扬蹄状态
     *
     * MC 1.21.11: clearStanding()
     * 同时清除扬蹄标志和计数器
     */
    void clearRearing();

    /**
     * @brief 检查是否正在吃
     * @return 是否正在吃
     */
    [[nodiscard]] bool isEating() const;

    /**
     * @brief 设置进食状态
     * @param eating 是否正在吃
     */
    void setEating(bool eating);

    /**
     * @brief 检查是否已繁殖
     * @return 是否已繁殖
     */
    [[nodiscard]] bool isBred() const;

    /**
     * @brief 设置繁殖状态
     * @param bred 是否已繁殖
     */
    void setBred(bool bred);

    /**
     * @brief 检查嘴巴是否张开
     * @return 嘴巴是否张开
     */
    [[nodiscard]] bool isMouthOpen() const;

    /**
     * @brief 设置嘴巴张开状态
     * @param open 嘴巴是否张开
     */
    void setMouthOpen(bool open);

    /**
     * @brief 获取愤怒音效
     *
     * 基类返回 nullptr，子类应重写提供具体音效。
     * MC 1.16.5: getAngrySound()
     *
     * @return 愤怒音效资源位置，如果没有返回空
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getAngrySound() const { return std::nullopt; }

    /**
     * @brief 获取进食音效
     *
     * MC 1.16.5: getEatSound()
     * 子类应重写提供具体音效。
     *
     * @return 进食音效资源位置
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getEatSound() const { return std::nullopt; }

    /**
     * @brief 获取主人UUID
     * @return 主人UUID字符串，如果没有主人返回空字符串
     */
    [[nodiscard]] const std::string& getOwnerUuid() const;

    /**
     * @brief 设置主人UUID
     * @param uuid 主人UUID字符串
     */
    void setOwnerUuid(const std::string& uuid);

    /**
     * @brief 清除主人UUID
     */
    void clearOwnerUuid();

    /**
     * @brief 通过UUID查找主人实体
     *
     * 使用 IWorld::getEntityByUuid() 进行 O(1) 查找，并验证实体类型为 LivingEntity。
     *
     * @return 主人实体指针，如果未找到返回 nullptr
     */
    [[nodiscard]] LivingEntity* getOwner() const;

    /**
     * @brief 获取驯服进度 (0-100)
     */
    [[nodiscard]] i32 getTemper() const;

    /**
     * @brief 增加驯服进度
     * @param amount 增加量
     * @return 是否达到驯服阈值
     */
    bool increaseTemper(i32 amount);

    /**
     * @brief 获取最大驯服进度
     */
    [[nodiscard]] i32 getMaxTemper() const;

    /**
     * @brief 张开马嘴（播放进食动画）
     *
     * MC 1.21.11: openMouth()
     * 仅在服务端调用，设置张嘴计数器和状态标志。
     * 计数器会在 tick() 中递增，超过 30 tick 后自动关闭。
     */
    void openMouth();

    /**
     * @brief 检查是否可以吃草
     *
     * MC 1.21.11: canEatGrass()
     * 大多数马类可以吃草，骷髅马和僵尸马覆写返回 false。
     */
    [[nodiscard]] virtual bool canEatGrass() const { return true; }

    /**
     * @brief 检查物品是否可用于驯服
     * @param itemStack 物品堆
     * @return 是否可用于驯服
     */
    [[nodiscard]] virtual bool isTameItem(const ItemStack& itemStack) const;

    /**
     * @brief 处理喂食
     *
     * MC 1.16.5: AbstractHorseEntity.handleEating()
     * 处理玩家喂食马匹的效果：
     * - 治疗生命值
     * - 加速幼体成长
     * - 增加驯服进度
     * - 触发繁殖（金苹果/金胡萝卜）
     *
     * @param player 喂食的玩家
     * @param itemStack 食物物品堆（可能被修改）
     * @return 是否成功喂食
     */
    virtual bool handleEating(Player* player, ItemStack& itemStack);

    /**
     * @brief 检查物品是否为马的食物
     *
     * MC 1.16.5: AbstractHorseEntity.func_230276_fq_()
     * 检查物品是否可用于喂食马匹（小麦、糖、干草块、苹果、金胡萝卜、金苹果）
     *
     * @param itemStack 物品堆
     * @return 是否为马的食物
     */
    [[nodiscard]] virtual bool isFoodItem(const ItemStack& itemStack) const;

    // ========== 装备系统 ==========

    /**
     * @brief 获取装备栏大小
     */
    [[nodiscard]] virtual i32 getInventorySize() const { return 2; } // 鞍槽 + 马铠槽

    /**
     * @brief 是否有马铠
     */
    [[nodiscard]] bool hasArmor() const;

    /**
     * @brief 设置马铠状态
     */
    void setArmor(bool armor);

    // ========== 速度和跳跃 ==========

    /**
     * @brief 获取移动速度
     */
    [[nodiscard]] f32 getSpeed() const;

    /**
     * @brief 获取马的基础生命值（HorseAttributeComponent.m_horseHealth）
     *
     * 供叶子类 registerAttributes 读取（迁移后字段进组件，叶子类不再能直接访问 m_horseHealth）。
     */
    [[nodiscard]] f32 getHorseHealth() const;

    /**
     * @brief 获取跳跃强度
     */
    [[nodiscard]] f32 getJumpStrength() const;

    /**
     * @brief 设置跳跃强度
     */
    void setJumpStrength(f32 strength);

    // ========== IEquipable 接口实现 ==========

    /**
     * @brief 获取装备槽数量
     */
    [[nodiscard]] i32 getEquipmentSlotCount() const override { return getInventorySize(); }

    /**
     * @brief 获取指定槽位的装备
     */
    [[nodiscard]] ItemStack getEquipment(i32 slot) const override;

    /**
     * @brief 设置指定槽位的装备
     */
    void setEquipment(i32 slot, const ItemStack& item) override;

    /**
     * @brief 检查是否可以装备指定物品
     *
     * MC 1.16.5: AbstractHorseEntity.replaceItemInInventory()
     * - 槽位 0（鞍槽）：只能放鞍（Items::SADDLE）
     * - 槽位 1（马铠/装饰槽）：需要子类实现 isValidArmorForSlot()
     *
     * @param item 要检查的物品
     * @param slot 目标槽位
     * @return 如果可以装备返回 true
     */
    [[nodiscard]] bool canEquip(const ItemStack& item, i32 slot) const override;

    /**
     * @brief 检查是否可以装备鞍
     *
     * MC 1.16.5: AbstractHorseEntity.func_230264_L_()
     * 大多数马类都可以装备鞍，但羊驼不行。
     *
     * @return 如果可以装备鞍返回 true
     */
    [[nodiscard]] virtual bool canEquipSaddle() const { return true; }

    /**
     * @brief 检查是否支持马铠/装饰槽位
     *
     * MC 1.16.5: AbstractHorseEntity.func_230276_fq_()
     * 只有 HorseEntity 和 LlamaEntity 支持此槽位。
     *
     * @return 如果支持马铠/装饰槽位返回 true
     */
    [[nodiscard]] virtual bool hasArmorSlot() const { return false; }

    /**
     * @brief 检查物品是否是有效的马铠/装饰
     *
     * MC 1.16.5: AbstractHorseEntity.isArmor(ItemStack)
     * 子类需要覆盖此方法：
     * - HorseEntity: 检查 HorseArmorItem
     * - LlamaEntity: 检查地毯
     *
     * @param item 要检查的物品
     * @return 如果是有效的马铠/装饰返回 true
     */
    [[nodiscard]] virtual bool isValidArmorForSlot(const ItemStack& item) const;

    // ========== 鞍系统 ==========

    /**
     * @brief 检查是否装备了鞍
     * MC 1.16.5: AbstractHorseEntity.isHorseSaddled()
     */
    [[nodiscard]] bool hasSaddle() const;

    /**
     * @brief 设置鞍的状态
     */
    void setSaddle(bool saddle);

    /**
     * @brief 检查是否可以被控制方向
     * MC 1.16.5: 马需要鞍才能被控制
     */
    [[nodiscard]] bool canBeSteered() const override;

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief AI 步进更新
     *
     * MC 1.21.11 AbstractHorse.aiStep()
     * 服务端逻辑：
     * - 随机尾巴摆动（1/200 概率）
     * - 自然恢复（1/900 概率，死亡时间为 0 时）
     * - 吃草触发（1/300 概率，未骑乘、脚下为草方块）
     * - 吃草计数器（超过 50 tick 停止吃草）
     */
    void aiStep() override;

    /**
     * @brief 从 NBT 读取额外数据（薄壳）
     *
     * 批次8 Step5：字段级 NBT 读写已搬 ComponentSerializerRegistry（HorseTaming/Jump/
     * Status/Attribute 四序列化器，loadAll 在本方法之前调）。本薄壳仅调基类 + initHorseChest：
     * initHorseChest 需在所有组件 load 完成后按新 NBT（装箱子后 getInventorySize 变大）
     * 重置库存规模，loadAll 无法感知"所有组件 load 完"时序，故保留此薄壳在末尾调 initHorseChest。
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    /**
     * @brief 处理玩家交互
     *
     * 处理玩家右键点击马匹时的交互：
     * - 被骑乘中/幼年：交给基类处理
     * - 已驯服 + Shift：打开背包界面
     * - 手持物品时：先尝试物品自身交互（如鞍），再尝试装备马铠/装饰
     * - 空手：让玩家骑乘
     *
     * @param player 与此实体交互的玩家
     * @param hand 玩家使用的手
     * @return 交互结果类型
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    /**
     * @brief 打开马的背包界面
     *
     * 条件：服务端 && (无骑乘者 || 骑乘者是自身) && 已驯服
     *
     * @param player 打开背包的玩家
     */
    virtual void openInventory(Player& player);

    /**
     * @brief 装备马铠/装饰到槽位 1
     *
     * 将物品装备到马铠/装饰槽，设置护甲状态，播放音效，消耗物品。
     *
     * @param player 装备物品的玩家
     * @param itemStack 要装备的物品堆（将被修改）
     */
    void equipArmor(Player& player, ItemStack& itemStack);

    /**
     * @brief 让玩家骑乘马匹
     *
     * 让玩家骑上马，触发 RunAroundLikeCrazyGoal（未驯服时）或正常骑乘。
     *
     * @param player 要骑乘的玩家
     */
    void doPlayerRide(Player& player);

    /**
     * @brief 骑乘移动处理
     * MC 1.16.5: travel(Vector3d)
     */
    void travel(f32 strafing, f32 vertical, f32 forward) override;

    /**
     * @brief 更新乘客位置
     *
     * MC 1.16.5: AbstractHorseEntity.updatePassenger(Entity)
     * 重写以处理扬蹄时的乘客位置偏移
     *
     * @param passenger 乘客实体
     */
    void updatePassengerPosition(Entity& passenger) override;

    /**
     * @brief 获取扬蹄动画进度（用于渲染插值）
     * @param partialTicks 部分tick时间
     * @return 扬蹄动画进度 (0.0-1.0)
     */
    [[nodiscard]] f32 getRearingAmount(f32 partialTicks) const;

    /**
     * @brief 获取低头吃草动画进度（用于渲染插值）
     * @param partialTicks 部分tick时间
     * @return 低头动画进度 (0.0-1.0)
     */
    [[nodiscard]] f32 getHeadLeanAmount(f32 partialTicks) const;

    /**
     * @brief 获取张嘴动画进度（用于渲染插值）
     * @param partialTicks 部分tick时间
     * @return 张嘴动画进度 (0.0-1.0)
     */
    [[nodiscard]] f32 getMouthOpennessAmount(f32 partialTicks) const;

protected:
    using LivingEntity::getEquipment;
    using LivingEntity::setEquipment;

    void registerGoals() override;
    void registerAttributes() override;
    void registerData() override;

    // ========== 状态标志辅助方法 ==========

    /**
     * @brief 聚合 6 状态标志写入 STATUS_PARAM DataParameter
     *
     * 读 HorseStatusComponent 6 bool（tame/saddled/bred/eating/rearing/mouthOpen）
     * 组合成 i8（bit1-6）调 m_dataManager.set(STATUS_PARAM, ...)。一次 setter 一次 set，
     * 替代旧 setHorseWatchableBoolean 每次 read-modify-write 的冗余读。6 个状态 setter
     * 写完组件字段后调本方法同步镜像下发客户端。
     */
    void _syncStatusFlags();

    /**
     * @brief 设置后代属性
     *
     * MC 1.16.5: AbstractHorseEntity.setOffspringAttributes()
     * 遗传公式：(父本基础值 + 母本基础值 + 随机变异值) / 3
     *
     * @param partner 配偶实体
     * @param offspring 后代实体
     */
    void setOffspringAttributes(const AgeableEntity& partner, AbstractHorseEntity& offspring);

    /**
     * @brief 获取随机生命值变异
     * MC 1.16.5: 15 + rand(8) + rand(9)，范围 15-30
     */
    [[nodiscard]] f32 getModifiedMaxHealth() const;

    /**
     * @brief 获取随机跳跃力变异
     * MC 1.16.5: 0.4 + rand*0.2*3，范围 0.4-1.0
     */
    [[nodiscard]] f64 getModifiedJumpStrength() const;

    /**
     * @brief 获取随机速度变异
     * MC 1.16.5: (0.45 + rand*0.3*3) * 0.25，范围 0.1125-0.3375
     */
    [[nodiscard]] f64 getModifiedMovementSpeed() const;

    // ========== 尺寸 ==========
    // 子类应该重写这些方法以提供正确的尺寸

    [[nodiscard]] f32 getBaseWidth() const override { return 1.3964844f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 1.6f; }

    /**
     * @brief 更新骑乘状态
     */
    void updateRiding();

    /**
     * @brief 更新跳跃蓄力
     */
    void updateJumpPower();

    /**
     * @brief 执行跳跃
     */
    void performJump();

    /**
     * @brief 更新加速状态
     */
    void updateBoost();

    /**
     * @brief 初始化随机属性
     *
     * 马的跳跃、速度、生命值在生成时随机确定
     */
    void initRandomAttributes();

    /**
     * @brief 初始化马背包
     * MC 1.16.5: initHorseChest()
     */
    void initHorseChest();

protected:
    // 骑乘状态（m_rider 不进组件：运行时指针、不存盘不同步、生命周期由 passengers 体系
    // 外部管理。getRider/setRider 高频 inline 保留 OOP 成员，对齐 minecart 乘客系统不进组件范式）
    Player* m_rider = nullptr;

private:
    // MC 1.21.11 数据参数
    static entity::DataParameter<i8> STATUS_PARAM; // 使用 i8 代替 u8（DataValue 支持的类型）

protected:
    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

public:
    // MC 1.21.11 状态标志位（public 供客户端 ClientEntity 解析 STATUS_PARAM 拆位使用）
    static constexpr i8 STATUS_FLAG_TAME = 2;        // bit 1: 已驯服
    static constexpr i8 STATUS_FLAG_SADDLE = 4;      // bit 2: 已装备鞍
    static constexpr i8 STATUS_FLAG_BRED = 8;        // bit 3: 已繁殖
    static constexpr i8 STATUS_FLAG_EATING = 16;     // bit 4: 正在吃
    static constexpr i8 STATUS_FLAG_REARING = 32;    // bit 5: 正在扬蹄
    static constexpr i8 STATUS_FLAG_MOUTH_OPEN = 64; // bit 6: 嘴张开

    /**
     * @brief 获取 STATUS_PARAM 的 DataParameter id（供客户端 ClientEntity hasParam/_readMetadata 判断）
     *
     * 返回 u16 id 而非 DataParameter 引用，对齐 TameableEntity::getTamedParamId 范式
     * （ClientEntity::_readMetadata 形参为 u16）。
     */
    [[nodiscard]] static u16 getStatusParamId() { return STATUS_PARAM.id(); }

private:
    // 常量
    static constexpr f32 MIN_SPEED = 0.1127f;      // 最小速度
    static constexpr f32 MAX_SPEED = 0.3375f;      // 最大速度
    static constexpr f32 MIN_JUMP = 0.4f;          // 最小跳跃
    static constexpr f32 MAX_JUMP = 1.0f;          // 最大跳跃
    static constexpr f32 MIN_HEALTH = 15.0f;       // 最小生命值
    static constexpr f32 MAX_HEALTH = 30.0f;       // 最大生命值
    static constexpr i32 MAX_BOOST_TIME = 300;     // 最大加速时间（ticks）
    static constexpr f32 JUMP_POWER_SCALE = 0.98f; // 跳跃蓄力缩放
};

} // namespace mc
