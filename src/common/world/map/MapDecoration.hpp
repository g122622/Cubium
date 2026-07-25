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

#include "core/Types.hpp"
#include "util/nbt/Nbt.hpp"
#include <memory>
#include <optional>
#include <string>

namespace mc::text {
class ITextComponent;
}

namespace mc::world::map {

/**
 * @brief 地图装饰类型
 *
 * 定义地图上显示的各种标记图标类型，与MC 1.16.5的MapDecoration.Type对应。
 * 部分类型只在展示框中渲染(renderedOnFrame=true)，
 * 部分类型有地图颜色(hasMapColor=true)用于物品栏显示。
 */
enum class DecorationType : u8 {
    PLAYER = 0,             // 玩家标记
    FRAME = 1,              // 物品展示框标记
    RED_MARKER = 2,         // 红色标记
    BLUE_MARKER = 3,        // 蓝色标记
    TARGET_X = 4,           // 目标X标记
    TARGET_POINT = 5,       // 目标点标记
    PLAYER_OFF_MAP = 6,     // 地图范围外的玩家
    PLAYER_OFF_LIMITS = 7,  // 远离地图的玩家（无限追踪）
    MANSION = 8,            // 林地府邸标记
    MONUMENT = 9,           // 海底神殿标记
    BANNER_WHITE = 10,      // 白色旗帜
    BANNER_ORANGE = 11,     // 橙色旗帜
    BANNER_MAGENTA = 12,    // 品红色旗帜
    BANNER_LIGHT_BLUE = 13, // 淡蓝色旗帜
    BANNER_YELLOW = 14,     // 黄色旗帜
    BANNER_LIME = 15,       // 黄绿色旗帜
    BANNER_PINK = 16,       // 粉色旗帜
    BANNER_GRAY = 17,       // 灰色旗帜
    BANNER_LIGHT_GRAY = 18, // 淡灰色旗帜
    BANNER_CYAN = 19,       // 青色旗帜
    BANNER_PURPLE = 20,     // 紫色旗帜
    BANNER_BLUE = 21,       // 蓝色旗帜
    BANNER_BROWN = 22,      // 棕色旗帜
    BANNER_GREEN = 23,      // 绿色旗帜
    BANNER_RED = 24,        // 红色旗帜
    BANNER_BLACK = 25,      // 黑色旗帜
    RED_X = 26,             // 红色X标记

    COUNT = 27
};

/**
 * @brief 获取装饰类型是否在展示框中渲染
 */
[[nodiscard]] bool isRenderedOnFrame(DecorationType type) noexcept;

/**
 * @brief 获取装饰类型是否有地图颜色
 */
[[nodiscard]] bool hasMapColor(DecorationType type) noexcept;

/**
 * @brief 获取装饰类型的地图颜色（用于物品栏显示）
 *
 * @return ARGB颜色值，如果没有地图颜色返回-1
 */
[[nodiscard]] i32 getMapColor(DecorationType type) noexcept;

/**
 * @brief 根据图标ID获取装饰类型
 */
[[nodiscard]] DecorationType decorationTypeByIcon(u8 icon) noexcept;

/**
 * @brief 根据字符串名称获取装饰类型
 *
 * 支持 MC 1.16.5 格式（如 "mansion"、"red_x"）和
 * 1.21.11 格式（如 "minecraft:mansion"、"minecraft:red_x"）。
 *
 * @param str 装饰类型名称字符串
 * @return 对应的装饰类型，无法识别时返回 nullopt
 */
[[nodiscard]] std::optional<DecorationType> decorationTypeFromString(const std::string& str) noexcept;

/**
 * @brief 将装饰类型转换为字符串名称（不含命名空间前缀）
 */
[[nodiscard]] const char* decorationTypeToString(DecorationType type) noexcept;

/**
 * @brief 地图装饰
 *
 * 表示地图上的一个标记图标，包含类型、位置、旋转和可选的自定义名称。
 */
class MapDecoration {
public:
    MapDecoration(
        DecorationType type, i8 x, i8 y, u8 rotation, std::unique_ptr<text::ITextComponent> customName = nullptr);

    ~MapDecoration();

    MapDecoration(const MapDecoration&) = delete;
    MapDecoration& operator=(const MapDecoration&) = delete;
    MapDecoration(MapDecoration&&) noexcept = default;
    MapDecoration& operator=(MapDecoration&&) noexcept = default;

    /**
     * @brief 深拷贝此装饰对象
     */
    [[nodiscard]] MapDecoration deepCopy() const;

    /**
     * @brief 从NBT读取装饰
     */
    static MapDecoration fromNbt(const nbt::tags::compound_tag& tag);

    /**
     * @brief 写入NBT
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    [[nodiscard]] DecorationType type() const noexcept { return m_type; }
    [[nodiscard]] i8 x() const noexcept { return m_x; }
    [[nodiscard]] i8 y() const noexcept { return m_y; }
    [[nodiscard]] u8 rotation() const noexcept { return m_rotation; }
    [[nodiscard]] const text::ITextComponent* customName() const noexcept { return m_customName.get(); }

    /**
     * @brief 获取图标字节值（用于序列化和纹理图集查找）
     */
    [[nodiscard]] u8 getIcon() const noexcept { return static_cast<u8>(m_type); }

    /**
     * @brief 是否在展示框中渲染
     */
    [[nodiscard]] bool renderedOnFrame() const noexcept { return isRenderedOnFrame(m_type); }

private:
    DecorationType m_type;
    i8 m_x;        // 地图坐标X (-128到127)
    i8 m_y;        // 地图坐标Y (-128到127)
    u8 m_rotation; // 旋转 (0-15，对应0-337.5度)
    std::unique_ptr<text::ITextComponent> m_customName;
};

} // namespace mc::world::map
