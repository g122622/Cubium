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

#include "common/network/buffer/ByteBuf.hpp"
#include "common/registry/RegistryAccess.hpp"

#include <optional>

namespace mc::network::buffer {

/**
 * @brief 持有注册表引用的字节缓冲（对应 Java RegistryFriendlyByteBuf）
 *
 * 在 ByteBuf 基础上挂载 const RegistryAccess&，使 codec 在序列化物品/方块/实体类型/
 * 生物群系时能按 VarInt 整数 ID 查回类型指针，或反向由类型取 ID。RegistryAccess 是非拥有
 * 聚合体，引用现有四大注册表单例，本缓冲亦不延长其生命周期。
 *
 * 用法：codec 显式构造本类（传入 RegistryAccess::instance()），读写注册表项时调用
 * writeItemHolder/readItemHolder 等类型化方法。普通定长/VarInt/String 读写继承自 ByteBuf。
 */
class RegistryByteBuf : public ByteBuf {
public:
    /**
     * @brief 默认构造（无注册表绑定），用于不需要注册表查表的纯字节缓冲场景
     *
     * 持有的注册表引用为 std::nullopt；调用 writeItemHolder 等注册表相关方法前须 bindRegistry。
     */
    RegistryByteBuf() = default;

    explicit RegistryByteBuf(const RegistryAccess& registry)
        : m_registry(&registry)
    {}

    /**
     * @brief 从外部字节序列构造（无注册表绑定，反序列化入口）
     *
     * 继承 ByteBuf 的字节视图构造；不绑注册表，需要查表的包 codec 须随后 bindRegistry。
     */
    explicit RegistryByteBuf(const u8* data, usize size)
        : ByteBuf(data, size)
    {}

    /**
     * @brief 从外部字节序列构造并绑定注册表（反序列化入口）
     */
    RegistryByteBuf(const u8* data, usize size, const RegistryAccess& registry)
        : ByteBuf(data, size)
        , m_registry(&registry)
    {}

    /**
     * @brief 绑定/重绑定注册表（同一缓冲可在不同阶段复用）
     */
    void bindRegistry(const RegistryAccess& registry) noexcept { m_registry = &registry; }

    [[nodiscard]] bool hasRegistry() const noexcept { return m_registry.has_value(); }
    [[nodiscard]] const RegistryAccess& registry() const noexcept { return **m_registry; }

    // ============================================================================
    // 物品 holder（VarInt itemId，0 表示空）
    // ============================================================================

    /**
     * @brief 写物品 holder：VarInt itemId（空气/空 = 0）
     *
     * 1.21.11 的 Item holder 仅承载 itemId（不含 count/组件 patch）——后者属于
     * ItemStack 的 OPTIONAL_STREAM_CODEC（VarInt(count)+Item holder+DataComponentPatch），
     * 由 JavaPlayCodecs::writeItemStack 承载，本方法只写裸 itemId。
     */
    void writeItemHolder(const Item* item);

    /**
     * @brief 读物品 holder：VarInt itemId → Item*（查不到返回 nullptr）
     */
    [[nodiscard]] Result<const Item*> readItemHolder();

    // ============================================================================
    // 方块状态 holder（VarInt blockStateId）
    // ============================================================================

    void writeBlockStateHolder(const BlockState* state);
    [[nodiscard]] Result<const BlockState*> readBlockStateHolder();

    // ============================================================================
    // 实体类型 holder（VarInt entityTypeId，按注册顺序）
    // ============================================================================

    void writeEntityTypeHolder(const entity::EntityType* type);
    [[nodiscard]] Result<const entity::EntityType*> readEntityTypeHolder();

private:
    std::optional<const RegistryAccess*> m_registry;
};

} // namespace mc::network::buffer
