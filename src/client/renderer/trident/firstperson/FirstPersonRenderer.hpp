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

#include "ArmPose.hpp"
#include "ItemCameraTransforms.hpp"
#include "ItemInHandRenderer.hpp"
#include "MatrixStack.hpp"
#include "PlayerModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include <array>
#include <limits>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

// 前向声明
namespace mc {
class Player;
} // namespace mc

namespace mc::client {
class ItemTextureAtlas;
}

namespace mc::client::renderer::trident::firstperson {

// 导入类型
using entity::model::ModelVertex;
using entity::pipeline::EntityMesh;
using entity::pipeline::EntityPipeline;
using entity::pipeline::EntityTextureAtlas;

/**
 * @brief 第一人称手部渲染器
 *
 * 负责渲染玩家视角下的手部和手持物品。
 *
 * 主要功能：
 * - 渲染玩家手臂（第一人称视角）
 * - 渲染手持物品
 * - 处理手臂挥动动画
 * - 处理使用物品动画（吃食物、拉弓等）
 * - 处理地图等特殊物品渲染
 */
class FirstPersonRenderer {
public:
    /**
     * @brief 手部渲染上下文
     *
     * 包含渲染所需的所有状态信息
     */
    struct RenderContext {
        /// 当前玩家
        mc::Player* player = nullptr;

        /// 部分 tick（用于动画插值）
        f64 partialTick = 0.0;

        /// 主手挥动进度 (0.0 - 1.0)
        f32 mainHandSwingProgress = 0.0f;

        /// 副手挥动进度 (0.0 - 1.0)
        f32 offHandSwingProgress = 0.0f;

        /// 主手装备进度 (0.0 - 1.0)
        f32 mainHandEquipProgress = 0.0f;

        /// 副手装备进度 (0.0 - 1.0)
        f32 offHandEquipProgress = 0.0f;

        /// 是否正在使用主手物品
        bool isMainHandActive = false;

        /// 是否正在使用副手物品
        bool isOffHandActive = false;

        /// 主手使用物品的 tick 数
        i32 mainHandUseCount = 0;

        /// 副手使用物品的 tick 数
        i32 offHandUseCount = 0;
    };

    FirstPersonRenderer();
    ~FirstPersonRenderer();

    // 禁止拷贝
    FirstPersonRenderer(const FirstPersonRenderer&) = delete;
    FirstPersonRenderer& operator=(const FirstPersonRenderer&) = delete;

    // ========== 初始化 ==========

    /**
     * @brief 初始化渲染器
     *
     * @param device Vulkan 设备
     * @param physicalDevice Vulkan 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列（必须与 commandPool 的队列族匹配）
     * @param renderPass 渲染通道
     * @param cameraDescriptorLayout 相机描述符布局
     * @param descriptorPool 描述符池
     * @param entityTextureAtlas 实体纹理图集（玩家皮肤）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkRenderPass renderPass,
        VkDescriptorSetLayout cameraDescriptorLayout,
        VkDescriptorPool descriptorPool,
        EntityTextureAtlas* entityTextureAtlas,
        u32 maxFramesInFlight,
        VkSampleCountFlagBits sampleCount);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 标记当前帧索引并推进延迟销毁队列
     *
     * 手臂/物品管线为每帧维护独立纹理描述符集，setTextureAtlas 仅写当前帧 set。
     * 必须在每帧录制第一人称 draw 之前调用。
     * @param frameIndex 当前帧索引（0 .. maxFramesInFlight-1）
     */
    void beginFrame(u32 frameIndex);

    /**
     * @brief 处理延迟销毁队列，释放足够久未被任何在飞帧引用的缓冲区
     *
     * 由 beginFrame 在上一帧 fence 等待后内部调用，外部通常无需直接调用。
     */
    void processPendingDestroys();

    // ========== 每帧更新 ==========

    /**
     * @brief 每游戏 tick 更新
     *
     * 更新手臂挥动动画、装备动画等状态。
     *
     * @param player 当前玩家（可为 nullptr）
     */
    void tick(Player* player);

    // ========== 渲染 ==========

    /**
     * @brief 渲染第一人称手部和物品
     *
     * 主渲染入口点，在实体渲染阶段之后、GUI 渲染阶段之前调用。
     *
     * @param cmd Vulkan 命令缓冲区
     * @param cameraDescriptorSet 相机描述符集
     * @param context 渲染上下文
     */
    void render(VkCommandBuffer cmd, VkDescriptorSet cameraDescriptorSet, const RenderContext& context);

    // ========== 动画控制 ==========

    /**
     * @brief 重置装备进度
     *
     * 当玩家切换手持物品时调用，触发装备动画。
     *
     * @param hand 手
     */
    void resetEquippedProgress(Hand hand);

    // ========== 访问器 ==========

    /**
     * @brief 获取玩家模型
     */
    [[nodiscard]] PlayerModel& model() { return m_model; }
    [[nodiscard]] const PlayerModel& model() const { return m_model; }

    /**
     * @brief 设置物品纹理图集
     *
     * 第一人称手持物品渲染使用该图集查询 UV。
     * 图集生命周期由 TridentEngine 管理，渲染器仅持有裸指针。
     *
     * @param itemTextureAtlas 物品纹理图集
     */
    void setItemTextureAtlas(const mc::client::ItemTextureAtlas* itemTextureAtlas);

    /**
     * @brief 获取手持物品渲染器
     */
    [[nodiscard]] ItemInHandRenderer& itemInHandRenderer() { return m_itemInHandRenderer; }
    [[nodiscard]] const ItemInHandRenderer& itemInHandRenderer() const { return m_itemInHandRenderer; }

    /**
     * @brief 设置玩家皮肤纹理位置
     *
     * 第一人称手臂网格会将 UV 重映射到该纹理区域。
     *
     * @param playerSkinLocation 玩家皮肤在实体图集中的资源键
     */
    void setPlayerSkinLocation(const ResourceLocation& playerSkinLocation);

    /**
     * @brief 获取当前玩家皮肤纹理位置
     */
    [[nodiscard]] const ResourceLocation& playerSkinLocation() const { return m_playerSkinLocation; }

private:
    // ========== 内部渲染方法 ==========

    /**
     * @brief 渲染单只手臂
     *
     * @param stack 矩阵栈
     * @param side 手侧（左/右）
     * @param equipProgress 装备进度
     * @param swingProgress 挥动进度
     */
    void _renderArmFirstPerson(MatrixStack& stack, HandSide side, f32 equipProgress, f32 swingProgress);

    /**
     * @brief 渲染手持物品（基础版本）
     *
     * @param stack 矩阵栈
     * @param player 玩家
     * @param itemStack 物品堆
     * @param side 手侧（左/右）
     * @param equipProgress 装备进度
     * @param swingProgress 挥动进度
     */
    void _renderItemInHand(MatrixStack& stack,
        mc::Player* player,
        const ItemStack& itemStack,
        HandSide side,
        f32 equipProgress,
        f32 swingProgress);

    /**
     * @brief 渲染手持物品（带使用状态）
     *
     * @param stack 矩阵栈
     * @param player 玩家
     * @param itemStack 物品堆
     * @param side 手侧（左/右）
     * @param equipProgress 装备进度
     * @param swingProgress 挥动进度
     * @param isUsingItem 是否正在使用物品
     * @param useCount 使用计数
     * @param partialTicks 部分 tick
     */
    void _renderItemInHand(MatrixStack& stack,
        mc::Player* player,
        const ItemStack& itemStack,
        HandSide side,
        f32 equipProgress,
        f32 swingProgress,
        bool isUsingItem,
        i32 useCount,
        f32 partialTicks);

    /**
     * @brief 渲染地图（特殊物品）
     */
    void _renderMapFirstPerson(
        MatrixStack& stack, const ItemStack& mapStack, f32 pitch, f32 equipProgress, f32 swingProgress);

    // ========== 变换方法 ==========

    /**
     * @brief 应用手部侧边变换
     *
     * 将手部放置在屏幕侧边的正确位置。
     */
    void _transformSideFirstPerson(MatrixStack& stack, HandSide side, f32 equipProgress);

    /**
     * @brief 应用第一人称挥动变换
     */
    void _transformFirstPerson(MatrixStack& stack, HandSide side, f32 swingProgress);

    /**
     * @brief 应用进食/饮用变换
     *
     * @param matrixStack 矩阵栈
     * @param partialTicks 部分 tick
     * @param side 手侧
     * @param item 物品堆
     * @param useCount 剩余使用时间
     */
    void _transformEatOrDrink(
        MatrixStack& matrixStack, f32 partialTicks, HandSide side, const ItemStack& item, i32 useCount);

    /**
     * @brief 应用拉弓变换
     *
     * @param stack 矩阵栈
     * @param partialTicks 部分 tick
     * @param side 手侧
     * @param useCount 使用计数
     * @param itemStack 弓物品堆（用于获取 getUseDuration）
     */
    void _transformBow(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useCount, const ItemStack& itemStack);

    /**
     * @brief 应用三叉戟投掷变换
     *
     * @param stack 矩阵栈
     * @param partialTicks 部分 tick
     * @param side 手侧
     * @param useCount 使用计数
     * @param itemStack 三叉戟物品堆（用于获取 getUseDuration）
     */
    void _transformSpear(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useCount, const ItemStack& itemStack);

    /**
     * @brief 应用弩装填变换
     *
     * @param stack 矩阵栈
     * @param partialTicks 部分 tick
     * @param side 手侧
     * @param useCount 使用计数
     * @param isCharged 是否已装填
     * @param itemStack 弩物品堆（用于获取 getChargeTime）
     */
    void _transformCrossbow(
        MatrixStack& stack, f32 partialTicks, HandSide side, i32 useCount, bool isCharged, const ItemStack& itemStack);

    /**
     * @brief 计算挥动动画参数
     *
     * TODO: 该方法已声明但未在当前代码路径中调用，待完整客户端动画系统接入后启用。
     */
    [[nodiscard]] f32 _getSwingProgress(f32 partialTicks, mc::Player* player, Hand hand) const;

    /**
     * @brief 确定手臂姿态
     */
    [[nodiscard]] ArmPose _determineArmPose(mc::Player* player, Hand hand) const;

    // ========== GPU 资源管理 ==========

    /**
     * @brief 确保手臂网格已创建
     */
    void _ensureArmMesh(Hand hand, HandSide primaryHand);

    /**
     * @brief 使手臂网格缓存失效
     */
    void _invalidateArmMeshes();

    /**
     * @brief 使手持物品网格缓存失效
     */
    void _invalidateItemMeshes();

    /**
     * @brief 确保手持物品网格已创建
     */
    void _ensureItemMesh(Hand hand, const ItemStack& itemStack);

    /**
     * @brief 清理已退休的手持物品网格
     */
    void _cleanupRetiredItemMeshes();

    /**
     * @brief 退休当前手持物品网格
     */
    void _retireItemMesh(EntityMesh& mesh);

    /**
     * @brief 销毁当前和已退休的手持物品网格
     */
    void _destroyItemMeshes();

    /**
     * @brief 创建或更新 GPU 缓冲区
     *
     * TODO: 该方法已声明但未实现，当前 GPU 缓冲区通过 EntityPipeline::createMesh 管理，
     * 后续如果需要增量更新顶点/索引数据，应实现此方法。
     */
    [[nodiscard]] Result<void> _createOrUpdateBuffers(
        const std::vector<ModelVertex>& vertices, const std::vector<u32>& indices);

    /**
     * @brief 将模型 UV 重映射到玩家皮肤图集区域
     */
    void _remapToPlayerSkinRegion(std::vector<ModelVertex>& vertices) const;

    // ========== 工具方法 ==========

    /**
     * @brief 获取玩家的主手
     */
    [[nodiscard]] static HandSide _getPrimaryHand(mc::Player* player);

    /**
     * @brief 获取手持物品
     */
    [[nodiscard]] static ItemStack _getHeldItem(mc::Player* player, Hand hand);

    /**
     * @brief 根据手槽位和主手设置解析实际左右手
     */
    [[nodiscard]] static HandSide _resolveHandSide(Hand hand, HandSide primaryHand);

    /**
     * @brief 根据手槽位选择对应的手臂模型部件
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> _selectArmModel(Hand hand, HandSide primaryHand) const;

private:
    // Vulkan 资源
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_cameraDescriptorLayout = VK_NULL_HANDLE;

    // 渲染管线（手臂与物品分离，避免同一描述符集在多帧飞行时被频繁更新）
    std::unique_ptr<EntityPipeline> m_armPipeline;
    std::unique_ptr<EntityPipeline> m_itemPipeline;

    // 主手/副手手臂网格（分别缓存，避免每帧销毁重建导致 GPU 资源生命周期问题）
    EntityMesh m_mainHandArmMesh;
    bool m_mainHandArmMeshValid = false;
    EntityMesh m_offHandArmMesh;
    bool m_offHandArmMeshValid = false;

    struct ItemMeshState {
        EntityMesh mesh;
        bool valid = false;
        ItemId itemId = std::numeric_limits<ItemId>::max();
    };

    struct RetiredItemMesh {
        EntityMesh mesh;
        u32 framesRemaining = 0;
    };

    // 手持物品网格（按手位分别缓存，旧网格会延迟若干帧后再回收）
    std::array<ItemMeshState, 2> m_itemMeshes;
    std::vector<RetiredItemMesh> m_retiredItemMeshes;
    u32 m_itemMeshRetirementFrames = 0;

    // 实体纹理图集
    EntityTextureAtlas* m_entityTextureAtlas = nullptr;

    // 物品纹理图集
    const mc::client::ItemTextureAtlas* m_itemTextureAtlas = nullptr;

    // 玩家皮肤纹理位置（默认：规范默认皮肤 slim/steve）
    ResourceLocation m_playerSkinLocation{"minecraft:textures/entity/player/slim/steve.png"};

    // 玩家模型
    PlayerModel m_model;

    // 手持物品渲染器
    ItemInHandRenderer m_itemInHandRenderer;

    // 动画状态
    f32 m_prevMainHandEquipProgress = 0.0f;
    f32 m_mainHandEquipProgress = 0.0f;
    f32 m_prevOffHandEquipProgress = 0.0f;
    f32 m_offHandEquipProgress = 0.0f;

    // 上一次持有的物品（用于检测切换）
    ItemStack m_prevMainHandItem;
    ItemStack m_prevOffHandItem;

    // 挥动进度
    f32 m_prevSwingProgress = 0.0f;
    f32 m_swingProgress = 0.0f;
    Hand m_swingHand = Hand::MainHand;

    // 当前帧缓存数据
    std::vector<ModelVertex> m_cachedVertices;
    std::vector<u32> m_cachedIndices;

    // 初始化标志
    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident::firstperson
