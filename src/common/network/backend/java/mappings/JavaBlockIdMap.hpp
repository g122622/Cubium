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
#include <vector>

namespace mc {
class Block;
} // namespace mc

namespace mc::network::backend::java {

/**
 * @brief 项目 Block ↔ Java Block 注册表 id 双向映射（Java 协议对齐层）
 *
 * vanilla 1.21.11 `ClientboundBlockEventPacket` 第4字段 blockId 是 Java
 * `minecraft:block` 注册表（`BuiltInRegistries.BLOCK`）的 registry id（通过
 * `ByteBufCodecs.registry(Registries.BLOCK)` 编码），由 vanilla `Blocks.java` 静态
 * 初始化块的注册调用顺序分配（air=0/stone=1/…/dirt=9/cobblestone=12/…，共 1166 条）。
 * 该注册表未由本项目 RegistryDataBuilder 同步（不在 23 个 SYNCHRONIZED_REGISTRIES），
 * 真 Java 客户端使用其内置 vanilla core 包注册表。
 *
 * 项目 `Block::blockId()` 是 BlockRegistry 注册时分配的内部序（`_allocateBlockId`：
 * air 强制 0，其余从 1 自增，顺序是 `VanillaBlocks::initialize` 的注册顺序，非 vanilla
 * 顺序），两套编号无关。`PlayerBroadcaster::broadcastBlockEventInRange` 曾错误地发项目
 * 内部 stateId（连内部 blockId 都不是），真 Java 客户端按 vanilla block id 校验会拒绝
 * 该事件（红石活塞/音符盒/箱子声效等异常，BUG#6）。
 *
 * 注意与 `JavaBlockStateIdMap` 的语义差异：本表是【Block 注册表 id】（方块本身的 id，
 * 如 dirt=9），非 state globalId（dirt 默认状态 globalId=10，由 `JavaBlockStateIdMap`
 * 翻译）。BlockEvent wire 第4字段是前者，level_chunk_with_light 的 states palette 是后者。
 *
 * 本表是纯协议对齐逻辑（项目内部 blockId ↔ Java wire blockId 翻译），不属 block 业务核心，
 * 故置于 network/backend/java 层（与 `JavaBlockStateIdMap`、`JavaItemIdMap` 同层），block
 * 子系统零感知。
 *
 * 数据源：`assets/data/blocks_prismarine_1.21.11.json`（PrismarineJS minecraft-data，已验证
 * id==index 即 vanilla 注册序），由离线脚本 `scripts/baking/bake_java_block_id_table.ts`
 * 预烘焙成紧凑 C++ 静态查找表（`generated/java_block_id_table.gen.cpp`，按 name 字典序排序
 * 的二分查找表 + 扁平字符串池 + 反向稠密数组），编译进 mc_common 只读数据段。运行时零 JSON
 * 解析、零堆分配。
 *
 * initialize() 遍历 `Block::forEachBlock`，对每个 block 取 `blockLocation().toString()`
 * 在预生成排序表里二分查找得 vanilla id，填 `m_nameToJava` 与 `m_javaToInternal`。须在
 * `VanillaBlocks::initialize()` 之后调用。
 *
 * 与 `JavaItemIdMap` 的兜底差异：item 内部 id 0 是无效占位（air 真实内部 id ≥1），故
 * `JavaItemIdMap` miss 须取 air 真实内部 id；而 **block 内部 id 0 就是 air（有效）**，
 * Java block id 0 也是 air，故本表 miss 直接返 0 即 air 本身，无需取 air 真实内部 id。
 */
class JavaBlockIdMap {
public:
    static JavaBlockIdMap& instance();

    JavaBlockIdMap() = default;
    ~JavaBlockIdMap() = default;
    JavaBlockIdMap(const JavaBlockIdMap&) = delete;
    JavaBlockIdMap& operator=(const JavaBlockIdMap&) = delete;

    /// 构建双向映射。须在 VanillaBlocks::initialize() 之后调用。可重复调用（幂等，先清空再重建）。
    [[nodiscard]] Result<void> initialize();

    /// 项目 Block → Java registry id（发侧）。未初始化时自动 initialize 一次（防御漏初始化致全发 0）。
    /// 查不到返回 0（air）并记 warn。
    [[nodiscard]] u32 toJavaRegistryId(const ::mc::Block& block) const;

    /// 项目内部 blockId → Java registry id（发侧，codec 边界用）。未初始化时自动 initialize 一次。
    /// 查不到或越界返回 0（air）并记 warn。
    [[nodiscard]] u32 toJavaRegistryId(u32 internalBlockId) const;

    /// block ResourceLocation 字符串（如 "minecraft:stone"）→ Java registry id。查不到返回 0（air）并 warn。
    [[nodiscard]] u32 toJavaRegistryId(std::string_view blockLocation) const;

    /// Java registry id → 项目内部 blockId（收侧）。查不到返回 0（air）并 warn。
    /// block 内部 id 0 即 air（有效），故 miss 直接返 0，无需像 JavaItemIdMap 取 air 真实内部 id。
    [[nodiscard]] u32 fromJavaRegistryId(u32 javaRegistryId) const;

    /// 是否已建立映射。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /// 已匹配的 block 对数（诊断用）。
    [[nodiscard]] size_t matchedCount() const noexcept { return m_nameToJava.size(); }

private:
    bool m_initialized = false;
    /// "minecraft:stone" → vanilla registry id
    std::unordered_map<std::string, u32> m_nameToJava;
    /// vanilla registry id → 项目内部 blockId。下标 = vanilla id（连续稠密 0..maxId）。
    std::vector<u32> m_javaToInternal;
    /// 项目内部 blockId → vanilla registry id。下标 = 内部 blockId（连续稠密 0..maxInternalId）。
    std::vector<u32> m_internalToJava;
};

} // namespace mc::network::backend::java
