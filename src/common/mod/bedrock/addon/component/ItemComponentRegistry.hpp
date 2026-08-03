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
#include "common/mod/bedrock/addon/component/CustomComponentParameters.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include <cstddef>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 单个物品自定义组件
 *
 * 包含组件名称和所有可选的事件回调。
 * 通过ItemComponentRegistry注册到指定的物品类型ID。
 *
 * 组件必须包含命名空间前缀（如"my_pack:special_ability"）。
 */
struct ItemCustomComponent {
    /** 组件名称，必须包含命名空间前缀 */
    std::string name;

    /** 从行为包JSON定义传入的参数 */
    CustomComponentParameters parameters;

    // ===== 事件回调 =====

    /** 物品使用（空中右键） */
    std::function<void(ItemComponentUseEvent&, const CustomComponentParameters&)> onUse;

    /** 物品对方块使用（右键方块） */
    std::function<void(ItemComponentUseOnEvent&, const CustomComponentParameters&)> onUseOn;

    /** 物品击中实体 */
    std::function<void(ItemComponentHitEntityEvent&, const CustomComponentParameters&)> onHitEntity;

    /** 物品挖掘方块 */
    std::function<void(ItemComponentMineBlockEvent&, const CustomComponentParameters&)> onMineBlock;

    /** 耐久伤害前（可修改伤害值） */
    std::function<void(ItemComponentBeforeDurabilityDamageEvent&, const CustomComponentParameters&)>
        onBeforeDurabilityDamage;

    /** 物品使用完成（蓄力完成） */
    std::function<void(ItemComponentCompleteUseEvent&, const CustomComponentParameters&)> onCompleteUse;

    /** 物品被消耗（食物/药水） */
    std::function<void(ItemComponentConsumeEvent&, const CustomComponentParameters&)> onConsume;
};

/**
 * @brief 物品自定义组件注册表
 *
 * 管理所有通过脚本注册的物品自定义组件。
 * 当Item虚拟方法被调用时，查找注册表并派发对应的事件回调。
 *
 * 线程安全：读操作使用共享锁，写操作使用独占锁。
 *
 * 使用示例（脚本侧）：
 * @code
 * system.beforeEvents.startup.subscribe((initEvent) => {
 *     initEvent.itemComponentRegistry.registerCustomComponent(
 *         'my_pack:thunder_sword',
 *         {
 *             onUse: (event) => { ... },
 *             onHitEntity: (event) => { ... }
 *         }
 *     );
 * });
 * @endcode
 */
class ItemComponentRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static ItemComponentRegistry& instance();

    /**
     * @brief 注册自定义组件到指定物品类型
     *
     * 同一物品类型可以注册多个组件，它们按注册顺序依次调用。
     *
     * @param itemTypeId 物品类型ID（如"minecraft:diamond_sword"）
     * @param component 自定义组件
     */
    void registerComponent(const std::string& itemTypeId, ItemCustomComponent component);

    /**
     * @brief 注销指定物品类型的指定组件
     *
     * @param itemTypeId 物品类型ID
     * @param componentName 组件名称
     * @return 注销的组件数量
     */
    size_t unregisterComponent(const std::string& itemTypeId, const std::string& componentName);

    /**
     * @brief 注销指定物品类型的所有组件
     *
     * @param itemTypeId 物品类型ID
     */
    void unregisterAll(const std::string& itemTypeId);

    // ===== 查询方法 =====

    [[nodiscard]] bool hasUseCallback(const std::string& itemTypeId) const;
    [[nodiscard]] bool hasUseOnCallback(const std::string& itemTypeId) const;
    [[nodiscard]] bool hasHitEntityCallback(const std::string& itemTypeId) const;
    [[nodiscard]] bool hasMineBlockCallback(const std::string& itemTypeId) const;
    [[nodiscard]] bool hasBeforeDurabilityDamageCallback(const std::string& itemTypeId) const;
    [[nodiscard]] bool hasCompleteUseCallback(const std::string& itemTypeId) const;
    [[nodiscard]] bool hasConsumeCallback(const std::string& itemTypeId) const;

    // ===== 派发方法 =====

    /** 派发onUse事件 */
    bool dispatchUse(const std::string& itemTypeId, ItemComponentUseEvent& event);

    /** 派发onUseOn事件 */
    bool dispatchUseOn(const std::string& itemTypeId, ItemComponentUseOnEvent& event);

    /** 派发onHitEntity事件 */
    bool dispatchHitEntity(const std::string& itemTypeId, ItemComponentHitEntityEvent& event);

    /** 派发onMineBlock事件 */
    bool dispatchMineBlock(const std::string& itemTypeId, ItemComponentMineBlockEvent& event);

    /**
     * @brief 派发onBeforeDurabilityDamage事件
     *
     * 此事件可修改event.durabilityDamage值。
     */
    bool dispatchBeforeDurabilityDamage(const std::string& itemTypeId, ItemComponentBeforeDurabilityDamageEvent& event);

    /** 派发onCompleteUse事件 */
    bool dispatchCompleteUse(const std::string& itemTypeId, ItemComponentCompleteUseEvent& event);

    /** 派发onConsume事件 */
    bool dispatchConsume(const std::string& itemTypeId, ItemComponentConsumeEvent& event);

    /**
     * @brief 清除所有已注册的组件
     */
    void clear() noexcept;

    /**
     * @brief 获取已注册组件的物品类型数量
     */
    [[nodiscard]] size_t registeredItemTypeCount() const noexcept;

    /**
     * @brief 获取指定物品类型的组件数量
     */
    [[nodiscard]] size_t componentCount(const std::string& itemTypeId) const noexcept;

private:
    ItemComponentRegistry() noexcept = default;

    std::unordered_map<std::string, std::vector<ItemCustomComponent>> m_components;

    struct CallbackFlags {
        u32 hasUse : 1 = 0;
        u32 hasUseOn : 1 = 0;
        u32 hasHitEntity : 1 = 0;
        u32 hasMineBlock : 1 = 0;
        u32 hasBeforeDurabilityDamage : 1 = 0;
        u32 hasCompleteUse : 1 = 0;
        u32 hasConsume : 1 = 0;
        u32 reserved : 25 = 0;
    };

    std::unordered_map<std::string, CallbackFlags> m_callbackFlags;

    /**
     * @brief 更新指定物品类型的回调标志
     *
     * 遍历该物品的所有组件，设置各事件回调是否存在。
     *
     * @param itemTypeId 物品类型ID
     */
    void _updateCallbackFlags(const std::string& itemTypeId);

    mutable std::shared_mutex m_mutex;
};

} // namespace mc::mod::bedrock::addon
