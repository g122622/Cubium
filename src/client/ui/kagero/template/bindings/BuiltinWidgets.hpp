#pragma once

#include "../../widget/Widget.hpp"
#include "../../widget/ButtonWidget.hpp"
#include "../../widget/TextWidget.hpp"
#include "../../widget/TextFieldWidget.hpp"
#include "../../widget/SliderWidget.hpp"
#include "../../widget/CheckboxWidget.hpp"
#include "../../widget/SlotWidget.hpp"
#include "../../widget/ScrollableWidget.hpp"
#include "../../widget/ListWidget.hpp"
#include "../../widget/Viewport3DWidget.hpp"
#include "../../widget/ContainerWidget.hpp"
#include "../../event/EventBus.hpp"
#include "../../event/InputEvents.hpp"
#include "../../event/UIEvents.hpp"
#include <functional>
#include <unordered_map>
#include <memory>
#include <map>

namespace mc::client::ui::kagero::tpl::bindings {

/**
 * @brief Widget创建函数类型
 */
using WidgetCreator = std::function<std::unique_ptr<widget::Widget>(
    const String& id, const std::map<String, String>& attrs)>;

/**
 * @brief 内置Widget注册表
 *
 * 管理所有内置Widget类型的创建和配置。
 * 提供Widget工厂方法供模板实例化使用。
 */
class BuiltinWidgets {
public:
    /**
     * @brief 获取单例实例
     */
    static BuiltinWidgets& instance();

    /**
     * @brief 初始化所有内置Widget
     */
    void initialize();

    /**
     * @brief 注册Widget创建器
     *
     * @param tagName 标签名
     * @param creator 创建函数
     */
    void registerCreator(const String& tagName, WidgetCreator creator);

    /**
     * @brief 创建Widget
     *
     * @param tagName 标签名
     * @param id Widget ID
     * @param attrs 属性映射
     * @return 创建的Widget，如果标签未知返回nullptr
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> create(
        const String& tagName,
        const String& id,
        const std::map<String, String>& attrs) const;

    /**
     * @brief 检查标签名是否注册
     */
    [[nodiscard]] bool hasTag(const String& tagName) const;

    /**
     * @brief 获取所有注册的标签名
     */
    [[nodiscard]] std::vector<String> registeredTags() const;

    /**
     * @brief 获取标签的默认属性
     */
    [[nodiscard]] std::map<String, String> getDefaultAttributes(const String& tagName) const;

private:
    BuiltinWidgets();
    ~BuiltinWidgets() = default;

    // 禁止拷贝
    BuiltinWidgets(const BuiltinWidgets&) = delete;
    BuiltinWidgets& operator=(const BuiltinWidgets&) = delete;

    void registerScreenWidget();
    void registerContainerWidget();
    void registerButtonWidget();
    void registerTextWidget();
    void registerTextFieldWidget();
    void registerSliderWidget();
    void registerCheckboxWidget();
    void registerImageWidget();
    void registerGridWidget();
    void registerSlotWidget();
    void registerScrollableWidget();
    void registerListWidget();
    void registerViewport3DWidget();

    std::unordered_map<String, WidgetCreator> m_creators;
    std::unordered_map<String, std::map<String, String>> m_defaultAttributes;
    bool m_initialized = false;
};

/**
 * @brief Widget属性助手
 *
 * 提供解析和应用Widget属性的工具方法
 */
namespace widget_attrs {

// ========== 位置和尺寸 ==========

/**
 * @brief 解析位置属性
 * @param value "x,y" 格式的字符串
 * @return pair<x, y>，解析失败返回 {0, 0}
 */
[[nodiscard]] std::pair<i32, i32> parsePosition(const String& value);

/**
 * @brief 解析尺寸属性
 * @param value "width,height" 格式的字符串
 * @return pair<width, height>，解析失败返回 {0, 0}
 */
[[nodiscard]] std::pair<i32, i32> parseSize(const String& value);

/**
 * @brief 应用位置属性
 */
void applyPosition(widget::Widget* widget, const String& value);

/**
 * @brief 应用尺寸属性
 */
void applySize(widget::Widget* widget, const String& value);

// ========== 颜色 ==========

/**
 * @brief 解析颜色值
 *
 * 支持格式:
 * - "#RRGGBB"
 * - "#RRGGBBAA"
 * - "rgb(r, g, b)"
 * - "rgba(r, g, b, a)"
 * - 颜色名称
 *
 * @param value 颜色字符串
 * @return ARGB颜色值，解析失败返回白色
 */
[[nodiscard]] u32 parseColor(const String& value);

/**
 * @brief 颜色转字符串
 */
[[nodiscard]] String colorToString(u32 color);

// ========== 锚点 ==========

/**
 * @brief 解析锚点值
 */
[[nodiscard]] Anchor parseAnchor(const String& value);

/**
 * @brief 锚点转字符串
 */
[[nodiscard]] String anchorToString(Anchor anchor);

// ========== 布尔值 ==========

/**
 * @brief 解析布尔值
 *
 * 支持值: "true", "false", "1", "0", "yes", "no"
 */
[[nodiscard]] bool parseBool(const String& value);

// ========== 数值 ==========

/**
 * @brief 解析整数
 */
[[nodiscard]] i32 parseInt(const String& value, i32 defaultValue = 0);

/**
 * @brief 解析浮点数
 */
[[nodiscard]] f32 parseFloat(const String& value, f32 defaultValue = 0.0f);

// ========== 范围 ==========

/**
 * @brief 解析范围值
 * @param value "min,max" 格式
 */
[[nodiscard]] std::pair<f32, f32> parseRange(const String& value);

} // namespace widget_attrs

/**
 * @brief 初始化内置Widget工厂
 *
 * 调用 BuiltinWidgets::instance().initialize()
 */
inline void registerBuiltinWidgets() {
    BuiltinWidgets::instance().initialize();
}

} // namespace mc::client::ui::kagero::tpl::bindings
