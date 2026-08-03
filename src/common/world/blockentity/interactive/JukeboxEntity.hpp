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
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/jukebox/JukeboxSongPlayer.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

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
 * - 歌曲播放完毕后自动停止
 * - 每20tick（1秒）发射音符粒子效果
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
     * 通过 JukeboxSong 注册表查找唱片对应的歌曲，
     * 并使用 JukeboxSongPlayer 开始播放。
     *
     * @param world 世界引用
     */
    void startPlaying(IWorld& world);

    /**
     * @brief 停止播放唱片
     *
     * 通过 JukeboxSongPlayer 停止播放并广播停止事件。
     *
     * @param world 世界引用
     */
    void stopPlaying(IWorld& world);

    /**
     * @brief 检查是否正在播放
     * @return 如果正在播放返回true
     */
    [[nodiscard]] bool isPlaying() const noexcept { return m_songPlayer.isPlaying(); }

    /**
     * @brief 获取红石比较器信号强度
     * @return 信号强度（0-15），根据唱片类型返回对应值
     */
    [[nodiscard]] i32 getComparatorSignal() const;

    /**
     * @brief 获取歌曲播放器
     * @return 歌曲播放器的引用
     */
    [[nodiscard]] JukeboxSongPlayer& getSongPlayer() noexcept { return m_songPlayer; }
    [[nodiscard]] const JukeboxSongPlayer& getSongPlayer() const noexcept { return m_songPlayer; }

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    /**
     * @brief 歌曲变化回调
     *
     * 当歌曲开始播放或停止播放时调用，更新邻居方块和标记脏数据。
     */
    void onSongChanged();

    SimpleInventory m_inventory;    ///< 1格物品存储
    JukeboxSongPlayer m_songPlayer; ///< 歌曲播放器
};

} // namespace blockentity
} // namespace mc
