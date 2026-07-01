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

#include "JavaChunkReader.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include <optional>
#include <set>
#include <vector>

namespace mc::nbt::tags {
struct compound_tag;
struct list_tag;
} // namespace mc::nbt::tags

namespace mc::world::storage::reader::java {

/**
 * @brief Java 列级读取器
 *
 * 负责列级 NBT 解包、状态过滤、坐标校验，以及把 section 级任务分派给 `JavaChunkReader`。
 */
class JavaColumnReader {
public:
    explicit JavaColumnReader(JavaChunkReader& chunkReader);

    [[nodiscard]] Result<std::optional<ChunkData>> readColumn(
        const std::vector<u8>& nbtData, ChunkCoord x, ChunkCoord z, DimensionId dimension);

private:
    [[nodiscard]] Result<void> _readSections(const nbt::tags::compound_tag& columnNbt,
        ChunkData& chunk,
        i32 dimMinHeight,
        i32 dimMaxHeight,
        bool dimHasSkyLight);
    [[nodiscard]] Result<void> _readBiomes(
        const nbt::tags::compound_tag& columnNbt, ChunkData& chunk, i32 dimMinHeight);
    void _readHeightmaps(const nbt::tags::compound_tag& columnNbt, ChunkData& chunk, i32 heightOffset);
    void _readEntities(const nbt::tags::compound_tag& columnNbt, ChunkData& chunk);
    void _readBlockEntities(const nbt::tags::compound_tag& columnNbt, ChunkData& chunk);

    JavaChunkReader& m_chunkReader;
};

} // namespace mc::world::storage::reader::java
