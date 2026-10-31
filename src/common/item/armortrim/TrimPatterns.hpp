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

class TrimPattern;

/**
 * @brief 盔甲纹饰图案注册表
 *
 * 承载数据包 `data/<ns>/trim_pattern/` 下的全部 JSON（原版 18 种）。
 *
 * 纹饰图案是纯数据包注册表，没有任何硬编码默认值。此前 Cubium 完全没有这一层实现，
 * 图案的贴图 ID、描述、是否贴花全都无处可查，纹饰台也就无法给出正确的图案列表。
 */
class TrimPatterns {
public:
    /**
     * @brief 按 ID 查找图案
     * @return 图案指针，未找到返回 nullptr
     */
    [[nodiscard]] static const TrimPattern* get(const ResourceLocation& id);

    /**
     * @brief 按 ID 查找图案
     * @return 图案指针，未找到返回 nullptr
     */
    [[nodiscard]] static const TrimPattern* get(const std::string& id);

    /**
     * @brief 统计已注册的图案数量
     */
    [[nodiscard]] static std::size_t size();

    /**
     * @brief 遍历全部图案
     *
     * 内部是无序 map，调用方需要自行排序后再比较。
     */
    static void forEachPattern(const std::function<void(const ResourceLocation&, const TrimPattern&)>& callback);

    /**
     * @brief 从数据包加载全部纹饰图案
     *
     * 按命名空间扫描 `<ns>/trim_pattern` 下的 `.json`；同一 ID 多数据包叠加时取
     * 优先级最高的版本（`versions.back()`）。
     *
     * @return 加载成功的图案数量，或错误
     */
    [[nodiscard]] static Result<std::size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPacks);

    /**
     * @brief 清空已加载的图案
     */
    static void clearAll();
};

} // namespace mc
