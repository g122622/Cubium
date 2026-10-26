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
#include <stdexcept>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc::diff::datapack {

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
 * @brief 按 `RegistryBootstrap` 的顺序初始化静态注册表
 *
 * 顺序有意义：方块标签引用方块、物品标签引用物品、方块物品引用方块与物品。
 */
inline void initializeRegistries()
{
    VanillaBlocks::initialize();
    BlockTags::initialize();
    Items::initialize();
    item::tag::ItemTags::initialize();
    item::enchant::EnchantmentRegistry::initialize();
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    entity::VanillaEntities::registerAll();
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

} // namespace mc::diff::datapack