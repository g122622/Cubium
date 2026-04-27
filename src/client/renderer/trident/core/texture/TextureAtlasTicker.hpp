#pragma once

#include "AnimatedSprite.hpp"
#include <vector>
#include <memory>

namespace mc::client::renderer::trident {

/**
 * @brief 纹理图集动画管理器
 *
 * 管理所有动画精灵，每游戏tick调用tick()更新动画状态。
 * 参考 MC 1.16.5 AtlasTexture 的动画更新机制。
 *
 * 使用方式：
 * 1. 在资源加载时，通过registerAnimatedSprite()注册动画精灵
 * 2. 在客户端主循环中，每tick调用tick()
 * 3. 在渲染前，调用uploadPendingFrames()上传需要更新的帧
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
     * @brief 上传所有待更新的帧
     * @param context Trident上下文
     * @param atlas 纹理图集
     *
     * 将所有需要更新的帧上传到GPU纹理。
     * 此方法应在渲染前调用。
     */
    [[nodiscard]] Result<void> uploadPendingFrames(
        TridentContext* context,
        TridentTextureAtlas& atlas);

    /**
     * @brief 清除所有动画精灵
     */
    void clear();

    /**
     * @brief 获取动画精灵数量
     */
    [[nodiscard]] usize spriteCount() const noexcept {
        return m_sprites.size();
    }

    /**
     * @brief 检查是否有动画精灵
     */
    [[nodiscard]] bool empty() const noexcept {
        return m_sprites.empty();
    }

private:
    std::vector<std::shared_ptr<AnimatedSprite>> m_sprites;
};

} // namespace mc::client::renderer::trident
