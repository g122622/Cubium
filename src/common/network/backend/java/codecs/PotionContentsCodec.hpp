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
#include "common/item/component/DataComponentMap.hpp"
#include "common/network/backend/java/codecs/MobEffectInstanceCodec.hpp"
#include "common/network/backend/java/mappings/JavaPotionIdMap.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace mc::network::backend::java {

/**
 * @brief PotionContents wire codec（1.21.11 PotionContents.STREAM_CODEC）
 *
 * vanilla wire 顺序（StreamCodec.composite）：
 *   Optional<Holder<Potion>> potion → Optional<Integer> customColor
 *   → List<MobEffectInstance> customEffects → Optional<String> customName
 *
 * Optional = Bool(present) + [value if present]；List = VarInt(count) + [item]*。
 * potion holder 是 potion 静态注册表的 VarInt id（JavaPotionIdMap 翻译）；项目以资源位置
 * 字符串承载，空串=无药水（potionId 为空时 present=false）。
 *
 * 供 DataComponentPatchWire 的 per-component codec（4c）在 POTION_CONTENTS 分支调用。
 */

/// 写 PotionContents 载荷。
inline void writePotionContentsPayload(
    ::mc::network::buffer::ByteBuf& buf, const ::mc::item::component::PotionContentsPayload& pc)
{
    // Optional<Holder<Potion>>：空串=absent，否则 present + VarInt(potionRegistryId)。
    if (pc.potionId.empty()) {
        buf.writeBool(false);
    } else {
        buf.writeBool(true);
        const u32 potionId = JavaPotionIdMap::instance().toJavaRegistryId(pc.potionId);
        buf.writeVarInt(static_cast<i32>(potionId));
    }
    // Optional<Integer> customColor。vanilla PotionContents.STREAM_CODEC 用 ByteBufCodecs.INT
    // （定长 4 字节大端 writeInt，非 VarInt），故此处须 writeI32 对齐，否则颜色为小正值时
    // VarInt(1 字节) vs INT(4 字节) 少 3 字节，客户端把后续 3 字节吞进 color 连锁错位。
    if (pc.customColor.has_value()) {
        buf.writeBool(true);
        buf.writeI32(*pc.customColor);
    } else {
        buf.writeBool(false);
    }
    // List<MobEffectInstance> customEffects。
    buf.writeVarInt(static_cast<i32>(pc.customEffects.size()));
    for (const auto& effect : pc.customEffects) {
        writeMobEffectInstance(buf, effect);
    }
    // Optional<String> customName。
    if (pc.customName.has_value()) {
        buf.writeBool(true);
        buf.writeString(*pc.customName);
    } else {
        buf.writeBool(false);
    }
}

/// 读 PotionContents 载荷。
[[nodiscard]] inline ::mc::Result<::mc::item::component::PotionContentsPayload> readPotionContentsPayload(
    ::mc::network::buffer::ByteBuf& buf)
{
    ::mc::item::component::PotionContentsPayload pc{};
    // Optional<Holder<Potion>>。
    bool hasPotion = false;
    MC_TRY_ASSIGN(hasPotion, buf.readBool());
    if (hasPotion) {
        i32 potionId = 0;
        MC_TRY_ASSIGN(potionId, buf.readVarInt());
        pc.potionId = JavaPotionIdMap::instance().fromJavaRegistryId(static_cast<u32>(potionId));
    }
    // Optional<Integer> customColor（定长 4 字节大端 INT，非 VarInt）。
    bool hasColor = false;
    MC_TRY_ASSIGN(hasColor, buf.readBool());
    if (hasColor) {
        i32 color = 0;
        MC_TRY_ASSIGN(color, buf.readI32());
        pc.customColor = color;
    }
    // List<MobEffectInstance> customEffects。
    i32 effectCount = 0;
    MC_TRY_ASSIGN(effectCount, buf.readVarInt());
    pc.customEffects.reserve(static_cast<size_t>(std::max(0, effectCount)));
    for (i32 i = 0; i < effectCount; ++i) {
        auto effectResult = readMobEffectInstance(buf);
        if (effectResult.failed()) {
            return effectResult.error();
        }
        pc.customEffects.push_back(effectResult.value());
    }
    // Optional<String> customName。
    bool hasName = false;
    MC_TRY_ASSIGN(hasName, buf.readBool());
    if (hasName) {
        std::string name;
        MC_TRY_ASSIGN(name, buf.readString());
        pc.customName = std::move(name);
    }
    return pc;
}

} // namespace mc::network::backend::java
