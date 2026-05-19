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

#include "ResourceMapperV112.hpp"
#include <algorithm>

namespace mc {
namespace resource {
namespace compat {
namespace v1_12 {

ResourceMapperV112::ResourceMapperV112()
    : m_textureMapper(TextureMapper::instance())
{}

std::string ResourceMapperV112::toUnifiedTexturePath(std::string_view path) const
{
    std::string result(path);

    // 将 textures/blocks/ 替换为 textures/block/
    const std::string oldPrefix = "textures/blocks/";
    const std::string newPrefix = "textures/block/";
    if (result.find(oldPrefix) == 0) {
        result = newPrefix + result.substr(oldPrefix.length());
    }

    // 将 textures/items/ 替换为 textures/item/
    const std::string oldItemPrefix = "textures/items/";
    const std::string newItemPrefix = "textures/item/";
    if (result.find(oldItemPrefix) == 0) {
        result = newItemPrefix + result.substr(oldItemPrefix.length());
    }

    // 将旧版名称转换为现代名称
    size_t lastSlash = result.find_last_of("/\\");
    size_t dotPos = result.find_last_of('.');
    if (lastSlash != std::string::npos && dotPos != std::string::npos && dotPos > lastSlash) {
        std::string dirPath = result.substr(0, lastSlash + 1);
        std::string baseName = result.substr(lastSlash + 1, dotPos - lastSlash - 1);
        std::string ext = result.substr(dotPos);

        std::string modernName = m_textureMapper.getModernName(baseName);
        if (!modernName.empty()) {
            result = dirPath + modernName + ext;
        }
    }

    return result;
}

std::vector<std::string> ResourceMapperV112::getTexturePathVariants(std::string_view unifiedPath) const
{
    std::vector<std::string> variants;

    // 首先，将统一（现代）路径转换为旧版格式
    std::string legacyPath = m_textureMapper.toLegacyPath(unifiedPath);

    // 始终先尝试统一路径（现代格式）
    variants.push_back(std::string(unifiedPath));

    // 然后尝试旧版路径（如果无映射可能与原路径相同）
    if (legacyPath != unifiedPath) {
        variants.push_back(legacyPath);
    }

    // 还要尝试 textures/blocks/ 前缀变体
    const std::string modernPrefix = "textures/block/";
    const std::string legacyPrefix = "textures/blocks/";

    if (unifiedPath.find(modernPrefix) != std::string::npos) {
        // 找到现代路径，添加旧版目录变体
        std::string altPath = std::string(unifiedPath);
        size_t pos = altPath.find(modernPrefix);
        if (pos != std::string::npos) {
            altPath.replace(pos, modernPrefix.length(), legacyPrefix);
            if (std::find(variants.begin(), variants.end(), altPath) == variants.end()) {
                variants.push_back(altPath);
            }
        }
    } else if (unifiedPath.find(legacyPrefix) != std::string::npos) {
        // 找到旧版路径，添加现代目录变体
        std::string altPath = std::string(unifiedPath);
        size_t pos = altPath.find(legacyPrefix);
        if (pos != std::string::npos) {
            altPath.replace(pos, legacyPrefix.length(), modernPrefix);
            if (std::find(variants.begin(), variants.end(), altPath) == variants.end()) {
                variants.push_back(altPath);
            }
        }
    }

    // 提取基本名称并尝试名称变体
    std::string pathStr(unifiedPath);
    size_t lastSlash = pathStr.find_last_of("/\\");
    size_t dotPos = pathStr.find_last_of('.');
    if (lastSlash != std::string::npos && dotPos != std::string::npos && dotPos > lastSlash) {
        std::string dirPath = pathStr.substr(0, lastSlash + 1);
        std::string baseName = pathStr.substr(lastSlash + 1, dotPos - lastSlash - 1);
        std::string ext = pathStr.substr(dotPos);

        auto nameVariants = m_textureMapper.getNameVariants(baseName);
        for (const auto& name : nameVariants) {
            if (name != baseName) {
                // 添加相同目录的变体
                std::string variantPath = dirPath + name + ext;
                if (std::find(variants.begin(), variants.end(), variantPath) == variants.end()) {
                    variants.push_back(variantPath);
                }

                // 添加交换目录的变体 (block <-> blocks)
                if (dirPath.find(modernPrefix) != std::string::npos) {
                    std::string legacyDir = dirPath;
                    size_t pos = legacyDir.find(modernPrefix);
                    legacyDir.replace(pos, modernPrefix.length(), legacyPrefix);
                    std::string legacyVariant = legacyDir + name + ext;
                    if (std::find(variants.begin(), variants.end(), legacyVariant) == variants.end()) {
                        variants.push_back(legacyVariant);
                    }
                }
            }
        }
    }

    return variants;
}

std::string ResourceMapperV112::toModernTextureName(std::string_view name) const
{
    return m_textureMapper.getModernName(name);
}

std::string ResourceMapperV112::toLegacyTextureName(std::string_view name) const
{
    return m_textureMapper.getLegacyName(name);
}

std::string ResourceMapperV112::toUnifiedModelPath(std::string_view path) const
{
    // 模型路径在 1.12 和 1.13+ 之间通常是一致的
    // 大多数模型不需要转换
    return std::string(path);
}

std::vector<std::string> ResourceMapperV112::getModelPathVariants(std::string_view unifiedPath) const
{
    // 模型路径通常相同
    // 只返回统一路径
    return {std::string(unifiedPath)};
}

std::string ResourceMapperV112::toUnifiedBlockStatePath(std::string_view path) const
{
    // 方块状态路径是一致的
    // 注意: 方块 ID 在 1.13 中有变化（扁平化），但文件路径相似
    return std::string(path);
}

} // namespace v1_12
} // namespace compat
} // namespace resource
} // namespace mc
