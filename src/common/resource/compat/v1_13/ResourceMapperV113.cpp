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

#include "ResourceMapperV113.hpp"
#include "../TextureMapper.hpp"
#include <algorithm>

namespace mc {
namespace resource {
namespace compat {
namespace v1_13 {

std::string ResourceMapperV113::toLegacyTextureName(std::string_view name) const
{
    // 为了与旧版资源包兼容
    return TextureMapper::instance().getLegacyName(name);
}

std::vector<std::string> ResourceMapperV113::getTexturePathVariants(std::string_view unifiedPath) const
{
    std::vector<std::string> variants;

    // 主要: 先尝试现代路径
    variants.push_back(std::string(unifiedPath));

    // 对于 1.13+ 包，也要尝试旧版路径作为回退
    // 这处理纹理可能以旧版名称存储的情况
    const TextureMapper& mapper = TextureMapper::instance();
    std::string legacyPath = mapper.toLegacyPath(unifiedPath);

    if (legacyPath != unifiedPath) {
        variants.push_back(legacyPath);
    }

    // 提取基本名称并尝试名称变体
    std::string pathStr(unifiedPath);
    size_t lastSlash = pathStr.find_last_of("/\\");
    size_t dotPos = pathStr.find_last_of('.');
    if (lastSlash != std::string::npos && dotPos != std::string::npos && dotPos > lastSlash) {
        std::string dirPath = pathStr.substr(0, lastSlash + 1);
        std::string baseName = pathStr.substr(lastSlash + 1, dotPos - lastSlash - 1);
        std::string ext = pathStr.substr(dotPos);

        auto nameVariants = mapper.getNameVariants(baseName);
        for (const auto& name : nameVariants) {
            if (name != baseName) {
                std::string variantPath = dirPath + name + ext;
                if (std::find(variants.begin(), variants.end(), variantPath) == variants.end()) {
                    variants.push_back(variantPath);
                }
            }
        }

        // 还要尝试旧版目录 (textures/blocks/)
        const std::string modernPrefix = "textures/block/";
        const std::string legacyPrefix = "textures/blocks/";
        if (dirPath.find(modernPrefix) != std::string::npos) {
            std::string legacyDir = dirPath;
            size_t pos = legacyDir.find(modernPrefix);
            legacyDir.replace(pos, modernPrefix.length(), legacyPrefix);

            for (const auto& name : nameVariants) {
                std::string variantPath = legacyDir + name + ext;
                if (std::find(variants.begin(), variants.end(), variantPath) == variants.end()) {
                    variants.push_back(variantPath);
                }
            }
        }
    }

    return variants;
}

} // namespace v1_13
} // namespace compat
} // namespace resource
} // namespace mc
