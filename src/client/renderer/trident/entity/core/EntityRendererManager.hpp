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

#include "client/renderer/trident/entity/core/AnimatedMeshCache.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/RendererFactory.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace mc {
class Entity;
class Item;
class ItemStack;
class Player;
struct TextureRegion;
} // namespace mc

namespace mc::client {
class ClientEntity;
class ItemTextureAtlas;
} // namespace mc::client

namespace mc::client::renderer::entity::model {
class EntityModel;
class BipedModel;
namespace player {
class PlayerModel;
} // namespace player
namespace monster {
class SkeletonModel;
class ZombieModel;
class DrownedModel;
} // namespace monster
namespace aquatic {
class DolphinModel;
} // namespace aquatic
} // namespace mc::client::renderer::entity::model

namespace mc::client::renderer::entity {

/**
 * @brief 实体渲染器管理器
 *
 * 管理所有实体渲染器，根据实体类型分派渲染。
 * 集成EntityPipeline进行Vulkan渲染。
 */
class EntityRendererManager {
public:
    EntityRendererManager();
    ~EntityRendererManager();

    // 禁止拷贝
    EntityRendererManager(const EntityRendererManager&) = delete;
    EntityRendererManager& operator=(const EntityRendererManager&) = delete;

    // ========== 渲染器管理 ==========

    /**
     * @brief 获取渲染器
     * @param typeId 实体类型ID
     * @return 渲染器指针，如果未注册返回nullptr
     */
    [[nodiscard]] core::EntityRenderer* getRenderer(const std::string& typeId);

    // ========== 实体网格缓存 ==========

    /**
     * @brief 获取或创建实体网格（静态网格）
     * @param entity 客户端实体
     * @return 网格指针，如果实体类型无渲染器返回nullptr
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateMesh(ClientEntity& entity);

    /**
     * @brief 获取或创建动画实体网格
     *
     * 使用动画缓存，根据动画状态决定是否需要重新生成网格。
     * 这是主要的动画渲染路径。
     *
     * @param entity 客户端实体
     * @param model 已设置动画角度的模型
     * @param context 动画上下文
     * @return 网格指针，如果实体类型无渲染器返回nullptr
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateAnimatedMesh(
        ClientEntity& entity, model::EntityModel& model, const core::AnimationContext& context);

    /**
     * @brief 获取或创建管线网格提供者的网格
     *
     * 通过 PipelineMeshProvider 接口生成自定义网格（箭、船、矿车等）。
     * 使用静态网格缓存，但对 needsMeshUpdate() 返回 true 的实体每帧更新。
     *
     * @param entity 客户端实体
     * @param provider 管线网格提供者
     * @return 网格指针，如果生成失败返回nullptr
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateProviderMesh(
        ClientEntity& entity, core::PipelineMeshProvider& provider);

    /**
     * @brief 更新实体网格
     *
     * 当实体动画变化时调用，重新生成网格。
     *
     * @param entity 客户端实体
     */
    void updateMesh(ClientEntity& entity);

    /**
     * @brief 移除实体网格
     * @param entityId 实体ID
     */
    void removeMesh(EntityInstanceId entityId);

    /**
     * @brief 移除实体的所有网格（静态+动画）
     *
     * 在实体 despawn 时调用，同时清理静态和动画缓存。
     * @param entityId 实体ID
     */
    void removeEntityMeshes(EntityInstanceId entityId);

    /**
     * @brief 清除所有实体网格
     */
    void clearMeshes();

    /**
     * @brief 清除动画网格缓存
     */
    void clearAnimatedMeshes();

    // ========== 渲染 ==========

    /**
     * @brief 渲染实体
     * @param entity 要渲染的实体
     * @param partialTicks 部分tick
     * @deprecated 使用 renderWithPipeline 代替
     */
    void render(Entity& entity, f64 partialTicks);

    /**
     * @brief 使用管线渲染实体
     * @param cmd 命令缓冲区
     * @param entity 客户端实体
     * @param partialTicks 部分tick
     */
    void renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks);

    /**
     * @brief 使用管线渲染实体（带视锥剔除）
     *
     * 首先检查实体的包围盒是否在视锥内，如果不在则跳过渲染。
     *
     * @param cmd 命令缓冲区
     * @param entity 客户端实体
     * @param partialTicks 部分tick
     * @param frustum 视锥体（用于剔除）
     * @return true 如果实体被渲染，false 如果被剔除
     */
    bool renderWithPipeline(
        VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks, const mc::math::frustum::Frustum& frustum);

    // ========== 管线 ==========

    /**
     * @brief 设置实体渲染管线
     */
    void setPipeline(pipeline::EntityPipeline* pipeline) { m_pipeline = pipeline; }

    /**
     * @brief 设置实体纹理图集（用于UV重映射）
     */
    void setTextureAtlas(const pipeline::EntityTextureAtlas* textureAtlas);

    /**
     * @brief 设置物品纹理图集（用于 ItemEntity 渲染）
     *
     * ItemTextureAtlas 在 TridentEngine::initializeItemRenderer() 中加载并注入。
     * 注意：与 setTextureAtlas 不同，本方法只赋值不清缓存——
     * 调用方需在图集首次注入后手动 clearMeshes()，强制旧 ItemEntity 网格重做 UV 映射。
     */
    void setItemTextureAtlas(::mc::client::ItemTextureAtlas* itemAtlas) { m_itemTextureAtlas = itemAtlas; }

    /**
     * @brief 获取物品纹理图集
     */
    [[nodiscard]] ::mc::client::ItemTextureAtlas* itemTextureAtlas() { return m_itemTextureAtlas; }

    /**
     * @brief 设置方块图集（blocks atlas 的 GPU 句柄，用于末影人手持方块渲染）
     *
     * blocks atlas 来自 AtlasManager，用于末影人手持方块层（HeldBlockLayer）
     * 的方块纹理采样。在 AtlasManager 加载完 blocks atlas 后调用此方法，
     * 将句柄注入到所有需要访问方块纹理的渲染器（如 EndermanRenderer）。
     *
     * @param imageView blocks atlas 的图像视图
     * @param sampler   blocks atlas 的采样器
     */
    void setBlockAtlas(VkImageView imageView, VkSampler sampler);

    /**
     * @brief 获取实体纹理图集（只读）
     *
     * 用于层渲染器访问纹理UV区域信息。
     */
    [[nodiscard]] const pipeline::EntityTextureAtlas* textureAtlas() const { return m_textureAtlas; }

    /**
     * @brief 设置相机描述符集
     * @param descriptorSet 相机描述符集（set = 0）
     */
    void setCameraDescriptorSet(VkDescriptorSet descriptorSet) { m_cameraDescriptorSet = descriptorSet; }

    /**
     * @brief 设置相机信息（用于名称标签渲染）
     *
     * 必须在每帧渲染实体前调用，以便名称标签渲染器进行视锥剔除和背面剔除。
     *
     * @param position 相机世界位置
     * @param viewMatrix 视图矩阵
     * @param frustum 视锥体
     */
    void setCameraInfo(
        const glm::dvec3& position, const glm::mat4& viewMatrix, const mc::math::frustum::Frustum& frustum);

    /**
     * @brief 获取实体渲染管线
     */
    [[nodiscard]] pipeline::EntityPipeline* pipeline() { return m_pipeline; }

    // ========== 渲染设置 ==========

    /**
     * @brief 设置是否渲染阴影
     */
    void setRenderShadows(bool render) { m_renderShadows = render; }

    /**
     * @brief 获取是否渲染阴影
     */
    [[nodiscard]] bool renderShadows() const { return m_renderShadows; }

    /**
     * @brief 设置是否渲染名称标签
     */
    void setRenderNameTags(bool render) { m_renderNameTags = render; }

    /**
     * @brief 获取是否渲染名称标签
     */
    [[nodiscard]] bool renderNameTags() const { return m_renderNameTags; }

    // ========== 本地玩家访问 ==========

    /**
     * @brief 设置本地玩家访问器
     *
     * 第三人称玩家渲染走 GPU 管线路径（_createModelForEntity），需要从本地 Player
     * 对象读取 use-item 状态以驱动弩装填/持握动画。远程玩家缺此状态，留待网络同步
     * 实现后补齐（见 _createModelForEntity 玩家分支的 TODO 注释）。
     *
     * @param localPlayerEntityId 本地玩家的实体 ID（用于在 ClientEntity 中识别）
     * @param accessor 返回本地 Player 指针的回调（可能返回 nullptr）
     */
    void setLocalPlayerAccessor(EntityInstanceId localPlayerEntityId, std::function<::mc::Player*()> accessor)
    {
        m_localPlayerEntityId = localPlayerEntityId;
        m_localPlayerAccessor = std::move(accessor);
    }

    // ========== 初始化 ==========

    /**
     * @brief 初始化默认渲染器
     */
    void initializeDefaults();

private:
    struct StaticMeshEntry {
        pipeline::EntityMesh mesh;
        u32 itemRenderStateVersion = 0;
        i32 xpOrbIconIndex = -1; // 经验球图标索引（用于检测 XP 值变化触发网格更新）
    };

    std::unordered_map<std::string, std::unique_ptr<core::EntityRenderer>> m_renderers;

    // 静态实体网格缓存（用于非动画实体，如 ItemEntity、ExperienceOrb）
    std::unordered_map<EntityInstanceId, StaticMeshEntry> m_meshes;

    // 动画实体网格缓存（用于动画实体）
    std::unique_ptr<core::AnimatedMeshCache> m_animatedMeshCache;

    // 管线
    pipeline::EntityPipeline* m_pipeline = nullptr;
    const pipeline::EntityTextureAtlas* m_textureAtlas = nullptr;
    ::mc::client::ItemTextureAtlas* m_itemTextureAtlas = nullptr; // 用于 ItemEntity 渲染

    // blocks atlas 的 GPU 句柄（来自 AtlasManager，用于末影人手持方块层）
    VkImageView m_blockImageView = VK_NULL_HANDLE;
    VkSampler m_blockSampler = VK_NULL_HANDLE;

    // 相机描述符集（set = 0）
    VkDescriptorSet m_cameraDescriptorSet = VK_NULL_HANDLE;

    // 相机信息（用于名称标签渲染）
    glm::dvec3 m_cameraPosition{0.0, 0.0, 0.0};
    glm::mat4 m_viewMatrix{1.0f};
    mc::math::frustum::Frustum m_frustum;
    bool m_hasCameraInfo = false;

    bool m_renderShadows = true;
    bool m_renderNameTags = true;

    // 本地玩家访问器（用于第三人称玩家 GPU 管线路径读取 use-item 状态）
    EntityInstanceId m_localPlayerEntityId = INVALID_ENTITY_ID;
    std::function<::mc::Player*()> m_localPlayerAccessor;

    /**
     * @brief 创建或获取渲染器
     */
    [[nodiscard]] core::EntityRenderer* _getOrCreateRenderer(const std::string& typeId);

    /**
     * @brief 生成实体模型网格
     * @param typeId 实体类型ID
     * @param vertices 输出顶点
     * @param indices 输出索引
     * @return 是否成功生成
     */
    bool _generateModelMesh(
        const std::string& typeId, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 生成 billboard 四边形网格
     *
     * 用于 ItemEntity 和 ExperienceOrb 等静态网格实体。
     * 生成一个双面的 billboard 四边形。
     *
     * @param vertices 输出顶点
     * @param indices 输出索引
     * @param width billboard 宽度
     * @param height billboard 高度
     */
    void _generateBillboardMesh(
        std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices, f64 width, f64 height);

    /**
     * @brief 将 ItemEntity 的 UV 映射到物品纹理图集
     *
     * 根据 ItemStack 中的物品获取纹理区域，重映射 UV 坐标
     *
     * @param entity 客户端实体
     * @param vertices 顶点数据（会被修改）
     */
    void _remapItemEntityUv(ClientEntity& entity, std::vector<model::ModelVertex>& vertices);

    /**
     * @brief 将模型局部UV映射到图集区域
     */
    void _remapUvToAtlasRegion(const std::string& normalizedTypeId, std::vector<model::ModelVertex>& vertices) const;

    /**
     * @brief 将 ExperienceOrb 的 UV 映射到精灵图集中对应的图标区域
     *
     * 经验球纹理为 64x64 精灵图集，4列×3行布局，每个图标 16x16 像素。
     * 根据 XP 值确定图标索引，然后将 UV 坐标映射到对应的子区域。
     *
     * @param xpValue 经验值
     * @param textureAtlas 纹理图集（用于获取图集基础区域）
     * @param vertices 顶点数据（会被修改）
     */
    void _remapExperienceOrbUv(
        i32 xpValue, const pipeline::EntityTextureAtlas& textureAtlas, std::vector<model::ModelVertex>& vertices) const;

    /**
     * @brief 计算 ItemEntity 浮动偏移
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return Y 轴偏移
     */
    [[nodiscard]] f64 _calculateItemBobOffset(const ClientEntity& entity, f64 partialTick) const;

    /**
     * @brief 计算 ItemEntity 旋转角度
     * @param entity 客户端物品实体
     * @param partialTick 部分 tick
     * @return 旋转角度（度）
     */
    [[nodiscard]] f64 _calculateItemRotation(const ClientEntity& entity, f64 partialTick) const;

    /**
     * @brief 计算 ExperienceOrb 浮动偏移
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return Y 轴偏移
     */
    [[nodiscard]] f64 _calculateExperienceOrbBobOffset(u32 ticksExisted, f64 partialTick) const;

    /**
     * @brief 判断实体是否使用动画网格
     *
     * ItemEntity 和 ExperienceOrb 使用静态网格，
     * 其他实体使用动画网格。
     */
    [[nodiscard]] bool _usesAnimatedMesh(const std::string& normalizedTypeId) const;

    /**
     * @brief 为实体创建模型并设置动画
     *
     * 根据实体类型创建模型，设置动画参数，返回模型引用。
     * 调用者需要在使用后销毁模型。
     *
     * @param entity 客户端实体
     * @param context 动画上下文（输出）
     * @return 模型指针，如果实体类型无模型返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<model::EntityModel> _createModelForEntity(
        ClientEntity& entity, core::AnimationContext& context);

    /**
     * @brief 为玩家模型设置弩装填/持握动画参数与 ArmPose
     *
     * 在 _createModelForEntity 玩家分支中调用。从本地 Player 对象读取
     * use-item 状态，计算 maxCrossbowChargeDuration 与 ticksUsingItem，
     * 并通过 PlayerArmPoseResolver 解析 ArmPose 设置到模型。
     * 远程玩家暂缺 use-item 状态，留 TODO 待网络同步实现。
     *
     * @param playerModel 已通过 setAngles 设置基础动画的玩家模型
     * @param entity 客户端实体（用于识别是否为本地玩家）
     * @param context 动画上下文（提供 partialTicks）
     */
    void _applyPlayerCrossbowState(
        model::player::PlayerModel& playerModel, ClientEntity& entity, const core::AnimationContext& context);

    /**
     * @brief 为骷髅模型设置手臂姿态
     *
     * 在 _createModelForEntity 骷髅分支中调用。从 ClientEntity 读取 isChargingBow()
     * （通过 AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM 同步），设置 SkeletonModel
     * 的右臂 ArmPose 为 BowAndArrow，触发 BipedModel::handleRightArmPose 的拉弓动画。
     *
     * 对应 MC 1.21.11 AbstractSkeletonRenderer.getArmPose：
     *   当 isAggressive && mainHandItem.is(Items.BOW) 时返回 BOW_AND_ARROW。
     * 本项目用 chargingBow 布尔字段替代 isAggressive + isHoldingBow 组合判断，
     * 由 AbstractSkeletonEntity::tick 根据 isUsingItem + 持弓状态设置。
     *
     * 覆盖类型：普通骷髅（skeleton）、流浪者（stray）、沼骸骨（bogged）。
     * 凋灵骷髅（wither_skeleton）不持弓，走 MeleeAttackGoal，不进入此分支。
     *
     * TODO: 弩姿态（CrossbowCharge/CrossbowHold）需要 use-item 状态网络同步
     *       （参考 _applyPlayerCrossbowState 的 TODO 注释），待 ClientEntity 增加
     *       isUsingItem/getActiveItem/getItemInUseCount 后在此处补齐。
     *
     * @param skeletonModel 已通过 setAngles 设置基础动画的骷髅模型
     * @param entity 客户端实体（提供 isChargingBow）
     * @param context 动画上下文（提供 limbSwing 等用于重新调用 setAngles）
     */
    void _applySkeletonArmPose(model::monster::SkeletonModel& skeletonModel,
        const ClientEntity& entity,
        const core::AnimationContext& context);

    /**
     * @brief 为僵尸模型（及其变体）推送激怒状态与挥手进度
     *
     * 在 _createModelForEntity 僵尸分支中调用。从 ClientEntity 读取 isAggressive()
     * （通过 MobEntity::DATA_MOB_FLAGS_PARAM 位 2 同步）与 getInterpolatedSwingProgress，
     * 推送到 ZombieModel 的 setAggressive / setSwingProgress，并重新调用 setAngles
     * 使 animateZombieArms 的攻击抬臂动画生效。
     *
     * 对应 MC 1.21.11 AbstractZombieModel.setupAnim() 中：
     *   AnimationUtils.animateZombieArms(leftArm, rightArm, isAggressive, renderState);
     * 其中 renderState.attackTime 对应本项目的 m_swingProgress。
     *
     * 覆盖类型：普通僵尸（zombie）、尸壳（husk）、溺尸（drowned）、僵尸村民
     * （zombie_villager）、巨人（giant）。这些模型均继承自 ZombieModel，
     * 故 dynamic_cast<ZombieModel*> 可统一命中。
     *
     * 数据流：
     *   服务端 MeleeAttackGoal::startExecuting → MobEntity::setAggressive(true)
     *     → DATA_MOB_FLAGS_PARAM 置位 MOB_FLAG_AGGRESSIVE → EntityTracker 广播
     *   客户端 ClientEntity::syncMetadataFromDataManager 读取位 2 → setIsAggressive
     *   EntityRendererManager::_applyZombieState 读取 isAggressive → ZombieModel::setAggressive
     *   ZombieModel::setAngles 中按 m_isAggressive 选择 -PI/1.5 或 -PI/2.25 基础角度
     *
     * 关键：setSwingProgress 必须在 setAngles 之前推送，因为 ZombieModel::setAngles
     * 直接读取 m_swingProgress 计算 f2/f3 攻击动画因子。_createModelForEntity 通用路径
     * 在创建模型后已调用过一次 setAngles，但那时 m_swingProgress 仍为模型默认值 0，
     * 且 m_isAggressive 也未设置，故攻击动画不正确。此处重新 setAngles 使激怒抬臂与
     * 挥手动画在 GPU 管线路径下真正生效，避免形成孤岛代码。
     *
     * TODO: MC 1.21.11 animateZombieArms 中有 flag = (swingAnimationType != STAB)
     *       守卫，持剑刺击时跳过僵尸手臂动画（改用 swingWeaponDown）。本项目尚未
     *       实现 SwingAnimationType，此处等价于 flag=true（非刺击）。待武器系统引入
     *       SwingAnimationType 后，需在此处读取并按 MC 原版守卫。
     *
     * @param zombieModel 已通过 setAngles 设置基础动画的僵尸模型
     * @param entity 客户端实体（提供 isAggressive / getInterpolatedSwingProgress）
     * @param context 动画上下文（提供 partialTicks / limbSwing 等用于重新调用 setAngles）
     */
    void _applyZombieState(
        model::monster::ZombieModel& zombieModel, const ClientEntity& entity, const core::AnimationContext& context);

    /**
     * @brief 为溺尸模型推送三叉戟投掷手臂姿态（ThrowSpear）
     *
     * 在 _createModelForEntity 溺尸分支中于 _applyZombieState 之前调用。对应 MC 1.21.11
     * DrownedRenderer.getArmPose：
     *   if (entity.getMainArm() == hand && entity.isAggressive() &&
     *       itemHeld.is(Items.TRIDENT))
     *     return HumanoidModel.ArmPose.THROW_TRIDENT;
     *
     * 本项目从 ClientEntity 读取 isAggressive() 与 getMainHandItem()，按
     * stack.getItem() == Items::TRIDENT 判定三叉戟。僵尸类实体在 MC 原版中始终为右撇子
     * （无 MainArm NBT 字段），故仅设置右臂 ArmPose，与 _applySkeletonArmPose 仅设置
     * 右臂 BowAndArrow 的处理方式一致。
     *
     * 必须在 _applyZombieState 之前调用：_applyZombieState 末尾会重新调用 setAngles，
     * DrownedModel::setAngles 在 super.setupAnim（animateZombieArms 覆盖手臂角度）之后
     * 重新应用 ThrowSpear 姿态（xRot = xRot*0.5 - PI, yRot = 0）。若 ThrowSpear 在
     * setAngles 之后才设置，则该重应用逻辑成为死代码。
     *
     * 数据流：
     *   服务端 DrownedEntity 构造时 15% 概率 m_hasTrident=true（RangedTridentAttackGoal
     *     持有三叉戟），MeleeAttackGoal::start → setAggressive(true) 置位 MOB_FLAG_AGGRESSIVE
     *   客户端 ClientEntity::syncMetadataFromDataManager 读取位 2 → setIsAggressive；
     *     getMainHandItem 由 EntitySpawnPacket/EntityEquipmentPacket 推送
     *   EntityRendererManager::_applyDrownedTridentPose 读取上述字段 → setRightArmPose
     *   DrownedModel::setAngles 读取 m_rightArmPose 重应用 ThrowSpear 角度
     *
     * @param drownedModel 已创建的溺尸模型
     * @param entity 客户端实体（提供 isAggressive / getMainHandItem）
     */
    void _applyDrownedTridentPose(model::monster::DrownedModel& drownedModel, const ClientEntity& entity);

    /**
     * @brief 为 BipedModel 派生模型推送鞘翅飞行状态与速度因子
     *
     * 在 _createModelForEntity 通用路径中调用，覆盖所有 BipedModel 派生模型
     * （玩家、僵尸、骷髅、末影人、猪灵等）。从 ClientEntity 读取 isFallFlying()
     * 与 velocity()，按 MC 1.21.11 HumanoidMobRenderer.extractHumanoidRenderState
     * 公式计算 speedValue：
     *   - 默认 1.0
     *   - 鞘翅飞行时 speedValue = (velocity.lengthSquared() / 0.2)^3
     *   - 钳制到 [1.0, +∞)
     * 并通过 setFallFlying/setSpeedValue 推送到模型，供 setAngles 使用。
     *
     * @param bipedModel 已创建的 BipedModel 派生模型
     * @param entity 客户端实体（提供 isFallFlying/velocity）
     */
    void _applyBipedElytraState(model::BipedModel& bipedModel, const ClientEntity& entity);

    /**
     * @brief 为海豚模型推送运动状态（水平速度平方）
     *
     * 在 _createModelForEntity 通用路径中调用。对应 MC 1.21.11 DolphinRenderer
     * 中 isMoving 的填充：
     *   p_364903_.isMoving = p_480257_.getDeltaMovement().horizontalDistanceSqr() > 1.0E-7;
     * 从 ClientEntity::velocity() 取 x/z 分量计算 horizontalDistanceSqr = vx*vx + vz*vz
     * （注意：只取水平分量 XZ，不包含 Y，与 MC 的 horizontalDistanceSqr 语义一致；
     *  不能直接用 velocity().lengthSquared()，那是 3D 含 Y 的）。
     * 通过 setMotionMagnitude 推送到 DolphinModel，setAngles 中据此判断是否播放
     * 游泳摆尾动画（tail/tailFin 的 cos 波形）。
     *
     * 必须在 setAngles 之前调用，因为 setAngles 直接读取 m_motionMagnitude。
     *
     * @param dolphinModel 已创建的海豚模型
     * @param entity 客户端实体（提供 velocity）
     */
    void _applyDolphinMotionState(model::aquatic::DolphinModel& dolphinModel, const ClientEntity& entity);
};

} // namespace mc::client::renderer::entity
