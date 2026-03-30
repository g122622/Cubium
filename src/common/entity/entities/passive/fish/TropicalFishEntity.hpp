#pragma once

#include "AbstractFishEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 热带鱼实体
 *
 * 生活在温水海洋的彩色鱼类。
 *
 * 特性：
 * - 变种：有很多颜色变种
 * - 群居：会与其他热带鱼聚在一起
 * - 掉落：热带鱼、骨头
 *
 * 参考 MC 1.16.5 TropicalFishEntity
 */
class TropicalFishEntity : public AbstractFishEntity {
public:
    /**
     * @brief 热带鱼形状
     */
    enum class FishShape : u8 {
        Kob = 0,      // 小丑鱼形状
        SunStreak = 1, // 条纹形状
        Snooper = 2,  // 尖嘴形状
        Dasher = 3,   // 飞鱼形状
        Brinely = 4,  // 圆形
        Spotty = 5,   // 斑点形状
        Flopper = 6,  // 翻车鱼形状
        Stripey = 7,  // 垂直条纹
        Glitter = 8,  // 闪光形状
        Blockfish = 9, // 方形
        Betty = 10,   // 贝塔鱼形状
        Clayfish = 11 // 粘土鱼形状
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    TropicalFishEntity(LegacyEntityType type, EntityId id);
    ~TropicalFishEntity() override = default;

    // 禁止拷贝
    TropicalFishEntity(const TropicalFishEntity&) = delete;
    TropicalFishEntity& operator=(const TropicalFishEntity&) = delete;

    // 允许移动
    TropicalFishEntity(TropicalFishEntity&&) = default;
    TropicalFishEntity& operator=(TropicalFishEntity&&) = default;

    /**
     * @brief 创建热带鱼实体
     * @param world 世界实例
     * @return 新的热带鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 变种系统 ==========

    /**
     * @brief 获取变种ID
     * 变种ID编码了形状、主体颜色和花纹颜色
     */
    [[nodiscard]] i32 getVariant() const { return m_variant; }

    /**
     * @brief 设置变种ID
     */
    void setVariant(i32 variant) { m_variant = variant; }

    /**
     * @brief 获取形状
     */
    [[nodiscard]] FishShape getShape() const;

    /**
     * @brief 获取主体颜色
     */
    [[nodiscard]] u8 getBaseColor() const;

    /**
     * @brief 获取花纹颜色
     */
    [[nodiscard]] u8 getPatternColor() const;

    /**
     * @brief 随机设置变种
     */
    void randomizeVariant();

    // ========== 群居 ==========

    /**
     * @brief 热带鱼会群游
     */
    [[nodiscard]] bool canSchool() const override { return true; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.1f; }

protected:
    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    i32 m_variant = 0; // 变种ID

    // 变种编码
    // variant = shape | (baseColor << 8) | (patternColor << 16)
    static constexpr i32 SHAPE_MASK = 0xFF;
    static constexpr i32 BASE_COLOR_MASK = 0xFF00;
    static constexpr i32 PATTERN_COLOR_MASK = 0xFF0000;
};

} // namespace mc
