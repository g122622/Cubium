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
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 模板方块信息
 */
struct BlockInfo {
    BlockPos pos;
    u32 blockStateId = 0;
    std::unique_ptr<nbt::CompoundTag> nbt;

    BlockInfo();
    BlockInfo(const BlockPos& p, u32 stateId);
    BlockInfo(const BlockInfo& other);
    BlockInfo(BlockInfo&& other) noexcept;
    BlockInfo& operator=(const BlockInfo& other);
    BlockInfo& operator=(BlockInfo&& other) noexcept;
    ~BlockInfo();
};

/**
 * @brief 处理后的方块信息
 */
struct ProcessedBlockInfo {
    BlockPos pos;
    u32 blockStateId = 0;
    std::unique_ptr<nbt::CompoundTag> nbt;

    ProcessedBlockInfo() = default;
    ProcessedBlockInfo(const BlockPos& p, u32 stateId)
        : pos(p)
        , blockStateId(stateId)
    {}
    ProcessedBlockInfo(const ProcessedBlockInfo& other);
    ProcessedBlockInfo(ProcessedBlockInfo&& other) noexcept;
    ProcessedBlockInfo& operator=(const ProcessedBlockInfo& other);
    ProcessedBlockInfo& operator=(ProcessedBlockInfo&& other) noexcept;
    ~ProcessedBlockInfo();

    /**
     * @brief 从 BlockInfo 创建 ProcessedBlockInfo
     * @param info 源方块信息
     * @return 处理后的方块信息
     */
    static ProcessedBlockInfo fromBlockInfo(const BlockInfo& info)
    {
        ProcessedBlockInfo result;
        result.pos = info.pos;
        result.blockStateId = info.blockStateId;
        if (info.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*info.nbt);
        }
        return result;
    }
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
