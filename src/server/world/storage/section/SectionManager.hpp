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

#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/storage/db/ConsistencyMode.hpp"
#include "server/world/storage/db/RocksDBDatabase.hpp"
#include "server/world/storage/db/SectionKey.hpp"
#include <span>

namespace mc::world::storage {

/** @brief 单维度段批量读写；批次持有快照，读取句柄短期固定底层缓冲区，不设常驻缓存。 */
class SectionManager {
public:
    struct Config {
        ConsistencyMode consistencyMode;
    };

    /** @brief 绑定数据库、维度与一致性配置。 */
    SectionManager(RocksDBDatabase& db, DimensionId dimension, const Config& config);

    /** @brief 按输入顺序批量读取；空 optional 只表示该段不存在。 */
    [[nodiscard]] Result<PinnedValues> loadSectionsSync(std::span<const SectionKey> keys);

    /** @brief 在数据稳定期间将一个区块的有效段直接追加到拥有数据的批次。 */
    [[nodiscard]] Result<void> appendChunk(rocksdb::WriteBatch& batch, const ChunkData& chunk);

    /** @brief 一次提交整个批次，返回段数；空批次不执行数据库写入。 */
    [[nodiscard]] Result<size_t> saveBatch(rocksdb::WriteBatch& batch, bool sync);

private:
    RocksDBDatabase& m_db;
    DimensionId m_dimension;
    std::string m_cfName;
    Config m_config;
};

} // namespace mc::world::storage
