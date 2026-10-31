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
#include "common/entity/decoration/PaintingVariant.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>

namespace mc {

namespace resource {
class DataPackRepository;
}

/**
 * @brief 画变体注册表
 *
 * 承载数据包 `data/<ns>/painting_variant/` 下的全部 JSON（原版 51 种）。
 *
 * 画变体是纯数据包注册表，没有任何硬编码默认值——注册表为空就等于「所有画都摆不出来」。
 * 此前 Cubium 完全没有这一层实现（只有物品组件里一个 `minecraft:painting/variant`
 * 的组件类型枚举），画的尺寸、贴图、标题、作者全都无处可查。
 */
class PaintingVariants {
public:
    /**
     * @brief 按 ID 查找变体
     * @return 变体指针，未找到返回 nullptr
     */
    [[nodiscard]] static const PaintingVariant* get(const ResourceLocation& id);

    /**
     * @brief 统计已注册的变体数量
     */
    [[nodiscard]] static std::size_t size();

    /**
     * @brief 遍历全部变体
     *
     * 内部是无序 map，调用方需要自行排序后再比较。
     */
    static void forEachVariant(const std::function<void(const ResourceLocation&, const PaintingVariant&)>& callback);

    /**
     * @brief 从数据包加载全部画变体
     *
     * 递归扫描 `painting_variant/` 目录下的全部 `.json`（原版这里没有子目录，但
     * 加载器不假设这一点）。
     *
     * @return 加载成功的变体数量，或错误
     */
    [[nodiscard]] static Result<std::size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPacks);

    /**
     * @brief 清空注册表（测试用）
     */
    static void clearAll();

private:
    static std::unordered_map<ResourceLocation, std::unique_ptr<PaintingVariant>>& _getVariants();
};

} // namespace mc
