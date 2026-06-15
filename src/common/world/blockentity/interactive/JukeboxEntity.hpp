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

#include "world/blockentity/ContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 唱片机方块实体
 *
 * 唱片机用于播放音乐唱片，特点：
 * - 1个槽位存放唱片
 * - 播放音乐时发射红石信号
 * - 可以被漏斗提取唱片
 *
 * 参考: net.minecraft.block.entity.JukeboxBlockEntity
 */
class JukeboxEntity : public ContainerBlockEntity {
public:
    /// 唱片机只有1个槽位
    static constexpr i32 SLOT_RECORD = 0;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit JukeboxEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~JukeboxEntity() noexcept override;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return 1; }

    // ========== 唱片机接口 ==========

    /**
     * @brief 获取唱片
     * @return 唱片物品
     */
    [[nodiscard]] ItemStack getRecord() const;

    /**
     * @brief 设置唱片并更新播放状态
     *
     * 设置唱片后自动更新 HAS_RECORD 方块状态，
     * 并根据唱片内容开始或停止播放。
     *
     * @param record 唱片物品
     * @param world 世界引用（用于更新方块状态和播放音效）
     */
    void setRecord(const ItemStack& record, IWorld& world);

    /**
     * @brief 检查是否有唱片
     * @return 如果有唱片返回true
     */
    [[nodiscard]] bool hasRecord() const;

    /**
     * @brief 开始播放唱片
     *
     * 向所有玩家广播唱片音效事件。
     *
     * @param world 世界引用
     */
    void startPlaying(IWorld& world);

    /**
     * @brief 停止播放唱片
     *
     * 向所有玩家广播停止音效事件。
     *
     * @param world 世界引用
     */
    void stopPlaying(IWorld& world);

    /**
     * @brief 检查是否正在播放
     * @return 如果正在播放返回true
     */
    [[nodiscard]] bool isPlaying() const noexcept { return m_isPlaying; }

    /**
     * @brief 获取红石比较器信号强度
     * @return 信号强度（0-15），根据唱片类型返回对应值
     */
    [[nodiscard]] i32 getComparatorSignal() const;

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    SimpleInventory m_inventory;     ///< 1格物品存储
    bool m_isPlaying = false;        ///< 是否正在播放
    i64 m_ticksSinceSongStarted = 0; ///< 歌曲开始播放后的tick计数
    i32 m_songLengthTicks = 0;       ///< 当前歌曲的总长度（ticks），0表示未知
};

} // namespace blockentity
} // namespace mc
