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

#include "common/core/Types.hpp"
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::world::storage::reader::java {

/// Java 版调色板条目：方块名 + 属性键值对
struct PaletteEntry {
    std::string blockName;
    std::map<std::string, std::string> properties;
};

/**
 * @brief Java 版方块状态字符串→内部 stateId 映射器
 *
 * 将 Java 版的方块状态（"minecraft:stone" + properties）
 * 映射到项目内部的 BlockState stateId。
 *
 * 使用缓存避免重复查找。
 */
class JavaBlockStateMapper {
public:
    JavaBlockStateMapper();

    /**
     * @brief 从 Java 版调色板条目映射到内部 stateId
     * @param entry 调色板条目（方块名 + 属性）
     * @return 内部 stateId，未识别返回 0（空气）
     */
    u32 mapBlockState(const PaletteEntry& entry);

    /**
     * @brief 批量映射调色板
     * @param entries 调色板条目列表
     * @return stateId 列表，与输入顺序一致
     */
    std::vector<u32> mapPalette(const std::vector<PaletteEntry>& entries);

private:
    /// 构建缓存键
    [[nodiscard]] std::string buildCacheKey(const PaletteEntry& entry) const;

    /// 缓存：完整方块状态字符串→stateId
    std::unordered_map<std::string, u32> m_cache;
};

} // namespace mc::world::storage::reader::java
