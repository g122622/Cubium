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

#include "AnimatedSprite.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>

namespace mc::client::renderer::trident {

/**
 * @brief 纹理图集动画管理器
 *
 * 管理所有动画精灵，每游戏tick调用tick()更新帧状态。
 * 待上传帧由 AtlasManager::uploadPendingAnimationFrames 经统一暂存池批量上传。
 *
 * @note 此类不是线程安全的。所有方法都应在主线程调用。
 */
class TextureAtlasTicker {
public:
    /**
     * @brief 默认构造函数
     */
    TextureAtlasTicker() = default;

    /**
     * @brief 析构函数
     */
    ~TextureAtlasTicker() = default;

    // 禁止拷贝
    TextureAtlasTicker(const TextureAtlasTicker&) = delete;
    TextureAtlasTicker& operator=(const TextureAtlasTicker&) = delete;

    // 允许移动
    TextureAtlasTicker(TextureAtlasTicker&&) noexcept = default;
    TextureAtlasTicker& operator=(TextureAtlasTicker&&) noexcept = default;

    /**
     * @brief 注册动画精灵
     * @param sprite 动画精灵
     *
     * 动画精灵会被添加到更新列表中，每tick自动更新。
     */
    void registerAnimatedSprite(std::shared_ptr<AnimatedSprite> sprite);

    /**
     * @brief 注销动画精灵
     * @param sprite 要注销的精灵指针
     */
    void unregisterAnimatedSprite(const AnimatedSprite* sprite);

    /**
     * @brief 每游戏tick更新动画状态
     *
     * 遍历所有动画精灵，调用其tick()方法。
     * 此方法应在客户端主循环中每tick调用一次。
     */
    void tick();

    /**
     * @brief 清除所有动画精灵
     */
    void clear();

    /**
     * @brief 获取动画精灵数量
     */
    mc::Size spriteCount() const noexcept { return m_sprites.size(); }

    /**
     * @brief 检查是否有动画精灵
     */
    bool empty() const noexcept { return m_sprites.empty(); }

    /**
     * @brief 按索引获取动画精灵
     * @param index 精灵索引
     * @return 精灵指针，索引越界返回nullptr
     */
    AnimatedSprite* getSprite(mc::Size index) { return index < m_sprites.size() ? m_sprites[index].get() : nullptr; }

    /**
     * @brief 按索引获取动画精灵（const 版本）
     * @param index 精灵索引
     * @return 精灵指针，索引越界返回nullptr
     */
    const AnimatedSprite* getSprite(mc::Size index) const
    {
        return index < m_sprites.size() ? m_sprites[index].get() : nullptr;
    }

private:
    std::vector<std::shared_ptr<AnimatedSprite>> m_sprites;
};

} // namespace mc::client::renderer::trident
