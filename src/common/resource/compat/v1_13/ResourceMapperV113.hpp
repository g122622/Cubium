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

#include "../ResourceMapper.hpp"

namespace mc {
namespace resource {
namespace compat {
namespace v1_13 {

/**
 * @brief MC 1.13+ 资源包的资源映射器
 *
 * MC 1.13+ 使用现代命名约定:
 * - 纹理路径: textures/block/, textures/item/
 * - 纹理名称: jungle_log, white_wool, granite 等
 * - 模型路径: models/block/, models/item/
 *
 * 此映射器本质上是直通的，但提供旧版名称回退以实现兼容性。
 */
class ResourceMapperV113 : public BaseResourceMapper {
public:
    ResourceMapperV113() = default;
    ~ResourceMapperV113() override = default;

    // -------------------------------------------------------------------------
    // 纹理路径转换
    // -------------------------------------------------------------------------

    std::string toUnifiedTexturePath(std::string_view path) const override
    {
        // 现代路径已经是统一的
        return std::string(path);
    }

    std::vector<std::string> getTexturePathVariants(std::string_view unifiedPath) const override;

    std::string toModernTextureName(std::string_view name) const override
    {
        // 已经是现代格式
        return std::string(name);
    }

    std::string toLegacyTextureName(std::string_view name) const override;

    // -------------------------------------------------------------------------
    // 模型路径转换
    // -------------------------------------------------------------------------

    std::string toUnifiedModelPath(std::string_view path) const override
    {
        // 现代模型路径已经是统一的
        return std::string(path);
    }

    std::vector<std::string> getModelPathVariants(std::string_view unifiedPath) const override
    {
        // 只返回统一路径
        return {std::string(unifiedPath)};
    }

    // -------------------------------------------------------------------------
    // 方块状态路径转换
    // -------------------------------------------------------------------------

    std::string toUnifiedBlockStatePath(std::string_view path) const override
    {
        // 现代方块状态路径已经是统一的
        return std::string(path);
    }

    // -------------------------------------------------------------------------
    // 包格式
    // -------------------------------------------------------------------------

    PackFormat getTargetFormat() const override { return PackFormat::V1_13_to_1_14; }
};

} // namespace v1_13
} // namespace compat
} // namespace resource
} // namespace mc
