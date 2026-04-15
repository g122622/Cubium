#pragma once

#include "AbstractGroupFishEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 热带鱼实体
 *
 * 对齐 1.16.5 TropicalFishEntity。当前先保留变种编码和群游层次，
 * 预定义花纹、桶数据和更完整的刷怪语义留到后续任务继续补。
 */
class TropicalFishEntity : public AbstractGroupFishEntity {
public:
    /**
     * @brief 热带鱼形状
     */
    enum class FishShape : u8 {
        Kob = 0,
        SunStreak = 1,
        Snooper = 2,
        Dasher = 3,
        Brinely = 4,
        Spotty = 5,
        Flopper = 6,
        Stripey = 7,
        Glitter = 8,
        Blockfish = 9,
        Betty = 10,
        Clayfish = 11
    };

    /**
     * @brief 构造热带鱼实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    TropicalFishEntity(LegacyEntityType type, EntityId id);
    ~TropicalFishEntity() override = default;

    TropicalFishEntity(const TropicalFishEntity&) = delete;
    TropicalFishEntity& operator=(const TropicalFishEntity&) = delete;
    TropicalFishEntity(TropicalFishEntity&&) = default;
    TropicalFishEntity& operator=(TropicalFishEntity&&) = default;

    /**
     * @brief 创建热带鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 获取变种 ID
     */
    [[nodiscard]] i32 getVariant() const { return m_variant; }

    /**
     * @brief 设置变种 ID
     */
    void setVariant(i32 variant) { m_variant = variant; }

    /**
     * @brief 获取鱼体形状
     */
    [[nodiscard]] FishShape getShape() const;

    /**
     * @brief 获取主色
     */
    [[nodiscard]] u8 getBaseColor() const;

    /**
     * @brief 获取花纹色
     */
    [[nodiscard]] u8 getPatternColor() const;

    /**
     * @brief 随机生成一个变种
     */
    void randomizeVariant();

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.1f; }

protected:
    void registerAttributes() override;

private:
    i32 m_variant = 0;

    static constexpr i32 SHAPE_MASK = 0xFF;
    static constexpr i32 BASE_COLOR_MASK = 0xFF00;
    static constexpr i32 PATTERN_COLOR_MASK = 0xFF0000;
};

} // namespace mc
