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
#include <cstdint>

namespace mc::world::storage {

/**
 * @brief 一致性模式
 *
 * 只控制一件事：RocksDB 的**默认**写入是否需要等待 WAL fsync。
 *
 * 无论哪个模式，WAL 都照常写入，因此进程崩溃后的数据都能由 WAL 回放恢复；各模式
 * 的差别只在于是否抵御操作系统崩溃/断电。需要额外保证的关键写入（关服全量保存、
 * 显式 flush）不必切换到 Strongest，直接在同步接口上要求一次 sync 即可
 * （见 put(..., sync=true) / writeBatch(batch, sync=true)）。
 */
enum class ConsistencyMode : u8 {
    /// 最终一致性：默认写入不等待 WAL fsync，依赖 WAL 回放与后台压缩持久化。
    /// 适合：正常游戏运行的区块/实体/玩家写入。
    Eventual,

    /// 强一致性：默认写入同样不逐条 fsync；关键写入由调用方显式要求同步。
    /// 适合：默认配置。
    Strong,

    /// 最强一致性：每次写入都等待 WAL fsync。
    /// 适合：崩溃测试、开发调试。
    Strongest
};

/**
 * @brief 该一致性模式下，默认写入是否需要等待 WAL fsync
 *
 * 描述的是"默认值"：即便返回 false，调用方仍可通过同步接口把单次写入提升为
 * 同步落盘，无需切换到 Strongest。
 *
 * @param mode 一致性模式
 * @return Strongest 为 true，其余为 false
 */
[[nodiscard]] constexpr bool consistencyModeSyncsEveryWrite(ConsistencyMode mode) noexcept
{
    return mode == ConsistencyMode::Strongest;
}

} // namespace mc::world::storage
