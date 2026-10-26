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
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentPatchNbt.hpp"
#include "common/item/component/DataComponentType.hpp"
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

namespace {

/// 资源 ID → 约定纹理路径：`<ns>:gui/...` → `<ns>:textures/gui/....png`
ResourceLocation _texturePath(const std::string& assetId)
{
    const auto colon = assetId.find(':');
    const std::string namespaceName = colon == std::string::npos ? "minecraft" : assetId.substr(0, colon);
    const std::string path = colon == std::string::npos ? assetId : assetId.substr(colon + 1);
    return ResourceLocation(namespaceName, "textures/" + path + ".png");
}

/// icon 的 `components`：`DataComponentPatch.CODEC` 的 JSON 形态（`!` 前缀表移除）
item::component::DataComponentPatch _parsePatch(const nlohmann::json& json)
{
    item::component::DataComponentPatch patch;
    for (const auto& [key, value] : json.items()) {
        const bool removed = !key.empty() && key[0] == '!';
        const std::string name = removed ? key.substr(1) : key;
        const auto type = item::component::componentTypeByName(name);
        if (!type.has_value()) {
            continue;
        }
        if (removed) {
            patch.remove(*type);
            continue;
        }
        item::component::DataComponentPayload payload{};
        if (item::component::jsonToComponentPayload(*type, value, payload)) {
            patch.add(*type, std::move(payload));
        }
    }
    return patch;
}

} // namespace

AdvancementDisplay::AdvancementDisplay(ItemStack icon,
    std::unique_ptr<text::ITextComponent> title,
    std::unique_ptr<text::ITextComponent> description,
    AdvancementFrame frame,
    bool showToast,
    bool announceToChat,
    bool hidden,
    std::optional<BackgroundTexture> background)
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
    //
    // 原版 1.21 起图标是 `ItemStack.CODEC` 的 JSON 形态：`{"id": "minecraft:stone",
    // "count": 1, "components": {...}}`（数量缺省 1，组件是本项目 DataComponentPatch 的
    // 同一种写法）。1.20 及更早的 `{"item": ..., "nbt": ...}` 形态在 1.21 已被移除。
    ItemStack icon;
    if (json.contains("icon")) {
        const auto& iconJson = json["icon"];
        if (iconJson.is_string()) {
            // 简写：直接给物品 ID
            auto* item = ItemRegistry::instance().getItem(ResourceLocation(iconJson.get<std::string>()));
            if (item != nullptr) {
                icon = ItemStack(item, 1);
            }
        } else if (iconJson.is_object() && iconJson.contains("id") && iconJson["id"].is_string()) {
            auto* item = ItemRegistry::instance().getItem(ResourceLocation(iconJson["id"].get<std::string>()));
            if (item != nullptr) {
                icon = ItemStack(item, iconJson.value("count", 1));
                // 组件补丁（`{"minecraft:banner_patterns": [...]}` 等）：与 NBT 共用同一解码路径
                if (iconJson.contains("components") && iconJson["components"].is_object()) {
                    icon.applyComponentPatch(_parsePatch(iconJson["components"]));
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
    //
    // 原版用 `ClientAsset.ResourceTexture` 解码：字符串形态的 `background` 是资源 ID，
    // 纹理路径由约定补全为 `textures/<path>.png`（`gui/advancements/backgrounds/stone`
    // → `textures/gui/advancements/backgrounds/stone.png`）。
    std::optional<BackgroundTexture> background;
    if (json.contains("background")) {
        const std::string assetId = json["background"].get<std::string>();
        background = BackgroundTexture{ResourceLocation(assetId), _texturePath(assetId)};
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

    // 图标：与原版 `ItemStack.CODEC` 同一种写法（`id` / `count` / `components`）。
    if (m_icon.isEmpty()) {
        json["icon"] = {};
    } else {
        nlohmann::json iconJson;
        iconJson["id"] = m_icon.getItem()->itemLocation().toString();
        iconJson["count"] = m_icon.getCount();
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

    // 背景：字符串形态只写资源 ID（纹理路径按约定可推导）
    if (m_background.has_value()) {
        json["background"] = m_background->id.toString();
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
