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

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mc {
class Item;
}

namespace mc::network::backend::java {

/**
 * @brief 项目 Item ↔ Java item registry id 双向映射（Java 协议对齐层）
 *
 * ItemStack wire（1.21.11 ItemStack.OPTIONAL_STREAM_CODEC）的 itemId 字段是 Java
 * `minecraft:item` 注册表（`BuiltInRegistries.ITEM`）的 registry id。该注册表未由
 * 本项目 RegistryDataBuilder 同步（不在 23 个 SYNCHRONIZED_REGISTRIES，与 entity_type /
 * block_entity_type 同类），真 Java 客户端使用其内置 vanilla core 包注册表，id 为 vanilla
 * 1.21.11 `Items.java` 静态字段声明顺序（air=0/stone=1/…/ominous_bottle=1504，共 1505 条）。
 *
 * 项目 `Item::itemId()` 是 ItemRegistry 注册时分配的内部序（`m_nextItemId` 从 1 自增，
 * 顺序是 `Items::initialize` 的业务分组，非 vanilla 顺序），两套编号无关。若直接发项目
 * 内部序，真 Java 客户端按 vanilla id 反查到错误物品（如挖石头掉成无关物品）——与
 * `JavaEntityTypeIdMap` 修过的 entity_type id 错位同构。
 *
 * 本表是纯协议对齐逻辑（项目内部 id ↔ Java wire id 翻译），不属 item 业务核心，故置于
 * network/backend/java 层（与 `JavaProtocolTables`、`JavaBlockStateIdMap` 同层），item 子系统
 * 零感知。另三张同类表（JavaEntityTypeIdMap/JavaBlockEntityTypeIdMap/JavaBiomeRegistryIdMap）
 * 现仍置于各自子系统下，可视情况陆续迁此层统一。
 *
 * 数据源：`assets/data/items_1.21.11.json`（PrismarineJS minecraft-data，已验证 id==index
 * 即 vanilla 注册序），由离线脚本 `scripts/baking/bake_java_item_table.ts` 预烘焙成紧凑
 * C++ 静态查找表（`generated/java_item_table.gen.cpp`，按 name 字典序排序的二分查找表 +
 * 扁平字符串池 + 反向稠密数组），编译进 mc_common 只读数据段。运行时零 JSON 解析、零堆分配。
 *
 * initialize() 遍历 `ItemRegistry::forEachItem`，对每个 item 取 `itemLocation().toString()`
 * 在预生成排序表里二分查找得 vanilla id，填 `m_nameToJava` 与 `m_javaToInternal`。须在
 * `Items::initialize()` 之后调用。
 *
 * 与 entity_type 表的关键差异：item wire id **双向流通**——服务端 `toItemStackView` 发
 * （项目 Item → vanilla id），客户端 `fromItemStackView` 收（vanilla id → 项目 Item），
 * 故本表须双向。边界收口于 `network/ir/ItemStackBridge`，业务侧 ItemStack 与 codec 均零感知。
 */
class JavaItemIdMap {
public:
    static JavaItemIdMap& instance();

    JavaItemIdMap() = default;
    ~JavaItemIdMap() = default;
    JavaItemIdMap(const JavaItemIdMap&) = delete;
    JavaItemIdMap& operator=(const JavaItemIdMap&) = delete;

    /// 构建双向映射。须在 Items::initialize() 之后调用。可重复调用（幂等，先清空再重建）。
    [[nodiscard]] Result<void> initialize();

    /// 项目 Item → Java registry id（发侧）。未初始化时自动 initialize 一次（防御漏初始化致全发 0）。
    /// 查不到返回 0（air）并记 warn。
    [[nodiscard]] u32 toJavaRegistryId(const ::mc::Item& item) const;

    /// item ResourceLocation 字符串（如 "minecraft:stone"）→ Java registry id。查不到返回 0（air）并 warn。
    [[nodiscard]] u32 toJavaRegistryId(std::string_view itemLocation) const;

    /// Java registry id → 项目内部 ItemId（收侧）。查不到返回项目 air 的内部 id（m_airInternalId）并 warn。
    /// 注意：项目 ItemId 0 是无效占位（ItemRegistry 保留 ID 0），air 真实内部 id ≥1，故 miss 不能返 0。
    [[nodiscard]] ::mc::ItemId fromJavaRegistryId(u32 javaRegistryId) const;

    /// 是否已建立映射。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /// 已匹配的 item 对数（诊断用）。
    [[nodiscard]] size_t matchedCount() const noexcept { return m_javaToInternal.size(); }

private:
    bool m_initialized = false;
    /// "minecraft:stone" → vanilla registry id
    std::unordered_map<std::string, u32> m_nameToJava;
    /// vanilla registry id → 项目内部 ItemId
    std::unordered_map<u32, ::mc::ItemId> m_javaToInternal;
    /// 项目 air 的真实内部 ItemId（所有 miss 的兜底）。ItemId 0 是无效占位，air ≥1。
    ::mc::ItemId m_airInternalId = 0;
};

} // namespace mc::network::backend::java
