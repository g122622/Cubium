#pragma once

#include "entity/entities/passive/basic/SheepEntity.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include "world/blockentity/interactive/BannerPattern.hpp"
#include <memory>
#include <vector>

namespace mc {

class IWorld;

namespace blockentity {

/**
 * @brief 旗帜图案数据
 *
 * 存储单个图案的信息：图案类型和颜色
 */
struct BannerPattern {
    BannerPatternType pattern = BannerPatternType::Base; ///< 图案类型
    DyeColor color = DyeColor::White;                    ///< 图案颜色

    BannerPattern() = default;
    BannerPattern(BannerPatternType p, DyeColor c)
        : pattern(p)
        , color(c)
    {}
};

/**
 * @brief 旗帜方块实体
 *
 * 旗帜用于显示自定义图案，特点：
 * - 支持最多6层图案叠加
 * - 每层图案有类型和颜色
 * - 墙挂式和站立式两种形态
 * - 16种底色可选
 *
 * 参考: net.minecraft.tileentity.BannerTileEntity
 */
class BannerEntity : public BlockEntity {
public:
    /// 最大图案层数
    static constexpr i32 MAX_PATTERNS = 6;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit BannerEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~BannerEntity() override;

    // ========== 图案接口 ==========

    /**
     * @brief 获取图案列表
     * @return 图案列表
     */
    [[nodiscard]] const std::vector<BannerPattern>& getPatterns() const { return m_patterns; }

    /**
     * @brief 获取图案数量
     * @return 图案数量
     */
    [[nodiscard]] i32 getPatternCount() const { return static_cast<i32>(m_patterns.size()); }

    /**
     * @brief 添加图案
     * @param pattern 图案
     * @return 如果添加成功返回true
     */
    bool addPattern(const BannerPattern& pattern);

    /**
     * @brief 设置图案列表
     * @param patterns 图案列表
     */
    void setPatterns(const std::vector<BannerPattern>& patterns);

    /**
     * @brief 移除最顶层的图案
     * @return 如果移除成功返回true
     */
    bool removeTopPattern();

    /**
     * @brief 清空所有图案
     */
    void clearPatterns();

    // ========== 颜色接口 ==========

    /**
     * @brief 获取底色
     * @return 底色（染料颜色）
     */
    [[nodiscard]] DyeColor getBaseColor() const { return m_baseColor; }

    /**
     * @brief 设置底色
     * @param color 底色（染料颜色）
     */
    void setBaseColor(DyeColor color);

    // ========== 辅助方法 ==========

    /**
     * @brief 检查是否有图案
     * @return 如果有图案返回true
     */
    [[nodiscard]] bool hasPatterns() const { return !m_patterns.empty(); }

    /**
     * @brief 获取图案纹理名称
     * @return 纹理名称（用于渲染）
     */
    [[nodiscard]] std::string getTextureName() const;

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return false; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    std::vector<BannerPattern> m_patterns;  ///< 图案列表
    DyeColor m_baseColor = DyeColor::White; ///< 底色（默认白色）
};

} // namespace blockentity
} // namespace mc
