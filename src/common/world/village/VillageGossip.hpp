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

#include "../../core/Types.hpp"
#include "VillageGossipType.hpp"
#include <optional>
#include <unordered_map>
#include <vector>

namespace mc {
namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt

namespace world {
namespace village {

/**
 * @brief 单条流言记录
 *
 * 记录某个玩家在村庄中的声誉变化事件
 */
struct GossipEntry {
    /// 流言类型
    VillageGossipType type;

    /// 累积值（事件次数）
    i32 value;

    /// 最后更新时间（游戏tick）
    i64 lastUpdateTime;

    /**
     * @brief 执行衰减
     * @param currentTime 当前游戏时间
     * @return 衰减后的值
     */
    i32 decay(i64 currentTime);
};

/**
 * @brief 村庄流言管理器
 *
 * 管理村庄中所有玩家的声誉，影响交易价格。
 *
 * 声誉系统：
 * - 范围: [-1000, +1000]
 * - 正面声誉降低交易价格
 * - 负面声誉提高交易价格
 * - 流言随时间衰减
 *
 * 参考 MC 1.16.5 GossipContainer
 */
class VillageGossipManager {
public:
    /**
     * @brief 默认构造函数
     */
    VillageGossipManager() = default;

    // ========== 流言操作 ==========

    /**
     * @brief 添加流言
     * @param playerId 玩家ID
     * @param type 流言类型
     * @param value 值（默认为1，表示一次事件）
     */
    void addGossip(u64 playerId, VillageGossipType type, i32 value = 1);

    /**
     * @brief 移除指定类型的流言
     * @param playerId 玩家ID
     * @param type 流言类型
     */
    void removeGossip(u64 playerId, VillageGossipType type);

    /**
     * @brief 清除玩家的所有流言
     * @param playerId 玩家ID
     */
    void clearGossip(u64 playerId);

    /**
     * @brief 清除所有流言
     */
    void clearAll();

    // ========== 查询 ==========

    /**
     * @brief 获取玩家声誉
     * @param playerId 玩家ID
     * @return 声誉值（范围 [-1000, +1000]）
     */
    [[nodiscard]] i32 getReputation(u64 playerId) const;

    /**
     * @brief 获取玩家指定类型的流言值
     * @param playerId 玩家ID
     * @param type 流言类型
     * @return 流言值（如果不存在返回0）
     */
    [[nodiscard]] i32 getGossipValue(u64 playerId, VillageGossipType type) const;

    /**
     * @brief 获取价格修正因子
     * @param playerId 玩家ID
     * @return 价格修正因子（0.5-1.5，声誉高则价格低）
     *
     * 计算公式：
     * - priceModifier = clamp(1.0 - reputation/1000.0, 0.5, 1.5)
     * - 声誉+1000 → 价格0.5倍
     * - 声誉0 → 价格1.0倍
     * - 声誉-1000 → 价格1.5倍
     */
    [[nodiscard]] f32 getPriceModifier(u64 playerId) const;

    /**
     * @brief 检查玩家是否有流言记录
     */
    [[nodiscard]] bool hasGossip(u64 playerId) const;

    /**
     * @brief 获取所有有流言记录的玩家ID
     */
    [[nodiscard]] std::vector<u64> getAllPlayers() const;

    // ========== 更新 ==========

    /**
     * @brief 每tick更新（处理衰减）
     * @param gameTime 当前游戏时间
     */
    void tick(i64 gameTime);

    // ========== 序列化 ==========

    /**
     * @brief 序列化到NBT
     */
    void serialize(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从NBT反序列化
     */
    void deserialize(const nbt::tags::compound_tag& tag);

private:
    /// 玩家ID -> 流言列表
    std::unordered_map<u64, std::vector<GossipEntry>> m_gossips;

    /// 声誉范围限制
    static constexpr i32 MIN_REPUTATION = -1000;
    static constexpr i32 MAX_REPUTATION = 1000;
};

} // namespace village
} // namespace world
} // namespace mc
