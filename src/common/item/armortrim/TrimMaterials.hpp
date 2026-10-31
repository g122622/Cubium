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
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace mc {

namespace resource {
class DataPackRepository;
}

class TrimMaterial;

/**
 * @brief 盔甲纹饰材料注册表
 *
 * 承载数据包 `data/<ns>/trim_material/` 下的全部 JSON（原版 11 种）。
 *
 * 与 `TrimPatterns` 一样是纯数据包注册表，没有任何硬编码默认值。
 */
class TrimMaterials {
public:
    /**
     * @brief 按 ID 查找材料
     * @return 材料指针，未找到返回 nullptr
     */
    [[nodiscard]] static const TrimMaterial* get(const ResourceLocation& id);

    /**
     * @brief 按 ID 查找材料
     * @return 材料指针，未找到返回 nullptr
     */
    [[nodiscard]] static const TrimMaterial* get(const std::string& id);

    /**
     * @brief 统计已注册的材料数量
     */
    [[nodiscard]] static std::size_t size();

    /**
     * @brief 遍历全部材料
     *
     * 内部是无序 map，调用方需要自行排序后再比较。
     */
    static void forEachMaterial(const std::function<void(const ResourceLocation&, const TrimMaterial&)>& callback);

    /**
     * @brief 从数据包加载全部纹饰材料
     *
     * @return 加载成功的材料数量，或错误
     */
    [[nodiscard]] static Result<std::size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPacks);

    /**
     * @brief 清空已加载的材料
     */
    static void clearAll();
};

} // namespace mc
