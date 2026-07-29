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

#include <vector>

namespace mc::world::block {

/**
 * @brief 内部 block stateId ↔ Java 全局 block state id 双向映射
 *
 * 项目 BlockRegistry 按【注册顺序】分配内部 stateId(从 0 连续递增,BlockRegistry.hpp
 * _allocateStateId = m_nextStateId++),与 Java Block.BLOCK_STATE_REGISTRY 的全局 id(按
 * vanilla 数据包注册顺序,0-29670 连续,air=0)是两套独立编号,无数学关系。真 Java 客户端
 * 解 level_chunk_with_light 的 states PalettedContainer 时,palette 写的必须是 Java 全局 id。
 *
 * 数据源:1.21.11 blocks.json,由离线脚本 scripts/baking/bake_java_id_tables.ts 预烘焙成
 * 紧凑 C++ 静态查找表(generated/java_block_state_table.gen.cpp,按 key 字典序排序的二分
 * 查找表 + 扁平字符串池),编译进 mc_common 只读数据段。运行时零 JSON 解析、零堆分配。
 *
 * initialize() 遍历 BlockRegistry::forEachBlockState,用 (blockLocation, IProperty::valueToString
 * 导出的 properties) 构造与烘焙脚本一致的 key,在预生成排序表里二分查找得 globalId,填入
 * 两个 vector<u32>(stateId 与 globalId 都连续稠密,直接作下标)。
 *
 * 须在 VanillaBlocks::initialize()(方块注册完成)之后调用 initialize()。
 *
 * 内存:常驻仅两个 vector<u32>(各 ~29000×4 ≈ 116KB,合计 ~230KB),远小于运行时解析
 * 6.5MB JSON 的 ~30MB nlohmann::json DOM 峰值。查表 O(log n) 二分。
 */
class JavaBlockStateIdMap {
public:
    static JavaBlockStateIdMap& instance();

    JavaBlockStateIdMap() = default;
    ~JavaBlockStateIdMap() = default;
    JavaBlockStateIdMap(const JavaBlockStateIdMap&) = delete;
    JavaBlockStateIdMap& operator=(const JavaBlockStateIdMap&) = delete;

    /**
     * @brief 构建双向映射
     *
     * 须在方块注册完成后调用。可重复调用(重复调用先清空再重建)。
     * @return 成功或错误(预生成表为空等)。
     */
    [[nodiscard]] Result<void> initialize();

    /// 内部 stateId → Java 全局 id;未建立映射或查不到返回 0(air)并记 warn。
    [[nodiscard]] u32 toJavaGlobalId(u32 internalStateId) const;

    /// Java 全局 id → 内部 stateId;查不到返回 0(air)并记 warn。
    [[nodiscard]] u32 fromJavaGlobalId(u32 javaGlobalId) const;

    /// 是否已建立映射(initialize 成功)。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /// 已匹配的 state 对数(诊断用)。
    [[nodiscard]] size_t matchedCount() const noexcept { return m_toJava.size(); }

private:
    bool m_initialized = false;
    /// 内部 stateId → Java globalId。下标 = stateId(连续稠密)。
    std::vector<u32> m_toJava;
    /// Java globalId → 内部 stateId。下标 = globalId(连续稠密)。
    std::vector<u32> m_fromJava;
};

} // namespace mc::world::block
