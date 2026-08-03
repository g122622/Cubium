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
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/network/backend/java/mappings/JavaMobEffectIdMap.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include <memory>

namespace mc::network::backend::java {

/**
 * @brief MobEffectInstance wire codec（1.21.11 MobEffectInstance.STREAM_CODEC）
 *
 * vanilla wire 顺序（StreamCodec.composite）：
 *   effect holder(VarInt registryId) → amplifier(VAR_INT) → duration(VAR_INT)
 *   → ambient(BOOL) → showParticles(BOOL) → showIcon(BOOL) → hiddenEffect(optional 递归 Details)
 *
 * effect holder 是 mob_effect 静态注册表的 VarInt id，由 JavaMobEffectIdMap 在项目
 * EffectType 与 vanilla registry id 间翻译（mob_effect 不经 RegistryData 同步，客户端按
 * MobEffects.bootstrap 顺序自分配 id）。1.21.11 已移除 factorData（改 BlendState，不入 wire）。
 *
 * hiddenEffect 用 Bool(present) 前缀 + 递归 Details（同此 7 字段，去掉 effect holder——
 * vanilla Details.STREAM_CODEC 不含 effect，因 hiddenEffect 复用外层 effect 类型）。本项目
 * EffectInstance.hiddenEffect 递归存完整 EffectInstance，编码时 hiddenEffect 段写 effect
 * holder 会被真客户端忽略路径出错——故 hiddenEffect 段按 vanilla Details（6 字段，无 effect
 * holder）编码，effect 类型复用外层。
 */

namespace detail {

/// 写 hiddenEffect 段（vanilla Details：6 字段，无 effect holder）。effect 类型复用外层。
inline void writeHiddenEffectDetails(
    ::mc::network::buffer::ByteBuf& buf, const ::mc::entity::effect::EffectInstance& inst)
{
    buf.writeVarInt(inst.amplifier());
    buf.writeVarInt(inst.duration());
    buf.writeBool(inst.isAmbient());
    buf.writeBool(inst.isVisible()); // showParticles
    buf.writeBool(inst.showIcon());
    const ::mc::entity::effect::EffectInstance* hidden = inst.hiddenEffect();
    if (hidden != nullptr) {
        buf.writeBool(true);
        writeHiddenEffectDetails(buf, *hidden); // 递归
    } else {
        buf.writeBool(false);
    }
}

/// 读 hiddenEffect 段（vanilla Details：6 字段，无 effect holder）。effect 类型由外层传入。
[[nodiscard]] inline ::mc::Result<::mc::entity::effect::EffectInstance> readHiddenEffectDetails(
    ::mc::network::buffer::ByteBuf& buf, ::mc::entity::effect::EffectType type)
{
    i32 amplifier = 0;
    i32 duration = 0;
    bool ambient = false;
    bool showParticles = false;
    bool showIcon = false;
    MC_TRY_ASSIGN(amplifier, buf.readVarInt());
    MC_TRY_ASSIGN(duration, buf.readVarInt());
    MC_TRY_ASSIGN(ambient, buf.readBool());
    MC_TRY_ASSIGN(showParticles, buf.readBool());
    MC_TRY_ASSIGN(showIcon, buf.readBool());
    ::mc::entity::effect::EffectInstance inst(type, duration, amplifier, ambient, showParticles, showIcon);

    bool hasHidden = false;
    MC_TRY_ASSIGN(hasHidden, buf.readBool());
    if (hasHidden) {
        auto hiddenResult = readHiddenEffectDetails(buf, type);
        if (hiddenResult.failed()) {
            return hiddenResult.error();
        }
        inst.setHiddenEffect(std::make_shared<::mc::entity::effect::EffectInstance>(hiddenResult.value()));
    }
    return inst;
}

} // namespace detail

/// 写完整 MobEffectInstance（含 effect holder）。
inline void writeMobEffectInstance(
    ::mc::network::buffer::ByteBuf& buf, const ::mc::entity::effect::EffectInstance& inst)
{
    const u32 effectId = JavaMobEffectIdMap::instance().toJavaRegistryId(inst.type());
    buf.writeVarInt(static_cast<i32>(effectId));
    buf.writeVarInt(inst.amplifier());
    buf.writeVarInt(inst.duration());
    buf.writeBool(inst.isAmbient());
    buf.writeBool(inst.isVisible()); // showParticles
    buf.writeBool(inst.showIcon());
    const ::mc::entity::effect::EffectInstance* hidden = inst.hiddenEffect();
    if (hidden != nullptr) {
        buf.writeBool(true);
        detail::writeHiddenEffectDetails(buf, *hidden);
    } else {
        buf.writeBool(false);
    }
}

/// 读完整 MobEffectInstance（含 effect holder）。
[[nodiscard]] inline ::mc::Result<::mc::entity::effect::EffectInstance> readMobEffectInstance(
    ::mc::network::buffer::ByteBuf& buf)
{
    i32 effectId = 0;
    MC_TRY_ASSIGN(effectId, buf.readVarInt());
    const auto type = JavaMobEffectIdMap::instance().fromJavaRegistryId(static_cast<u32>(effectId));
    i32 amplifier = 0;
    i32 duration = 0;
    bool ambient = false;
    bool showParticles = false;
    bool showIcon = false;
    MC_TRY_ASSIGN(amplifier, buf.readVarInt());
    MC_TRY_ASSIGN(duration, buf.readVarInt());
    MC_TRY_ASSIGN(ambient, buf.readBool());
    MC_TRY_ASSIGN(showParticles, buf.readBool());
    MC_TRY_ASSIGN(showIcon, buf.readBool());
    ::mc::entity::effect::EffectInstance inst(type, duration, amplifier, ambient, showParticles, showIcon);

    bool hasHidden = false;
    MC_TRY_ASSIGN(hasHidden, buf.readBool());
    if (hasHidden) {
        auto hiddenResult = detail::readHiddenEffectDetails(buf, type);
        if (hiddenResult.failed()) {
            return hiddenResult.error();
        }
        inst.setHiddenEffect(std::make_shared<::mc::entity::effect::EffectInstance>(hiddenResult.value()));
    }
    return inst;
}

} // namespace mc::network::backend::java
