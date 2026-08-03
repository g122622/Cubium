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
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    VillagerEntity(EntityInstanceId id);

    ~VillagerEntity() noexcept override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    /**
     * @brief 重写死亡回调
     *
     * 村民死亡时释放占用的POI（床位、工作站、聚集点），
     * 并通知村庄管理器该村民已离开村庄。
     * 参考 MC Java Villager.die() -> releaseAllPois()
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
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    WanderingTraderEntity(EntityInstanceId id);

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
