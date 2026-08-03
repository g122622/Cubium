/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/network/backend/java/mappings/JavaEnchantmentIdMap.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include <string>

namespace mc::network::backend::java {

/**
 * @brief ItemEnchantments wire codec（1.21.11 ItemEnchantments.STREAM_CODEC）
 *
 * vanilla wire：Map<Holder<Enchantment>, Integer>，经 ByteBufCodecs.map 写为
 *   VarInt(entryCount) + [ VarInt(enchantRegistryId) + VarInt(level) ]*
 * key 是 enchantment 注册表 holder id（JavaEnchantmentIdMap 翻译），value 是裸 VarInt 等级。
 *
 * 供 DataComponentPatchWire 的 per-component codec（4c）在 ENCHANTMENTS 分支调用。
 * 项目 EnchantmentContainer 用 vector<EnchantmentInstance>（id 字符串 + level）承载，
 * 编码时按 getAll() 顺序逐项转 wire id，解码时按 wire id 还原字符串 id。
 */

/// 写 ItemEnchantments 载荷。
inline void writeItemEnchantments(
    ::mc::network::buffer::ByteBuf& buf, const ::mc::item::enchant::EnchantmentContainer& ench)
{
    const auto& all = ench.getAll();
    buf.writeVarInt(static_cast<i32>(all.size()));
    for (const auto& entry : all) {
        const u32 enchantId = JavaEnchantmentIdMap::instance().toJavaRegistryId(entry.enchantmentId);
        buf.writeVarInt(static_cast<i32>(enchantId));
        buf.writeVarInt(entry.level);
    }
}

/// 读 ItemEnchantments 载荷。
[[nodiscard]] inline ::mc::Result<::mc::item::enchant::EnchantmentContainer> readItemEnchantments(
    ::mc::network::buffer::ByteBuf& buf)
{
    ::mc::item::enchant::EnchantmentContainer ench{};
    i32 count = 0;
    MC_TRY_ASSIGN(count, buf.readVarInt());
    if (count < 0) {
        return ::mc::Error(::mc::ErrorCode::InvalidData, "ItemEnchantments count is negative", "readItemEnchantments");
    }
    for (i32 i = 0; i < count; ++i) {
        i32 enchantId = 0;
        i32 level = 0;
        MC_TRY_ASSIGN(enchantId, buf.readVarInt());
        MC_TRY_ASSIGN(level, buf.readVarInt());
        std::string name = JavaEnchantmentIdMap::instance().fromJavaRegistryId(static_cast<u32>(enchantId));
        if (!name.empty()) {
            ench.set(name, level);
        }
    }
    return ench;
}

} // namespace mc::network::backend::java
