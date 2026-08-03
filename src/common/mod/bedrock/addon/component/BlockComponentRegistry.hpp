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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/CustomComponentParameters.hpp"
#include <cstddef>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 单个方块自定义组件
 *
 * 包含组件名称和所有可选的事件回调。
 * 通过BlockComponentRegistry注册到指定的方块类型ID。
 *
 * 回调签名统一为 void(EventType&, const CustomComponentParameters&)，
 * 其中EventType是具体的方块组件事件类型。
 *
 * 组件必须包含命名空间前缀（如"my_pack:custom_behavior"）。
 */
struct BlockCustomComponent {
    /** 组件名称，必须包含命名空间前缀（如"my_pack:custom_behavior"） */
    std::string name;

    /** 从行为包JSON定义传入的参数 */
    CustomComponentParameters parameters;

    // ===== 事件回调 =====
    // 每个回调对应Bedrock API中的一个BlockCustomComponent事件。
    // 未设置的回调保持为空（std::function默认构造为空）。

    /** 实体踩上方块 */
    std::function<void(BlockComponentStepOnEvent&, const CustomComponentParameters&)> onStepOn;

    /** 实体离开方块 */
    std::function<void(BlockComponentStepOffEvent&, const CustomComponentParameters&)> onStepOff;

    /** 方块被放置 */
    std::function<void(BlockComponentOnPlaceEvent&, const CustomComponentParameters&)> onPlace;

    /** 方块被破坏（通用） */
    std::function<void(BlockComponentBreakEvent&, const CustomComponentParameters&)> onBreak;

    /** 玩家破坏方块 */
    std::function<void(BlockComponentPlayerBreakEvent&, const CustomComponentParameters&)> onPlayerBreak;

    /** 玩家与方块交互（右键） */
    std::function<void(BlockComponentPlayerInteractEvent&, const CustomComponentParameters&)> onPlayerInteract;

    /** 玩家放置方块前（可取消） */
    std::function<void(BlockComponentPlayerPlaceBeforeEvent&, const CustomComponentParameters&)> beforeOnPlayerPlace;

    /** 实体落在方块上 */
    std::function<void(BlockComponentEntityFallOnEvent&, const CustomComponentParameters&)> onEntityFallOn;

    /** 随机刻 */
    std::function<void(BlockComponentRandomTickEvent&, const CustomComponentParameters&)> onRandomTick;

    /** 计划刻 */
    std::function<void(BlockComponentTickEvent&, const CustomComponentParameters&)> onTick;

    /** 红石更新 */
    std::function<void(BlockComponentRedstoneUpdateEvent&, const CustomComponentParameters&)> onRedstoneUpdate;

    /** 实体事件 */
    std::function<void(BlockComponentEntityEvent&, const CustomComponentParameters&)> onEntity;

    /** 方块状态变化 */
    std::function<void(BlockComponentBlockStateChangeEvent&, const CustomComponentParameters&)> onBlockStateChange;
};

/**
 * @brief 方块自定义组件注册表
 *
 * 管理所有通过脚本注册的方块自定义组件。
 * 当Block虚拟方法被调用时，查找注册表并派发对应的事件回调。
 *
 * 线程安全：读操作使用共享锁，写操作使用独占锁。
 *
 * 使用示例（脚本侧）：
 * @code
 * system.beforeEvents.startup.subscribe((initEvent) => {
 *     initEvent.blockComponentRegistry.registerCustomComponent(
 *         'my_pack:pressure_plate',
 *         {
 *             onStepOn: (event) => { ... },
 *             onStepOff: (event) => { ... }
 *         }
 *     );
 * });
 * @endcode
 */
class BlockComponentRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static BlockComponentRegistry& instance();

    /**
     * @brief 注册自定义组件到指定方块类型
     *
     * 同一方块类型可以注册多个组件，它们按注册顺序依次调用。
     * 组件名称必须包含命名空间前缀。
     *
     * @param blockTypeId 方块类型ID（如"minecraft:stone"）
     * @param component 自定义组件
     */
    void registerComponent(const std::string& blockTypeId, BlockCustomComponent component);

    /**
     * @brief 注销指定方块类型的所有组件
     *
     * @param blockTypeId 方块类型ID
     * @return 注销的组件数量
     */
    size_t unregisterComponent(const std::string& blockTypeId, const std::string& componentName);

    /**
     * @brief 注销指定方块类型的所有组件
     *
     * @param blockTypeId 方块类型ID
     */
    void unregisterAll(const std::string& blockTypeId);

    // ===== 查询方法 =====

    /** 是否有onStepOn回调 */
    [[nodiscard]] bool hasStepOnCallback(const std::string& blockTypeId) const;
    /** 是否有onStepOff回调 */
    [[nodiscard]] bool hasStepOffCallback(const std::string& blockTypeId) const;
    /** 是否有onPlace回调 */
    [[nodiscard]] bool hasPlaceCallback(const std::string& blockTypeId) const;
    /** 是否有onBreak回调 */
    [[nodiscard]] bool hasBreakCallback(const std::string& blockTypeId) const;
    /** 是否有onPlayerBreak回调 */
    [[nodiscard]] bool hasPlayerBreakCallback(const std::string& blockTypeId) const;
    /** 是否有onPlayerInteract回调 */
    [[nodiscard]] bool hasPlayerInteractCallback(const std::string& blockTypeId) const;
    /** 是否有beforeOnPlayerPlace回调 */
    [[nodiscard]] bool hasPlayerPlaceBeforeCallback(const std::string& blockTypeId) const;
    /** 是否有onEntityFallOn回调 */
    [[nodiscard]] bool hasEntityFallOnCallback(const std::string& blockTypeId) const;
    /** 是否有onRandomTick回调 */
    [[nodiscard]] bool hasRandomTickCallback(const std::string& blockTypeId) const;
    /** 是否有onTick回调 */
    [[nodiscard]] bool hasTickCallback(const std::string& blockTypeId) const;
    /** 是否有onRedstoneUpdate回调 */
    [[nodiscard]] bool hasRedstoneUpdateCallback(const std::string& blockTypeId) const;
    /** 是否有onEntity回调 */
    [[nodiscard]] bool hasEntityCallback(const std::string& blockTypeId) const;
    /** 是否有onBlockStateChange回调 */
    [[nodiscard]] bool hasBlockStateChangeCallback(const std::string& blockTypeId) const;

    // ===== 派发方法 =====
    // 由Block虚拟方法调用点触发。调用所有已注册组件的对应回调。
    // 返回true表示至少有一个组件处理了该事件。

    /** 派发onStepOn事件 */
    bool dispatchStepOn(const std::string& blockTypeId, BlockComponentStepOnEvent& event);

    /** 派发onStepOff事件 */
    bool dispatchStepOff(const std::string& blockTypeId, BlockComponentStepOffEvent& event);

    /** 派发onPlace事件 */
    bool dispatchPlace(const std::string& blockTypeId, BlockComponentOnPlaceEvent& event);

    /** 派发onBreak事件 */
    bool dispatchBreak(const std::string& blockTypeId, BlockComponentBreakEvent& event);

    /** 派发onPlayerBreak事件 */
    bool dispatchPlayerBreak(const std::string& blockTypeId, BlockComponentPlayerBreakEvent& event);

    /** 派发onPlayerInteract事件 */
    bool dispatchPlayerInteract(const std::string& blockTypeId, BlockComponentPlayerInteractEvent& event);

    /**
     * @brief 派发beforeOnPlayerPlace事件
     *
     * 此事件可取消。如果任何回调设置了event.cancel = true，
     * 方块放置应被取消。
     *
     * @return true表示事件被取消
     */
    bool dispatchPlayerPlaceBefore(const std::string& blockTypeId, BlockComponentPlayerPlaceBeforeEvent& event);

    /** 派发onEntityFallOn事件 */
    bool dispatchEntityFallOn(const std::string& blockTypeId, BlockComponentEntityFallOnEvent& event);

    /** 派发onRandomTick事件 */
    bool dispatchRandomTick(const std::string& blockTypeId, BlockComponentRandomTickEvent& event);

    /** 派发onTick事件 */
    bool dispatchTick(const std::string& blockTypeId, BlockComponentTickEvent& event);

    /** 派发onRedstoneUpdate事件 */
    bool dispatchRedstoneUpdate(const std::string& blockTypeId, BlockComponentRedstoneUpdateEvent& event);

    /** 派发onEntity事件 */
    bool dispatchEntity(const std::string& blockTypeId, BlockComponentEntityEvent& event);

    /** 派发onBlockStateChange事件 */
    bool dispatchBlockStateChange(const std::string& blockTypeId, BlockComponentBlockStateChangeEvent& event);

    /**
     * @brief 清除所有已注册的组件
     *
     * 在脚本系统关闭或重载时调用。
     */
    void clear();

    /**
     * @brief 获取已注册组件的方块类型数量
     */
    [[nodiscard]] size_t registeredBlockTypeCount() const;

    /**
     * @brief 获取指定方块类型的组件数量
     */
    [[nodiscard]] size_t componentCount(const std::string& blockTypeId) const;

private:
    BlockComponentRegistry() = default;
    ~BlockComponentRegistry() = default;

    // ===== 私有方法 =====

    /** 注册或更新回调标志位 */
    void _updateCallbackFlags(const std::string& blockTypeId);

    // ===== 数据成员 =====

    /** blockTypeId → 组件列表 */
    std::unordered_map<std::string, std::vector<BlockCustomComponent>> m_components;

    /** 快速查询位掩码：每种事件类型一个位，避免逐组件查找 */
    struct CallbackFlags {
        u32 hasStepOn : 1 = 0;
        u32 hasStepOff : 1 = 0;
        u32 hasPlace : 1 = 0;
        u32 hasBreak : 1 = 0;
        u32 hasPlayerBreak : 1 = 0;
        u32 hasPlayerInteract : 1 = 0;
        u32 hasPlayerPlaceBefore : 1 = 0;
        u32 hasEntityFallOn : 1 = 0;
        u32 hasRandomTick : 1 = 0;
        u32 hasTick : 1 = 0;
        u32 hasRedstoneUpdate : 1 = 0;
        u32 hasEntity : 1 = 0;
        u32 hasBlockStateChange : 1 = 0;
        u32 reserved : 19 = 0;
    };

    /** blockTypeId → 回调标志位 */
    std::unordered_map<std::string, CallbackFlags> m_callbackFlags;

    mutable std::shared_mutex m_mutex;
};

} // namespace mc::mod::bedrock::addon
