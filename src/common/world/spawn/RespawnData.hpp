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
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>
#include <string>

namespace mc {

/**
 * @brief 出生数据（维度 + 方块位置 + 朝向）
 *
 * 同时用于两处语义：
 * - 世界出生点（level.dat 的 spawn 复合标签、ClientboundSetDefaultSpawnPosition）；
 * - 玩家个人重生点（在玩家存档的 respawn 复合标签中，作为 RespawnConfig 的载荷）。
 *
 * 位置是"方块坐标"语义：非强制重生点落在床/重生锚方块上，世界出生点落在
 * 出生点方块本身（其上方一格为玩家脚部所在的空气格）。
 *
 * 编码键（NBT 复合标签）：dimension（维度资源位置字符串）、pos（3 个 int）、
 * yaw（[-180, 180]）、pitch（[-90, 90]）。
 */
struct RespawnData {
    /// 维度 + 方块位置
    GlobalPos globalPos;

    /// 朝向（水平，度，[-180, 180]）
    f32 yaw = 0.0f;

    /// 俯仰（垂直，度，[-90, 90]）
    f32 pitch = 0.0f;

    /**
     * @brief 归一化朝向并构造出生数据
     *
     * yaw 折叠到 [-180, 180]，pitch 夹紧到 [-90, 90]。
     */
    [[nodiscard]] static RespawnData of(DimensionId dimension, const BlockPos& pos, f32 yaw, f32 pitch);

    /** @brief 维度 ID */
    [[nodiscard]] DimensionId dimension() const noexcept { return globalPos.getDimensionId(); }

    /** @brief 方块位置 */
    [[nodiscard]] const BlockPos& pos() const noexcept { return globalPos.getPos(); }

    /**
     * @brief 编码为 NBT 复合标签
     * @param tag 输出的复合标签
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从 NBT 复合标签解码
     * @param tag 输入的复合标签
     * @return 解码结果；缺少 dim、pos 非法时返回 nullopt
     */
    [[nodiscard]] static std::optional<RespawnData> fromNbt(const nbt::tags::compound_tag& tag);

    /** @brief 相等比较（维度、位置、朝向完全一致） */
    [[nodiscard]] bool operator==(const RespawnData& other) const noexcept
    {
        return globalPos == other.globalPos && yaw == other.yaw && pitch == other.pitch;
    }
    [[nodiscard]] bool operator!=(const RespawnData& other) const noexcept { return !(*this == other); }

    /** @brief 比较全局位置（忽略朝向）——用于"同一切重生点"判定 */
    [[nodiscard]] bool isSamePosition(const RespawnData& other) const noexcept { return globalPos == other.globalPos; }
};

namespace world::storage {

/**
 * @brief 存档级世界出生数据
 *
 * 整个存档只有一份，保存在主世界 level.dat 的 spawn 复合标签中；维度由数据自身携带，
 * 但当前只支持主世界维度。
 */
class LevelRespawnData {
public:
    /// 默认出生数据：主世界原点、朝向 (0, 0)。
    LevelRespawnData() = default;

    /// 获取原始出生数据（未做世界边界修正）
    [[nodiscard]] const RespawnData& get() const;

    /// 设置原始出生数据
    void set(const RespawnData& data);

private:
    RespawnData m_data;
};

} // namespace world::storage

} // namespace mc