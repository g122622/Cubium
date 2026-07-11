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

#include "client/renderer/trident/entity/effect/fire/FireAnimationState.hpp"
#include "client/renderer/trident/entity/effect/fire/FireTextureLoader.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc {
class Entity;

namespace client {
class ClientEntity;
}

namespace client::renderer::entity::pipeline {
class EntityPipeline; // 前向声明
class EntityTextureAtlas;
} // namespace client::renderer::entity::pipeline

namespace client::renderer::entity::effect::fire {

/**
 * @brief 着火效果渲染器
 *
 * 用于渲染实体身上的火焰效果。
 *
 * 火焰纹理使用方块纹理 fire_0.png 和 fire_1.png（动画）。
 * 纹理布局：[fire_0 全部帧][fire_1 全部帧] 纵向拼接为单张 VkImage，
 * 渲染时通过 UV 偏移选择当前动画帧（无需逐帧上传到 GPU）。
 */
class FireEffect {
public:
    /**
     * @brief 初始化着火效果系统
     *
     * 仅初始化 Vulkan 句柄与程序化占位纹理，不访问资源包。
     * 真实火焰纹理通过 loadTexture() 在资源管理器就绪后注入。
     *
     * @param device Vulkan 设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @return 成功或错误
     */
    static bool initialize(
        VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);

    /**
     * @brief 从资源包加载并上传火焰纹理到 GPU
     *
     * 必须在 initialize() 成功后调用。可安全重复调用（用于热重载）：
     * 会等待设备空闲、销毁旧纹理资源后重新创建。
     * 同时重置动画播放状态。
     *
     * @param resourcePacks 资源包列表（按优先级从低到高）
     * @return 成功或错误
     */
    static bool loadTexture(const std::vector<IResourcePack*>& resourcePacks);

    /**
     * @brief 清理着火效果系统
     */
    static void cleanup();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized();

    /**
     * @brief 每游戏 tick 更新火焰动画
     *
     * 推进 fire_0 和 fire_1 的帧计数器。
     * 应在客户端主循环的 tickTextureAnimations 阶段调用。
     */
    static void tick();

    /**
     * @brief 检查实体是否在燃烧（Entity 版本）
     * @param entity 实体
     * @return 是否在燃烧
     */
    [[nodiscard]] static bool isBurning(Entity& entity);

    /**
     * @brief 检查实体是否在燃烧（ClientEntity 版本）
     * @param entity 客户端实体
     * @return 是否在燃烧
     */
    [[nodiscard]] static bool isBurningClient(::mc::client::ClientEntity& entity);

    /**
     * @brief 渲染实体着火效果（Entity 版本，用于旧路径）
     * @param entity 实体
     * @param partialTicks 部分tick
     */
    static void renderFire(Entity& entity, f64 partialTicks);

    /**
     * @brief 渲染实体着火效果（ClientEntity + Vulkan 版本）
     * @param cmd Vulkan 命令缓冲区
     * @param entity 客户端实体
     * @param partialTicks 部分tick
     * @param pipeline 实体渲染管线
     */
    static void renderFire(
        VkCommandBuffer cmd, ::mc::client::ClientEntity& entity, f64 partialTicks, pipeline::EntityPipeline& pipeline);

    /**
     * @brief 获取火焰纹理视图
     */
    [[nodiscard]] static VkImageView fireTextureView() { return s_fireTextureView; }

    /**
     * @brief 获取火焰纹理采样器
     */
    [[nodiscard]] static VkSampler fireSampler() { return s_fireSampler; }

    /**
     * @brief 获取固定光照值（15728640 = 0xF00000）
     */
    [[nodiscard]] static constexpr i32 getFullLight() { return FULL_LIGHT; }

private:
    FireEffect() = delete;
    ~FireEffect() = delete;

    /**
     * @brief 渲染多层火焰
     *
     * 使用循环绘制多层火焰，每层递减尺寸。
     * 偶数层使用 fire_0 纹理，奇数层使用 fire_1 纹理（MC 原版行为）。
     * UV 的 V 坐标根据当前动画帧索引偏移。
     *
     * @param cmd 命令缓冲区
     * @param entity 实体
     * @param partialTicks 部分tick
     * @param pipeline 渲染管线
     * @param cameraYaw 相机偏航角（用于 billboard 朝向）
     */
    static void _renderFireLayers(VkCommandBuffer cmd,
        ::mc::client::ClientEntity& entity,
        f64 partialTicks,
        pipeline::EntityPipeline& pipeline,
        f32 cameraYaw);

    /**
     * @brief 销毁已存在的火焰纹理 Vulkan 资源
     *
     * 用于 loadTexture 热重载前清理旧资源。
     */
    static void _destroyFireTexture();

    /**
     * @brief 创建火焰纹理资源
     *
     * 将所有帧（fire_0 + fire_1）纵向拼接上传为单张 VkImage。
     *
     * @param pixels 像素数据
     * @param width 宽度
     * @param height 总高度（所有帧拼接后的高度）
     * @return 成功或错误
     */
    static bool _createFireTexture(const std::vector<u8>& pixels, u32 width, u32 height);

    /**
     * @brief 生成插值帧像素数据
     *
     * 当 metadata.interpolate=true 时，根据 progress 在当前帧和下一帧
     * 之间逐像素 lerp，R/G/B 三通道线性插值，A 通道保留当前帧 alpha。
     * 算法与 MC 1.16.5 TextureAtlasSprite.InterpolationData 一致，
     * 也与项目 AnimatedSprite::_generateInterpolatedFrame 一致。
     *
     * @param currentFrame 当前帧像素数据
     * @param nextFrame 下一帧像素数据
     * @param progress 插值进度（0.0=完全当前帧，1.0=完全下一帧）
     * @return 插值后的像素数据
     */
    static std::vector<u8> _generateInterpolatedFrame(
        const u8* currentFrame, const u8* nextFrame, u32 pixelCount, f32 progress);

    /**
     * @brief 上传火焰纹理子区域到 VkImage
     *
     * 通过 staging buffer 把指定矩形区域的像素上传到 s_fireTexture。
     * 用于插值模式下每 tick 把混合后的帧像素写回 VkImage 对应位置。
     *
     * @param pixels 像素数据（RGBA）
     * @param dstX 目标 X 坐标（像素）
     * @param dstY 目标 Y 坐标（像素）
     * @param width 区域宽度
     * @param height 区域高度
     * @return 成功或错误
     */
    static bool _uploadTextureRegion(const u8* pixels, u32 dstX, u32 dstY, u32 width, u32 height);

    /**
     * @brief 推进单个动画状态的插值帧上传
     *
     * 若该动画状态启用了 interpolate，则根据 frameProgress() 生成
     * 插值帧并上传到 VkImage 的对应区域。
     *
     * @param state 动画状态
     * @param isFire1 是否为 fire_1（决定 VkImage 中的 Y 偏移）
     */
    static void _tickInterpolation(FireAnimationState& state, bool isFire1);

    /**
     * @brief 生成火焰四边形网格
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param width 宽度
     * @param height 高度
     * @param u0 UV 左上角 U
     * @param v0 UV 左上角 V
     * @param u1 UV 右下角 U
     * @param v1 UV 右下角 V
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     * @param transformIndex 变换索引（用于 billboard）
     */
    static void _generateFireQuad(f64 x,
        f64 y,
        f64 z,
        f64 width,
        f64 height,
        f32 u0,
        f32 v0,
        f32 u1,
        f32 v1,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        u32 transformIndex = 0);

    /**
     * @brief 计算火焰偏移（用于动画）
     */
    [[nodiscard]] static f64 _computeFireOffset(f64 time, f64 seed);

    /**
     * @brief 计算 billboard 变换矩阵
     */
    static void _computeBillboardMatrices(const Vector3f& position, std::array<std::array<f64, 16>, 2>& outMatrices);

    static bool s_initialized;
    static VkDevice s_device;
    static VkPhysicalDevice s_physicalDevice;
    static VkCommandPool s_commandPool;
    static VkQueue s_graphicsQueue;

    // 火焰纹理资源
    static VkImage s_fireTexture;
    static VkDeviceMemory s_fireTextureMemory;
    static VkImageView s_fireTextureView;
    static VkSampler s_fireSampler;
    static u32 s_fireTextureWidth;
    static u32 s_fireTextureHeight;

    // CPU 端像素副本：用于插值模式下逐像素 lerp 生成混合帧
    // 布局与 VkImage 一致：[fire_0 全部帧][fire_1 全部帧] 纵向拼接
    static std::vector<u8> s_firePixelsCPU;
    // 单帧像素数（宽×高，不含通道数），用于插值帧生成
    static u32 s_fireFramePixelCount;

    // 动画状态
    /// fire_0 的动画播放状态
    static FireAnimationState s_fire0Animation;
    /// fire_1 的动画播放状态
    static FireAnimationState s_fire1Animation;
    /// fire_0 的帧数（在纹理中的起始偏移为 0）
    static u32 s_fire0FrameCount;
    /// fire_1 的帧数（在纹理中的起始帧偏移为 s_fire0FrameCount）
    static u32 s_fire1FrameCount;

    // 固定全亮光照值 (0xF00000 = 15728640)
    static constexpr i32 FULL_LIGHT = 15728640;
};

} // namespace client::renderer::entity::effect::fire
} // namespace mc
