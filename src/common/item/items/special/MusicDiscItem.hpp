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
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {

namespace item::items {

/**
 * @brief 音乐唱片物品
 *
 * 音乐唱片可以放入唱片机播放音乐，并且产生红石比较器信号。
 * 每种唱片有不同的比较器信号强度 (1-15) 和对应的声音事件。
 *
 * 唱片放入/取出唱片机的逻辑由 JukeboxBlock::onBlockActivated() 处理，
 * MusicDiscItem 主要提供唱片元数据（信号强度、声音事件ID）和身份识别。
 *
 * 参考: net.minecraft.item.MusicDiscItem
 */
class MusicDiscItem : public Item {
public:
    /**
     * @brief 构造音乐唱片物品
     * @param comparatorOutput 比较器信号强度 (1-15)
     * @param soundEventId 声音事件ID (如 "minecraft:music_disc.13")
     * @param properties 物品属性
     */
    MusicDiscItem(i32 comparatorOutput, const ResourceLocation& soundEventId, ItemProperties properties);

    ~MusicDiscItem() noexcept override = default;

    /**
     * @brief 获取比较器信号强度
     * @return 信号强度 (1-15)
     */
    [[nodiscard]] i32 getComparatorOutput() const noexcept { return m_comparatorOutput; }

    /**
     * @brief 获取声音事件ID
     * @return 声音事件的资源位置
     */
    [[nodiscard]] const ResourceLocation& getSoundEventId() const noexcept { return m_soundEventId; }

    /**
     * @brief 检查该物品是否为音乐唱片
     * @return 始终返回 true
     */
    [[nodiscard]] bool isMusicDisc() const noexcept override { return true; }

private:
    i32 m_comparatorOutput;          ///< 比较器信号强度 (1-15)
    ResourceLocation m_soundEventId; ///< 声音事件ID
};

} // namespace item::items
} // namespace mc
