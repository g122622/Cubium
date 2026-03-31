#pragma once

#include "../../../../../common/core/Types.hpp"
#include <glm/glm.hpp>

namespace mc::client::renderer::trident::particle {

/**
 * @brief 精灵接口
 *
 * 定义粒子纹理精灵的通用接口。
 * 支持静态精灵和动画精灵。
 */
class ISprite {
public:
    virtual ~ISprite() = default;

    /**
     * @brief 获取当前帧的 UV 坐标
     *
     * @param age 粒子年龄（ticks）
     * @param maxAge 粒子最大年龄（ticks）
     * @return UV 坐标 (minU, minV, maxU, maxV)
     */
    [[nodiscard]] virtual glm::vec4 getFrameUV(f32 age, f32 maxAge) const = 0;

    /**
     * @brief 获取随机帧的 UV 坐标
     *
     * 用于随机选择初始帧的粒子。
     *
     * @param seed 随机种子
     * @return UV 坐标 (minU, minV, maxU, maxV)
     */
    [[nodiscard]] virtual glm::vec4 getRandomFrameUV(u32 seed) const = 0;

    /**
     * @brief 是否为动画精灵
     *
     * @return 是否有多个帧
     */
    [[nodiscard]] virtual bool isAnimated() const = 0;

    /**
     * @brief 获取帧数
     *
     * @return 帧数（静态精灵返回 1）
     */
    [[nodiscard]] virtual u32 frameCount() const = 0;

    /**
     * @brief 获取每帧时间
     *
     * @return 每帧持续时间（秒），静态精灵返回 0
     */
    [[nodiscard]] virtual f32 frameTime() const = 0;
};

} // namespace mc::client::renderer::trident::particle
