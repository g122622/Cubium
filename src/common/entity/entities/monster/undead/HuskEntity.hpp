#pragma once

#include "ZombieEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 尸壳实体
 *
 * 在沙漠中生成的僵尸变种。
 *
 * 特性：
 * - 沙漠生成：在沙漠生物群系生成
 * - 不燃烧：在阳光下不燃烧
 * - 饥饿攻击：攻击会使玩家饥饿
 *
 * 参考 MC 1.16.5 HuskEntity
 */
class HuskEntity : public ZombieEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    HuskEntity(LegacyEntityType type, EntityId id);

    ~HuskEntity() override = default;

    // 禁止拷贝
    HuskEntity(const HuskEntity&) = delete;
    HuskEntity& operator=(const HuskEntity&) = delete;

    // 允许移动
    HuskEntity(HuskEntity&&) = default;
    HuskEntity& operator=(HuskEntity&&) = default;

    /**
     * @brief 创建尸壳实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 阳光燃烧 ==========

    /**
     * @brief 尸壳不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

protected:
    void registerAttributes() override;
};

} // namespace mc
