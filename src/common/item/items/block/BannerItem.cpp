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
 * IMPLIED, INCLUDING BY NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "item/items/block/BannerItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/WallOrFloorItem.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/interactive/BannerPattern.hpp"
#include "item/core/ItemStack.hpp"
#include "resource/LanguageManager.hpp"
#include "world/block/blocks/decorative/BannerBlock.hpp"
#include "world/blockentity/interactive/BannerEntity.hpp"
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item {

BannerItem::BannerItem(const Block& floorBlock, const Block& wallBlock, ItemProperties properties)
    : WallOrFloorItem(floorBlock, wallBlock, std::move(properties))
{}

DyeColor BannerItem::getColor() const
{
    const auto* bannerBlock = dynamic_cast<const blocks::AbstractBannerBlock*>(&block());
    if (bannerBlock != nullptr) {
        return bannerBlock->getColor();
    }
    return DyeColor::White;
}

void BannerItem::addInformation(
    const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const
{
    WallOrFloorItem::addInformation(stack, world, tooltip, advanced);

    // 从BlockEntityTag读取图案并显示（最多6层）
    // 不依赖 world，world 为 null 时仍可从 NBT 读取图案并翻译
    const nlohmann::json* tag = stack.getChildTag("BlockEntityTag");
    if (tag == nullptr || !tag->contains("Patterns")) {
        return;
    }

    const auto& patternsJson = (*tag)["Patterns"];
    if (!patternsJson.is_array()) {
        return;
    }

    i32 count = 0;
    for (const auto& patternJson : patternsJson) {
        if (count >= blockentity::BannerEntity::MAX_PATTERNS) {
            break;
        }

        if (patternJson.contains("Pattern") && patternJson.contains("Color")) {
            std::string hashName = patternJson["Pattern"].get<std::string>();
            i32 colorId = patternJson["Color"].get<i32>();
            blockentity::BannerPatternType patternType = blockentity::BannerPatterns::byHash(hashName);
            DyeColor dyeColor = static_cast<DyeColor>(colorId);

            // 生成翻译键: block.minecraft.banner.<pattern_name>.<color_name>
            // 颜色名使用小写枚举转字符串
            static const char* DYE_COLOR_NAMES[] = {"white",
                "orange",
                "magenta",
                "light_blue",
                "yellow",
                "lime",
                "pink",
                "gray",
                "light_gray",
                "cyan",
                "purple",
                "blue",
                "brown",
                "green",
                "red",
                "black"};

            std::string colorName = (colorId >= 0 && colorId < 16) ? DYE_COLOR_NAMES[colorId] : "white";

            std::string translationKey =
                "block.minecraft.banner." + blockentity::BannerPatterns::getFileName(patternType) + "." + colorName;

            tooltip.push_back(LanguageManager::instance().get(translationKey));
        }
        ++count;
    }
}

std::vector<std::pair<std::string, i32>> BannerItem::getPatternListFromStack(const ItemStack& stack)
{
    std::vector<std::pair<std::string, i32>> result;

    const nlohmann::json* tag = stack.getChildTag("BlockEntityTag");
    if (tag == nullptr || !tag->contains("Patterns")) {
        return result;
    }

    const auto& patternsJson = (*tag)["Patterns"];
    if (!patternsJson.is_array()) {
        return result;
    }

    for (const auto& patternJson : patternsJson) {
        if (patternJson.contains("Pattern") && patternJson.contains("Color")) {
            std::string hashName = patternJson["Pattern"].get<std::string>();
            i32 colorId = patternJson["Color"].get<i32>();
            result.emplace_back(hashName, colorId);
        }
    }

    return result;
}

} // namespace item
} // namespace mc
