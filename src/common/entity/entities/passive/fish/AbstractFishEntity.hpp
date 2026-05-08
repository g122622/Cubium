#pragma once

#include "../water/WaterMobEntity.hpp"

namespace mc {

/**
 * @brief 鱼类实体基类
 *
 * 对齐 1.16.5 的 AbstractFishEntity，只保留所有鱼共享的游泳、
 * 离水扑腾与基础空气供应语义。群游逻辑由 AbstractGroupFishEntity 承载。
 */
class AbstractFishEntity : public WaterMobEntity {
public:
    /**
     * @brief 构造鱼类实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    AbstractFishEntity(LegacyEntityType type, EntityId id);
    ~AbstractFishEntity() override = default;

    AbstractFishEntity(const AbstractFishEntity&) = delete;
    AbstractFishEntity& operator=(const AbstractFishEntity&) = delete;
    AbstractFishEntity(AbstractFishEntity&&) = default;
    AbstractFishEntity& operator=(AbstractFishEntity&&) = default;

    /**
     * @brief 鱼默认只能在水中生成
     */
    [[nodiscard]] bool canSpawnInWater() const override { return true; }

    /**
     * @brief 默认鱼类不具备群游语义
     */
    [[nodiscard]] virtual bool canSchool() const { return false; }

    /**
     * @brief 当前是否处于游泳状态
     */
    [[nodiscard]] bool isSwimming() const { return m_swimming; }

    /**
     * @brief 设置游泳状态
     */
    void setSwimming(bool swimming) { m_swimming = swimming; }

    /**
     * @brief 当前游泳朝向
     */
    [[nodiscard]] f32 getSwimAngle() const { return m_swimAngle; }

    /**
     * @brief 设置游泳朝向
     */
    void setSwimAngle(f32 angle) { m_swimAngle = angle; }

    /**
     * @brief 当前是否处于离水扑腾状态
     */
    [[nodiscard]] bool isFlopping() const { return m_flopping; }

    /**
     * @brief 设置离水扑腾状态
     */
    void setFlopping(bool flopping) { m_flopping = flopping; }

    // ========== 桶装鱼支持 ==========

    /**
     * @brief 检查是否来自桶
     *
     * 从桶放出的鱼不会消失。
     * 参考 MC 1.16.5 AbstractFishEntity.isFromBucket()
     *
     * @return 如果是从桶放出的鱼返回 true
     */
    [[nodiscard]] bool isFromBucket() const { return m_fromBucket; }

    /**
     * @brief 设置是否来自桶
     *
     * 当从鱼桶放出鱼时调用此方法设置为 true。
     * 参考 MC 1.16.5 AbstractFishEntity.setFromBucket()
     *
     * @param fromBucket 是否来自桶
     */
    void setFromBucket(bool fromBucket) { m_fromBucket = fromBucket; }

    /**
     * @brief 检查是否应阻止消失
     *
     * 从桶放出的鱼永远不会消失。
     * 参考 MC 1.16.5 AbstractFishEntity.preventDespawn()
     *
     * @return 如果来自桶或正在被骑乘返回 true
     */
    [[nodiscard]] bool preventDespawn() const override {
        return WaterMobEntity::preventDespawn() || m_fromBucket;
    }

    /**
     * @brief 检查是否可以消失
     *
     * 从桶放出的鱼或有自定义名称的鱼不会消失。
     * 参考 MC 1.16.5 AbstractFishEntity.canDespawn()
     *
     * @param distanceToClosestPlayer 到最近玩家的距离
     * @return 如果可以消失返回 true
     */
    [[nodiscard]] bool canDespawn(double distanceToClosestPlayer) const override {
        (void)distanceToClosestPlayer;
        return !m_fromBucket && !hasCustomName();
    }

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 更新游泳状态
     */
    void updateSwimming();

    /**
     * @brief 更新离水扑腾状态
     */
    void updateFlopping();

private:
    bool m_swimming = false;
    f32 m_swimAngle = 0.0f;
    bool m_flopping = false;
    i32 m_flopTimer = 0;
    bool m_fromBucket = false;  // 是否来自桶（从桶放出的鱼不会消失）

    static constexpr i32 MAX_AIR_SUPPLY = 480;
};

} // namespace mc
