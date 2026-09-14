/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

#include <string>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 结构包资源源抽象接口
 *
 * 解耦 TemplateManager 与具体包类型（基岩版 BehaviorPack / Java 数据包）。
 * 实现方按基岩版语义解析 `namespace:path` -> `structures/<namespace>/<path>.mcstructure`，
 * 返回文件原始字节。TemplateManager 经此接口读取基岩版结构资源，
 * 无需依赖 `mc::mod::bedrock::addon::BehaviorPack`（避免 worldgen 反向依赖 addon 模块）。
 *
 * 生命周期：实现方需保证在 TemplateManager 使用期间存活；TemplateManager 持非拥有指针。
 */
class IStructurePackSource {
public:
    virtual ~IStructurePackSource() = default;

    /**
     * @brief 读取结构资源字节
     * @param namespaceId 资源命名空间（如 "startertests"）
     * @param path 资源路径（如 "mediumglass"）
     * @return 文件字节内容；文件不存在或读取失败返回失败 Result
     */
    [[nodiscard]] virtual Result<std::vector<u8>> readStructure(
        const std::string& namespaceId, const std::string& path) const = 0;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
