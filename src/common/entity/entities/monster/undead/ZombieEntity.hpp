#pragma once

#include "../MonsterEntity.hpp"
#include "../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 僵尸实体
 *
 * 最常见的亡灵怪物。
 *
 * 特性：
 * - 攻击：近战攻击玩家和村民
 * - 燃烧：在阳光下燃烧
 * - 增援：被攻击时有概率召唤增援
 * - 感染：杀死村民会将其转化为僵尸村民
 * - 变种：可转化为溺尸
 *
 * 参考 MC 1.16.5 ZombieEntity
 */
class ZombieEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    ZombieEntity(LegacyEntityType type, EntityId id);
    ~ZombieEntity() override = default;

    // 禁止拷贝
    ZombieEntity(const ZombieEntity&) = delete;
    ZombieEntity& operator=(const ZombieEntity&) = delete;

    // 允许移动
    ZombieEntity(ZombieEntity&&) = default;
    ZombieEntity& operator=(ZombieEntity&&) = default;

    /**
     * @brief 创建僵尸实体
     * @param world 世界实例
     * @return 新的僵尸实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 增援系统 ==========

    /**
     * @brief 是否可以召唤增援
     */
    [[nodiscard]] bool canSummonReinforcements() const { return m_canSummonReinforcements; }

    /**
     * @brief 设置是否可以召唤增援
     */
    void setCanSummonReinforcements(bool canSummon) { m_canSummonReinforcements = canSummon; }

    /**
     * @brief 是否正在召唤增援
     */
    [[nodiscard]] bool isSummoningReinforcements() const { return m_summoningReinforcements; }

    /**
     * @brief 尝试召唤增援
     */
    void trySummonReinforcements();

    // ========== 转化系统 ==========

    /**
     * @brief 是否正在转化为溺尸
     */
    [[nodiscard]] bool isConverting() const { return m_converting; }

    /**
     * @brief 获取转化时间
     */
    [[nodiscard]] i32 getConversionTime() const { return m_conversionTime; }

    /**
     * @brief 设置转化时间
     */
    void setConversionTime(i32 time) { m_conversionTime = time; }

    // ========== 婴儿状态 ==========

    /**
     * @brief 是否是婴儿僵尸
     */
    [[nodiscard]] bool isBaby() const { return m_isBaby; }

    /**
     * @brief 设置婴儿状态
     */
    void setBaby(bool baby) { m_isBaby = baby; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return m_isBaby ? 0.93f : 1.74f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 增援系统
    bool m_canSummonReinforcements = false;
    bool m_summoningReinforcements = false;

    // 转化系统
    bool m_converting = false;
    i32 m_conversionTime = 0;

    // 婴儿状态
    bool m_isBaby = false;

    // 常量
    static constexpr i32 REINFORCEMENT_CHANCE = 100; // 1/100 概率召唤增援
    static constexpr i32 CONVERSION_DURATION = 300;   // 15秒转化时间
};

} // namespace mc
