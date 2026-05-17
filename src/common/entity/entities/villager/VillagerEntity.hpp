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

#include "../../../world/block/BlockPos.hpp"
#include "../../ai/brain/Brain.hpp"
#include "../../ai/brain/memory/MemoryModuleType.hpp"
#include "../../ai/brain/schedule/Schedule.hpp"
#include "AbstractVillagerEntity.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace entity {

/**
 * @brief 村民实体
 *
 * 可交易的NPC村民，具有职业系统和繁殖能力。
 * 使用Brain系统进行高级AI控制。
 *
 * 参考 MC 1.16.5 VillagerEntity
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
    VillagerEntity(LegacyEntityType type, EntityId id);

    ~VillagerEntity() override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

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
     * 参考 MC 1.16.5 VillagerEntity.func_230293_i_()
     * 村民可拾取的物品：面包、土豆、胡萝卜、小麦、小麦种子、甜菜根、甜菜根种子
     * 农民职业额外可拾取：小麦、小麦种子、甜菜根种子、骨粉
     */
    [[nodiscard]] bool canPickUpItem(const ItemStack& itemStack) const;

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
     * 参考 MC 1.16.5 VillagerEntity.func_242368_a()
     * 村民在聚集时会互相传播流言，影响玩家声誉。
     */
    void spreadGossipTo(VillagerEntity* other);

    /**
     * @brief 尝试传播流言
     *
     * 在 play() 中调用，检查是否应该传播流言。
     */
    void trySpreadGossip();

    /**
     * @brief 补充交易物品
     */
    void restockTrades();

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
     *
     * 参考 MC 1.16.5 LivingEntity.startSleeping()
     */
    void startSleeping(BlockPos pos);

    /**
     * @brief 停止睡眠
     *
     * 参考 MC 1.16.5 LivingEntity.wakeUp()
     */
    void stopSleeping();

    /**
     * @brief 检查是否在夜间时间
     * @return 是否是夜间
     */
    [[nodiscard]] bool isNightTime() const;

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
    i32 m_lastRestock = 0;
    bool m_needsRestock = false;

    // 声音冷却
    i32 m_soundCooldown = 0;

    // 睡眠状态
    std::optional<BlockPos> m_sleepingPos;

    // 流言传播
    i64 m_lastGossipSpreadTime = 0;  // 上次传播流言的游戏时间

    // Brain系统
    std::unique_ptr<VillagerBrain> m_brain;
};

/**
 * @brief 流浪商人实体
 *
 * 随机生成的商人，交易物品固定。
 *
 * 参考 MC 1.16.5 WanderingTraderEntity
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
    WanderingTraderEntity(LegacyEntityType type, EntityId id);

    ~WanderingTraderEntity() override = default;

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
     * @brief 补充交易
     */
    void restockTrades();

    /**
     * @brief 生成贸易羊驼
     */
    void spawnLlamas();

protected:
    void registerGoals() override;
    void registerAttributes() override;
    void updateOffers() override;

private:
    i32 m_despawnDelay = 0;   // 消失倒计时
    i32 m_llamaCount = 0;     // 贸易羊驼数量
    bool m_hasLlamas = false; // 是否已生成羊驼
    i32 m_tradeCount = 0;     // 交易次数
};

} // namespace entity
} // namespace mc
