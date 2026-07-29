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
#include "world/blockentity/BlockEntityType.hpp"

#include <string>
#include <unordered_map>

namespace mc {

/**
 * @brief BlockEntityType → Java block_entity_type registry id 映射
 *
 * level_chunk_with_light 的 blockEntities 字段里，每个 block entity 携带 VarInt(typeRegistryId)，
 * 即 Java `minecraft:block_entity_type` 注册表的 id。该注册表未由本项目 RegistryDataBuilder 同步
 * （不在 SYNCHRONIZED_REGISTRIES），真 Java 客户端使用其内置 vanilla core 包注册表，id 为
 * vanilla 1.21.11 `BlockEntityType` 静态字段声明顺序（BuiltInRegistries.BLOCK_ENTITY_TYPE 注册顺序）。
 *
 * 因此 registry id 是固定的 vanilla 顺序（49 条，id 0-48），与项目内部 BlockEntityType 枚举值无关。
 * initialize() 建 ResourceLocation(由 blockEntityTypeToId 返回) → vanilla registry id 查表。
 * toJavaRegistryId(BlockEntityType) 经 blockEntityTypeToId 取 ResourceLocation 再查表。
 *
 * 项目未实现的 vanilla 类型（hanging_sign/calibrated_sculk_sensor 等）不影响：项目不产生那些
 * block entity。项目所有 48 个 BlockEntityType 枚举对应的 ResourceLocation 均能在 vanilla 表命中。
 */
class JavaBlockEntityTypeIdMap {
public:
    static JavaBlockEntityTypeIdMap& instance();

    JavaBlockEntityTypeIdMap() = default;
    ~JavaBlockEntityTypeIdMap() = default;
    JavaBlockEntityTypeIdMap(const JavaBlockEntityTypeIdMap&) = delete;
    JavaBlockEntityTypeIdMap& operator=(const JavaBlockEntityTypeIdMap&) = delete;

    /// 构建映射。可在任意时刻调用（仅依赖 blockEntityTypeToId，无注册顺序依赖）。可重复调用。
    [[nodiscard]] Result<void> initialize();

    /// BlockEntityType → Java registry id；查不到返回 0（furnace）并记 warn。
    [[nodiscard]] u32 toJavaRegistryId(BlockEntityType type) const;

    /// Java registry id → BlockEntityType；查不到返回 Unknown 并记 warn。
    [[nodiscard]] BlockEntityType fromJavaRegistryId(u32 javaRegistryId) const;

    /// 是否已建立映射。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

private:
    bool m_initialized = false;
    /// ResourceLocation 字符串（如 "minecraft:furnace"）→ registry id
    std::unordered_map<std::string, u32> m_nameToId;
    /// registry id → BlockEntityType
    std::unordered_map<u32, BlockEntityType> m_idToType;
};

} // namespace mc
