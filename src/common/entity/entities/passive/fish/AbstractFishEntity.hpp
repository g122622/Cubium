#pragma once

#include "../water/WaterMobEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 鱼类实体基类
 *
 * 所有鱼类（鳕鱼、鲑鱼、河豚、热带鱼）的基类。
 *
 * 特性：
 * - 游泳：在水中游动
 * - 群游：多个个体会聚在一起
 * - 离水死亡：离开水会逐渐死亡
 * - 被捕捉：可以用桶捕捉
 * - 掉落：死后掉落物品
 *
 * 参考 MC 1.16.5 AbstractFishEntity / AbstractGroupFishEntity
 */
class AbstractFishEntity : public WaterMobEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AbstractFishEntity(LegacyEntityType type, EntityId id);
    ~AbstractFishEntity() override = default;

    // 禁止拷贝
    AbstractFishEntity(const AbstractFishEntity&) = delete;
    AbstractFishEntity& operator=(const AbstractFishEntity&) = delete;

    // 允许移动
    AbstractFishEntity(AbstractFishEntity&&) = default;
    AbstractFishEntity& operator=(AbstractFishEntity&&) = default;

    // ========== 游泳行为 ==========

    /**
     * @brief 是否可以生成
     */
    [[nodiscard]] bool canSpawnInWater() const override { return true; }

    /**
     * @brief 是否在游泳
     */
    [[nodiscard]] bool isSwimming() const { return m_swimming; }

    /**
     * @brief 设置游泳状态
     */
    void setSwimming(bool swimming) { m_swimming = swimming; }

    /**
     * @brief 获取游泳方向
     */
    [[nodiscard]] f32 getSwimAngle() const { return m_swimAngle; }

    /**
     * @brief 设置游泳方向
     */
    void setSwimAngle(f32 angle) { m_swimAngle = angle; }

    // ========== 群居行为 ==========

    /**
     * @brief 是否会群游
     */
    [[nodiscard]] virtual bool canSchool() const { return false; }

    /**
     * @brief 获取群居范围
     */
    [[nodiscard]] f32 getSchoolingRange() const { return m_schoolingRange; }

    /**
     * @brief 设置群居范围
     */
    void setSchoolingRange(f32 range) { m_schoolingRange = range; }

    /**
     * @brief 获取群居成员数量
     */
    [[nodiscard]] i32 getMaxGroupSize() const { return m_maxGroupSize; }

    // ========== 离水行为 ==========

    /**
     * @brief 是否在扑腾（离开水）
     */
    [[nodiscard]] bool isFlopping() const { return m_flopping; }

    /**
     * @brief 设置扑腾状态
     */
    void setFlopping(bool flopping) { m_flopping = flopping; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 行为更新 ==========
    void updateSwimming();
    void updateFlopping();

private:
    // 游泳状态
    bool m_swimming = false;
    f32 m_swimAngle = 0.0f;

    // 群居状态
    f32 m_schoolingRange = 5.0f;
    i32 m_maxGroupSize = 5;

    // 扑腾状态
    bool m_flopping = false;
    i32 m_flopTimer = 0;
    static constexpr i32 MAX_AIR_SUPPLY = 480; // 24秒
};

} // namespace mc
