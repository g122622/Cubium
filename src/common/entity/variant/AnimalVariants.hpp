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
#include "common/entity/variant/AnimalVariant.hpp"
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

/**
 * @brief 动物变体注册表（通用实现）
 *
 * 原版把每一类变体放在独立的注册表里（`wolf_variant` / `cat_variant` / ...），
 * 但它们的**数据形状完全一致**，解析逻辑也一致，因此这里用一个通用加载器，
 * 每类变体持有一个独立的条目表。
 *
 * 六类变体此前在 Cubium 中都没有实现：变体的贴图与出生条件全都无处可查，
 * 狼 / 猫 / 蛙 / 牛 / 鸡 / 猪的变体选择也就无从谈起。
 */
class AnimalVariants {
public:
    /**
     * @brief 一类变体的条目表
     */
    using Table = std::unordered_map<ResourceLocation, std::unique_ptr<AnimalVariant>>;

    /**
     * @brief 取得某一类变体的条目表
     *
     * @param kind 注册表目录名，如 `wolf_variant`
     */
    [[nodiscard]] static Table& table(const std::string& kind);

    /**
     * @brief 从数据包加载某一类变体
     *
     * 扫描 `<ns>/<kind>` 下的全部 `.json`；同一 ID 多数据包叠加时取优先级最高的
     * 版本（`versions.back()`）。
     *
     * @param dataPacks 数据包仓库
     * @param kind 注册表目录名，如 `wolf_variant`
     * @return 加载成功的变体数量，或错误
     */
    [[nodiscard]] static Result<std::size_t> loadFromDataPackRepository(
        const resource::DataPackRepository& dataPacks, const std::string& kind);

    /**
     * @brief 清空某一类变体
     */
    static void clear(const std::string& kind);
};

} // namespace mc
