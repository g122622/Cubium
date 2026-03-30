#pragma once

#include "../undead/ZombieEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

// Forward declarations
class VillagerData;

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
     * @brief 获取村民职业
     */
    [[nodiscard]] i32 getVillagerProfession() const { return m_profession; }

    /**
     * @brief 设置村民职业
     */
    void setVillagerProfession(i32 profession) { m_profession = profession; }

    /**
     * @brief 获取村民类型
     */
    [[nodiscard]] i32 getVillagerType() const { return m_villagerType; }

    /**
     * @brief 设置村民类型
     */
    void setVillagerType(i32 type) { m_villagerType = type; }

    /**
     * @brief 获取交易等级
     */
    [[nodiscard]] i32 getTradingLevel() const { return m_tradingLevel; }

    /**
     * @brief 设置交易等级
     */
    void setTradingLevel(i32 level) { m_tradingLevel = level; }

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
     */
    void startConverting();

    /**
     * @brief 取消治愈
     */
    void stopConverting();

    /**
     * @brief 完成治愈，变成村民
     */
    void finishConverting();

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 村民数据
    i32 m_profession = 0;       // 职业
    i32 m_villagerType = 0;     // 类型（平原、沙漠等）
    i32 m_tradingLevel = 1;     // 交易等级

    // 治愈状态
    bool m_converting = false;
    i32 m_conversionTime = 0;

    // 常量
    static constexpr i32 DEFAULT_CONVERSION_TIME = 3600;  // 3分钟（游戏时间）
};

} // namespace mc
