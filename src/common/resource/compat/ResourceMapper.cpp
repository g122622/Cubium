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

#include "ResourceMapper.hpp"
#include "v1_12/ResourceMapperV112.hpp"
#include "v1_13/ResourceMapperV113.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace resource {
namespace compat {

bool BaseResourceMapper::hasResourceVariant(const IResourcePack& pack, std::string_view unifiedPath) const
{
    // 先尝试纹理路径
    auto texPaths = getTexturePathVariants(unifiedPath);
    for (const auto& path : texPaths) {
        if (pack.hasResource(path)) {
            return true;
        }
    }

    // 尝试模型路径
    auto modelPaths = getModelPathVariants(unifiedPath);
    for (const auto& path : modelPaths) {
        if (pack.hasResource(path)) {
            return true;
        }
    }

    // 尝试原样
    std::string filePath = ResourceLocation(unifiedPath).toFilePath();
    return pack.hasResource(filePath);
}

Result<std::vector<u8>> BaseResourceMapper::readResourceVariant(
    const IResourcePack& pack, std::string_view unifiedPath) const
{
    // 先尝试纹理路径
    auto texPaths = getTexturePathVariants(unifiedPath);
    auto result = tryReadFromPaths(pack, texPaths);
    if (result.success()) {
        return result;
    }

    // 尝试模型路径
    auto modelPaths = getModelPathVariants(unifiedPath);
    result = tryReadFromPaths(pack, modelPaths);
    if (result.success()) {
        return result;
    }

    // 尝试原样
    std::string filePath = ResourceLocation(unifiedPath).toFilePath();
    if (pack.hasResource(filePath)) {
        return pack.readResource(filePath);
    }

    return Error(ErrorCode::ResourceNotFound, "未找到资源: " + std::string(unifiedPath));
}

Result<std::vector<u8>> BaseResourceMapper::tryReadFromPaths(
    const IResourcePack& pack, const std::vector<std::string>& paths) const
{
    for (const auto& path : paths) {
        if (pack.hasResource(path)) {
            auto result = pack.readResource(path);
            if (result.success()) {
                return result;
            }
        }
    }
    return Error(ErrorCode::ResourceNotFound, "在任何变体路径中均未找到资源");
}

std::unique_ptr<ResourceMapper> ResourceMapper::create(PackFormat format)
{
    switch (format) {
        case PackFormat::V1_6_to_1_8:
        case PackFormat::V1_9_to_1_10:
        case PackFormat::V1_11_to_1_12:
            return std::make_unique<v1_12::ResourceMapperV112>();

        case PackFormat::V1_13_to_1_14:
        case PackFormat::V1_15_to_1_16_1:
        case PackFormat::V1_16_2_to_1_16_5:
        case PackFormat::V1_17:
        case PackFormat::V1_18:
        case PackFormat::V1_19:
            return std::make_unique<v1_13::ResourceMapperV113>();

        default:
            // 未知格式默认使用现代映射器
            spdlog::warn("未知的包格式 {}，默认使用 1.13+ 映射器", static_cast<i32>(format));
            return std::make_unique<v1_13::ResourceMapperV113>();
    }
}

} // namespace compat
} // namespace resource
} // namespace mc
