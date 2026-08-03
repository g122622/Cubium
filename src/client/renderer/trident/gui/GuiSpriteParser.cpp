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

#include "GuiSpriteParser.hpp"
#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <exception>
#include <string>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::client::renderer::trident::gui {

namespace {
using json = nlohmann::json;
} // namespace

Result<GuiSpriteDefinition> GuiSpriteParser::parse(const std::string& jsonContent, i32 atlasWidth, i32 atlasHeight)
{

    GuiSpriteDefinition result;

    try {
        json root = json::parse(jsonContent);

        // 解析纹理路径
        if (root.contains("texture")) {
            result.texture = root["texture"].get<std::string>();
        }

        // 解析精灵
        if (root.contains("sprites") && root["sprites"].is_object()) {
            for (const auto& [id, spriteObj] : root["sprites"].items()) {
                auto spriteResult = _parseSprite(id, &spriteObj, atlasWidth, atlasHeight);
                if (spriteResult.success()) {
                    result.sprites[id] = spriteResult.value();
                }
            }
        }

        // 解析九宫格
        if (root.contains("nine_patch") && root["nine_patch"].is_object()) {
            for (const auto& [id, ninePatchObj] : root["nine_patch"].items()) {
                auto ninePatchResult = _parseNinePatch(&ninePatchObj);
                if (ninePatchResult.success()) {
                    result.ninePatches[id] = ninePatchResult.value();

                    // 将九宫格数据关联到对应的精灵
                    auto it = result.sprites.find(id);
                    if (it != result.sprites.end()) {
                        it->second.ninePatch = ninePatchResult.value();
                    }
                }
            }
        }

        // 将状态变体关联到精灵
        // 支持格式: "button": { "normal": "button_normal", "hover": "button_hover", "disabled": "button_disabled" }
        if (root.contains("state_variants") && root["state_variants"].is_object()) {
            for (const auto& [baseId, variants] : root["state_variants"].items()) {
                if (!variants.is_object()) continue;

                auto baseIt = result.sprites.find(baseId);
                if (baseIt == result.sprites.end()) continue;

                if (variants.contains("hover")) {
                    baseIt->second.hoverSprite = variants["hover"].get<std::string>();
                }
                if (variants.contains("disabled")) {
                    baseIt->second.disabledSprite = variants["disabled"].get<std::string>();
                }
            }
        }

        return result;
    }
    catch (const json::parse_error& e) {
        return Error(ErrorCode::ResourceParseError, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError, std::string("Error parsing sprite definition: ") + e.what());
    }
}

Result<GuiSpriteDefinition> GuiSpriteParser::parseFromResourcePack(
    IResourcePack& resourcePack, const std::string& spriteDefPath, i32 atlasWidth, i32 atlasHeight)
{

    // 构建资源路径
    std::string resourcePath = spriteDefPath;
    if (resourcePath.find(':') != std::string::npos) {
        // 转换 minecraft:gui/sprites/widgets.json -> assets/minecraft/gui/sprites/widgets.json
        auto colonPos = resourcePath.find(':');
        std::string namespace_ = resourcePath.substr(0, colonPos);
        std::string path = resourcePath.substr(colonPos + 1);
        resourcePath = namespace_ + "/" + path;
    }

    // 检查资源是否存在
    if (!resourcePack.hasResource(resource::PackType::ClientResources, resourcePath)) {
        return Error(ErrorCode::NotFound, std::string("Sprite definition not found: ") + spriteDefPath);
    }

    // 读取资源
    auto readResult = resourcePack.readTextResource(resource::PackType::ClientResources, resourcePath);
    if (readResult.failed()) {
        return Error(ErrorCode::FileReadFailed, std::string("Failed to read sprite definition: ") + spriteDefPath);
    }

    return parse(readResult.value(), atlasWidth, atlasHeight);
}

Result<GuiSprite> GuiSpriteParser::_parseSprite(
    const std::string& id, const void* jsonObj, i32 atlasWidth, i32 atlasHeight)
{

    const json& obj = *static_cast<const json*>(jsonObj);

    if (!obj.is_object()) {
        return Error(ErrorCode::ResourceParseError, std::string("Sprite '") + id + "' is not an object");
    }

    // 必需字段
    if (!obj.contains("x") || !obj.contains("y") || !obj.contains("width") || !obj.contains("height")) {
        return Error(ErrorCode::ResourceParseError,
            std::string("Sprite '") + id + "' missing required fields (x, y, width, height)");
    }

    i32 x = obj["x"].get<i32>();
    i32 y = obj["y"].get<i32>();
    i32 width = obj["width"].get<i32>();
    i32 height = obj["height"].get<i32>();

    // 创建精灵
    GuiSprite sprite(id, x, y, width, height, atlasWidth, atlasHeight);

    // 可选字段：悬停和禁用状态变体
    if (obj.contains("hover")) {
        sprite.hoverSprite = obj["hover"].get<std::string>();
    }
    if (obj.contains("disabled")) {
        sprite.disabledSprite = obj["disabled"].get<std::string>();
    }

    return sprite;
}

Result<GuiNinePatch> GuiSpriteParser::_parseNinePatch(const void* jsonObj)
{
    const json& obj = *static_cast<const json*>(jsonObj);

    if (!obj.is_object()) {
        return Error(ErrorCode::ResourceParseError, "Nine-patch is not an object");
    }

    GuiNinePatch ninePatch;

    if (obj.contains("left")) {
        ninePatch.left = obj["left"].get<i32>();
    }
    if (obj.contains("top")) {
        ninePatch.top = obj["top"].get<i32>();
    }
    if (obj.contains("right")) {
        ninePatch.right = obj["right"].get<i32>();
    }
    if (obj.contains("bottom")) {
        ninePatch.bottom = obj["bottom"].get<i32>();
    }

    return ninePatch;
}

} // namespace mc::client::renderer::trident::gui
