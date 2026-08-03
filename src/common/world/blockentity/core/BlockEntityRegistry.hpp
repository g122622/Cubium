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

#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

/**
 * @brief 方块实体注册表
 *
 * 管理所有方块实体类型的注册和创建。
 * 提供工厂方法注册和从JSON反序列化创建方块实体。
 *
 * 参考: net.minecraft.tileentity.TileEntityType
 *
 * 用法示例:
 * @code
 * // 注册方块实体类型
 * BlockEntityRegistry::instance().registerType(
 *     BlockEntityType::Chest,
 *     [](const BlockPos& pos) { return std::make_unique<ChestEntity>(pos); }
 * );
 *
 * // 创建方块实体
 * auto entity = BlockEntityRegistry::instance().create(BlockEntityType::Chest, BlockPos(0, 0, 0));
 *
 * // 从JSON创建
 * nlohmann::json data = {{"id", "minecraft:chest"}, {"x", 0}, {"y", 0}, {"z", 0}};
 * auto entity = BlockEntityRegistry::instance().createFromJson(data);
 * @endcode
 *
 * 线程安全:
 * - 注册应在启动时完成（主线程）
 * - 创建操作是线程安全的
 */
class BlockEntityRegistry {
public:
    /**
     * @brief 方块实体工厂函数类型
     * @param pos 方块位置
     * @return 新创建的方块实体
     */
    using Factory = std::function<std::unique_ptr<BlockEntity>(const BlockPos& pos)>;

    /**
     * @brief 获取单例实例
     * @return 注册表实例引用
     */
    static BlockEntityRegistry& instance();

    // 禁用拷贝和移动
    BlockEntityRegistry(const BlockEntityRegistry&) = delete;
    BlockEntityRegistry& operator=(const BlockEntityRegistry&) = delete;

    // ========== 注册 ==========

    /**
     * @brief 注册方块实体类型
     * @param type 方块实体类型
     * @param factory 工厂函数
     *
     * 如果类型已注册，将覆盖原有工厂。
     */
    void registerType(BlockEntityType type, Factory factory);

    /**
     * @brief 批量注册所有内置方块实体类型
     *
     * 在游戏启动时调用一次。
     */
    void registerBuiltinTypes();

    // ========== 创建 ==========

    /**
     * @brief 创建方块实体
     * @param type 方块实体类型
     * @param pos 方块位置
     * @return 新创建的方块实体，如果类型未注册返回nullptr
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> create(BlockEntityType type, const BlockPos& pos) const;

    /**
     * @brief 从JSON数据创建方块实体
     * @param data JSON数据（必须包含"id"字段）
     * @return 新创建的方块实体，如果类型未知返回nullptr
     *
     * JSON格式:
     * @code
     * {
     *     "id": "minecraft:chest",
     *     "x": 100,
     *     "y": 64,
     *     "z": -200,
     *     ... // 其他方块实体特定数据
     * }
     * @endcode
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createFromJson(const nlohmann::json& data) const;

    // ========== 查询 ==========

    /**
     * @brief 检查类型是否已注册
     * @param type 方块实体类型
     * @return 如果已注册返回true
     */
    [[nodiscard]] bool hasType(BlockEntityType type) const;

    /**
     * @brief 获取已注册类型数量
     * @return 类型数量
     */
    [[nodiscard]] size_t typeCount() const { return m_factories.size(); }

private:
    BlockEntityRegistry() = default;
    ~BlockEntityRegistry() = default;

    std::unordered_map<BlockEntityType, Factory> m_factories;
};

// ========== 便捷注册宏 ==========

/**
 * @brief 方块实体注册器
 *
 * 用于在全局初始化时注册方块实体类型。
 *
 * 用法示例:
 * @code
 * static BlockEntityRegistrar<ChestEntity> chestRegistrar(BlockEntityType::Chest);
 * @endcode
 */
template <typename T>
class BlockEntityRegistrar {
public:
    explicit BlockEntityRegistrar(BlockEntityType type)
    {
        BlockEntityRegistry::instance().registerType(
            type, [](const BlockPos& pos) { return std::make_unique<T>(pos); });
    }
};

} // namespace blockentity
} // namespace mc
