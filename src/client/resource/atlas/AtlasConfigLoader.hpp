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

#include "client/resource/atlas/AtlasSource.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <memory>
#include <vector>

namespace mc::client::resource::atlas {

/**
 * @brief atlas 配置加载器
 *
 * 对齐原版 SpriteSourceList.load(rm, atlasId)：
 * 遍历资源包列表（正序=低→高优先级），对每个包读取
 * assets/<ns>/atlases/<atlasId.path()>.json，解析 sources 并 **拼接**（addAll 不覆盖）。
 * 单个包缺失 atlas JSON 不算错误（低优先级包可不提供），单个包解析失败只记日志不中断。
 *
 * sprite 级覆盖语义不在本层处理，而在 SpriteSourceOutput::build() 阶段（后 add 覆盖先 add）。
 */
struct AtlasConfigLoader {
    /**
     * @brief 从资源包列表加载指定 atlas 的全部 sources（多包拼接）
     *
     * @param packs 资源包列表（正序：靠前=低优先级，靠后=高优先级）
     * @param atlasId atlas 资源位置（如 minecraft:blocks），其 path() 用于定位
     *                assets/<ns>/atlases/<path>.json
     * @return 拼接后的 source 列表；任何包都没提供该 atlas 时返回空列表（非错误）
     */
    [[nodiscard]] static Result<std::vector<std::unique_ptr<AtlasSource>>> load(
        const std::vector<ResourcePackPtr>& packs, const ResourceLocation& atlasId);
};

} // namespace mc::client::resource::atlas
