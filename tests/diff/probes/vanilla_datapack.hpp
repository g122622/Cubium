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

/**
 * @file vanilla_datapack.hpp
 * @brief 探针共用的「原版数据包定位 + 注册表初始化」。
 *
 * 探针不启动服务端，但数据驱动的注册表（方块、物品、标签、附魔……）必须先就位，
 * 否则按资源名解析的数据全部落空——表现为「加载成功但内容为空」，差分毫无意义。
 */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc::diff::datapack {

inline void loadDataPackItemTags(const std::filesystem::path& itemTagDirectory);

class EnchantmentTags;

/// 数据包根目录（含 `data/` 的那一层）。由 harness 通过环境变量传入。
[[nodiscard]] inline std::filesystem::path root()
{
    const char* path = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (path == nullptr || *path == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    return std::filesystem::path(path);
}

/**
 * @brief 递归列出目录下的全部 `.json`，按路径字典序返回
 *
 * 顺序必须稳定：探针的 `list` 输出会与原版侧逐行比对。
 */
[[nodiscard]] inline std::vector<std::filesystem::path> jsonFiles(const std::filesystem::path& directory)
{
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(directory)) {
        return files;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

/**
 * @brief 数据包附魔标签表（`<pack>/data/minecraft/tags/enchantment`）。
 *
 * 原版 1.21 起附魔标签只存在于数据包里；条件谓词的 `#tag` 写法（例如
 * `#minecraft:smelts_loot`）靠它判断成员。与服务端启动流程一致：先登记全部文件，
 * 再统一展开 `#other_tag` 引用，最终得到传递闭包。
 */
class EnchantmentTags {
public:
    /// 从数据包目录载入；目录不存在时表为空（标签是可选数据，不报错）。
    static void load(const std::filesystem::path& directory)
    {
        collect(directory);
        for (const auto& [name, unused] : raw()) {
            static_cast<void>(resolved(name));
        }
    }

    /// 标签名 → 成员附魔 ID（已展开嵌套引用）；未知标签返回空。
    [[nodiscard]] static std::vector<std::string> members(const std::string& tagId)
    {
        const auto iter = memberships().find(normalize(tagId));
        return iter == memberships().end() ? std::vector<std::string>{} : iter->second;
    }

private:
    /// 原始成员（可能含 `#tag` 引用）与「正在展开」标记（用于断开环）。
    struct RawTag {
        std::vector<std::string> entries;
        bool resolving = false;
    };

    static std::unordered_map<std::string, std::vector<std::string>>& memberships()
    {
        static std::unordered_map<std::string, std::vector<std::string>> s_memberships;
        return s_memberships;
    }

    static std::unordered_map<std::string, RawTag>& raw()
    {
        static std::unordered_map<std::string, RawTag> s_raw;
        return s_raw;
    }

    static std::string normalize(const std::string& name)
    {
        return name.find(':') == std::string::npos ? "minecraft:" + name : name;
    }

    static void collect(const std::filesystem::path& directory)
    {
        for (const auto& file : jsonFiles(directory)) {
            std::ifstream stream(file);
            nlohmann::json contents;
            try {
                stream >> contents;
            }
            catch (const nlohmann::json::parse_error& error) {
                std::fprintf(stderr, "enchantment tags: %s: %s\n", file.filename().string().c_str(), error.what());
                continue;
            }
            if (!contents.contains("values") || !contents["values"].is_array()) {
                continue;
            }
            std::string relative = std::filesystem::relative(file, directory).generic_string();
            relative.resize(relative.size() - 5); // 去掉 ".json"
            RawTag& tag = raw()[normalize("minecraft:" + relative)];
            tag.entries.clear();
            for (const auto& value : contents["values"]) {
                // 成员既可以是字符串，也可以是 `{"id": ..., "required": false}` 对象。
                std::string id;
                if (value.is_string()) {
                    id = value.get<std::string>();
                } else if (value.is_object() && value.contains("id") && value["id"].is_string()) {
                    id = value["id"].get<std::string>();
                } else {
                    continue;
                }
                if (!id.empty() && id.front() == '#') {
                    tag.entries.push_back("#" + normalize(id.substr(1)));
                } else {
                    tag.entries.push_back(normalize(id));
                }
            }
        }
    }

    /// 递归展开 `#tag` 引用；环上的边按空成员处理，保证结果确定。
    static const std::vector<std::string>& resolved(const std::string& tagId)
    {
        const std::string normalized = normalize(tagId);
        auto& cache = memberships();
        if (const auto iter = cache.find(normalized); iter != cache.end()) {
            return iter->second;
        }
        std::vector<std::string> flattened;
        const auto rawIter = raw().find(normalized);
        if (rawIter != raw().end() && !rawIter->second.resolving) {
            rawIter->second.resolving = true;
            for (const auto& entry : rawIter->second.entries) {
                if (entry.empty() || entry.front() != '#') {
                    flattened.push_back(entry);
                    continue;
                }
                const auto& nested = resolved(entry.substr(1));
                flattened.insert(flattened.end(), nested.begin(), nested.end());
            }
            rawIter->second.resolving = false;
            // 只去重、不排序：原版标签的成员顺序就是数据包里声明的顺序（嵌套引用在出现的位置展开），
            // 而 `getRandomElementOf(tag)` 之类的逻辑按下标抽取成员，排序会让抽取结果整体错位。
            std::vector<std::string> unique;
            unique.reserve(flattened.size());
            for (auto& member : flattened) {
                if (std::find(unique.begin(), unique.end(), member) == unique.end()) {
                    unique.push_back(std::move(member));
                }
            }
            flattened = std::move(unique);
        }
        return cache.emplace(normalized, std::move(flattened)).first->second;
    }
};

/**
 * @brief 按 `RegistryBootstrap` 的顺序初始化静态注册表，并载入数据包标签
 *
 * 顺序有意义：方块物品要在物品注册后建立、物品标签要引用已注册的物品。
 * 物品标签来自数据包 `data/minecraft/tags/item/*.json`——原版的标签表就来自数据包。
 */
inline void initializeRegistries()
{
    VanillaBlocks::initialize();
    BlockTags::initialize();
    Items::initialize();
    item::enchant::EnchantmentRegistry::initialize();
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    entity::VanillaEntities::registerAll();
    // 数据包是物品标签的唯一来源（与服务端启动流程一致），不启用内置快照。
    loadDataPackItemTags(root() / "data" / "minecraft" / "tags" / "item");
    EnchantmentTags::load(root() / "data" / "minecraft" / "tags" / "enchantment");
}

/**
 * @brief 载入数据包物品标签目录（`<pack>/data/minecraft/tags/item`）
 *
 * 与服务端 `ItemTagLoader` 走同一套合并 / 应用 / 展开逻辑：文件里的 `#tag` 引用、
 * `{id, required}` 对象、`replace` 语义都与运行时一致。
 */
inline void loadDataPackItemTags(const std::filesystem::path& itemTagDirectory)
{
    std::unordered_map<ResourceLocation, nlohmann::json> compiled;
    for (const auto& file : jsonFiles(itemTagDirectory)) {
        std::ifstream stream(file);
        std::string contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        const std::string relative = std::filesystem::relative(file, itemTagDirectory).generic_string();
        const ResourceLocation location("minecraft:" + relative.substr(0, relative.size() - 5));
        auto tagJson = item::tag::ItemTags::compileTagJson(contents, location);
        if (tagJson.failed()) {
            std::fprintf(stderr, "item tags: %s\n", tagJson.error().message().c_str());
            continue;
        }
        compiled[location] = std::move(tagJson).value();
    }

    for (const auto& [location, unused] : compiled) {
        if (item::tag::ItemTags::getTag(location) == nullptr) {
            item::tag::ItemTags::registerTag(location);
        }
    }
    // 探针只注册数据包用得到的物品：未注册的必需成员在这里是预期内的缺口，记一条
    // 错误（与原版重载失败同一口径）后继续——否则所有依赖标签的探针都会停摆。
    for (const auto& [location, contents] : compiled) {
        if (auto applied = item::tag::ItemTags::applyTagJson(contents, location); applied.failed()) {
            std::fprintf(stderr, "item tags: %s\n", applied.error().message().c_str());
        }
    }
    if (auto linked = item::tag::ItemTags::linkAndExpand(); linked.failed()) {
        std::fprintf(stderr, "item tags: %s\n", linked.error().message().c_str());
    }
}

} // namespace mc::diff::datapack
