/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include <string>

namespace mc {

/**
 * @brief 统一注册表访问入口
 *
 * 聚合持有四大注册表（Item/Block/Entity/Biome）的非拥有引用，为网络层 codec
 * 提供按整数 ID / 资源位置查表的统一入口。对应 MC Java 1.21.11 的 RegistryAccess
 * （registry-of-registries）角色——RegistryByteBuf 持有本对象，序列化物品/方块/
 * 实体类型时按 VarInt 整数 ID 查回类型指针。
 *
 * 设计要点：
 * - 非拥有：仅引用现有单例（XxxRegistry::instance()），不延长其生命周期。
 * - 不统一接口：四大注册表历史接口不一（key 类型有 u32/u16/string/ResourceLocation
 *   之别，返回有指针/引用之别），强行套泛型 Registry<T> 基类会引入 adapter 层
 *   违反"不加兼容层"准则。故本类提供具名类型化方法，直接转发各单例。
 * - 网络层只用整数 ID 路径（itemId/blockId/stateId/entityTypeId/biomeId），
 *   byKey 路径供存档/命令等复用。
 */
class RegistryAccess {
public:
    /**
     * @brief 获取默认单例（引用各注册表的 instance()）
     *
     * 适用于无显式注册表注入的场景（网络层编解码、命令系统等）。
     */
    static RegistryAccess& instance();

    // 禁止拷贝和移动：聚合引用不应被复制
    RegistryAccess(const RegistryAccess&) = delete;
    RegistryAccess& operator=(const RegistryAccess&) = delete;
    RegistryAccess(RegistryAccess&&) = delete;
    RegistryAccess& operator=(RegistryAccess&&) = delete;

    // ============================================================================
    // 物品注册表
    // ============================================================================

    /**
     * @brief 按整数 ID 查物品（网络层 VarInt itemId 解码后用）
     */
    [[nodiscard]] Item* itemById(ItemId id) const { return ItemRegistry::instance().getItem(id); }

    /**
     * @brief 按资源位置查物品
     */
    [[nodiscard]] Item* itemByKey(const ResourceLocation& key) const { return ItemRegistry::instance().getItem(key); }

    /**
     * @brief 取物品的整数 ID（网络层编码用）
     *
     * 物品不存在返回 0（空气）。
     */
    [[nodiscard]] ItemId itemIdOf(const Item* item) const { return item != nullptr ? item->itemId() : 0; }

    // ============================================================================
    // 方块注册表
    // ============================================================================

    /**
     * @brief 按方块整数 ID 查方块
     */
    [[nodiscard]] Block* blockById(u32 id) const { return BlockRegistry::instance().getBlock(id); }

    /**
     * @brief 按资源位置查方块
     */
    [[nodiscard]] Block* blockByKey(const ResourceLocation& key) const
    {
        return BlockRegistry::instance().getBlock(key);
    }

    /**
     * @brief 按方块状态整数 ID 查方块状态（区块 palette 索引、BlockUpdate 等用）
     */
    [[nodiscard]] BlockState* blockStateById(u32 stateId) const
    {
        return BlockRegistry::instance().getBlockState(stateId);
    }

    // ============================================================================
    // 实体类型注册表
    // ============================================================================

    /**
     * @brief 按整数 ID 查实体类型（网络层 SpawnEntity 的 VarInt 类型 ID 解码后用）
     *
     * ID 由 EntityRegistry::registerType 按注册顺序分配。
     */
    [[nodiscard]] const entity::EntityType* entityTypeById(u32 id) const
    {
        return entity::EntityRegistry::instance().getTypeById(id);
    }

    /**
     * @brief 按资源位置名查实体类型
     */
    [[nodiscard]] const entity::EntityType* entityTypeByKey(const std::string& name) const
    {
        return entity::EntityRegistry::instance().getType(name);
    }

    /**
     * @brief 取实体类型的整数 ID（网络层编码用）
     *
     * 类型为空返回 0。
     */
    [[nodiscard]] u32 entityTypeIdOf(const entity::EntityType* type) const { return type != nullptr ? type->id() : 0; }

    // ============================================================================
    // 生物群系注册表
    // ============================================================================

    /**
     * @brief 按整数 ID 查生物群系
     *
     * BiomeId 当前为硬编码常量（BiomeIds.hpp），与原版 ID 一致。
     */
    [[nodiscard]] const world::biome::Biome& biomeById(BiomeId id) const
    {
        return world::biome::BiomeRegistry::instance().get(id);
    }

private:
    RegistryAccess() = default;
};

} // namespace mc
