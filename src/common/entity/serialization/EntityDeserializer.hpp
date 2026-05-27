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
#include <memory>
#include <vector>

namespace mc {
class Entity;
class IWorld;

namespace entity::serialization {

/**
 * @brief 实体反序列化器
 *
 * 从 NBT 数据创建实体实例。参考 MC 1.16.5 EntityType.loadEntityAndExecute()。
 *
 * 流程：
 * 1. 读取 "id" 标签获取实体类型字符串
 * 2. 通过 EntityRegistry 查找 EntityType
 * 3. 调用 EntityType::create() 创建实例
 * 4. 调用 Entity::readFromNBT() 填充数据
 * 5. 处理 Passengers 递归加载
 */
class EntityDeserializer {
public:
    /**
     * @brief 从 NBT 反序列化实体
     * @param tag NBT 复合标签
     * @param world 世界引用（用于实体创建）
     * @return 实体实例或错误
     */
    static Result<std::unique_ptr<Entity>> deserialize(const nbt::tags::compound_tag& tag, IWorld* world);

    /**
     * @brief 从二进制数据反序列化实体
     * @param data 压缩的 NBT 二进制数据
     * @param world 世界引用
     * @return 实体实例或错误
     */
    static Result<std::unique_ptr<Entity>> deserializeFromBinary(const std::vector<u8>& data, IWorld* world);

    /**
     * @brief 将实体序列化为二进制数据
     * @param entity 实体引用
     * @return 压缩的 NBT 二进制数据或错误
     */
    static Result<std::vector<u8>> serializeToBinary(const Entity& entity);
};

} // namespace entity::serialization
} // namespace mc
