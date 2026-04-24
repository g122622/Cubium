#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>
#include <unordered_map>
#include <functional>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;

namespace client::renderer::entity::core {

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
