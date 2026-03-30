#pragma once

#include "SkeletonEntity.hpp"
#include "../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 流浪者实体
 *
 * 在雪地生物群系生成的骷髅变种。
 *
 * 特性：
 * - 雪地生成：在冰原、冻洋等生物群系生成
 * - 缓慢箭：射出的箭会给予缓慢效果
 * - 不燃烧：在阳光下不燃烧
 *
 * 参考 MC 1.16.5 StrayEntity
 */
class StrayEntity : public SkeletonEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    StrayEntity(LegacyEntityType type, EntityId id);

    ~StrayEntity() override = default;

    // 禁止拷贝
    StrayEntity(const StrayEntity&) = delete;
    StrayEntity& operator=(const StrayEntity&) = delete;

    // 允许移动
    StrayEntity(StrayEntity&&) = default;
    StrayEntity& operator=(StrayEntity&&) = default;

    /**
     * @brief 创建流浪者实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 阳光燃烧 ==========

    /**
     * @brief 流浪者不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

protected:
    void registerAttributes() override;
};

} // namespace mc
