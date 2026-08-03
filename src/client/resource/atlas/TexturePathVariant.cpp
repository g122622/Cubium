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

#include "client/resource/atlas/TexturePathVariant.hpp"
#include <string>
#include <string_view>

namespace mc::client::resource::atlas {

std::string TexturePathVariant::getAltTexturePath(const std::string& path)
{
    constexpr std::string_view blockModern = "textures/block/";
    constexpr std::string_view blockLegacy = "textures/blocks/";
    constexpr std::string_view itemModern = "textures/item/";
    constexpr std::string_view itemLegacy = "textures/items/";

    if (path.size() > blockModern.size() && path.compare(0, blockModern.size(), blockModern) == 0) {
        // 1.13+ 单数 -> 1.12 复数：textures/block/ -> textures/blocks/
        return "textures/blocks/" + path.substr(blockModern.size());
    }
    if (path.size() > blockLegacy.size() && path.compare(0, blockLegacy.size(), blockLegacy) == 0) {
        // 1.12 复数 -> 1.13+ 单数：textures/blocks/ -> textures/block/
        return "textures/block/" + path.substr(blockLegacy.size());
    }
    if (path.size() > itemModern.size() && path.compare(0, itemModern.size(), itemModern) == 0) {
        return "textures/items/" + path.substr(itemModern.size());
    }
    if (path.size() > itemLegacy.size() && path.compare(0, itemLegacy.size(), itemLegacy) == 0) {
        return "textures/item/" + path.substr(itemLegacy.size());
    }

    // 实体纹理路径变体：1.13+ 子目录格式 <-> 1.12 扁平格式
    // textures/entity/pig/pig.png <-> textures/entity/pig.png
    constexpr std::string_view entityPrefix = "textures/entity/";
    if (path.size() > entityPrefix.size() && path.compare(0, entityPrefix.size(), entityPrefix) == 0) {
        std::string_view afterPrefix(path.data() + entityPrefix.size(), path.size() - entityPrefix.size());
        auto slashPos = afterPrefix.find('/');
        if (slashPos != std::string_view::npos) {
            // 子目录格式：textures/entity/<name>/<filename>
            std::string_view dirName = afterPrefix.substr(0, slashPos);
            std::string_view fileName = afterPrefix.substr(slashPos + 1);
            // 提取扩展名以便转换后保留
            std::string_view extension;
            auto dotPos = fileName.rfind('.');
            if (dotPos != std::string_view::npos) {
                extension = fileName.substr(dotPos);
                fileName = fileName.substr(0, dotPos);
            }
            if (dirName == fileName) {
                // textures/entity/<name>/<name>[.ext] -> textures/entity/<name>[.ext]
                return std::string(entityPrefix) + std::string(dirName) + std::string(extension);
            }
        } else {
            // 扁平格式：textures/entity/<name>[.ext] -> textures/entity/<name>/<name>[.ext]
            std::string_view namePart = afterPrefix;
            std::string_view extension;
            auto dotPos = afterPrefix.rfind('.');
            if (dotPos != std::string_view::npos) {
                namePart = afterPrefix.substr(0, dotPos);
                extension = afterPrefix.substr(dotPos);
            }
            return std::string(entityPrefix) + std::string(namePart) + "/" + std::string(namePart) +
                std::string(extension);
        }
    }

    return {};
}

} // namespace mc::client::resource::atlas
