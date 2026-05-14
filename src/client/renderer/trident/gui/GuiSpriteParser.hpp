#pragma once

#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <unordered_map>
#include <vector>

namespace mc {
class IResourcePack;
}

namespace mc::client::renderer::trident::gui {

/**
 * @brief GUI纹理精灵定义文件
 *
 * 从JSON文件解析的精灵定义数据结构。
 */
struct GuiSpriteDefinition {
    std::string texture;                                       ///< 纹理路径（如 "minecraft:textures/gui/widgets.png"）
    std::unordered_map<std::string, GuiSprite> sprites;        ///< 精灵ID -> 精灵数据
    std::unordered_map<std::string, GuiNinePatch> ninePatches; ///< 精灵ID -> 九宫格数据
};

/**
 * @brief GUI精灵解析器
 *
 * 从JSON文件解析精灵定义。支持以下格式：
 *
 * @code{.json}
 * {
 *   "texture": "minecraft:textures/gui/widgets.png",
 *   "sprites": {
 *     "button_normal": { "x": 0, "y": 66, "width": 200, "height": 20 },
 *     "button_hover": { "x": 0, "y": 86, "width": 200, "height": 20 }
 *   },
 *   "nine_patch": {
 *     "button_normal": { "left": 4, "top": 4, "right": 196, "bottom": 16 }
 *   }
 * }
 * @endcode
 */
class GuiSpriteParser {
public:
    /**
     * @brief 从JSON字符串解析精灵定义
     * @param jsonContent JSON内容
     * @param atlasWidth 图集宽度（用于UV计算）
     * @param atlasHeight 图集高度（用于UV计算）
     * @return 解析结果
     */
    [[nodiscard]] static Result<GuiSpriteDefinition> parse(
        const std::string& jsonContent, i32 atlasWidth = 256, i32 atlasHeight = 256);

    /**
     * @brief 从资源包解析精灵定义
     * @param resourcePack 资源包
     * @param spriteDefPath 精灵定义文件路径（如 "minecraft:gui/sprites/widgets.json"）
     * @param atlasWidth 图集宽度
     * @param atlasHeight 图集高度
     * @return 解析结果
     */
    [[nodiscard]] static Result<GuiSpriteDefinition> parseFromResourcePack(
        IResourcePack& resourcePack, const std::string& spriteDefPath, i32 atlasWidth = 256, i32 atlasHeight = 256);

private:
    /**
     * @brief 解析精灵对象
     */
    [[nodiscard]] static Result<GuiSprite> parseSprite(
        const std::string& id, const void* jsonObj, i32 atlasWidth, i32 atlasHeight);

    /**
     * @brief 解析九宫格对象
     */
    [[nodiscard]] static Result<GuiNinePatch> parseNinePatch(const void* jsonObj);
};

} // namespace mc::client::renderer::trident::gui
