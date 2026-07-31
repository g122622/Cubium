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

#include <string>
#include <string_view>
#include <unordered_map>

namespace mc {

/**
 * @brief EntityType.m_name → Java entity_type registry id 映射
 *
 * AddEntity（clientbound/minecraft:add_entity / 1.21.11 统一 spawn）的 entityTypeId 字段
 * 是 Java `minecraft:entity_type` 注册表的 id。该注册表未由本项目 RegistryDataBuilder 同步
 * （不在 23 个 SYNCHRONIZED_REGISTRIES，与 block_entity_type 同类），真 Java 客户端使用其
 * 内置 vanilla core 包注册表，id 为 vanilla 1.21.11 `EntityType.java` 静态 `register("name",...)`
 * 调用顺序（字母序，157 条，id 0..156）。
 *
 * 因此 registry id 是固定的 vanilla 顺序，与项目内部 EntityType::id()（VanillaEntities.cpp 的
 * registerType 注册序，PIG=0/COW=1/.../ITEM=82）完全不同。若直接发项目 id，客户端会按 vanilla
 * id 反查到错误实体类型——例如项目 item=82 对应 vanilla id 82=mangrove_chest_boat，客户端把掉落物
 * 渲染成红树木运输船，随后 ItemEntity 的 field8(ItemStack) 撞上 chest_boat 的 field8(Int) 致
 * set_entity_data 类型校验崩溃（disconnect-2026-07-31_09.37.02-client.txt）。
 *
 * initialize() 建 name→id / id→name 双向表。toJavaRegistryId(name) 先查别名表（项目特有键映射到
 * 选定的 vanilla id，如 minecraft:potion→splash_potion），再查 vanilla 主表。船类（boat/chest_boat）
 * 由 BoatEntity/ChestBoatEntity 的 getJavaEntityTypeId() override 按木种拼出变体名再查本表，不走别名。
 */
class JavaEntityTypeIdMap {
public:
    static JavaEntityTypeIdMap& instance();

    JavaEntityTypeIdMap() = default;
    ~JavaEntityTypeIdMap() = default;
    JavaEntityTypeIdMap(const JavaEntityTypeIdMap&) = delete;
    JavaEntityTypeIdMap& operator=(const JavaEntityTypeIdMap&) = delete;

    /// 构建映射。仅依赖硬编码 vanilla 表，无注册顺序依赖，可重复调用（幂等）。
    [[nodiscard]] Result<void> initialize();

    /// EntityType.m_name（如 "minecraft:item"）→ Java registry id；查不到返回 0（acacia_boat）并 warn。
    /// 未初始化时自动 initialize() 一次（防御漏初始化致全发 id 0）。
    [[nodiscard]] u32 toJavaRegistryId(std::string_view name) const;

    /// Java registry id → name（调试用）；查不到返回空串并 warn。
    [[nodiscard]] std::string_view fromJavaRegistryId(u32 javaRegistryId) const;

    /// 是否已建立映射。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

private:
    bool m_initialized = false;
    /// EntityType.m_name（含 minecraft: 前缀）→ vanilla registry id
    std::unordered_map<std::string, u32> m_nameToId;
    /// vanilla registry id → name
    std::unordered_map<u32, std::string> m_idToName;
};

} // namespace mc
