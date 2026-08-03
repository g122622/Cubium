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

#include "BlockModelLoader.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <map>
#include <memory>
#include <vector>

namespace mc {

/**
 * @brief 方块状态加载器
 *
 * 解析 blockstates 目录下的 JSON 文件，管理方块状态到模型的映射
 */
class BlockStateLoader {
public:
    BlockStateLoader() = default;

    // 从资源包加载方块状态
    [[nodiscard]] Result<void> loadFromResourcePack(IResourcePack& resourcePack);

    // 获取方块状态定义
    [[nodiscard]] const BlockStateDefinition* getBlockState(const ResourceLocation& blockId) const;

    // 清除缓存
    void clearCache();

    // 获取所有已加载的方块状态名称
    [[nodiscard]] std::vector<ResourceLocation> getLoadedBlockStates() const;

private:
    std::map<ResourceLocation, BlockStateDefinition> m_blockStates;
};

} // namespace mc
