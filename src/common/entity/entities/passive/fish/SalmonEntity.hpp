#pragma once

#include "AbstractGroupFishEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 鲑鱼实体
 *
 * 对齐 1.16.5 SalmonEntity。鲑鱼属于群游鱼类，但最大群体大小
 * 与鳕鱼不同，vanilla 固定为 5。
 */
class SalmonEntity : public AbstractGroupFishEntity {
public:
    /**
     * @brief 构造鲑鱼实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    SalmonEntity(LegacyEntityType type, EntityId id);
    ~SalmonEntity() override = default;

    SalmonEntity(const SalmonEntity&) = delete;
    SalmonEntity& operator=(const SalmonEntity&) = delete;
    SalmonEntity(SalmonEntity&&) = default;
    SalmonEntity& operator=(SalmonEntity&&) = default;

    /**
     * @brief 创建鲑鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief vanilla 鲑鱼最大群体大小为 5
     */
    [[nodiscard]] i32 getMaxGroupSize() const override { return 5; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.1f; }

protected:
    void registerAttributes() override;
};

} // namespace mc
