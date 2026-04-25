#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <functional>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;

namespace client::renderer::entity::pipeline {
class EntityPipeline;  // 前向声明
}

namespace client::renderer::entity::model {
class EntityModel;
}

namespace client::renderer::entity::core {

// 前向声明
struct AnimationContext;

/**
 * @brief 实体渲染器基类
 *
 * 所有实体渲染器的基类，定义渲染接口。
 *
 * 参考 MC 1.16.5 EntityRenderer
 */
class EntityRenderer {
public:
    EntityRenderer() = default;
    virtual ~EntityRenderer() = default;

    /**
     * @brief 渲染实体
     * @param entity 要渲染的实体
     * @param partialTicks 部分tick（用于插值）
     */
    virtual void render(Entity& entity, f64 partialTicks) = 0;

    /**
     * @brief 渲染实体的阴影
     * @param entity 要渲染阴影的实体
     * @param partialTicks 部分tick
     */
    virtual void renderShadow(Entity& entity, f64 partialTicks);

    /**
     * @brief 渲染实体的名称标签
     * @param entity 要渲染名称标签的实体
     */
    virtual void renderNameTag(Entity& entity);

    // ========== 动画支持 ==========

    /**
     * @brief 是否支持动画
     *
     * LivingEntity 渲染器返回 true，静态实体（如 ItemEntity）返回 false
     */
    [[nodiscard]] virtual bool supportsAnimation() const { return false; }

    /**
     * @brief 计算动画上下文并设置模型角度
     *
     * 只有 supportsAnimation() 返回 true 的渲染器需要实现此方法。
     * 此方法会设置模型的旋转角度以匹配当前动画状态。
     *
     * @param entity 实体
     * @param partialTicks 部分 tick
     * @param context 输出的动画上下文
     * @param model 输出的模型指针（用于网格生成）
     */
    virtual void computeAnimationContext(
        Entity& entity,
        f64 partialTicks,
        AnimationContext& context,
        std::unique_ptr<model::EntityModel>& model
    ) {
        // 默认空实现，不支持动画的渲染器不需要重写
    }

    /**
     * @brief 渲染层（GPU管线路径）
     *
     * 只有 LivingEntity 渲染器需要实现此方法。
     * 渲染实体上的附加层（盔甲、手持物品等）。
     *
     * @param entity 实体
     * @param cmd Vulkan 命令缓冲区
     * @param context 动画上下文
     * @param pipeline 实体渲染管线
     */
    virtual void renderLayersPipeline(
        Entity& entity,
        VkCommandBuffer cmd,
        const AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) {
        // 默认空实现，不支持层的渲染器不需要重写
        (void)entity;
        (void)cmd;
        (void)context;
        (void)pipeline;
    }

    /**
     * @brief 是否支持层渲染
     *
     * LivingEntity 渲染器返回 true。
     */
    [[nodiscard]] virtual bool supportsLayers() const { return false; }

    // ========== 渲染属性 ==========

    /**
     * @brief 获取阴影大小
     */
    [[nodiscard]] f64 shadowSize() const { return m_shadowSize; }

    /**
     * @brief 设置阴影大小
     */
    void setShadowSize(f64 size) { m_shadowSize = size; }

    /**
     * @brief 获取阴影透明度
     */
    [[nodiscard]] f64 shadowAlpha() const { return m_shadowAlpha; }

    /**
     * @brief 设置阴影透明度
     */
    void setShadowAlpha(f64 alpha) { m_shadowAlpha = alpha; }

protected:
    f64 m_shadowSize = 0.5f;     // 阴影半径
    f64 m_shadowAlpha = 0.8f;    // 阴影透明度

    /**
     * @brief 讨论是否应该渲染阴影
     */
    [[nodiscard]] bool shouldRenderShadow(Entity& entity) const;

    /**
     * @brief 计算阴影缩放
     */
    [[nodiscard]] f64 getShadowScale(Entity& entity, f64 partialTicks) const;
};

/**
 * @brief 实体渲染器工厂
 *
 * 根据实体类型创建对应的渲染器。
 */
class EntityRendererFactory {
public:
    using CreatorFunc = std::unique_ptr<EntityRenderer>(*)();

    /**
     * @brief 注册渲染器
     * @param typeId 实体类型ID
     * @param creator 创建函数
     */
    static void registerRenderer(const String& typeId, CreatorFunc creator);

    /**
     * @brief 创建渲染器
     * @param typeId 实体类型ID
     * @return 对应的渲染器，如果没有则返回nullptr
     */
    static std::unique_ptr<EntityRenderer> createRenderer(const String& typeId);

private:
    static std::unordered_map<String, CreatorFunc> s_creators;
};

} // namespace mc::client::renderer::entity::core
} // namespace mc
