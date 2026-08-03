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
#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::text {

/**
 * @brief 点击事件动作类型
 *
 * 参考: net.minecraft.util.text.event.ClickEvent.Action
 */
enum class ClickAction : u8 {
    OpenUrl,        // 打开 URL
    OpenFile,       // 打开文件
    RunCommand,     // 执行命令
    SuggestCommand, // 建议命令（填入输入框）
    CopyToClipboard // 复制到剪贴板
};

/**
 * @brief 点击事件
 *
 * 当玩家点击文本时执行的动作。
 * 参考: net.minecraft.util.text.event.ClickEvent
 */
class ClickEvent {
public:
    ClickEvent() = default;

    /**
     * @brief 构造点击事件
     * @param action 动作类型
     * @param value 值（URL、命令、文件路径等）
     */
    ClickEvent(ClickAction action, std::string value)
        : m_action(action)
        , m_value(std::move(value))
    {}

    // ========== 访问器 ==========

    [[nodiscard]] ClickAction getAction() const noexcept { return m_action; }
    [[nodiscard]] const std::string& getValue() const noexcept { return m_value; }

    /**
     * @brief 检查事件是否有效
     * @return 如果事件有效返回 true
     */
    [[nodiscard]] bool isValid() const noexcept { return !m_value.empty(); }

    // ========== 序列化 ==========

    /**
     * @brief 序列化为 JSON
     * @return JSON 对象
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 从 JSON 反序列化
     * @param json JSON 对象
     * @return 点击事件对象
     */
    static ClickEvent fromJson(const nlohmann::json& json);

    // ========== 比较 ==========

    bool operator==(const ClickEvent& other) const noexcept
    {
        return m_action == other.m_action && m_value == other.m_value;
    }
    bool operator!=(const ClickEvent& other) const noexcept { return !(*this == other); }

private:
    ClickAction m_action = ClickAction::RunCommand;
    std::string m_value;
};

/**
 * @brief 悬停事件动作类型
 *
 * 参考: net.minecraft.util.text.event.HoverEvent.Action
 */
enum class HoverAction : u8 {
    ShowText,  // 显示文本
    ShowItem,  // 显示物品
    ShowEntity // 显示实体
};

/**
 * @brief 悬停事件
 *
 * 当玩家悬停在文本上时显示的内容。
 * 参考: net.minecraft.util.text.event.HoverEvent
 */
class HoverEvent {
public:
    HoverEvent() = default;

    /**
     * @brief 构造显示文本的悬停事件
     * @param text 显示的文本（JSON 格式或纯文本）
     */
    static HoverEvent showText(std::string text)
    {
        HoverEvent event;
        event.m_action = HoverAction::ShowText;
        event.m_value = std::move(text);
        return event;
    }

    /**
     * @brief 构造显示物品的悬停事件
     * @param itemData 物品数据（物品 ID、NBT 等）
     */
    static HoverEvent showItem(std::string itemData)
    {
        HoverEvent event;
        event.m_action = HoverAction::ShowItem;
        event.m_value = std::move(itemData);
        return event;
    }

    /**
     * @brief 构造显示实体的悬停事件
     * @param entityData 实体数据（实体 ID、类型、名称等）
     */
    static HoverEvent showEntity(std::string entityData)
    {
        HoverEvent event;
        event.m_action = HoverAction::ShowEntity;
        event.m_value = std::move(entityData);
        return event;
    }

    // ========== 访问器 ==========

    [[nodiscard]] HoverAction getAction() const noexcept { return m_action; }
    [[nodiscard]] const std::string& getValue() const noexcept { return m_value; }

    /**
     * @brief 检查事件是否有效
     * @return 如果事件有效返回 true
     */
    [[nodiscard]] bool isValid() const noexcept { return !m_value.empty(); }

    // ========== 序列化 ==========

    /**
     * @brief 序列化为 JSON
     * @return JSON 对象
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 从 JSON 反序列化
     * @param json JSON 对象
     * @return 悬停事件对象
     */
    static HoverEvent fromJson(const nlohmann::json& json);

    // ========== 比较 ==========

    bool operator==(const HoverEvent& other) const noexcept
    {
        return m_action == other.m_action && m_value == other.m_value;
    }
    bool operator!=(const HoverEvent& other) const noexcept { return !(*this == other); }

private:
    HoverAction m_action = HoverAction::ShowText;
    std::string m_value;
};

// ========== 内联实现 ==========

inline nlohmann::json ClickEvent::toJson() const
{
    nlohmann::json json = nlohmann::json::object();

    const char* actionName = nullptr;
    switch (m_action) {
        case ClickAction::OpenUrl:
            actionName = "open_url";
            break;
        case ClickAction::OpenFile:
            actionName = "open_file";
            break;
        case ClickAction::RunCommand:
            actionName = "run_command";
            break;
        case ClickAction::SuggestCommand:
            actionName = "suggest_command";
            break;
        case ClickAction::CopyToClipboard:
            actionName = "copy_to_clipboard";
            break;
    }

    json["action"] = actionName;
    json["value"] = m_value;

    return json;
}

inline ClickEvent ClickEvent::fromJson(const nlohmann::json& json)
{
    if (!json.is_object() || !json.contains("action") || !json.contains("value")) {
        return ClickEvent();
    }

    const auto& actionStr = json["action"].get<std::string>();
    const auto& value = json["value"].get<std::string>();

    ClickAction action = ClickAction::RunCommand;
    if (actionStr == "open_url") {
        action = ClickAction::OpenUrl;
    } else if (actionStr == "open_file") {
        action = ClickAction::OpenFile;
    } else if (actionStr == "run_command") {
        action = ClickAction::RunCommand;
    } else if (actionStr == "suggest_command") {
        action = ClickAction::SuggestCommand;
    } else if (actionStr == "copy_to_clipboard") {
        action = ClickAction::CopyToClipboard;
    }

    return ClickEvent(action, value);
}

inline nlohmann::json HoverEvent::toJson() const
{
    nlohmann::json json = nlohmann::json::object();

    const char* actionName = nullptr;
    switch (m_action) {
        case HoverAction::ShowText:
            actionName = "show_text";
            break;
        case HoverAction::ShowItem:
            actionName = "show_item";
            break;
        case HoverAction::ShowEntity:
            actionName = "show_entity";
            break;
    }

    json["action"] = actionName;
    json["value"] = m_value;

    return json;
}

inline HoverEvent HoverEvent::fromJson(const nlohmann::json& json)
{
    if (!json.is_object() || !json.contains("action") || !json.contains("value")) {
        return HoverEvent();
    }

    const auto& actionStr = json["action"].get<std::string>();
    const auto& value = json["value"].get<std::string>();

    if (actionStr == "show_text") {
        return HoverEvent::showText(value);
    } else if (actionStr == "show_item") {
        return HoverEvent::showItem(value);
    } else if (actionStr == "show_entity") {
        return HoverEvent::showEntity(value);
    }

    return HoverEvent();
}

} // namespace mc::text
