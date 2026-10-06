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

#include "common/core/Result.hpp"

#include <cstddef>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

/**
 * @brief 流体标签 JSON 加载器
 *
 * 从数据包加载流体标签 JSON 文件（data/<namespace>/tags/fluid/），
 * 支持多数据包合并（replace/追加）、# 标签引用与 required 语义。
 *
 * 复用通用两阶段加载骨架（GenericTagLoader），成员为流体资源位置，
 * 经 FluidRegistry 校验存在性。
 *
 * 必须在 FluidTags::initialize() 之后调用，以确保内置默认标签已注册
 * （数据包在默认值之上追加或替换）。
 *
 * JSON 格式:
 * @code
 * {
 *   "replace": false,
 *   "values": ["minecraft:water", "minecraft:flowing_water"]
 * }
 * @endcode
 */
class FluidTagLoader {
public:
    FluidTagLoader() = delete;

    /**
     * @brief 从数据包仓库加载所有流体标签
     *
     * @param dataPackList 数据包仓库
     * @return 处理的标签数量，或错误信息
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有流体标签
     *
     * 不支持多数据包合并语义，适用于测试或单包加载场景。
     *
     * @param pack 资源包
     * @return 处理的标签数量，或错误信息
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);
};

} // namespace mc
