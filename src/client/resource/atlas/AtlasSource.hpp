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
 */

#pragma once

#include "client/resource/atlas/IdentifierPattern.hpp"
#include "client/resource/atlas/SpriteLoader.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::client::resource::atlas {

/**
 * @brief sprite 累积器
 *
 * 对齐原版 SpriteSource.Output。按 sources 执行顺序累积 sprite→SpriteLoader 映射，
 * 实现原版语义：
 * - add：后执行覆盖先执行（同名 sprite，后 add 的 loader 胜出）
 * - removeAll：从已累积集合移除匹配项（filter source 用，只影响之前的 source）
 *
 * build() 产出最终唯一的 sprite 列表（已处理覆盖与移除），
 * 喂给 TextureAtlasBuilder 前保证 sprite 名唯一，
 * 规避 builder 的 m_addedLocations 去重丢弃高优先级包的同名 sprite。
 */
class SpriteSourceOutput {
public:
    /// 添加/覆盖一个 sprite（后执行覆盖先执行）
    void add(const ResourceLocation& spriteName, SpriteLoader loader);

    /// 从已累积集合移除匹配模式的 sprite
    void removeAll(const IdentifierPattern& pattern);

    /// 产出最终唯一 sprite 列表（按首次添加顺序，已应用覆盖与移除）
    [[nodiscard]] std::vector<std::pair<ResourceLocation, SpriteLoader>> build() const;

    /// 当前累积的 sprite 数量（含已被 remove 的不计）
    [[nodiscard]] size_t size() const;

private:
    struct Entry {
        ResourceLocation name;
        SpriteLoader loader;
        bool removed = false;
    };
    std::vector<Entry> m_entries;
    // sprite 名 -> m_entries 索引，用于 O(1) 覆盖
    std::unordered_map<ResourceLocation, size_t> m_index;
};

/**
 * @brief atlas 源抽象基类
 *
 * 对齐原版 SpriteSource。每个具体 source 在 run() 中执行其语义
 * （往 output add sprite 或 removeAll）。
 */
class AtlasSource {
public:
    virtual ~AtlasSource() = default;

    /**
     * @brief 在 output 上执行本 source 的语义
     *
     * @param pack 当前资源包（directory source 用 listResources 枚举纹理；
     *             unstitch/paletted 在此解码大图/调色板）
     * @param output sprite 累积器
     * @return 成功或错误（单个 source 失败不中断整体加载，由调用方决定）
     */
    [[nodiscard]] virtual Result<void> run(IResourcePack& pack, SpriteSourceOutput& output) const = 0;

    /// 用于日志/调试
    [[nodiscard]] virtual std::string describe() const = 0;
};

} // namespace mc::client::resource::atlas
