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

#include "Block.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "common/perfetto/TraceEvents.hpp"

namespace mc {

/**
 * @brief 方块注册表
 *
 * 单例模式，管理所有方块的注册和查找。
 *
 * 参考: net.minecraft.block.Block.Registry
 *
 * 用法示例:
 * @code
 * // 注册方块
 * auto& stone = BlockRegistry::instance().registerBlock<SimpleBlock>(
 *     ResourceLocation("minecraft:stone"),
 *     BlockProperties(Material::ROCK).hardness(1.5f)
 * );
 *
 * // 获取方块
 * Block* block = BlockRegistry::instance().getBlock(
 *     ResourceLocation("minecraft:stone")
 * );
 *
 * // 获取方块状态
 * BlockState* state = BlockRegistry::instance().getBlockState(1);
 * @endcode
 */
class BlockRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static BlockRegistry& instance();

    // 禁止拷贝和移动
    BlockRegistry(const BlockRegistry&) = delete;
    BlockRegistry& operator=(const BlockRegistry&) = delete;

    /**
     * @brief 注册方块
     * @tparam BlockType 方块类型
     * @param id 方块资源位置
     * @param args 方块构造函数参数
     * @return 方块引用
     */
    template <typename BlockType, typename... Args>
    BlockType& registerBlock(const ResourceLocation& id, Args&&... args)
    {
        MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Client.Initialization, "registerBlock", "id", id.toString());

        static_assert(std::is_base_of_v<Block, BlockType>, "BlockType must inherit from Block");

        // 避免重复注册同一资源位置导致旧状态指针悬挂
        auto existingIt = m_blocks.find(id);
        if (existingIt != m_blocks.end()) {
            auto* existing = dynamic_cast<BlockType*>(existingIt->second.get());
            if (existing == nullptr) {
                throw std::logic_error("Block id already registered with different type: " + id.toString());
            }
            return *existing;
        }

        auto block = std::make_unique<BlockType>(std::forward<Args>(args)...);
        block->m_blockLocation = id;

        // MC原版行为：若未显式设置掉落表ID且未禁止，则自动推导为 "<namespace>:blocks/<path>"
        // 例如 minecraft:stone → minecraft:blocks/stone
        if (block->m_lootTableId.empty() && !block->m_noLootTable && id.isValid()) {
            block->m_lootTableId = id.namespace_() + ":blocks/" + id.path();
        }

        // 动态分配方块ID（AIR 获得 ID 0）
        u32 blockId = _allocateBlockId(id);
        block->m_blockId = blockId;

        // 注册所有状态
        const auto& validStates = block->stateContainer().validStates();
        if (validStates.empty()) {
            throw std::logic_error("Block has no valid states: " + id.toString());
        }

        for (const auto& state : validStates) {
            if (!state) {
                throw std::logic_error("Block has null state in container: " + id.toString());
            }

            state->m_blockId = block->m_blockId;
            u32 stateId = _allocateStateId();
            const_cast<BlockState*>(state.get())->m_stateId = stateId;
            m_statesById[stateId] = state.get();
        }

        BlockType* ptr = block.get();
        // 确保 vector 大小足够，用 blockId 作为索引
        if (m_blocksById.size() <= block->m_blockId) {
            m_blocksById.resize(block->m_blockId + 1, nullptr);
        }
        m_blocksById[blockId] = ptr;
        m_blocks[id] = std::move(block);
        m_numericIds[id] = blockId;

        return *ptr;
    }

    /**
     * @brief 根据ID获取方块
     */
    [[nodiscard]] Block* getBlock(u32 blockId) const
    {
        if (blockId >= m_blocksById.size()) {
            return nullptr;
        }
        return m_blocksById[blockId];
    }

    /**
     * @brief 根据资源位置获取方块
     */
    [[nodiscard]] Block* getBlock(const ResourceLocation& id) const
    {
        auto it = m_blocks.find(id);
        return it != m_blocks.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 根据状态ID获取方块状态
     */
    [[nodiscard]] BlockState* getBlockState(u32 stateId) const
    {
        auto it = m_statesById.find(stateId);
        return it != m_statesById.end() ? it->second : nullptr;
    }

    /**
     * @brief 遍历所有方块
     */
    void forEachBlock(std::function<void(Block&)> callback)
    {
        for (auto& [id, block] : m_blocks) {
            callback(*block);
        }
    }

    /**
     * @brief 遍历所有方块状态
     */
    void forEachBlockState(std::function<void(const BlockState&)> callback)
    {
        for (const auto& [id, block] : m_blocks) {
            (void)id;
            if (!block) {
                continue;
            }
            for (const auto& state : block->stateContainer().validStates()) {
                if (state) {
                    callback(*state);
                }
            }
        }
    }

    /**
     * @brief 获取方块数量
     */
    [[nodiscard]] size_t blockCount() const { return m_blocks.size(); }

    /**
     * @brief 获取状态数量
     */
    [[nodiscard]] size_t stateCount() const { return m_statesById.size(); }

    /**
     * @brief 根据资源位置获取方块状态（便捷方法）
     * @param id 方块资源位置
     * @return 默认方块状态，如果不存在返回空气状态
     */
    [[nodiscard]] const BlockState* get(const ResourceLocation& id) const
    {
        Block* block = getBlock(id);
        return block ? &block->defaultState() : airState();
    }

    /**
     * @brief 获取空气方块状态
     */
    [[nodiscard]] const BlockState* airState() const
    {
        auto* air = getBlock(ResourceLocation("minecraft:air"));
        return air ? &air->defaultState() : nullptr;
    }

    ~BlockRegistry() noexcept = default;

private:
    BlockRegistry() = default;

    /**
     * @brief 分配方块ID
     *
     * 对于 minecraft:air 方块，返回 0 作为保留 ID。
     */
    u32 _allocateBlockId(const ResourceLocation& id)
    {
        // AIR 方块始终获得 ID 0
        if (id == ResourceLocation("minecraft:air")) {
            return 0;
        }
        // 其他方块从 1 开始
        return m_nextBlockId++;
    }

    /**
     * @brief 分配状态ID
     */
    u32 _allocateStateId() noexcept { return m_nextStateId++; }

    std::unordered_map<ResourceLocation, std::unique_ptr<Block>> m_blocks;
    std::unordered_map<ResourceLocation, u32> m_numericIds; // 字符串ID -> 数字ID
    std::vector<Block*> m_blocksById;
    std::unordered_map<u32, BlockState*> m_statesById;
    u32 m_nextBlockId = 1; // 0保留给空气
    u32 m_nextStateId = 0;
};

} // namespace mc
