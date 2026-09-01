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
#include "common/entity/ai/brain/Brain.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace mc {
namespace entity {

/**
 * @brief 村民实体
 *
 * 可交易的NPC村民，具有职业系统和繁殖能力。
 * 使用Brain系统进行高级AI控制。
 */
class VillagerEntity : public AbstractVillagerEntity {
public:
    // Brain类型别名
    using VillagerBrain = ai::brain::Brain<VillagerEntity>;

    /**
     * @brief 工作站点类型
     */
    enum class Workstation : u8 {
        None = 0,
        Smoker,           // 烟熏炉 - 屠夫
        BlastFurnace,     // 高炉 - 盔甲匠
        CartographyTable, // 制图台 - 制图师
        BrewingStand,     // 酿造台 - 牧师
        Composter,        // 堆肥桶 - 农民
        Barrel,           // 木桶 - 渔夫
        FletchingTable,   // 制箭台 - 制箭师
        Cauldron,         // 炼药锅 - 皮革匠
        Lectern,          // 讲台 - 图书管理员
        Stonecutter,      // 切石机 - 石匠
        SmithingTable,    // 锻造台 - 工具匠/武器匠
        Loom              // 织布机 - 牧羊人
    };

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param registry 实体注册表（ECS）
     */
    VillagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~VillagerEntity() noexcept override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    /**
     * @brief 村民不自然消失（对齐 vanilla AgeableMob.removeWhenFarAway 默认 false）
     *
     * DespawnManager::shouldDespawn 用 canDespawn(distance) 判断是否消失。
     * MobEntity 默认 return true（距玩家远时消失），但村民应保留在村庄中不自然消失
     * （对齐 vanilla Villager 继承 AgeableMob，removeWhenFarAway 默认 false）。
     * 否则 GameTest 中村民距 SimulatedPlayer 超过消失距离会 despawn，
     * 无法被僵尸杀死触发感染转化转化。
     */
    [[nodiscard]] bool canDespawn(double distanceToClosestPlayer) const noexcept override
    {
        (void)distanceToClosestPlayer;
        return false;
    }

    /**
     * @brief 重写死亡回调
     *
     * 村民死亡时释放占用的POI（床位、工作站、聚集点），
     * 并通知村庄管理器该村民已离开村庄。
     * 参考 MC Java Villager.die() -> releaseAllPois()
     *
     * 此外实现村民→僵尸村民感染转化（对齐 MC Java 1.21.11 Zombie.killedEntity +
     * Zombie.convertVillagerToZombieVillager）：当村民被僵尸系生物（ZombieEntity 及其子类
     * Husk/ZombieVillager，排除 ZombifiedPiglin——它不继承 ZombieEntity，对齐 Java 只有
     * Zombie 系感染）杀死时，按难度概率（Easy/Peaceful 0%、Normal 50%、Hard 100%）转化为
     * 僵尸村民而非真正死亡。转化保留村民的职业/类型/等级/经验数据、位置、旋转、婴儿状态、装备。
     * 转化成功则跳过正常死亡流程（不掉落经验/物品、不继续父类 die）。
     */
    void die(DamageSource& cause) override;

    /**
     * @brief 重写实体移除回调
     *
     * 村民被移除时释放占用的POI并通知村庄管理器。
     * 与 die() 不同，remove() 在实体被标记为移除时调用（如死亡动画结束后、
     * 区块卸载、/kill 命令等场景）。
     */
    void remove() override;

    /**
     * @brief 释放村民占用的所有POI
     *
     * 释放 HOME、JOB_SITE、POTENTIAL_JOB_SITE、MEETING_POINT 对应的 POI 占用，
     * 并通知村庄管理器该村民已离开村庄。
     * 参考 MC Java Villager.releasePoi() / releaseAllPois()
     */
    void releaseAllPois();

    // ========== 雷击 ==========

    /**
     * @brief 被闪电击中时的回调
     *
     * 对齐 vanilla Villager#thunderHit（Villager.java:773-787）：
     *   if (level.getDifficulty() != PEACEFUL) {
     *       Witch witch = convertTo(WITCH, ConversionParams.single(this, false, false), p -> {
     *           p.finalizeSpawn(level, difficulty, EntitySpawnReason.CONVERSION, null);
     *           p.setPersistenceRequired();
     *           this.releaseAllPois();
     *       });
     *       if (witch == null) super.thunderHit(level, lightning);   // 转化失败回退基类受5伤害
     *   } else {
     *       super.thunderHit(level, lightning);                      // 和平难度也调基类受5伤害
     *   }
     * 转化成功时不调 super（女巫不受伤），原体经 convertTo 内部 discard。ConversionParams 第三个
     * 参数 false 表示不保留装备（女巫不继承村民装备）。wiki tech_村民.txt#闪电：村民被闪电击中
     * 在非和平难度转化为女巫。
     *
     * @param lightning 击中此村民的闪电实体
     */
    void onStruckByLightning(entity::LightningBoltEntity* lightning) override;

    /**
     * @brief 重写被攻击回调
     *
     * 当村民被玩家攻击时，广播愤怒粒子效果（VillagerAngry）。
     * 同时触发村庄声望事件 VILLAGER_HURT。
     */
    void setLastHurtBy(LivingEntity* attacker) override;

    // ========== Brain系统 ==========

    /**
     * @brief 获取Brain
     */
    [[nodiscard]] VillagerBrain& brain() { return *m_brain; }
    [[nodiscard]] const VillagerBrain& brain() const { return *m_brain; }

    /**
     * @brief 初始化Brain（注册记忆模块、传感器、日程等）
     */
    void initializeBrain();

    // ========== 村民数据 ==========

    /**
     * @brief 获取村民数据
     */
    [[nodiscard]] const VillagerData& villagerData() const { return m_villagerData; }

    /**
     * @brief 设置村民数据
     */
    void setVillagerData(const VillagerData& data) { m_villagerData = data; }

    // ========== 职业 ==========

    /**
     * @brief 获取职业
     */
    [[nodiscard]] VillagerProfession profession() const { return m_villagerData.profession(); }

    /**
     * @brief 设置职业
     */
    void setProfession(VillagerProfession profession);

    /**
     * @brief 获取村民类型
     */
    [[nodiscard]] VillagerType villagerType() const { return m_villagerData.type(); }

    /**
     * @brief 设置村民类型
     */
    void setVillagerType(VillagerType type) { m_villagerData.setType(type); }

    // ========== 等级 ==========

    /**
     * @brief 获取等级
     */
    [[nodiscard]] i32 level() const { return m_villagerData.level(); }

    /**
     * @brief 设置等级
     */
    void setLevel(i32 level) { m_villagerData.setLevel(level); }

    /**
     * @brief 增加经验
     */
    void addVillagerExperience(i32 amount) { m_villagerData.addExperience(amount); }

    /**
     * @brief 获取交易等级（重写）
     */
    [[nodiscard]] i32 getTradingLevel() const override { return m_villagerData.level(); }

    /**
     * @brief 交易经验奖励（重写）
     *
     * 交易成功后给予村民经验，并可能生成经验球。
     */
    void rewardTradeXp(MerchantOffer& offer) override;

    // ========== 工作 ==========

    /**
     * @brief 获取工作站点位置
     */
    [[nodiscard]] BlockPos workStation() const { return m_workStation; }

    /**
     * @brief 设置工作站点
     */
    void setWorkStation(BlockPos pos) { m_workStation = pos; }

    /**
     * @brief 是否在工作站点
     */
    [[nodiscard]] bool isAtWorkstation() const { return m_atWorkstation; }

    /**
     * @brief 是否在工作
     */
    [[nodiscard]] bool isWorking() const { return m_working; }

    // ========== 繁殖 ==========

    /**
     * @brief 是否可以拾取物品
     * @param itemStack 物品堆
     * @return 是否可以拾取该物品
     *
     * 村民可拾取的物品：面包、土豆、胡萝卜、小麦、小麦种子、甜菜根、甜菜根种子
     * 农民职业额外可拾取：小麦、小麦种子、甜菜根种子、骨粉
     */
    [[nodiscard]] bool canPickUpItem(const ItemStack& itemStack) const;

    /**
     * @brief 重写 Mob.wantsToPickUp（对齐 MC Java 1.21.11 Villager.wantsToPickUp）
     *
     * MobEntity::tick 的 looting 段（对齐 Mob.aiStep）扫描 AABB 内 ItemEntity 时，
     * 对每个候选物品调 wantsToPickUp 判定是否需要拾取。基类默认实现为 canHoldItem
     * （装备槽语义），村民覆写为转调 canPickUpItem（食物/种子语义 + 库存可放入校验），
     * 使 MobEntity::tick 的拾取扫描能正确选中村民关心的食物物品。
     */
    [[nodiscard]] bool wantsToPickUp(const ItemStack& itemStack) const override;

    /**
     * @brief 重写 Mob.pickUpItem（对齐 MC Java 1.21.11 Villager.pickUpItem → InventoryCarrier.pickUpItem）
     *
     * 将 ItemEntity 的物品堆放入村民库存（SimpleInventory::addItem），处理部分装入的剩余 count：
     * 全部装入则移除 ItemEntity，部分装入则把剩余 count 写回 ItemEntity（对齐 Java InventoryCarrier.pickUpItem
     * 的 discard/setCount 分支）。
     *
     * 拾取后若库存食物点数（countFoodPointsInInventory）达到繁殖门槛（WANTS_MORE_FOOD_THRESHOLD=12），
     * 调 setWillingToBreed(true) 标记繁殖意愿。这是用布尔标志模拟 Java 1.21.11 Villager.canBreed()
     * 的 `foodLevel + countFoodPointsInInventory() >= 12` 食物门槛——Cubium 暂未实现 foodLevel
     * 字段与 eatAndDigestFood 链路，故以 willingToBreed 布尔替代，使自动拾取食物能驱动 VillagerBreedGoal。
     *
     * TODO: 对齐 Java 1.21.11 完整食物点数链路（foodLevel 字段 + eatUntilFull/eatAndDigestFood/
     *   digestFood + canBreed 基于 foodLevel 阈值判定），届时可移除 willingToBreed 布尔替代。
     */
    void pickUpItem(ItemEntity& itemEntity) override;

    // ========== 食物点数系统 ==========

    /**
     * @brief 食物点数映射表
     *
     * 定义每种繁殖物品的点数值：
     * - 面包: 4点
     * - 土豆: 1点
     * - 胡萝卜: 1点
     * - 甜菜根: 1点
     *
     * 使用函数而非静态常量，避免静态初始化顺序问题
     *（Items::BREAD 等指针在 Items::initialize() 后才有效）。
     */
    static const std::unordered_map<const Item*, i32>& foodPoints();

    /**
     * @brief 食物过剩阈值
     *
     * 村民库存中食物点数 >= 此值时视为食物过剩（hasExcessFood）。
     */
    static constexpr i32 EXCESS_FOOD_THRESHOLD = 24;

    /**
     * @brief 食物需求阈值
     *
     * 村民库存中食物点数 < 此值时视为需要食物（wantsMoreFood）。
     */
    static constexpr i32 WANTS_MORE_FOOD_THRESHOLD = 12;

    /**
     * @brief 计算村民库存中的食物点数
     *
     * 按照 foodPoints() 映射计算每种繁殖物品的数量 × 点数之和。
     *
     * @return 食物点数总和
     */
    [[nodiscard]] i32 countFoodPointsInInventory() const;

    /**
     * @brief 村民是否有过剩食物
     *
     * 当食物点数 >= EXCESS_FOOD_THRESHOLD (24) 时返回 true。
     */
    [[nodiscard]] bool hasExcessFood() const;

    /**
     * @brief 村民是否需要更多食物
     *
     * 当食物点数 < WANTS_MORE_FOOD_THRESHOLD (12) 时返回 true。
     */
    [[nodiscard]] bool wantsMoreFood() const;

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const;

    std::unique_ptr<AgeableEntity> createChild();

    // ========== 其他 ==========

    /**
     * @brief 是否为傻子村民
     */
    [[nodiscard]] bool isNitwit() const { return m_villagerData.profession() == VillagerProfession::Nitwit; }

    /**
     * @brief 是否可以工作
     */
    [[nodiscard]] bool canWork() const;

    /**
     * @brief 休息
     */
    void rest();

    /**
     * @brief 工作
     */
    void work();

    /**
     * @brief 玩耍
     */
    void play();

    /**
     * @brief 传播流言给另一个村民
     * @param other 目标村民
     *
     * 村民在聚集时会互相传播流言，影响玩家声誉。
     */
    void spreadGossipTo(VillagerEntity* other);

    /**
     * @brief 尝试传播流言
     *
     * 在 play() 中调用，检查是否应该传播流言。
     */
    void trySpreadGossip();

    // ========== 补货系统 ==========

    /**
     * @brief 检查村民是否应该补货
     *
     * 补货判定逻辑：
     * 1. 检测是否跨天（距上次补货超过12000tick 或 新的一天开始），若跨天则重置每日补货次数
     * 2. 判断是否允许补货（今日补货次数 < 2，且第二次补货需间隔2400tick）
     * 3. 判断是否需要补货（任意交易被使用过）
     * @return 是否应该补货
     */
    [[nodiscard]] bool shouldRestock();

    /**
     * @brief 执行补货
     *
     * 补货执行逻辑：
     * 1. 更新所有交易的需求值
     * 2. 重置所有交易的使用次数
     * 3. 向正在交易的玩家重新发送交易列表
     * 4. 记录补货时间
     * 5. 增加今日补货次数
     */
    void restock() override;

    /**
     * @brief 检查是否有交易需要补货
     * @return 任意交易被使用过则返回true
     */
    [[nodiscard]] bool needsToRestock() const;

    /**
     * @brief 检查是否允许补货
     *
     * 补货许可判定逻辑：
     * - 今日首次补货总是允许
     * - 第二次补货需要距离上次补货至少2400tick（2分钟）
     * - 每日最多补货2次
     */
    [[nodiscard]] bool allowedToRestock() const;

    /**
     * @brief 重置每日补货次数
     *
     * 同时补偿错过的补货需求更新（catchUpDemand）。
     * 如果昨日补货少于2次，对每次未补货执行额外的 resetUses + updateDemand。
     */
    void resetNumberOfRestocks();

    // ========== 睡眠 ==========

    /**
     * @brief 检查是否正在睡眠
     * @return 是否正在睡眠
     */
    [[nodiscard]] bool isSleeping() const;

    /**
     * @brief 获取睡眠位置
     * @return 睡眠位置（如果未在睡眠返回空）
     */
    [[nodiscard]] std::optional<BlockPos> getSleepingPosition() const { return m_sleepingPos; }

    /**
     * @brief 开始睡眠
     * @param pos 床位位置
     */
    void startSleeping(BlockPos pos);

    /**
     * @brief 停止睡眠
     */
    void stopSleeping();

    /**
     * @brief 检查是否在夜间时间
     * @return 是否是夜间
     */
    [[nodiscard]] bool isNightTime() const;

    /**
     * @brief 检查是否在工作时间
     * @return 是否是工作时间
     */
    [[nodiscard]] bool isWorkTime() const;

protected:
    void registerGoals() override;
    void registerAttributes() override;
    void updateOffers() override;

private:
    VillagerData m_villagerData;

    // 工作站点
    BlockPos m_workStation;
    bool m_atWorkstation = false;
    bool m_working = false;

    // 行为状态
    i32 m_workTime = 0;
    i32 m_numberOfRestocksToday = 0; // 今日补货次数（最多2次）
    i64 m_lastRestockGameTime = 0;   // 上次补货的游戏时间（tick）
    i64 m_lastRestockCheckDay = 0;   // 上次检查补货的游戏天数

    // 声音冷却
    i32 m_soundCooldown = 0;

    // 睡眠状态
    std::optional<BlockPos> m_sleepingPos;

    // 流言传播
    i64 m_lastGossipSpreadTime = 0; // 上次传播流言的游戏时间

    // 交易升级状态
    Player* m_lastTradedPlayer = nullptr;           // 最后交易的玩家（用于声望更新和粒子效果）
    i32 m_updateMerchantTimer = 0;                  // 交易升级计时器（40 ticks，仅在非交易状态递减）
    bool m_increaseProfessionLevelOnUpdate = false; // 计时器到期时是否升级并补充交易
    i32 m_prevLevelBeforeTrade = 0;                 // 交易前的等级（用于多级升级时生成中间等级交易）

    // Brain系统
    std::unique_ptr<VillagerBrain> m_brain;

    // POI释放守卫
    bool m_poisReleased = false; // 防止 die() 和 remove() 双重释放

    // ========== 私有辅助方法 ==========

    /**
     * @brief 尝试将村民感染转化为僵尸村民
     *
     * 对齐 MC Java 1.21.11 Zombie.killedEntity → Zombie.convertVillagerToZombieVillager。
     * 在 die() 最前面调用：检查伤害来源是否为僵尸系生物（ZombieEntity 子类，含 Husk/ZombieVillager，
     * 排除 ZombifiedPiglin），按难度感染概率（DifficultyHelper::getVillagerInfectionChance：
     * Easy/Peaceful 0%、Normal 50%、Hard 100%）决定是否转化。
     *
     * 转化流程（参照 ZombieVillagerEntity::finishConverting 反向范式 + ZombieEntity::convertToDrowned）：
     * 1. 创建 ZombieVillagerEntity（经 EntityRegistry 工厂，失败回退直接构造 + setTypeId）
     * 2. 复制位置、旋转
     * 3. 复制 VillagerData（职业/类型/等级/经验）——对齐 Java setVillagerData
     * 4. 复制婴儿状态（isChild → setBaby）
     * 5. 复制装备（逐槽，对齐 Java ConversionParams 保留装备语义）
     * 6. 复制自定义名称、持久化状态
     * 7. finalizeSpawn（SpawnReason::Conversion，按难度初始化属性）
     * 8. spawnEntity 生成到世界
     * 9. 播放感染音效 ENTITY_ZOMBIE_INFECT（对齐 Java levelEvent 1026）
     * 10. 清空原村民装备（防死亡掉落）+ remove() 移除原村民
     *
     * TODO: VillagerData 之外的 Gossips（流言）与 TradeOffers（交易列表）当前未实现完整序列化
     * （见 ZombieVillagerEntity.hpp 私有成员注释），故转化时暂不复制，待序列化就绪后补全
     * （对齐 Java convertVillagerToZombieVillager 的 setGossips/setTradeOffers）。
     *
     * @param cause 死亡伤害来源（用于提取攻击者）
     * @return true 表示已转化（调用方应跳过正常死亡流程），false 表示未转化（继续正常死亡）
     */
    bool _tryConvertToZombieVillager(DamageSource& cause);

    /**
     * @brief 处理交易声望更新和开心粒子效果
     *
     * 交易完成后更新村庄声望
     * 并播放开心村民粒子。每笔交易仅触发一次。
     */
    void _handleTradeReputation();

    /**
     * @brief 升级村民职业等级并补充新等级的交易
     *
     * 升级村民职业等级逻辑：
     * 1. 增加村民等级（VillagerData.level + 1）
     * 2. 为新等级生成交易并追加到现有交易列表
     *
     * 注意：此方法仅追加新等级的交易，不替换现有交易。
     */
    void _increaseMerchantCareer();

    /**
     * @brief 补偿错过的需求更新
     *
     * 需求补偿逻辑：
     * 如果昨日补货少于2次，对每次未补货执行 resetUses + updateDemand。
     * 这确保交易需求值在跨天后正确反映交易历史。
     */
    void _catchUpDemand();
};

/**
 * @brief 流浪商人实体
 *
 * 随机生成的商人，交易物品固定。
 */
class WanderingTraderEntity : public AbstractVillagerEntity {
public:
    using MobEntity::canDespawn;

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param registry 实体注册表（ECS）
     */
    WanderingTraderEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~WanderingTraderEntity() noexcept override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== 流浪商人特有 ==========

    /**
     * @brief 获取消失时间
     */
    [[nodiscard]] i32 despawnDelay() const { return m_despawnDelay; }

    /**
     * @brief 设置消失时间
     */
    void setDespawnDelay(i32 delay) { m_despawnDelay = delay; }

    /**
     * @brief 是否可以消失
     */
    [[nodiscard]] bool canDespawn() const { return m_despawnDelay <= 0; }

    /**
     * @brief 获取贸易羊驼数量
     */
    [[nodiscard]] i32 llamaCount() const { return m_llamaCount; }

    /**
     * @brief 设置贸易羊驼数量
     */
    void setLlamaCount(i32 count) { m_llamaCount = count; }

    /**
     * @brief 获取游荡目标位置
     */
    [[nodiscard]] BlockPos wanderTarget() const { return m_wanderTarget; }

    /**
     * @brief 设置游荡目标位置
     */
    void setWanderTarget(const BlockPos& pos) { m_wanderTarget = pos; }

    /**
     * @brief 生成贸易羊驼
     */
    void spawnLlamas();

    /**
     * @brief 交易经验奖励（流浪商人版本）
     *
     * 流浪商人没有升级系统，但会生成经验球给玩家。
     * 经验球值 = 3 + random(0~3)，即 3~6。
     */
    void rewardTradeXp(MerchantOffer& offer) override;

protected:
    void registerGoals() override;
    void registerAttributes() override;
    void updateOffers() override;

private:
    i32 m_despawnDelay = 0;   // 消失倒计时
    i32 m_llamaCount = 0;     // 贸易羊驼数量
    bool m_hasLlamas = false; // 是否已生成羊驼
    i32 m_tradeCount = 0;     // 交易次数
    BlockPos m_wanderTarget;  // 游荡目标位置
};

} // namespace entity
} // namespace mc
