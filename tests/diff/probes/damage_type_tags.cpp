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
 * @file damage_type_tags.cpp
 * @brief 差分能力：伤害类型标签（数据包 `data/<ns>/tags/damage_type/`）。
 *
 * `list`：全部标签 ID（排序）
 * `run` ：{ "tag": "minecraft:is_fire" }
 *         → { "tag": "...", "members": ["minecraft:in_fire", "minecraft:on_fire", ...] }
 *
 * 纯数据查表：每个标签包含哪些伤害类型。成员**排序后**比较——原版标签是集合，
 * 顺序没有语义。
 *
 * 这批标签几乎全是玩法开关：能不能穿甲（`bypasses_armor`）、是不是摔落伤害
 * （`is_fall`，影响落地保护与摔落靴）、会不会烧装备架（`burns_armor_stands`）、
 * 是否触发守卫者荆棘（`avoids_guardian_thorns`）……漏一个成员就有一类伤害行为走偏。
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
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTag.hpp"
#include "common/entity/damage/tag/DamageTypeTagLoader.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 确保伤害类型标签已初始化并**从数据包填充成员**。
 *
 * 只调 `initialize()` 会得到空标签（这个坑在 entity_type_tags / enchantment_tags 上
 * 都踩过），必须接着走加载器。
 */
void ensureTags()
{
    static bool s_initialized = false;
    if (s_initialized) {
        return;
    }
    DamageTypeTags::initialize();

    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    // 仓库扫描的是「包含若干数据包的目录」，环境变量给的是数据包本身那一层，故取上一级。
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("damage_type_tags: 数据包扫描失败：" + packRoot.string());
    }

    const auto loadResult = DamageTypeTagLoader::loadFromDataPackRepository(*repository);
    if (loadResult.failed()) {
        throw std::runtime_error("damage_type_tags: 伤害类型标签加载失败：" + loadResult.error().toString());
    }
    s_initialized = true;
}

void writeList(FILE* out)
{
    ensureTags();
    std::vector<std::string> names;
    DamageTypeTags::forEachTag([&](DamageTypeTag& tag) {
        names.push_back(tag.getId().toString());
    });
    // 注册表内部无序，排序后再输出，两侧才可比。
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

    DamageTypeTag* tag = DamageTypeTags::getTag(ResourceLocation(requested));
    if (tag == nullptr) {
        return json{{"tag", requested}, {"missing", true}}.dump();
    }

    std::vector<std::string> members;
    members.reserve(tag->getDamageTypes().size());
    for (const DamageType type : tag->getDamageTypes()) {
        members.push_back(DamageTypeNames::getResourceLocation(type).toString());
    }
    // 集合语义：排序后再输出。
    std::sort(members.begin(), members.end());

    return json{{"tag", requested}, {"members", std::move(members)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("damage_type_tags", &writeList, &runLine)

} // namespace mc::diff
