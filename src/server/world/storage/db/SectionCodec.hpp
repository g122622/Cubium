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
#include "common/core/Types.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/storage/db/SectionKey.hpp"
#include <span>
#include <rocksdb/write_batch.h>

namespace mc::world::storage {

/** @brief 原生段存档：直接传输紧凑数组，压缩完全由 RocksDB 负责。 */
class SectionCodec {
public:
    /** @brief 紧凑值的精确字节数，供调用方预分配最终批次。 */
    [[nodiscard]] static size_t encodedSize(const ChunkSection* section);

    /** @brief 将段内数组直接收集进批次；返回前批次已拥有完整快照。 */
    [[nodiscard]] static Result<void> appendToBatch(rocksdb::WriteBatch& batch,
        rocksdb::ColumnFamilyHandle* columnFamily,
        const SectionKey& key,
        const ChunkData& chunk,
        i32 sectionIndex);

    /** @brief 从读取缓冲区直接恢复最终段数组与生物群系，不接受其他存档布局。 */
    [[nodiscard]] static Result<void> restore(std::span<const u8> bytes, ChunkData& chunk, i32 sectionIndex);
};

} // namespace mc::world::storage
