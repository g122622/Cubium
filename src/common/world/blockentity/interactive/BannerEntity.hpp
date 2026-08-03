/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "util/color/DyeColor.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include "world/blockentity/interactive/BannerPattern.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class BlockState;
class ItemStack;

namespace text {
class ITextComponent;
}

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
    ~BannerEntity() noexcept override;

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

    // ========== 自定义名称 ==========

    /**
     * @brief 设置自定义显示名称
     * @param name 自定义名称组件
     */
    void setCustomDisplayName(std::unique_ptr<text::ITextComponent> name);

    /**
     * @brief 获取自定义显示名称
     * @return 自定义名称，如果没有返回nullptr
     */
    [[nodiscard]] const text::ITextComponent* getCustomDisplayName() const;

    /**
     * @brief 检查是否有自定义显示名称
     */
    [[nodiscard]] bool hasCustomDisplayName() const { return m_customName != nullptr; }

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

    // ========== ItemStack 互操作 ==========

    /**
     * @brief 从ItemStack加载图案数据
     *
     * 从物品的BlockEntityTag.Patterns读取图案并设置到方块实体。
     *
     * @param stack 旗帜物品
     * @param baseColor 旗帜底色
     */
    void loadFromItemStack(const ItemStack& stack, DyeColor baseColor);

    /**
     * @brief 生成包含图案数据的ItemStack
     *
     * 根据底色找到对应旗帜物品，将图案数据写入BlockEntityTag。
     *
     * @param state 当前方块状态
     * @return 包含图案数据的物品堆
     */
    [[nodiscard]] ItemStack getItem(const BlockState& state) const;

    // ========== 静态工具方法 ==========

    /**
     * @brief 从ItemStack获取图案列表
     * @param stack 旗帜物品
     * @return 图案列表，如果没有图案返回空列表
     */
    [[nodiscard]] static std::vector<BannerPattern> getPatternsFromItemStack(const ItemStack& stack);

    /**
     * @brief 从ItemStack获取图案数量
     * @param stack 旗帜物品
     * @return 图案数量
     */
    [[nodiscard]] static i32 getPatternCount(const ItemStack& stack);

    /**
     * @brief 从ItemStack移除最顶层图案数据
     * @param stack 旗帜物品
     */
    static void removeBannerData(ItemStack& stack);

    /**
     * @brief 组合底色和图案为渲染数据
     *
     * 将底色作为第一层（Base图案），然后追加所有图案层。
     *
     * @param baseColor 底色
     * @param patterns 图案列表
     * @return 完整的图案+颜色列表（用于渲染）
     */
    [[nodiscard]] static std::vector<std::pair<BannerPatternType, DyeColor>> composePatterns(
        DyeColor baseColor, const std::vector<BannerPattern>& patterns);

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return false; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    bool loadFromNBT(const nbt::tags::compound_tag& tag) override;
    void saveToNBT(nbt::tags::compound_tag& tag) const override;

    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    std::vector<BannerPattern> m_patterns;              ///< 图案列表
    DyeColor m_baseColor = DyeColor::White;             ///< 底色（默认白色）
    std::unique_ptr<text::ITextComponent> m_customName; ///< 自定义名称
};

} // namespace blockentity
} // namespace mc
