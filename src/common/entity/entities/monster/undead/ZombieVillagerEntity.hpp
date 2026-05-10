#pragma once

#include "../undead/ZombieEntity.hpp"
#include "../../villager/VillagerEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 僵尸村民实体
 *
 * 被僵尸感染的村民，可以通过虚弱药水和金苹果治愈。
 *
 * 特性：
 * - 感染：村民被僵尸攻击后可能变成僵尸村民
 * - 治愈：使用虚弱药水+金苹果可治愈为村民
 * - 职业：保留村民的职业信息
 * - 等级：保留村民的交易等级
 * - 幼体：可能是小僵尸村民
 *
 * 治愈加速：
 * - 铁栏杆和床会加速治愈过程
 * - 力量效果会加速治愈（每级减少 10% 时间）
 * - 基础治愈时间：3600 ticks（3分钟）
 *
 * 参考 MC 1.16.5 ZombieVillagerEntity
 */
class ZombieVillagerEntity : public ZombieEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    ZombieVillagerEntity(LegacyEntityType type, EntityId id);
    ~ZombieVillagerEntity() override = default;

    // 禁止拷贝
    ZombieVillagerEntity(const ZombieVillagerEntity&) = delete;
    ZombieVillagerEntity& operator=(const ZombieVillagerEntity&) = delete;

    // 允许移动
    ZombieVillagerEntity(ZombieVillagerEntity&&) = default;
    ZombieVillagerEntity& operator=(ZombieVillagerEntity&&) = default;

    /**
     * @brief 创建僵尸村民实体
     * @param world 世界实例
     * @return 新的僵尸村民实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 村民数据 ==========

    /**
     * @brief 获取村民数据
     */
    [[nodiscard]] const entity::VillagerData& villagerData() const { return m_villagerData; }

    /**
     * @brief 设置村民数据
     */
    void setVillagerData(const entity::VillagerData& data) { m_villagerData = data; }

    /**
     * @brief 获取村民职业
     */
    [[nodiscard]] entity::VillagerProfession getProfession() const {
        return m_villagerData.profession();
    }

    /**
     * @brief 设置村民职业
     */
    void setProfession(entity::VillagerProfession profession) {
        m_villagerData.setProfession(profession);
    }

    /**
     * @brief 获取村民类型
     */
    [[nodiscard]] entity::VillagerType getVillagerType() const {
        return m_villagerData.type();
    }

    /**
     * @brief 设置村民类型
     */
    void setVillagerType(entity::VillagerType type) {
        m_villagerData.setType(type);
    }

    /**
     * @brief 获取交易等级
     */
    [[nodiscard]] i32 getTradingLevel() const { return m_villagerData.level(); }

    /**
     * @brief 设置交易等级
     */
    void setTradingLevel(i32 level) { m_villagerData.setLevel(level); }

    /**
     * @brief 获取交易经验
     */
    [[nodiscard]] i32 getTradingExperience() const { return m_villagerData.experience(); }

    /**
     * @brief 设置交易经验
     */
    void setTradingExperience(i32 exp) { m_villagerData.setExperience(exp); }

    // ========== 治愈系统 ==========

    /**
     * @brief 是否正在治愈
     */
    [[nodiscard]] bool isConverting() const { return m_converting; }

    /**
     * @brief 获取治愈时间
     * @return 剩余治愈时间（ticks），0表示未在治愈
     */
    [[nodiscard]] i32 getConversionTime() const { return m_conversionTime; }

    /**
     * @brief 设置治愈时间
     * @param time 治愈时间（ticks）
     */
    void setConversionTime(i32 time);

    /**
     * @brief 开始治愈过程
     * @param starterUuid 发起治愈的玩家UUID（可能为空）
     * @param time 治愈时间（ticks），如果为-1则使用随机时间
     */
    void startConverting(const std::string& starterUuid, i32 time = -1);

    /**
     * @brief 取消治愈
     */
    void stopConverting();

    /**
     * @brief 完成治愈，变成村民
     */
    void finishConverting();

    /**
     * @brief 获取治愈进度（每tick减少的治愈时间）
     *
     * 铁栏杆和床会加速治愈：
     * - 在 4x4x4 范围内，每个铁栏杆或床有 30% 概率增加治愈进度
     * - 基础进度为 1
     *
     * @return 治愈进度（每tick减少的时间）
     */
    [[nodiscard]] i32 getConversionProgress() const;

    /**
     * @brief 获取治愈发起者的UUID
     */
    [[nodiscard]] const std::string& getConversionStarterUuid() const {
        return m_conversionStarterUuid;
    }

    /**
     * @brief 设置治愈发起者的UUID
     */
    void setConversionStarterUuid(const std::string& uuid) {
        m_conversionStarterUuid = uuid;
    }

    // ========== 交互 ==========

    /**
     * @brief 玩家交互
     *
     * 玩家使用金苹果对虚弱状态的僵尸村民右键时开始治愈。
     *
     * @param player 玩家
     * @param hand 手（主手/副手）
     * @return 交互结果
     */
    // ActionResultType interact(Player& player, Hand hand) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override;

    // ========== 生命周期 ==========

    void tick() override;

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

    /**
     * @brief 获取脚步声
     */
    [[nodiscard]] std::optional<ResourceLocation> getStepSound() const;

    // ========== 生成控制 ==========

    /**
     * @brief 是否可以自然消失
     *
     * 正在治愈的僵尸村民不能消失。
     * 有经验的僵尸村民不能消失（交易过的村民被感染）。
     */
    [[nodiscard]] bool canDespawn(f64 distanceToClosestPlayer) const override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    // ========== 溺水转化 ==========

    /**
     * @brief 僵尸村民不应该溺水转化
     */
    [[nodiscard]] bool shouldDrown() const override { return false; }

private:
    // 村民数据
    entity::VillagerData m_villagerData;

    // 交易数据（NBT 存储）
    // 注意：当前未实现完整的 MerchantOffers 序列化
    // std::unique_ptr<MerchantOffers> m_offers;

    // 流言数据（NBT 存储）
    // 注意：当前未实现完整的 Gossips 序列化
    // nbt::CompoundTag m_gossips;

    // 治愈状态
    bool m_converting = false;
    i32 m_conversionTime = 0;
    std::string m_conversionStarterUuid;  // 发起治愈的玩家UUID

    // 常量
    static constexpr i32 DEFAULT_CONVERSION_TIME = 3600;  // 3分钟（游戏时间）
    static constexpr i32 MIN_CONVERSION_TIME = 1;         // 最小治愈时间
    static constexpr i32 MAX_CONVERSION_TIME = 6000;      // 最大治愈时间（5分钟）
    static constexpr f32 CONVERSION_SPEEDUP_CHANCE = 0.3f; // 铁栏杆/床加速概率
    static constexpr i32 CONVERSION_SPEEDUP_RANGE = 4;     // 检测范围 4x4x4
};

} // namespace mc
