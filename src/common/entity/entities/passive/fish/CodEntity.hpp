#pragma once

#include "AbstractGroupFishEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 鳕鱼实体
 *
 * 对齐 1.16.5 CodEntity。鳕鱼属于群游鱼类，沿用
 * AbstractGroupFishEntity 的默认群体大小语义。
 */
class CodEntity : public AbstractGroupFishEntity {
public:
    /**
     * @brief 构造鳕鱼实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    CodEntity(LegacyEntityType type, EntityId id);
    ~CodEntity() override = default;

    CodEntity(const CodEntity&) = delete;
    CodEntity& operator=(const CodEntity&) = delete;
    CodEntity(CodEntity&&) = default;
    CodEntity& operator=(CodEntity&&) = default;

    /**
     * @brief 创建鳕鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.15f; }

protected:
    void registerAttributes() override;
};

} // namespace mc
