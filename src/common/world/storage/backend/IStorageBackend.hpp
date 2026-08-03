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
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/world/storage/core/SaveFormat.hpp"
#include "common/world/storage/player/PlayerSaveData.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::world::storage {

/**
 * @brief 外来存档存储后端接口
 *
 * 为非 Native 格式（Java Anvil、Bedrock LevelDB）提供统一的只读访问接口。
 * Native 格式仍由 SingleLevelStorageManager 内部的 RocksDB 管线处理，
 * 不经过此接口。
 *
 * 所有实现必须支持：
 * - 打开/关闭存档目录
 * - 读取区块数据
 * - 列举已有区块
 * - 读取玩家数据
 * - 读取世界元数据（level.dat）
 */
class IStorageBackend {
public:
    virtual ~IStorageBackend() noexcept = default;

    // ========== 生命周期 ==========

    /**
     * @brief 打开存档目录
     * @param worldPath 存档目录路径
     * @return 成功或错误
     */
    virtual Result<void> open(const std::filesystem::path& worldPath, const SaveFormatInfo& formatInfo) = 0;

    /**
     * @brief 关闭存档并释放资源
     */
    virtual void close() = 0;

    /**
     * @brief 检查是否已打开
     */
    [[nodiscard]] virtual bool isOpen() const = 0;

    // ========== 区块读取 ==========

    /**
     * @brief 读取完整区块
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param dimension 维度 ID
     * @return 区块数据，不存在返回空 optional
     */
    [[nodiscard]] virtual Result<std::optional<ChunkData>> loadChunk(
        ChunkCoord x, ChunkCoord z, DimensionId dimension) = 0;

    /**
     * @brief 列举指定维度中所有存在的区块坐标
     * @param dimension 维度 ID
     * @return 区块坐标列表
     */
    [[nodiscard]] virtual Result<std::vector<ChunkPos>> listChunks(DimensionId dimension) = 0;

    // ========== 玩家数据 ==========

    /**
     * @brief 读取玩家数据
     * @param uuid 玩家 UUID
     * @return 玩家数据，不存在返回空 optional
     */
    [[nodiscard]] virtual Result<std::optional<PlayerSaveData>> loadPlayer(const std::string& uuid) = 0;

    /**
     * @brief 列举所有玩家 UUID
     * @return UUID 列表
     */
    [[nodiscard]] virtual Result<std::vector<std::string>> listPlayerUuids() = 0;

    // ========== 世界元数据 ==========

    /**
     * @brief 读取世界运行时数据（level.dat 内容）
     * @return 运行时数据
     */
    [[nodiscard]] virtual Result<LevelRuntimeData> loadLevelData() = 0;

    // ========== 格式信息 ==========

    /**
     * @brief 获取存档格式
     */
    [[nodiscard]] virtual SaveFormat format() const = 0;

    /**
     * @brief 获取格式详细信息
     */
    [[nodiscard]] virtual const SaveFormatInfo& formatInfo() const = 0;

    /**
     * @brief 是否为只读
     *
     * 外来格式后端始终返回 true。
     */
    [[nodiscard]] virtual bool isReadonly() const = 0;

    /**
     * @brief 获取存档目录路径
     */
    [[nodiscard]] virtual const std::filesystem::path& worldPath() const = 0;
};

} // namespace mc::world::storage
