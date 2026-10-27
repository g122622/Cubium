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
#include "common/util/nbt/Nbt.hpp"
#include "common/world/spawn/RespawnData.hpp"
#include <optional>

namespace mc {

/**
 * @brief 玩家个人重生点配置
 *
 * 由床、重生锚、/spawnpoint 命令设置，随玩家存档持久化（NBT 键 respawn）。
 *
 * forced 语义：为 true 时（仅 /spawnpoint 命令产生）重生位置不再要求是床或重生锚，
 * 只要目标方块与它上方一格都"可在此重生"（非固体、非液体）即生效，且不消耗重生锚充能。
 */
struct RespawnConfig {
    /// 重生位置与朝向
    RespawnData respawnData;

    /// 是否强制（跳过床/重生锚校验）
    bool forced = false;

    /**
     * @brief 重生点所在维度配置为空时回退到主世界
     */
    [[nodiscard]] static DimensionId getDimensionOrDefault(const RespawnConfig* config);

    /**
     * @brief 是否与另一配置指向同一切重生点（只比较维度与方块位置）
     *
     * 朝向与 forced 不参与比较：在同一个方块上重新设置朝向不会触发"重生点已设置"提示。
     */
    [[nodiscard]] bool isSamePosition(const std::optional<RespawnConfig>& other) const noexcept;

    /**
     * @brief 编码为 NBT 复合标签（dimension/pos/yaw/pitch/forced）
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从 NBT 复合标签解码
     * @return 解码结果；缺少合法 RespawnData 时返回 nullopt
     */
    [[nodiscard]] static std::optional<RespawnConfig> fromNbt(const nbt::tags::compound_tag& tag);
};

} // namespace mc
