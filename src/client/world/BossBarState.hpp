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

#include "common/core/Types.hpp"

#include <vector>

namespace mc::client {

/**
 * @brief 客户端 Boss 条镜像状态
 *
 * 由 boss_event(clientbound id=9) 包同步自服务端。服务端 BossInfo/ServerBossInfo 等类位于
 * src/server/bossbar，依赖 IServer 连接管理器，客户端不复用；此处仅保留渲染所需的镜像状态。
 *
 * 注意：刻意不放进 mc::client::world 子命名空间。BossBarState 被 ClientApplication.hpp 直接
 * include，若声明于 mc::client::world 会令该子命名空间在所有引入 ClientApplication.hpp 的 TU
 * 中提前具名存在，破坏既有 world::biome / world::chunk / world::DimensionRenderSettings 等
 * 非限定名的"先 mc::client::world 再回退 mc::world"两段查找——一旦 mc::client::world 具名但
 * 缺目标成员，C++ 不再回退 mc::world 而直接报错。故与 ClientMapDataCache 同置于 mc::client。
 *
 * nameNbtBytes 保留原始 Component NBT wire 字节，渲染时用 componentNbtBytesToPlainText
 * 还原显示文本（与 Title/SystemChat 分支同套有损纯文本还原）。
 *
 * TODO: 当前客户端无 Boss 条 HUD 渲染，状态先行落地。未来在 HUD 层新增 BossBarWidget，
 * 从 ClientApplication::m_bossBars 取状态渲染。
 */
struct BossBarState {
    /// Boss 名称（opaque Component NBT wire 字节，ADD/UPDATE_NAME 写入）
    std::vector<u8> nameNbtBytes;

    /// 进度 0..1（ADD/UPDATE_PROGRESS 写入）
    f32 progress = 1.0f;

    /// 颜色 ordinal 0-6（BossInfoColor：Pink/Blue/Red/Green/Yellow/Purple/White）
    i32 color = 0;

    /// 覆盖层 ordinal 0-4（BossInfoOverlay：Progress/Notched6/Notched10/Notched12/Notched20）
    i32 overlay = 0;

    /// flags 位域：bit0=DARKEN_SKY bit1=PLAY_END_BOSS_MUSIC bit2=CREATE_FOG
    bool darkenSky = false;
    bool playEndBossMusic = false;
    bool createFog = false;
};

/**
 * @brief 从 boss_event 包的 flags 字节拆出三个属性位
 *
 * 与服务端 ServerDragonBossBar::packBossFlags 严格互逆。
 */
inline void applyBossFlags(BossBarState& state, u8 flags) noexcept
{
    state.darkenSky = (flags & 0x01) != 0;
    state.playEndBossMusic = (flags & 0x02) != 0;
    state.createFog = (flags & 0x04) != 0;
}

} // namespace mc::client
