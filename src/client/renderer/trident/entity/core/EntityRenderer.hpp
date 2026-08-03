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
#include "common/util/math/Vector3.hpp"
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;

namespace client {
class ClientEntity; // 前向声明
}

namespace client::renderer::entity::pipeline {
class EntityPipeline;     // 前向声明
class EntityTextureAtlas; // 前向声明
} // namespace client::renderer::entity::pipeline

namespace client::renderer::entity::model {
class EntityModel;
struct ModelVertex;
} // namespace client::renderer::entity::model

namespace client::renderer::entity::core {

// 前向声明
struct AnimationContext;

/**
 * @brief 管线网格提供者接口
 *
 * 为不支持 ModelFactory 的渲染器提供自定义网格生成能力。
 * 例如：Arrow, Boat, Minecart, FishingBobber 等实体的自定义几何体。
 *
 * 使用方式：在 EntityRenderer 子类中同时实现此接口，
 * 并重写 getPipelineMeshProvider() 返回 this。
 */
class PipelineMeshProvider {
public:
    virtual ~PipelineMeshProvider() = default;

    /**
     * @brief 生成网格顶点和索引
     * @param entity 客户端实体
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     * @return 是否成功生成
     */
    [[nodiscard]] virtual bool generateMesh(
        ::mc::client::ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices) = 0;

    /**
     * @brief 是否需要每帧更新网格
     * @param entity 客户端实体
     * @return true 如果网格需要更新（如动画实体）
     */
    [[nodiscard]] virtual bool needsMeshUpdate(::mc::client::ClientEntity& entity) const
    {
        (void)entity;
        return false;
    }

    /**
     * @brief 获取拓扑类型
     * @return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST 或 VK_PRIMITIVE_TOPOLOGY_LINE_LIST 等
     */
    [[nodiscard]] virtual VkPrimitiveTopology getTopology() const { return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; }
};

/**
 * @brief 实体渲染器基类
 *
 * 所有实体渲染器的基类，定义渲染接口。
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

    // ========== ClientEntity 版本的渲染方法 ==========

    /**
     * @brief 渲染层（GPU管线路径，ClientEntity 版本）
     *
     * 这是客户端渲染使用的主要方法，可以访问 ClientEntity 的扩展属性。
     * 默认实现调用 Entity 版本，子类可以重写此方法以访问 ClientEntity 特有属性。
     *
     * @param entity 客户端实体
     * @param cmd Vulkan 命令缓冲区
     * @param context 动画上下文
     * @param pipeline 实体渲染管线
     */
    virtual void renderLayersPipelineClient(::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const AnimationContext& context,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 渲染阴影（ClientEntity 版本）
     * @param entity 客户端实体
     * @param partialTicks 部分tick
     * @param cmd Vulkan 命令缓冲区
     * @param pipeline 实体渲染管线
     */
    virtual void renderShadowClient(
        ::mc::client::ClientEntity& entity, f64 partialTicks, VkCommandBuffer cmd, pipeline::EntityPipeline& pipeline);

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
        Entity& entity, f64 partialTicks, AnimationContext& context, std::unique_ptr<model::EntityModel>& model)
    {
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
        Entity& entity, VkCommandBuffer cmd, const AnimationContext& context, pipeline::EntityPipeline& pipeline)
    {
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

    /**
     * @brief 是否使用全亮光照渲染
     *
     * 返回 true 时，实体在黑暗环境中也会以最大亮度渲染，
     * 对应 MC Java 中 EntityRenderer.getBlockLightLevel() 返回 15 的行为。
     * 例如：烈焰人、岩浆怪、末影之眼、火球等发光实体。
     *
     * @return true 表示使用全亮光照（忽略环境光照），false 表示正常光照
     */
    [[nodiscard]] virtual bool isFullbright() const { return false; }

    /**
     * @brief 获取管线网格提供者
     *
     * 如果渲染器支持自定义网格生成（如 Arrow、Boat、FishingBobber），
     * 返回 PipelineMeshProvider 指针；否则返回 nullptr。
     * 默认实现返回 nullptr。
     *
     * @return 管线网格提供者指针，或 nullptr
     */
    [[nodiscard]] virtual PipelineMeshProvider* getPipelineMeshProvider() { return nullptr; }

    /**
     * @brief 计算自定义模型矩阵
     *
     * 默认情况下，EntityRendererManager 会构建一个通用模型矩阵：
     *   rotateY(yaw) * scale(-1, -1, 1) * translate(0, 1.501, 0)
     * 用于普通实体（生物等）。
     *
     * 某些实体（船、矿车等）需要完全不同的变换链（参见 MC Java 的
     * AbstractBoatRenderer / AbstractMinecartRenderer），此时可重写本方法
     * 返回 true 并写出 outMatrix，管理器会用该矩阵替换默认矩阵。
     *
     * 重写时通常还需要重写 getPipelineMeshProvider()，由
     * PipelineMeshProvider 在 generateMesh 中输出"像素空间"几何体
     * （scale = 1.0），最终由 drawMesh 的 MODEL_SCALE (1/16) 缩放到世界。
     *
     * @param entity 客户端实体
     * @param partialTicks 部分 tick（用于插值）
     * @param outMatrix 输出 4x4 行主序模型矩阵
     * @param outHurtTime 输出 hurtTime（传给着色器的红色闪烁因子，0 表示不闪烁）
     * @param outDeathTime 输出 deathTime（传给着色器的死亡淡出因子）
     * @return true 表示使用自定义矩阵；false 表示使用默认矩阵
     */
    [[nodiscard]] virtual bool computeCustomModelMatrix(::mc::client::ClientEntity& entity,
        f64 partialTicks,
        std::array<f64, 16>& outMatrix,
        f32& outHurtTime,
        f32& outDeathTime)
    {
        (void)entity;
        (void)partialTicks;
        (void)outMatrix;
        (void)outHurtTime;
        (void)outDeathTime;
        return false;
    }

    /**
     * @brief 设置纹理图集
     *
     * 用于层渲染器访问纹理UV区域信息。渲染器应在层渲染前将图集传递给需要的层。
     * 默认实现为空，子类（如 VillagerRenderer）可重写以传递图集给层渲染器。
     *
     * @param atlas 纹理图集指针
     */
    virtual void setTextureAtlas(const pipeline::EntityTextureAtlas* atlas) { (void)atlas; }

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
    f64 m_shadowSize = 0.5;  // 阴影半径
    f64 m_shadowAlpha = 0.8; // 阴影透明度

    /**
     * @brief 判断是否应该渲染阴影
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
    using CreatorFunc = std::unique_ptr<EntityRenderer> (*)();

    /**
     * @brief 注册渲染器
     * @param typeId 实体类型ID
     * @param creator 创建函数
     */
    static void registerRenderer(const std::string& typeId, CreatorFunc creator);

    /**
     * @brief 创建渲染器
     * @param typeId 实体类型ID
     * @return 对应的渲染器，如果没有则返回nullptr
     */
    static std::unique_ptr<EntityRenderer> createRenderer(const std::string& typeId);

private:
    static std::unordered_map<std::string, CreatorFunc> s_creators;
};

} // namespace client::renderer::entity::core
} // namespace mc
