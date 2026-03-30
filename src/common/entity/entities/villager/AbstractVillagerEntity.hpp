#pragma once

#include "../../core/AgeableEntity.hpp"
#include "../../inventory/Inventory.hpp"
#include <memory>

namespace mc {

// 前向声明
class Player;
class ItemStack;
class MerchantOffer;
class MerchantOffers;

namespace entity {

/**
 * @brief 村民职业枚举
 *
 * 参考 MC 1.16.5 VillagerProfession
 */
enum class VillagerProfession : u8 {
    None = 0,       // 无职业（傻子）
    Armorer,        // 盔甲匠
    Butcher,        // 屠夫
    Cartographer,   // 制图师
    Cleric,         // 牧师
    Farmer,         // 农民
    Fisherman,      // 渔夫
    Fletcher,       // 制箭师
    Leatherworker,  // 皮革匠
    Librarian,      // 图书管理员
    Mason,          // 石匠
    Nitwit,         // 傻子
    Shepherd,       // 牧羊人
    Toolsmith,      // 工具匠
    Weaponsmith     // 武器匠
};

/**
 * @brief 村民类型枚举（外观）
 *
 * 参考 MC 1.16.5 VillagerType
 */
enum class VillagerType : u8 {
    Desert = 0,     // 沙漠
    Jungle,         // 丛林
    Plains,         // 平原
    Savanna,        // 热带草原
    Snow,           // 雪
    Swamp,          // 沼泽
    Taiga           // 针叶林
};

/**
 * @brief 村民数据
 *
 * 存储村民的职业、类型、等级等信息。
 *
 * 参考 MC 1.16.5 VillagerData
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
 *
 * 参考 MC 1.16.5 AbstractVillagerEntity
 */
class AbstractVillagerEntity : public AgeableEntity {
public:
    AbstractVillagerEntity(LegacyEntityType type, EntityId id);
    ~AbstractVillagerEntity() override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== 交易系统 ==========

    /**
     * @brief 获取交易列表
     */
    [[nodiscard]] MerchantOffers* getOffers() { return m_offers.get(); }
    [[nodiscard]] const MerchantOffers* getOffers() const { return m_offers.get(); }

    /**
     * @brief 设置交易列表
     */
    void setOffers(std::unique_ptr<MerchantOffers> offers);

    /**
     * @brief 是否有交易
     */
    [[nodiscard]] bool hasOffers() const { return m_offers != nullptr; }

    /**
     * @brief 更新交易列表
     */
    virtual void updateOffers();

    /**
     * @brief 与玩家开始交易
     * @param player 玩家
     */
    virtual void startTrading(Player* player);

    /**
     * @brief 结束交易
     */
    virtual void stopTrading();

    /**
     * @brief 获取当前交易对象
     */
    [[nodiscard]] Player* tradingPlayer() const { return m_tradingPlayer; }

    /**
     * @brief 是否正在交易
     */
    [[nodiscard]] bool isTrading() const { return m_tradingPlayer != nullptr; }

    // ========== 经验和等级 ==========

    /**
     * @brief 获取经验值
     */
    [[nodiscard]] i32 experience() const { return m_experience; }

    /**
     * @brief 设置经验值
     */
    void setExperience(i32 exp) { m_experience = exp; }

    /**
     * @brief 增加经验值
     */
    void addExperience(i32 amount);

    /**
     * @brief 获取经验条填充度（0-1）
     */
    [[nodiscard]] f32 experienceProgress() const;

    // ========== 库存 ==========

    /**
     * @brief 获取库存
     */
    [[nodiscard]] Inventory& inventory() { return *m_inventory; }
    [[nodiscard]] const Inventory& inventory() const { return *m_inventory; }

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

protected:
    // 交易列表
    std::unique_ptr<MerchantOffers> m_offers;

    // 当前交易玩家
    Player* m_tradingPlayer = nullptr;

    // 经验值
    i32 m_experience = 0;

    // 库存
    std::unique_ptr<Inventory> m_inventory;

    // 繁殖意愿
    bool m_willingToBreed = false;

    // 交易次数（用于升级）
    i32 m_tradesMade = 0;
};

} // namespace entity
} // namespace mc
