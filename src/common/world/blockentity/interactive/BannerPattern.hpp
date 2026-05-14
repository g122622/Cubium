#pragma once

#include "core/Types.hpp"
#include <string>

namespace mc {
namespace blockentity {

/**
 * @brief 旗帜图案枚举
 *
 * MC 1.16.5 共有 37 种旗帜图案，其中：
 * - BASE: 底色图案（特殊，不消耗图案卷）
 * - 基础图案（SQUARE_*, STRIPE_*, 等）：可通过织布机制作
 * - 特殊图案（GLOBE, CREEPER, SKULL, FLOWER, MOJANG, PIGLIN）：需要特殊图案卷
 *
 * 参考: net.minecraft.tileentity.BannerPattern
 */
enum class BannerPatternType : u8 {
    // 底色图案（特殊）
    Base = 0, // base - 底色

    // 方形图案
    SquareBottomLeft = 1,  // square_bottom_left - 左下角方形
    SquareBottomRight = 2, // square_bottom_right - 右下角方形
    SquareTopLeft = 3,     // square_top_left - 左上角方形
    SquareTopRight = 4,    // square_top_right - 右上角方形

    // 条纹图案
    StripeBottom = 5,     // stripe_bottom - 底部条纹
    StripeTop = 6,        // stripe_top - 顶部条纹
    StripeLeft = 7,       // stripe_left - 左侧条纹
    StripeRight = 8,      // stripe_right - 右侧条纹
    StripeCenter = 9,     // stripe_center - 中央竖条纹
    StripeMiddle = 10,    // stripe_middle - 中央横条纹
    StripeDownright = 11, // stripe_downright - 右下斜纹
    StripeDownleft = 12,  // stripe_downleft - 左下斜纹
    StripeSmall = 13,     // small_stripes - 细条纹

    // 十字图案
    Cross = 14,         // cross - 斜十字
    StraightCross = 15, // straight_cross - 正十字

    // 三角图案
    TriangleBottom = 16,  // triangle_bottom - 底部三角
    TriangleTop = 17,     // triangle_top - 顶部三角
    TrianglesBottom = 18, // triangles_bottom - 底部锯齿
    TrianglesTop = 19,    // triangles_top - 顶部锯齿

    // 对角图案
    DiagonalLeft = 20,        // diagonal_left - 左对角
    DiagonalRight = 21,       // diagonal_up_right - 右对角
    DiagonalLeftMirror = 22,  // diagonal_up_left - 左对角镜像
    DiagonalRightMirror = 23, // diagonal_right - 右对角镜像

    // 中心图案
    CircleMiddle = 24,  // circle - 圆形
    RhombusMiddle = 25, // rhombus - 菱形

    // 半幅图案
    HalfVertical = 26,         // half_vertical - 左半幅
    HalfHorizontal = 27,       // half_horizontal - 上半幅
    HalfVerticalMirror = 28,   // half_vertical_right - 右半幅
    HalfHorizontalMirror = 29, // half_horizontal_bottom - 下半幅

    // 边框图案
    Border = 30,      // border - 边框
    CurlyBorder = 31, // curly_border - 波浪边框

    // 渐变图案
    Gradient = 32,   // gradient - 下渐变
    GradientUp = 33, // gradient_up - 上渐变

    // 特殊纹理图案
    Bricks = 34,  // bricks - 砖块纹理
    Globe = 35,   // globe - 地球（特殊图案卷）
    Creeper = 36, // creeper - 苦力怕（特殊图案卷）
    Skull = 37,   // skull - 骷髅（特殊图案卷）
    Flower = 38,  // flower - 花朵（特殊图案卷）
    Mojang = 39,  // mojang - Mojang 标志（特殊图案卷）
    Piglin = 40,  // piglin - 猪灵（特殊图案卷）

    Count = 41 // 图案总数
};

/**
 * @brief 旗帜图案工具类
 *
 * 提供 BannerPatternType 的辅助方法
 */
class BannerPatterns {
public:
    /**
     * @brief 根据哈希名获取图案类型
     *
     * 用于从 NBT 数据解析图案。
     * MC 1.16.5 使用 2-3 字符的短名作为哈希名。
     *
     * @param hashName 哈希名（如 "bs", "ts", "cr"）
     * @return 图案类型，如果未找到返回 Base
     */
    [[nodiscard]] static BannerPatternType byHash(const std::string& hashName);

    /**
     * @brief 获取图案的哈希名
     * @param type 图案类型
     * @return 哈希名（如 "bs", "ts", "cr"）
     */
    [[nodiscard]] static std::string getHashName(BannerPatternType type);

    /**
     * @brief 获取图案的文件名
     *
     * 用于渲染纹理。
     *
     * @param type 图案类型
     * @return 文件名（如 "stripe_bottom", "cross"）
     */
    [[nodiscard]] static std::string getFileName(BannerPatternType type);

    /**
     * @brief 检查图案是否需要特殊图案卷
     *
     * GLOBE, CREEPER, SKULL, FLOWER, MOJANG, PIGLIN 需要特殊图案卷。
     *
     * @param type 图案类型
     * @return 如果需要特殊图案卷返回 true
     */
    [[nodiscard]] static bool hasPatternItem(BannerPatternType type);

    /**
     * @brief 检查图案是否是底色图案
     * @param type 图案类型
     * @return 如果是底色图案返回 true
     */
    [[nodiscard]] static bool isBase(BannerPatternType type) { return type == BannerPatternType::Base; }
};

} // namespace blockentity
} // namespace mc
