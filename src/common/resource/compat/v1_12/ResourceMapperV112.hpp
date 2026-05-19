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
#include "../TextureMapper.hpp"

namespace mc {
namespace resource {
namespace compat {
namespace v1_12 {

/**
 * @brief MC 1.11-1.12.2 资源包的资源映射器
 *
 * 处理以下转换:
 * - 纹理路径: textures/blocks/ -> textures/block/
 * - 纹理名称: log_jungle -> jungle_log, wool_colored_white -> white_wool 等
 * - 模型路径: 通常不变
 *
 * 参考: net.minecraft.client.resources.LegacyResourcePackWrapper
 */
class ResourceMapperV112 : public BaseResourceMapper {
public:
    ResourceMapperV112();
    ~ResourceMapperV112() override = default;

    // -------------------------------------------------------------------------
    // 纹理路径转换
    // -------------------------------------------------------------------------

    std::string toUnifiedTexturePath(std::string_view path) const override;

    std::vector<std::string> getTexturePathVariants(std::string_view unifiedPath) const override;

    std::string toModernTextureName(std::string_view name) const override;

    std::string toLegacyTextureName(std::string_view name) const override;

    // -------------------------------------------------------------------------
    // 模型路径转换
    // -------------------------------------------------------------------------

    std::string toUnifiedModelPath(std::string_view path) const override;

    std::vector<std::string> getModelPathVariants(std::string_view unifiedPath) const override;

    // -------------------------------------------------------------------------
    // 方块状态路径转换
    // -------------------------------------------------------------------------

    std::string toUnifiedBlockStatePath(std::string_view path) const override;

    // -------------------------------------------------------------------------
    // 包格式
    // -------------------------------------------------------------------------

    PackFormat getTargetFormat() const override { return PackFormat::V1_11_to_1_12; }

private:
    const TextureMapper& m_textureMapper;
};

} // namespace v1_12
} // namespace compat
} // namespace resource
} // namespace mc
