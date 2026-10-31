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

/**
 * @file painting_variant.cpp
 * @brief 差分能力：画变体（数据包 `data/<ns>/painting_variant/` 下的 JSON）。
 *
 * `list`：全部变体 ID（排序）
 * `run` ：{ "variant": "minecraft:kebab" }
 *         → { "width": 1, "height": 1, "assetId": "minecraft:kebab",
 *             "titleKey": "painting.minecraft.kebab.title",
 *             "authorKey": "painting.minecraft.kebab.author" }
 *
 * 对应原版
 * `record PaintingVariant(int width, int height, Identifier assetId,
 *  Optional<Component> title, Optional<Component> author)`。
 *
 * title/author 比的是**翻译键**而不是渲染后的文本：画作标题不参与玩法，翻译键才稳定
 * 可比（唱片机歌曲能力里踩过 `getString()` 的坑）。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/decoration/PaintingVariant.hpp"
#include "common/entity/decoration/PaintingVariants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 确保画变体已从数据包加载。
 *
 * 画变体是纯数据包注册表，没有任何硬编码默认值——不加载数据包就一个变体都没有。
 */
void ensureVariants()
{
    static bool s_initialized = false;
    if (s_initialized) {
        return;
    }

    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    // 仓库扫描的是「包含若干数据包的目录」，环境变量给的是数据包本身那一层，故取上一级。
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("painting_variant: 数据包扫描失败：" + packRoot.string());
    }

    const auto loadResult = PaintingVariants::loadFromDataPackRepository(*repository);
    if (loadResult.failed()) {
        throw std::runtime_error("painting_variant: 画变体加载失败：" + loadResult.error().toString());
    }
    s_initialized = true;
}

void writeList(FILE* out)
{
    ensureVariants();
    std::vector<std::string> names;
    PaintingVariants::forEachVariant([&](const ResourceLocation& id, const PaintingVariant&) {
        names.push_back(id.toString());
    });
    // 注册表内部无序，排序后再输出，两侧才可比。
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureVariants();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("variant").get<std::string>();

    const PaintingVariant* variant = PaintingVariants::get(ResourceLocation(requested));
    if (variant == nullptr) {
        return json{{"variant", requested}, {"missing", true}}.dump();
    }

    return json{{"variant", requested},
        {"width", variant->getWidth()},
        {"height", variant->getHeight()},
        {"assetId", variant->getAssetId().toString()},
        {"titleKey", variant->getTitleKey().value_or("")},
        {"authorKey", variant->getAuthorKey().value_or("")}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("painting_variant", &writeList, &runLine)

} // namespace mc::diff
