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
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"

#include <memory>

namespace mc::world::chunk {

class ChunkData;

/**
 * @brief ChunkData ↔ LevelChunkWithLight IR 转换层（vanilla 1.21.11 线语义）
 *
 * 贯彻 IR 思想：本层把项目内部 ChunkData 翻译为 vanilla 语义的
 * ir::play::LevelChunkWithLight 结构体（含 PalettedContainerWire/HeightmapEntryWire/
 * BlockEntityInfoWire/光照 masks/updates），上层业务（服务端发送、客户端接收）只与本层
 * 交互，不碰任何 wire 字节。只有 levelChunkWithLightCodec（仅远程 Java 客户端经
 * JavaBackend 走）把 IR 结构体编码成 vanilla wire 字节。
 *
 * 依赖三个 id 映射表（须在调用前 initialize）：
 *   - JavaBlockStateIdMap：内部 block stateId ↔ Java 全局 block state id
 *   - JavaBiomeRegistryIdMap：内部 BiomeId ↔ Java biome registry id
 *   - JavaBlockEntityTypeIdMap：BlockEntityType ↔ Java block_entity_type registry id
 *
 * buildLevelChunkWithLightIR 用于服务端发送侧（ChunkData → IR）。
 * readLevelChunkWithLightIR 用于客户端接收侧（IR → ChunkData）。
 */
class VanillaChunkWire {
public:
    VanillaChunkWire() = delete;

    /**
     * @brief 从 ChunkData 构建 vanilla 1.21.11 LevelChunkWithLight IR
     *
     * @param chunk 源区块数据（须已完成方块/生物群系/高度图/光照填充）
     * @return IR 结构体或错误
     *
     * 须在三个 id 映射表 initialize 之后调用。
     */
    [[nodiscard]] static Result<mc::network::ir::play::LevelChunkWithLight> buildLevelChunkWithLightIR(
        const ChunkData& chunk);

    /**
     * @brief 从 LevelChunkWithLight IR 还原 ChunkData（客户端接收侧）
     *
     * @param ir 接收到的 IR 结构体（本地客户端经 LocalTransport 直传，或远程经 codec 解码）
     * @return 新建的 ChunkData 或错误
     *
     * 须在三个 id 映射表 initialize 之后调用。
     */
    [[nodiscard]] static Result<std::unique_ptr<ChunkData>> readLevelChunkWithLightIR(
        const mc::network::ir::play::LevelChunkWithLight& ir);
};

} // namespace mc::world::chunk
