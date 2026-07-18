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

#include <string>
#include <string_view>

namespace mc::client::resource::atlas {

/**
 * @brief 纹理路径变体兼容工具
 *
 * MC 1.13+ 使用单数目录（textures/block/、textures/item/），
 * MC 1.12 使用复数目录（textures/blocks/、textures/items/）；
 * 实体纹理在 1.13+ 子目录格式与 1.12 扁平格式间也存在差异。
 *
 * 本工具仅用于查询层回退：图集内部 sprite 名严格使用原版单数约定，
 * 当模型 JSON 使用旧路径时，通过本方法计算变体路径再次查找。
 */
struct TexturePathVariant {
    /**
     * @brief 计算给定路径的另一种变体路径
     *
     * 转换规则：
     * - textures/block/ <-> textures/blocks/
     * - textures/item/  <-> textures/items/
     * - textures/entity/<name>/<name> <-> textures/entity/<name>（子目录 <-> 扁平）
     *
     * @param path 资源路径（如 "textures/block/stone"）
     * @return 变体路径；若无变体返回空字符串
     */
    [[nodiscard]] static std::string getAltTexturePath(const std::string& path);
};

} // namespace mc::client::resource::atlas
