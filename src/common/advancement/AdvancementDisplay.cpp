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

#include "AdvancementDisplay.hpp"
#include "common/advancement/AdvancementFrame.hpp"
#include "common/core/Result.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

AdvancementDisplay::AdvancementDisplay(ItemStack icon,
    std::unique_ptr<text::ITextComponent> title,
    std::unique_ptr<text::ITextComponent> description,
    AdvancementFrame frame,
    bool showToast,
    bool announceToChat,
    bool hidden,
    std::optional<ResourceLocation> background)
    : m_icon(std::move(icon))
    , m_title(std::move(title))
    , m_description(std::move(description))
    , m_frame(frame)
    , m_showToast(showToast)
    , m_announceToChat(announceToChat)
    , m_hidden(hidden)
    , m_background(std::move(background))
{}

Result<AdvancementDisplay> AdvancementDisplay::fromJson(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError, "Display must be a JSON object");
    }

    // 解析图标
    ItemStack icon;
    if (json.contains("icon")) {
        const auto& iconJson = json["icon"];
        if (iconJson.is_string()) {
            // 简单格式："icon": "minecraft:diamond"
            ResourceLocation itemId(iconJson.get<std::string>());
            auto* item = ItemRegistry::instance().getItem(itemId);
            if (item) {
                icon = ItemStack(item, 1);
            }
        } else if (iconJson.is_object()) {
            // 完整格式："icon": {"item": "minecraft:diamond", "nbt": "{...}"}
            if (iconJson.contains("item")) {
                ResourceLocation itemId(iconJson["item"].get<std::string>());
                auto* item = ItemRegistry::instance().getItem(itemId);
                if (item) {
                    icon = ItemStack(item, 1);
                }
            }
            // 解析NBT数据应用到物品栈
            if (iconJson.contains("nbt")) {
                if (iconJson["nbt"].is_string()) {
                    // Mojangson 字符串格式
                    auto parsedTag = nbt::parseMojangson(iconJson["nbt"].get<std::string>());
                    if (parsedTag) {
                        nlohmann::json jsonTag = nbt::nbtToJson(*parsedTag);
                        if (jsonTag.is_object() && !jsonTag.empty()) {
                            icon.mergeTag(jsonTag);
                        }
                    }
                } else if (iconJson["nbt"].is_object()) {
                    // JSON 对象格式
                    auto parsedTag = nbt::jsonToNbt(iconJson["nbt"]);
                    if (parsedTag) {
                        nlohmann::json jsonTag = nbt::nbtToJson(*parsedTag);
                        if (jsonTag.is_object() && !jsonTag.empty()) {
                            icon.mergeTag(jsonTag);
                        }
                    }
                }
            }
        }
    }

    // 解析标题
    std::unique_ptr<text::ITextComponent> title;
    if (json.contains("title")) {
        title = text::ITextComponent::fromJson(json["title"]);
    } else {
        // 默认标题
        title = std::make_unique<text::StringTextComponent>("Unknown");
    }

    // 解析描述
    std::unique_ptr<text::ITextComponent> description;
    if (json.contains("description")) {
        description = text::ITextComponent::fromJson(json["description"]);
    } else {
        // 默认描述
        description = std::make_unique<text::StringTextComponent>("");
    }

    // 解析框架类型
    AdvancementFrame frame = AdvancementFrame::Task;
    if (json.contains("frame")) {
        frame = parseFrame(json["frame"].get<std::string>());
    }

    // 解析标志
    bool showToast = json.value("show_toast", true);
    bool announceToChat = json.value("announce_to_chat", true);
    bool hidden = json.value("hidden", false);

    // 解析背景（仅根成就）
    std::optional<ResourceLocation> background;
    if (json.contains("background")) {
        background = ResourceLocation(json["background"].get<std::string>());
    }

    return AdvancementDisplay(std::move(icon),
        std::move(title),
        std::move(description),
        frame,
        showToast,
        announceToChat,
        hidden,
        std::move(background));
}

nlohmann::json AdvancementDisplay::toJson() const
{
    nlohmann::json json;

    // 图标
    if (!m_icon.isEmpty()) {
        nlohmann::json iconJson;
        iconJson["item"] = m_icon.getItem()->itemLocation().toString();
        // 序列化NBT数据
        const nlohmann::json* tag = m_icon.getTag();
        if (tag && tag->is_object() && !tag->empty()) {
            // 将 JSON 标签转回 NBT 再序列化为 Mojangson 字符串
            auto nbtTag = nbt::jsonToNbt(*tag);
            if (nbtTag) {
                std::string mojangson = std::to_string(*nbtTag);
                if (!mojangson.empty() && mojangson != "{}") {
                    iconJson["nbt"] = mojangson;
                }
            }
        }
        json["icon"] = std::move(iconJson);
    }

    // 标题和描述
    if (m_title) {
        json["title"] = m_title->toJson();
    }
    if (m_description) {
        json["description"] = m_description->toJson();
    }

    // 框架类型
    json["frame"] = toString(m_frame);

    // 标志
    json["show_toast"] = m_showToast;
    json["announce_to_chat"] = m_announceToChat;
    json["hidden"] = m_hidden;

    // 背景
    if (m_background.has_value()) {
        json["background"] = m_background->toString();
    }

    return json;
}

std::unique_ptr<text::ITextComponent> AdvancementDisplay::getTitleCopy() const
{
    if (m_title) {
        return m_title->deepCopy();
    }
    return std::make_unique<text::StringTextComponent>("");
}

} // namespace mc::advancement
