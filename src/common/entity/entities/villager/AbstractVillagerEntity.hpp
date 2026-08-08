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
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/INamedContainerProvider.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class Player;
class ItemStack;

// 使用命名空间中的类型
using MerchantOffers = world::village::trade::MerchantOffers;
using MerchantOffer = world::village::trade::MerchantOffer;

namespace entity {

/**
 * @brief 村民职业枚举
 */
enum class VillagerProfession : u8 {
    None = 0,      // 无职业（傻子）
    Armorer,       // 盔甲匠
    Butcher,       // 屠夫
    Cartographer,  // 制图师
    Cleric,        // 牧师
    Farmer,        // 农民
    Fisherman,     // 渔夫
    Fletcher,      // 制箭师
    Leatherworker, // 皮革匠
    Librarian,     // 图书管理员
    Mason,         // 石匠
    Nitwit,        // 傻子
    Shepherd,      // 牧羊人
    Toolsmith,     // 工具匠
    Weaponsmith    // 武器匠
};

/**
 * @brief 村民类型枚举（外观）
 */
enum class VillagerType : u8 {
    Desert = 0, // 沙漠
    Jungle,     // 丛林
    Plains,     // 平原
    Savanna,    // 热带草原
    Snow,       // 雪
    Swamp,      // 沼泽
    Taiga       // 针叶林
};

/**
 * @brief 村民数据
 *
 * 存储村民的职业、类型、等级等信息。
 */
class VillagerData {
public:
    VillagerData() = default;
    VillagerData(VillagerType type, VillagerProfession profession, i32 level);

    // ========== 类型 ==========

    [[nodiscard]] VillagerType type() const { return m_type; }
    void setType(VillagerType type) { m_type = type; }

    // ========== 职业 ==========

    [[nodiscard]] VillagerProfession profession() const { return m_profession; }
    void setProfession(VillagerProfession profession);

    // ========== 等级 ==========

    [[nodiscard]] i32 level() const { return m_level; }
    void setLevel(i32 level);

    /**
     * @brief 增加经验值
     * @param amount 经验值数量
     */
    void addExperience(i32 amount);

    /**
     * @brief 获取当前经验值
     */
    [[nodiscard]] i32 experience() const { return m_experience; }

    /**
     * @brief 设置经验值
     */
    void setExperience(i32 exp) { m_experience = exp; }

    // ========== 等级上限 ==========

    /**
     * @brief 获取升级所需经验
     * @param level 当前等级
     * @return 升到下一级所需经验
     */
    [[nodiscard]] static i32 getExperienceForLevel(i32 level);

    /**
     * @brief 获取等级上限
     */
    [[nodiscard]] static constexpr i32 getMaxLevel() { return 5; }

private:
    VillagerType m_type = VillagerType::Plains;
    VillagerProfession m_profession = VillagerProfession::None;
    i32 m_level = 1;
    i32 m_experience = 0;
};

/**
 * @brief 抽象村民实体基类
 *
 * 所有可交易NPC的基类（村民、流浪商人）。
 * 实现 INamedContainerProvider 接口，支持旁观者模式玩家与村民交互。
 */
class AbstractVillagerEntity : public AgeableEntity,
                               public INamedContainerProvider,
                               public world::village::trade::IMerchant {
public:
    AbstractVillagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~AbstractVillagerEntity() override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== INamedContainerProvider 接口实现 ==========

    /**
     * @brief 创建交易菜单
     *
     * 当玩家与村民交互时调用，创建村民交易界面的容器菜单。
     *
     * @param containerId 容器ID（由服务端分配）
     * @param player 打开容器的玩家
     * @return 创建的容器菜单
     */
    [[nodiscard]] std::unique_ptr<AbstractContainerMenu> createMenu(i32 containerId, Player& player) override;

    /**
     * @brief 获取显示名称
     *
     * 返回村民在交易界面标题栏显示的名称。
     * 可以是自定义名称或默认翻译键。
     *
     * @return 显示名称
     */
    [[nodiscard]] std::string getDisplayName() const override;

    // ========== 交易系统 ==========

    /**
     * @brief 是否有交易
     */
    [[nodiscard]] bool hasOffers() const override { return m_offers != nullptr; }

    /**
     * @brief 更新交易列表
     */
    virtual void updateOffers();

    /**
     * @brief 获取交易等级（子类实现）
     *
     * VillagerEntity 返回 villagerData.level()
     * WanderingTraderEntity 返回 0（流浪商人没有等级系统）
     */
    [[nodiscard]] virtual i32 getTradingLevel() const { return 0; }

    /**
     * @brief 获取经验条填充度（0-1）
     */
    [[nodiscard]] f32 experienceProgress() const;

    // ========== 库存 ==========

    /**
     * @brief 获取库存
     */
    [[nodiscard]] IInventory& inventory() { return *m_inventory; }
    [[nodiscard]] const IInventory& inventory() const { return *m_inventory; }

    // ========== 其他 ==========

    /**
     * @brief 是否愿意繁殖
     */
    [[nodiscard]] bool isWillingToBreed() const { return m_willingToBreed; }

    /**
     * @brief 设置繁殖意愿
     */
    void setWillingToBreed(bool willing) { m_willingToBreed = willing; }

    /**
     * @brief 重置繁殖意愿
     */
    void resetBreedWillingness();

    // ========== IMerchant 接口实现 ==========

    /**
     * @brief 获取交易列表（IMerchant 接口）
     */
    [[nodiscard]] MerchantOffers& getOffers() override;
    [[nodiscard]] const MerchantOffers& getOffers() const override;

    /**
     * @brief 设置交易列表（IMerchant 接口）
     */
    void setOffers(MerchantOffers offers) override;

    /**
     * @brief 获取当前交易对象（IMerchant 接口）
     */
    [[nodiscard]] Player* getTradingPlayer() const override { return m_tradingPlayer; }

    /**
     * @brief 开始交易（IMerchant 接口）
     */
    void startTrading(Player* player) override;

    /**
     * @brief 结束交易（IMerchant 接口）
     */
    void stopTrading() override;

    /**
     * @brief 是否正在交易（IMerchant 接口）
     */
    [[nodiscard]] bool isTrading() const override { return m_tradingPlayer != nullptr; }

    /**
     * @brief 获取经验值（IMerchant 接口）
     */
    [[nodiscard]] i32 getExperience() const override { return m_experience; }

    /**
     * @brief 设置经验值（IMerchant 接口）
     */
    void setExperience(i32 exp) override { m_experience = exp; }

    /**
     * @brief 增加经验值（IMerchant 接口）
     */
    void addExperience(i32 amount) override;

    /**
     * @brief 补货（IMerchant 接口）
     */
    void restock() override;

    /**
     * @brief 通知交易执行（IMerchant 接口）
     */
    void notifyTrade(MerchantOffer& offer) override;

    /**
     * @brief 通知交易更新（IMerchant 接口）
     */
    void notifyTradeUpdated(const ItemStack& resultStack) override;

    /**
     * @brief 覆盖交易列表（IMerchant 接口，用于客户端同步）
     */
    void overrideOffers(MerchantOffers offers) override;

    /**
     * @brief 获取村民经验值（IMerchant 接口）
     */
    [[nodiscard]] i32 getVillagerXp() const override { return m_experience; }

    /**
     * @brief 覆盖经验值（IMerchant 接口）
     */
    void overrideXp(i32 xp) override { m_experience = xp; }

    /**
     * @brief 是否显示经验进度条（IMerchant 接口）
     */
    [[nodiscard]] bool showProgressBar() const override { return true; }

    /**
     * @brief 是否可以补货（IMerchant 接口）
     */
    [[nodiscard]] bool canRestock() const override { return true; }

    /**
     * @brief 是否在客户端（IMerchant 接口）
     */
    [[nodiscard]] bool isClientSide() const override;

    /**
     * @brief 检查商人是否仍可被指定玩家交易（IMerchant 接口）
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 获取商人对应的实体（IMerchant 接口）
     */
    [[nodiscard]] Entity* asEntity() override { return this; }
    [[nodiscard]] const Entity* asEntity() const override { return this; }

    /**
     * @brief 通知交易执行后的经验奖励（子类实现）
     * @param offer 已执行的交易
     */
    virtual void rewardTradeXp(MerchantOffer& offer) = 0;

    /**
     * @brief 播放交易音效（确认/否定）
     * @param success 是否交易成功（true=确认音效，false=否定音效）
     */
    void playTradeSound(bool success);

protected:
    // 交易列表
    std::unique_ptr<MerchantOffers> m_offers;

    // 当前交易玩家
    Player* m_tradingPlayer = nullptr;

    // 经验值
    i32 m_experience = 0;

    // 库存
    std::unique_ptr<IInventory> m_inventory;

    // 繁殖意愿
    bool m_willingToBreed = false;

    // 交易次数（用于升级）
    i32 m_tradesMade = 0;
};

} // namespace entity
} // namespace mc
