#pragma once

#include "common/core/Types.hpp"
#include "../Glyph.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace mc::client::ui::kagero {

// 引入Colors命名空间中的颜色常量
using mc::client::Colors::WHITE;
using mc::client::Colors::BLACK;
using mc::client::Colors::RED;
using mc::client::Colors::GREEN;
using mc::client::Colors::BLUE;
using mc::client::Colors::fromARGB;

/**
 * @brief 矩形区域
 *
 * 用于Widget的边界和布局计算
 */
struct Rect {
    i32 x = 0;          ///< 左边界
    i32 y = 0;          ///< 上边界
    i32 width = 0;      ///< 宽度
    i32 height = 0;     ///< 高度

    Rect() = default;
    Rect(i32 x_, i32 y_, i32 w_, i32 h_)
        : x(x_), y(y_), width(w_), height(h_) {}

    /**
     * @brief 获取右边界
     */
    [[nodiscard]] i32 right() const { return x + width; }

    /**
     * @brief 获取下边界
     */
    [[nodiscard]] i32 bottom() const { return y + height; }

    /**
     * @brief 获取中心X坐标
     */
    [[nodiscard]] i32 centerX() const { return x + width / 2; }

    /**
     * @brief 获取中心Y坐标
     */
    [[nodiscard]] i32 centerY() const { return y + height / 2; }

    /**
     * @brief 检查点是否在矩形内
     */
    [[nodiscard]] bool contains(i32 px, i32 py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }

    /**
     * @brief 检查是否与另一个矩形相交
     */
    [[nodiscard]] bool intersects(const Rect& other) const {
        return x < other.right() && right() > other.x &&
               y < other.bottom() && bottom() > other.y;
    }

    /**
     * @brief 获取与另一个矩形的交集
     */
    [[nodiscard]] Rect intersection(const Rect& other) const {
        i32 newX = std::max(x, other.x);
        i32 newY = std::max(y, other.y);
        i32 newRight = std::min(right(), other.right());
        i32 newBottom = std::min(bottom(), other.bottom());
        if (newRight > newX && newBottom > newY) {
            return Rect(newX, newY, newRight - newX, newBottom - newY);
        }
        return Rect();
    }

    /**
     * @brief 检查矩形是否有效
     */
    [[nodiscard]] bool isValid() const {
        return width > 0 && height > 0;
    }
};

/**
 * @brief 边距
 */
struct Margin {
    i32 left = 0;
    i32 top = 0;
    i32 right = 0;
    i32 bottom = 0;

    Margin() = default;
    explicit Margin(i32 all) : left(all), top(all), right(all), bottom(all) {}
    Margin(i32 horizontal, i32 vertical)
        : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    Margin(i32 l, i32 t, i32 r, i32 b)
        : left(l), top(t), right(r), bottom(b) {}

    [[nodiscard]] i32 horizontal() const { return left + right; }
    [[nodiscard]] i32 vertical() const { return top + bottom; }
};

/**
 * @brief 内边距
 */
struct Padding {
    i32 left = 0;
    i32 top = 0;
    i32 right = 0;
    i32 bottom = 0;

    Padding() = default;
    explicit Padding(i32 all) : left(all), top(all), right(all), bottom(all) {}
    Padding(i32 horizontal, i32 vertical)
        : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    Padding(i32 l, i32 t, i32 r, i32 b)
        : left(l), top(t), right(r), bottom(b) {}

    [[nodiscard]] i32 horizontal() const { return left + right; }
    [[nodiscard]] i32 vertical() const { return top + bottom; }
};

/**
 * @brief 锚点位置
 *
 * 用于Widget的定位和对齐
 */
enum class Anchor : u8 {
    TopLeft,        ///< 左上角
    TopCenter,      ///< 顶部居中
    TopRight,       ///< 右上角
    CenterLeft,     ///< 左侧居中
    Center,         ///< 正中心
    CenterRight,    ///< 右侧居中
    BottomLeft,     ///< 左下角
    BottomCenter,   ///< 底部居中
    BottomRight     ///< 右下角
};

/**
 * @brief 键盘修饰键
 */
enum class KeyMods : u8 {
    None = 0,
    Shift = 1 << 0,
    Control = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3,
    CapsLock = 1 << 4,
    NumLock = 1 << 5
};

/**
 * @brief 键盘修饰键位运算
 */
inline KeyMods operator|(KeyMods a, KeyMods b) {
    return static_cast<KeyMods>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline KeyMods operator&(KeyMods a, KeyMods b) {
    return static_cast<KeyMods>(static_cast<u8>(a) & static_cast<u8>(b));
}

inline bool hasMod(KeyMods mods, KeyMods flag) {
    return (mods & flag) == flag;
}

/**
 * @brief 鼠标按钮
 */
enum class MouseButton : u8 {
    None = 0,
    Left = 1,
    Right = 2,
    Middle = 3,
    Button4 = 4,
    Button5 = 5
};

/**
 * @brief 键盘动作
 */
enum class KeyAction : u8 {
    Release = 0,    ///< 释放
    Press = 1,      ///< 按下
    Repeat = 2      ///< 重复（按住不放）
};

} // namespace mc::client::ui::kagero
