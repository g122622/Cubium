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
 * @file enchantment_tags.cpp
 * @brief 差分能力：附魔标签（数据包 `data/minecraft/tags/enchantment/` 下的 JSON）。
 *
 * `list`：全部标签 ID（排序）
 * `run` ：{ "tag": "minecraft:treasure" }
 *         → { "tag": "...", "members": ["minecraft:binding_curse", ...] }
 *
 * 纯数据查表：每个标签包含哪些附魔。成员**排序后**比较——原版标签是集合，
 * 顺序没有语义（唯一例外是 `tooltip_order`，它的顺序决定提示框排列，但那属于
 * 另一个能力要管的事，这里只比集合）。
 *
 * 这批标签承担了附魔的获取途径与行为开关：附魔台能不能刷出、能不能交易、战利品与
 * 生物装备各自允许哪些、某些方块行为是否被抑制。漏一个成员或错放一个成员，
 * 在玩法上表现为「某个附魔莫名刷不出来」或「某个附魔莫名出现在交易里」，很难靠
 * 单点测试发现。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/enchantment/EnchantmentTag.hpp"
#include "common/item/enchantment/EnchantmentTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 确保附魔标签已初始化并**从数据包填充成员**。
 *
 * 附魔标签没有任何硬编码默认值，成员全部来自数据包。只调 `initialize()` 会得到
 * 15 个空标签——表现为「能力全绿但毫无信息量」，这个坑在 entity_type_tags 上踩过。
 *
 * `EnchantmentRegistry` 必须先初始化：加载器的成员解析回调要用 `has()` 校验附魔
 * 是否存在，注册表没初始化就会把所有成员判成不存在。
 */
void ensureTags()
{
    static bool s_initialized = false;
    if (s_initialized) {
        return;
    }
    if (!item::enchant::EnchantmentRegistry::isInitialized()) {
        item::enchant::EnchantmentRegistry::initialize();
    }
    EnchantmentTags::initialize();

    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    // 仓库扫描的是「包含若干数据包的目录」，环境变量给的是数据包本身那一层，故取上一级。
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("enchantment_tags: 数据包扫描失败：" + packRoot.string());
    }

    const auto loadResult = EnchantmentTags::loadFromDataPackRepository(*repository);
    if (loadResult.failed()) {
        throw std::runtime_error("enchantment_tags: 附魔标签加载失败：" + loadResult.error().toString());
    }
    s_initialized = true;
}

void writeList(FILE* out)
{
    ensureTags();
    std::vector<std::string> names;
    EnchantmentTags::forEachTag([&](EnchantmentTag& tag) {
        names.push_back(tag.getId().toString());
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureTags();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("tag").get<std::string>();

    EnchantmentTag* tag = EnchantmentTags::getTag(ResourceLocation(requested));
    if (tag == nullptr) {
        return json{{"tag", requested}, {"missing", true}}.dump();
    }

    std::vector<std::string> members;
    members.reserve(tag->getEnchantmentIds().size());
    for (const auto& id : tag->getEnchantmentIds()) {
        members.push_back(id.toString());
    }
    // 集合语义：排序后再输出。
    std::sort(members.begin(), members.end());

    return json{{"tag", requested}, {"members", std::move(members)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("enchantment_tags", &writeList, &runLine)

} // namespace mc::diff
