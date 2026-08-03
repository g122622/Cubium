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
#include "common/util/nbt/Nbt.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc {

class BlockEntity;

/**
 * @brief 方块实体 NBT 反序列化器
 *
 * 从 NBT 数据创建方块实体实例。
 *
 * 流程：
 * 1. 读取 "id" 标签获取方块实体类型字符串
 * 2. 通过 blockEntityTypeFromId() 解析为 BlockEntityType
 * 3. 通过 BlockEntityRegistry::create() 创建实例
 * 4. 调用 BlockEntity::loadFromNBT() 填充数据
 */
namespace blockentity {

class BlockEntityDeserializer {
public:
    /**
     * @brief 从 NBT 反序列化方块实体
     *
     * @param tag NBT 复合标签（必须包含 "id", "x", "y", "z" 字段）
     * @return 方块实体实例或错误
     */
    static Result<std::unique_ptr<BlockEntity>> deserialize(const nbt::tags::compound_tag& tag);

    /**
     * @brief 从二进制数据反序列化方块实体列表
     *
     * 二进制格式为 gzip 压缩的 NBT 列表标签。
     *
     * @param data 压缩的 NBT 二进制数据
     * @return 方块实体列表或错误
     */
    static Result<std::vector<std::unique_ptr<BlockEntity>>> deserializeListFromBinary(const std::vector<u8>& data);

    /**
     * @brief 将方块实体列表序列化为二进制数据
     *
     * @param blockEntities 方块实体列表
     * @return 压缩的 NBT 二进制数据或错误
     */
    static Result<std::vector<u8>> serializeListToBinary(
        const std::vector<std::reference_wrapper<const BlockEntity>>& blockEntities);
};

} // namespace blockentity
} // namespace mc
