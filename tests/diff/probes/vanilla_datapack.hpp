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
#include <memory>
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
#include "common/item/tag/ItemTagLoader.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/ZipResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/block/BlockTagLoader.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc::diff::datapack {

inline void loadDataPackItemTags();
inline void loadDataPackBlockTags();

class EnchantmentTags;

/// 数据包目录本身（含 `data/` 与 `pack.mcmeta` 的那一层）。由 harness 通过环境变量传入。
[[nodiscard]] inline std::filesystem::path root()
{
    const char* path = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (path == nullptr || *path == '\0') {
        throw std::runtime_error(
            "MC_DIFF_VANILLA_DATAPACK is not set: expected the vanilla data pack directory (the one with pack.mcmeta)");
    }
    return std::filesystem::path(path);
}

/**
 * @brief 构建数据包仓库（来源与 Java oracle 完全一致：merged jar 内嵌的 data/）
 *
 * 必须与 oracle 读同一份数据。此前用 `MC_DIFF_VANILLA_DATAPACK`（磁盘上的数据包目录）时，
 * 两侧数据不是同一构建：例如 jar 内 `small_flowers` 不含 cactus_flower / wildflowers，
 * 磁盘副本却含有，于是差分报出大量「Cubium 多出成员」的假差异。
 *
 * merged jar 是普通 zip，`ZipResourcePack` 直接按 zip 根作为包根读取，无需解压。
 */
[[nodiscard]] inline std::unique_ptr<resource::DataPackRepository> scanDataPack()
{
    const char* jarPath = std::getenv("MC_VANILLA_MERGED_JAR");
    if (jarPath == nullptr || *jarPath == '\0') {
        throw std::runtime_error("MC_VANILLA_MERGED_JAR is not set: expected the deobfuscated merged jar that the "
                                 "Java oracle also reads its data pack from");
    }
    const std::filesystem::path jar(jarPath);

    // jar 不是数据包目录也不是 .zip：把其中的 data/ 解到一个稳定的缓存目录再当数据包加载。
    // 目录名带 jar 的大小与修改时间，jar 换了就会重新解包。
    const auto jarSize = std::filesystem::file_size(jar);
    const auto jarTime = std::filesystem::last_write_time(jar).time_since_epoch().count();
    const std::filesystem::path extractRoot = std::filesystem::temp_directory_path() /
        ("cubium-diff-vanilla-" + std::to_string(static_cast<unsigned long long>(jarSize)) + "-" +
            std::to_string(static_cast<long long>(jarTime)));

    if (!std::filesystem::exists(extractRoot / "data" / "minecraft" / "tags")) {
        auto zip = resource::ZipResourcePack::create(jar);
        if (zip.failed()) {
            throw std::runtime_error(
                "failed to open the vanilla jar '" + jar.string() + "': " + zip.error().toString());
        }
        auto pack = std::move(zip).value();
        if (const auto initialized = pack->initialize(); initialized.failed()) {
            throw std::runtime_error(
                "failed to initialize the vanilla jar '" + jar.string() + "': " + initialized.error().toString());
        }
        // 空目录 = 类型目录根（ZipResourcePack 会补成 "data/" 前缀后做前缀匹配）。
        const auto entries = pack->listResources(resource::PackType::ServerData, "", ".json");
        if (entries.failed()) {
            throw std::runtime_error("failed to list the vanilla jar's data/: " + entries.error().toString());
        }
        std::size_t written = 0;
        for (const std::string& relative : entries.value()) {
            const auto contents = pack->readResource(resource::PackType::ServerData, relative);
            if (contents.failed()) {
                continue;
            }
            // listResources 返回的是相对类型目录（data/）的路径，落盘时补回 data/ 前缀。
            const std::filesystem::path target = extractRoot / "data" / relative;
            std::filesystem::create_directories(target.parent_path());
            std::ofstream targetStream(target, std::ios::binary);
            targetStream.write(reinterpret_cast<const char*>(contents.value().data()),
                static_cast<std::streamsize>(contents.value().size()));
            targetStream.close();
            ++written;
        }
        if (written == 0) {
            throw std::runtime_error("the vanilla jar contains no data/*.json entries (listed " +
                std::to_string(entries.value().size()) + ")");
        }
        // 数据包目录必须有 pack.mcmeta 才会被识别为资源包。
        std::ofstream(extractRoot / "pack.mcmeta")
            << R"({"pack": {"description": "vanilla data extracted from the diff jar", "pack_format": 94}})";
    }

    // 解包目录本身就是数据包根（含 data/ 与 pack.mcmeta），直接作为包加入；
    // scanDirectory 的语义是"扫描一个包含多个包的目录"，用在这里会漏掉它。
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto added = repository->addPack(extractRoot, true, 0);
    if (added.failed()) {
        throw std::runtime_error("failed to load the vanilla data pack extracted at '" + extractRoot.string() +
            "': " + added.error().toString());
    }
    if (!added.value().initialized) {
        throw std::runtime_error("the extracted vanilla data pack failed to initialize: " + extractRoot.string() +
            " (" + added.value().error + ")");
    }
    return repository;
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
    // VanillaBlocks::initialize 内部已完成 BlockTags::initialize（标签表现已完全由数据包
    // 构建，服务端在 RegistryBootstrap 中先 clear 再走 BlockTagLoader 重建）。
    VanillaBlocks::initialize();
    Items::initialize();
    item::enchant::EnchantmentRegistry::initialize();
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    entity::VanillaEntities::registerAll();
    // 与服务端一致：以上只注册了方块物品，另有大量普通物品只由 Items::initialize 覆盖。
    // 数据包是标签表的唯一来源（与服务端启动流程一致），不启用内置快照。
    loadDataPackBlockTags();
    loadDataPackItemTags();
    EnchantmentTags::load(root() / "data" / "minecraft" / "tags" / "enchantment");
}

/**
 * @brief 用数据包重建方块标签（`<pack>/data/<ns>/tags/block`）
 *
 * 直接走运行时加载器 `BlockTagLoader`，成员校验、`#tag` 展开、`replace` 语义与
 * 服务端启动流程完全一致。空结果视为失败：标签静默为空会让所有依赖标签的比对变成
 * 「两边都空」的假通过。
 */
inline void loadDataPackBlockTags()
{
    const auto repository = scanDataPack();
    // 数据包是标签表的唯一来源：清空后整体重建（与服务端 RegistryBootstrap 同口径）。
    BlockTags::clear();
    const auto loaded = BlockTagLoader::loadFromDataPackRepository(*repository);
    if (loaded.failed()) {
        throw std::runtime_error("block tags: " + loaded.error().toString());
    }
    if (loaded.value() == 0) {
        throw std::runtime_error("block tags: data pack contains no block tags");
    }
}

/**
 * @brief 用数据包重建物品标签（`<pack>/data/<ns>/tags/item`）
 *
 * 直接走运行时加载器 `ItemTagLoader`：合并 / 应用 / 展开、`{id, required}` 对象与
 * `replace` 语义都与服务端启动流程完全一致，探针不再自己复刻一份。
 */
inline void loadDataPackItemTags()
{
    const auto repository = scanDataPack();
    item::tag::ItemTags::clear();
    const auto loaded = item::tag::ItemTagLoader::loadFromDataPackRepository(*repository);
    if (loaded.failed()) {
        throw std::runtime_error("item tags: " + loaded.error().message());
    }
    if (loaded.value() == 0) {
        throw std::runtime_error("item tags: data pack contains no item tags");
    }
}

} // namespace mc::diff::datapack
