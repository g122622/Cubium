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
 * @file entity_type_tags.cpp
 * @brief 差分能力：实体类型标签（`#minecraft:undead` 等）。
 *
 * `list`：全部标签 ID（排序）
 * `run` ：{ "tag": "minecraft:undead" }
 *         → { "tag": "...", "members": ["minecraft:zombie", ...] }
 *
 * 纯数据查表：每个标签包含哪些实体类型。成员**排序后**比较——原版标签是
 * `Set<EntityType>`，顺序没有任何语义。
 *
 * 标签在玩法里承担了大量分类判断（亡灵怕治疗、节肢受节肢杀手加成、日晒燃烧、
 * 可被傀儡接受礼物……），漏一个成员或错放一个成员都很难在单点测试里发现。
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
#include "common/entity/tag/EntityTypeTag.hpp"
#include "common/entity/tag/EntityTypeTagLoader.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 确保实体类型标签已初始化并**从数据包填充成员**。
 *
 * 只调 `EntityTypeTags::initialize()` 不够：它只把内建的若干标签对象建出来，成员集来自
 * 数据包文件（`data/minecraft/tags/entity_type/*.json`）。服务端启动时
 * `RegistryBootstrap` 会在 `initialize()` 之后接着调 `EntityTypeTagLoader`，
 * 探针必须复现同样两步，否则拿到的标签成员是空的——表现为「能力通过但毫无信息量」。
 */
void ensureTags()
{
    static bool s_initialized = false;
    if (s_initialized) {
        return;
    }
    EntityTypeTags::initialize();

    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    // 仓库扫描的是「包含若干数据包的目录」，环境变量给的是数据包本身那一层，故取上一级。
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("entity_type_tags: 数据包扫描失败：" + packRoot.string());
    }

    const auto loadResult = EntityTypeTagLoader::loadFromDataPackRepository(*repository);
    if (loadResult.failed()) {
        throw std::runtime_error("entity_type_tags: 实体标签加载失败：" + loadResult.error().toString());
    }
    s_initialized = true;
}

void writeList(FILE* out)
{
    ensureTags();
    std::vector<std::string> names;
    EntityTypeTags::forEachTag([&](EntityTypeTag& tag) {
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

    EntityTypeTag* tag = EntityTypeTags::getTag(ResourceLocation(requested));
    if (tag == nullptr) {
        return json{{"tag", requested}, {"missing", true}}.dump();
    }

    // 成员排序：原版标签是集合，顺序无语义。
    std::vector<std::string> members;
    members.reserve(tag->getEntityTypeIds().size());
    for (const auto& id : tag->getEntityTypeIds()) {
        members.push_back(id.toString());
    }
    std::sort(members.begin(), members.end());

    return json{{"tag", requested}, {"members", std::move(members)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("entity_type_tags", &writeList, &runLine)

} // namespace mc::diff
