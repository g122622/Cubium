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

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明 GLFW 按键常量
// 这些值与 GLFW 按键常量一致，避免直接依赖 GLFW 头文件
namespace mc {

/**
 * @brief 按键常量
 *
 * 与 GLFW 按键常量一致的键码定义，避免在公共头文件中包含 GLFW。
 * 参考: https://www.glfw.org/docs/latest/group__keys.html
 */
namespace Keys {
// 未知键
constexpr i32 Unknown = -1;

// 功能键
constexpr i32 Space = 32;
constexpr i32 Apostrophe = 39; // '
constexpr i32 Comma = 44;      // ,
constexpr i32 Minus = 45;      // -
constexpr i32 Period = 46;     // .
constexpr i32 Slash = 47;      // /

// 数字键 0-9
constexpr i32 D0 = 48;
constexpr i32 D1 = 49;
constexpr i32 D2 = 50;
constexpr i32 D3 = 51;
constexpr i32 D4 = 52;
constexpr i32 D5 = 53;
constexpr i32 D6 = 54;
constexpr i32 D7 = 55;
constexpr i32 D8 = 56;
constexpr i32 D9 = 57;

// 字母键 A-Z
constexpr i32 A = 65;
constexpr i32 B = 66;
constexpr i32 C = 67;
constexpr i32 D = 68;
constexpr i32 E = 69;
constexpr i32 F = 70;
constexpr i32 G = 71;
constexpr i32 H = 72;
constexpr i32 I = 73;
constexpr i32 J = 74;
constexpr i32 K = 75;
constexpr i32 L = 76;
constexpr i32 M = 77;
constexpr i32 N = 78;
constexpr i32 O = 79;
constexpr i32 P = 80;
constexpr i32 Q = 81;
constexpr i32 R = 82;
constexpr i32 S = 83;
constexpr i32 T = 84;
constexpr i32 U = 85;
constexpr i32 V = 86;
constexpr i32 W = 87;
constexpr i32 X = 88;
constexpr i32 Y = 89;
constexpr i32 Z = 90;

// 分号、等号
constexpr i32 Semicolon = 59; // ;
constexpr i32 Equal = 61;     // =

// 括号、反斜杠、反引号
constexpr i32 LeftBracket = 91;  // [
constexpr i32 Backslash = 92;    // '\'
constexpr i32 RightBracket = 93; // ]
constexpr i32 GraveAccent = 96;  // `

// 非 US 键盘布局键
constexpr i32 World1 = 161;
constexpr i32 World2 = 162;

// 功能键 F1-F25
constexpr i32 F1 = 290;
constexpr i32 F2 = 291;
constexpr i32 F3 = 292;
constexpr i32 F4 = 293;
constexpr i32 F5 = 294;
constexpr i32 F6 = 295;
constexpr i32 F7 = 296;
constexpr i32 F8 = 297;
constexpr i32 F9 = 298;
constexpr i32 F10 = 299;
constexpr i32 F11 = 300;
constexpr i32 F12 = 301;
constexpr i32 F13 = 302;
constexpr i32 F14 = 303;
constexpr i32 F15 = 304;
constexpr i32 F16 = 305;
constexpr i32 F17 = 306;
constexpr i32 F18 = 307;
constexpr i32 F19 = 308;
constexpr i32 F20 = 309;
constexpr i32 F21 = 310;
constexpr i32 F22 = 311;
constexpr i32 F23 = 312;
constexpr i32 F24 = 313;
constexpr i32 F25 = 314;

// 方向键
constexpr i32 Up = 265;
constexpr i32 Down = 264;
constexpr i32 Left = 263;
constexpr i32 Right = 262;

// 控制键
constexpr i32 LeftShift = 340;
constexpr i32 RightShift = 344;
constexpr i32 LeftControl = 341;
constexpr i32 RightControl = 345;
constexpr i32 LeftAlt = 342;
constexpr i32 RightAlt = 346;
constexpr i32 LeftSuper = 343;
constexpr i32 RightSuper = 347;

// 其他特殊键
constexpr i32 Escape = 256;
constexpr i32 Enter = 257;
constexpr i32 Tab = 258;
constexpr i32 Backspace = 259;
constexpr i32 Insert = 260;
constexpr i32 Delete = 261;
constexpr i32 Home = 268;
constexpr i32 End = 269;
constexpr i32 PageUp = 266;
constexpr i32 PageDown = 267;
constexpr i32 CapsLock = 280;
constexpr i32 ScrollLock = 281;
constexpr i32 NumLock = 282;
constexpr i32 PrintScreen = 283;
constexpr i32 Pause = 284;

// 小键盘
constexpr i32 KP_0 = 320;
constexpr i32 KP_1 = 321;
constexpr i32 KP_2 = 322;
constexpr i32 KP_3 = 323;
constexpr i32 KP_4 = 324;
constexpr i32 KP_5 = 325;
constexpr i32 KP_6 = 326;
constexpr i32 KP_7 = 327;
constexpr i32 KP_8 = 328;
constexpr i32 KP_9 = 329;
constexpr i32 KP_Decimal = 330;
constexpr i32 KP_Divide = 331;
constexpr i32 KP_Multiply = 332;
constexpr i32 KP_Subtract = 333;
constexpr i32 KP_Add = 334;
constexpr i32 KP_Enter = 335;
constexpr i32 KP_Equal = 336;

// 菜单键
constexpr i32 Menu = 348;

// 鼠标按键
namespace Mouse {
constexpr i32 Button1 = 0; // 左键
constexpr i32 Button2 = 1; // 右键
constexpr i32 Button3 = 2; // 中键
constexpr i32 Button4 = 3;
constexpr i32 Button5 = 4;
constexpr i32 Button6 = 5;
constexpr i32 Button7 = 6;
constexpr i32 Button8 = 7;
constexpr i32 Left = Button1;
constexpr i32 Right = Button2;
constexpr i32 Middle = Button3;
} // namespace Mouse
} // namespace Keys

/**
 * @brief 按键绑定类
 *
 * 管理单个按键绑定，包括绑定 ID、默认按键、当前按键和分类。
 * 支持按键重映射和分类管理。
 *
 * 使用示例:
 * @code
 * // 创建按键绑定
 * KeyBinding forward("key.forward", Keys::W, "key.categories.movement");
 * KeyBinding jump("key.jump", Keys::Space, "key.categories.movement");
 *
 * // 检查按键状态
 * if (forward.isPressed()) {
 *     player.moveForward();
 * }
 *
 * // 重新映射按键
 * jump.setKey(Keys::E);
 *
 * // 重置为默认
 * jump.resetToDefault();
 *
 * // 更新所有按键状态（每帧调用）
 * KeyBinding::updateAll(pressedKeys);
 * @endcode
 */
class KeyBinding {
public:
    /**
     * @brief 按键状态回调
     * @param binding 触发的按键绑定
     * @param pressed 是否按下
     */
    using StateCallback = std::function<void(const KeyBinding& binding, bool pressed)>;

    /**
     * @brief 构造按键绑定
     *
     * @param id 绑定 ID，格式为 "key.xxx"（如 "key.forward"）
     * @param defaultKey 默认按键码（GLFW 键码）
     * @param category 分类 ID（如 "key.categories.movement"）
     */
    KeyBinding(std::string id, i32 defaultKey, std::string category);

    /**
     * @brief 析构函数
     */
    ~KeyBinding();

    // 禁止拷贝
    KeyBinding(const KeyBinding&) = delete;
    KeyBinding& operator=(const KeyBinding&) = delete;

    // 允许移动
    KeyBinding(KeyBinding&&) noexcept;
    KeyBinding& operator=(KeyBinding&&) noexcept;

    // ========================================================================
    // 属性访问
    // ========================================================================

    /**
     * @brief 获取绑定 ID
     */
    [[nodiscard]] const std::string& id() const noexcept { return m_id; }

    /**
     * @brief 获取默认按键码
     */
    [[nodiscard]] i32 defaultKey() const noexcept { return m_defaultKey; }

    /**
     * @brief 获取当前按键码
     */
    [[nodiscard]] i32 key() const noexcept { return m_currentKey; }

    /**
     * @brief 获取分类 ID
     */
    [[nodiscard]] const std::string& category() const noexcept { return m_category; }

    /**
     * @brief 获取翻译键（用于 UI 显示）
     * @return 翻译键，如 "key.forward"
     */
    [[nodiscard]] const std::string& translationKey() const noexcept { return m_id; }

    // ========================================================================
    // 按键设置
    // ========================================================================

    /**
     * @brief 设置按键码
     * @param key 新的按键码
     */
    void setKey(i32 key);

    /**
     * @brief 重置为默认按键
     */
    void resetToDefault();

    /**
     * @brief 检查是否使用默认按键
     */
    [[nodiscard]] bool isDefault() const noexcept;

    // ========================================================================
    // 按键状态
    // ========================================================================

    /**
     * @brief 检查按键是否当前被按下
     *
     * TODO：需要先调用 updateAll() 更新状态。
     */
    [[nodiscard]] bool isPressed() const noexcept;

    /**
     * @brief 检查按键是否刚按下（本帧按下）
     */
    [[nodiscard]] bool isJustPressed() const noexcept;

    /**
     * @brief 检查按键是否刚释放（本帧释放）
     */
    [[nodiscard]] bool isJustReleased() const noexcept;

    // ========================================================================
    // 静态方法 - 全局管理
    // ========================================================================

    /**
     * @brief 通过 ID 查找按键绑定
     * @param id 绑定 ID
     * @return 按键绑定指针，找不到返回 nullptr
     */
    [[nodiscard]] static KeyBinding* find(const std::string& id);

    /**
     * @brief 获取分类下的所有按键绑定
     * @param category 分类 ID
     * @return 按键绑定列表
     */
    [[nodiscard]] static std::vector<KeyBinding*> getByCategory(const std::string& category);

    /**
     * @brief 获取所有分类
     * @return 分类 ID 列表
     */
    [[nodiscard]] static std::vector<std::string> getCategories();

    /**
     * @brief 更新所有按键状态
     *
     * 每帧调用一次，传入当前按下的按键集合。
     *
     * @param pressedKeys 当前按下的按键码集合
     * @param justPressedKeys 本帧刚按下的按键码集合
     * @param justReleasedKeys 本帧刚释放的按键码集合
     */
    static void updateAll(const std::vector<i32>& pressedKeys,
        const std::vector<i32>& justPressedKeys,
        const std::vector<i32>& justReleasedKeys);

    /**
     * @brief 重置所有按键绑定到默认值
     */
    static void resetAllToDefault();

    /**
     * @brief 设置按键状态变化回调
     * @param callback 回调函数
     */
    static void setStateCallback(StateCallback callback);

    // ========================================================================
    // 序列化
    // ========================================================================

    /**
     * @brief 序列化所有按键绑定到 JSON
     * @param j JSON 对象
     */
    static void serializeAll(nlohmann::json& j);

    /**
     * @brief 从 JSON 反序列化所有按键绑定
     * @param j JSON 对象
     */
    static void deserializeAll(const nlohmann::json& j);

private:
    std::string m_id;       // 绑定 ID（如 "key.forward"）
    i32 m_defaultKey;       // 默认按键码
    i32 m_currentKey;       // 当前按键码
    std::string m_category; // 分类（如 "key.categories.movement"）

    // 状态
    bool m_pressed = false;
    bool m_justPressed = false;
    bool m_justReleased = false;

    // 静态注册表
    static std::map<std::string, KeyBinding*> s_bindings;
    static std::map<std::string, std::vector<KeyBinding*>> s_categoryBindings;
    static StateCallback s_stateCallback;

    // 注册到全局
    void _registerBinding();
    // 从全局注销
    void _unregisterBinding();
};

} // namespace mc
