#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 巨人实体
 *
 * 非常巨大的敌对生物，只能通过命令生成。
 *
 * 特性：
 * - 巨大体型：高度近12格
 * - 高攻击力：高伤害攻击
 * - 高生命值：100点生命
 * - 无AI：没有智能行为
 * - 无生成：只能通过命令生成
 *
 * 参考 MC 1.16.5 GiantEntity
 */
class GiantEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    GiantEntity(LegacyEntityType type, EntityId id);
    ~GiantEntity() override = default;

    // 禁止拷贝
    GiantEntity(const GiantEntity&) = delete;
    GiantEntity& operator=(const GiantEntity&) = delete;

    // 允许移动
    GiantEntity(GiantEntity&&) = default;
    GiantEntity& operator=(GiantEntity&&) = default;

    /**
     * @brief 创建巨人实体
     * @param world 世界实例
     * @return 新的巨人实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 10.44f; }

    /**
     * @brief 巨人不会燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

} // namespace mc
