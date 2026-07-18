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

#include "FirstPersonRenderer.hpp"
#include "FirstPersonTransforms.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/util/BlockMeshBuilder.hpp"
#include "client/renderer/trident/item/ItemMeshBuilder.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/map/FilledMapItem.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/item/items/weapon/ShieldItem.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/block/Block.hpp"
#include <cmath>
#include <limits>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::firstperson {

using namespace mc::math;

namespace {

[[nodiscard]] std::size_t handIndex(Hand hand)
{
    return hand == Hand::MainHand ? 0u : 1u;
}

// 将向量长度归一化，长度过小时回退为零向量，避免数值抖动。
[[nodiscard]] Vector3f normalizeSafe(const Vector3f& value)
{
    const f32 lengthSq = value.x * value.x + value.y * value.y + value.z * value.z;
    if (lengthSq <= 1e-8f) {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    const f32 invLength = 1.0f / std::sqrt(lengthSq);
    return Vector3f(value.x * invLength, value.y * invLength, value.z * invLength);
}

[[nodiscard]] Vector3f cross(const Vector3f& lhs, const Vector3f& rhs)
{
    return Vector3f(lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x);
}

/**
 * @brief 将第一人称模型根矩阵对齐到相机朝向
 *
 * 按相机前向定义构造基向量：
 * forward = (-sin(yaw)*cos(pitch), sin(pitch), cos(yaw)*cos(pitch))。
 *
 * 由于 OpenGL/Vulkan 视空间前向为 -Z，模型矩阵的第 3 列使用 -forward，
 * 这样可与相机 view 矩阵精确抵消，避免水平转头时手臂发生自转。
 */
void applyCameraAlignedBasis(MatrixStack& stack, const Player& player)
{
    const f32 pitchRad = math::toRadians(static_cast<f32>(player.pitch()));
    const f32 yawRad = math::toRadians(static_cast<f32>(player.yaw()));

    const Vector3f forward = normalizeSafe(
        Vector3f(-std::sin(yawRad) * std::cos(pitchRad), std::sin(pitchRad), std::cos(yawRad) * std::cos(pitchRad)));

    const Vector3f worldUp(0.0f, 1.0f, 0.0f);
    const Vector3f right = normalizeSafe(cross(forward, worldUp));
    const Vector3f up = normalizeSafe(cross(right, forward));

    Matrix4f& matrix = stack.last();
    matrix = Matrix4f::identity();

    // 以列向量方式写入基向量，和 shader 侧 mat4 乘法保持一致。
    matrix(0, 0) = right.x;
    matrix(1, 0) = right.y;
    matrix(2, 0) = right.z;

    matrix(0, 1) = up.x;
    matrix(1, 1) = up.y;
    matrix(2, 1) = up.z;

    matrix(0, 2) = -forward.x;
    matrix(1, 2) = -forward.y;
    matrix(2, 2) = -forward.z;
}

} // namespace

// ============================================================================
// 构造函数和析构函数
// ============================================================================

FirstPersonRenderer::FirstPersonRenderer()
    : m_model(false) // 默认使用标准手臂
{}

FirstPersonRenderer::~FirstPersonRenderer()
{
    destroy();
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> FirstPersonRenderer::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkRenderPass renderPass,
    VkDescriptorSetLayout cameraDescriptorLayout,
    VkDescriptorPool descriptorPool,
    EntityTextureAtlas* entityTextureAtlas,
    u32 maxFramesInFlight,
    VkSampleCountFlagBits sampleCount)
{
    if (device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Device is null");
    }

    if (graphicsQueue == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Graphics queue is null");
    }

    destroy();

    if (maxFramesInFlight == 0) {
        return Error(ErrorCode::InvalidArgument, "maxFramesInFlight must be greater than zero");
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_renderPass = renderPass;
    m_cameraDescriptorLayout = cameraDescriptorLayout;
    m_descriptorPool = descriptorPool;
    m_entityTextureAtlas = entityTextureAtlas;
    m_itemMeshRetirementFrames = maxFramesInFlight;

    // 创建渲染管线（手臂与物品分离，避免同一描述符集在多帧飞行时反复改写）
    m_armPipeline = std::make_unique<EntityPipeline>();
    m_itemPipeline = std::make_unique<EntityPipeline>();

    auto result = m_armPipeline->initialize(device,
        physicalDevice,
        graphicsQueue,
        renderPass,
        cameraDescriptorLayout,
        descriptorPool,
        commandPool,
        sampleCount,
        maxFramesInFlight);

    if (result.failed()) {
        destroy();
        return result.error();
    }

    result = m_itemPipeline->initialize(device,
        physicalDevice,
        graphicsQueue,
        renderPass,
        cameraDescriptorLayout,
        descriptorPool,
        commandPool,
        sampleCount,
        maxFramesInFlight);

    if (result.failed()) {
        destroy();
        return result.error();
    }

    // 设置手臂纹理图集（初始化阶段无在飞帧，写入所有帧的描述符集）
    if (entityTextureAtlas && entityTextureAtlas->isBuilt()) {
        m_armPipeline->setTextureAtlasAllFrames(entityTextureAtlas->imageView(), entityTextureAtlas->sampler());
    }

    // 初始化手持物品渲染器
    auto itemRendererResult = m_itemInHandRenderer.initialize();
    if (itemRendererResult.failed()) {
        destroy();
        spdlog::error("Failed to initialize item in hand renderer: {}", itemRendererResult.error().toString());
        return itemRendererResult.error();
    }

    m_initialized = true;
    spdlog::info("FirstPersonRenderer: Initialized");
    return {};
}

void FirstPersonRenderer::destroy()
{
    _destroyItemMeshes();
    _invalidateArmMeshes();

    // 销毁手持物品渲染器
    m_itemInHandRenderer.destroy();

    // 销毁管线
    m_itemPipeline.reset();
    m_armPipeline.reset();

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_renderPass = VK_NULL_HANDLE;
    m_descriptorPool = VK_NULL_HANDLE;
    m_entityTextureAtlas = nullptr;
    m_itemTextureAtlas = nullptr;
    m_itemMeshRetirementFrames = 0;
    m_initialized = false;
}

void FirstPersonRenderer::beginFrame(u32 frameIndex)
{
    if (m_armPipeline && m_armPipeline->isInitialized()) {
        m_armPipeline->beginFrame(frameIndex);
    }
    if (m_itemPipeline && m_itemPipeline->isInitialized()) {
        m_itemPipeline->beginFrame(frameIndex);
    }
}

void FirstPersonRenderer::processPendingDestroys()
{
    if (m_armPipeline && m_armPipeline->isInitialized()) {
        m_armPipeline->processPendingDestroys();
    }
    if (m_itemPipeline && m_itemPipeline->isInitialized()) {
        m_itemPipeline->processPendingDestroys();
    }
}

void FirstPersonRenderer::setItemTextureAtlas(const mc::client::ItemTextureAtlas* itemTextureAtlas)
{
    m_itemTextureAtlas = itemTextureAtlas;

    // 图集切换后强制重建手持物品网格，避免旧 UV 映射失效。
    _invalidateItemMeshes();

    if (m_itemPipeline && m_itemTextureAtlas && m_itemTextureAtlas->isValid()) {
        // 图集切换需写入所有帧的描述符集，否则在飞帧仍引用旧图集。
        m_itemPipeline->setTextureAtlasAllFrames(m_itemTextureAtlas->imageView(), m_itemTextureAtlas->sampler());
    }
}

void FirstPersonRenderer::setBlockAtlas(VkImageView imageView, VkSampler sampler)
{
    m_blockImageView = imageView;
    m_blockSampler = sampler;

    // 方块物品 3D 网格 UV 基于 blocks atlas，图集切换后强制重建避免 UV 失效。
    _invalidateItemMeshes();
}

void FirstPersonRenderer::setPlayerSkinLocation(const ResourceLocation& playerSkinLocation)
{
    m_playerSkinLocation = playerSkinLocation;

    // 皮肤切换后强制重建手臂网格，确保 UV 立即生效。
    _invalidateArmMeshes();

    if (m_armPipeline && m_entityTextureAtlas && m_entityTextureAtlas->isBuilt()) {
        m_armPipeline->setTextureAtlasAllFrames(m_entityTextureAtlas->imageView(), m_entityTextureAtlas->sampler());
    }
}

// ============================================================================
// 每帧更新
// ============================================================================

void FirstPersonRenderer::tick(Player* player)
{
    // 保存上一帧的装备进度（tick() 的 oMainHandHeight/oOffHandHeight 赋值）。
    m_prevMainHandEquipProgress = m_mainHandEquipProgress;
    m_prevOffHandEquipProgress = m_offHandEquipProgress;

    // 获取当前手持物品
    ItemStack mainHandItem = player != nullptr ? _getHeldItem(player, Hand::MainHand) : ItemStack();
    ItemStack offHandItem = player != nullptr ? _getHeldItem(player, Hand::OffHand) : ItemStack();

    // 物品切换检测：当 visible item 与当前槽位不一致时，target 归零触发下落动画；
    // 当高度降到 < 0.1 时再刷新 visible item（mainHandHeight < 0.1F 时才更新 mainHandItem）。
    const bool mainHandUnchanged = (m_prevMainHandItem == mainHandItem);
    const bool offHandUnchanged = (m_prevOffHandItem == offHandItem);

    // isHandsBusy()：骑船转向输入时为 true，此时两手快速下降 0.4/tick。
    // 项目船舵输入未接入客户端玩家状态，按 false 处理。
    const bool handsBusy = false;

    if (handsBusy) {
        m_mainHandEquipProgress = clamp(m_mainHandEquipProgress - 0.4f, 0.0f, 1.0f);
        m_offHandEquipProgress = clamp(m_offHandEquipProgress - 0.4f, 0.0f, 1.0f);
    } else {
        // f = getItemSwapScale(1.0)：基于独立的 itemSwapTicker，仅随主手物品种类切换重置，
        // 与攻击冷却解耦（Player.getItemSwapScale）。
        const f32 f = (player != nullptr) ? player->getItemSwapScale(1.0f) : 1.0f;
        // 主手 target = (物品未变 ? f^3 : 0)；副手 target = (物品未变 ? 1 : 0)。
        const f32 mainTarget = mainHandUnchanged ? (f * f * f) : 0.0f;
        const f32 offTarget = offHandUnchanged ? 1.0f : 0.0f;

        // 以 ±0.4/tick 的速率向 target 逼近（Mth.clamp(target - height, -0.4, 0.4)）。
        m_mainHandEquipProgress += clamp(mainTarget - m_mainHandEquipProgress, -0.4f, 0.4f);
        m_offHandEquipProgress += clamp(offTarget - m_offHandEquipProgress, -0.4f, 0.4f);
        m_mainHandEquipProgress = clamp(m_mainHandEquipProgress, 0.0f, 1.0f);
        m_offHandEquipProgress = clamp(m_offHandEquipProgress, 0.0f, 1.0f);
    }

    // 高度降到 < 0.1 后刷新 visible item，使下一帧 target 重新生效（完成切换动画的下半段）。
    if (m_mainHandEquipProgress < 0.1f) {
        m_prevMainHandItem = mainHandItem;
    }
    if (m_offHandEquipProgress < 0.1f) {
        m_prevOffHandItem = offHandItem;
    }

    // 更新挥动进度（由 render 方法中的 RenderContext 提供）
    m_prevSwingProgress = m_swingProgress;
}

// ============================================================================
// 渲染
// ============================================================================

void FirstPersonRenderer::render(VkCommandBuffer cmd, VkDescriptorSet cameraDescriptorSet, const RenderContext& context)
{
    if (!m_initialized) {
        return;
    }

    _cleanupRetiredItemMeshes();

    if (context.player == nullptr || m_armPipeline == nullptr) {
        return;
    }

    Player* player = context.player;

    // D7: 使用望远镜瞄准时不渲染任何手部（renderArmWithItem 外层 if(!isScoping)）。
    if (player->isScoping()) {
        return;
    }

    // 获取玩家主手设置
    HandSide primaryHand = _getPrimaryHand(player);

    // 计算插值后的装备进度（renderHandsWithItems：
    //   equipProgress = swapAnimationScale(item) * (1 - lerp(partial, oHeight, height))
    // 项目无 per-item swapAnimationScale 数据组件，按默认 1.0 处理）。
    // 注意 height=1 表示完全可见，equipProgress=0 表示完全可见；返回值是“隐藏度”，
    // 供 applyItemArmTransform/renderPlayerArm 的 equip*-0.6 下落使用。
    const f32 partial = static_cast<f32>(context.partialTick);
    f32 mainEquipProgress = computeEquipProgress(m_prevMainHandEquipProgress, m_mainHandEquipProgress, partial, 1.0f);
    f32 offEquipProgress = computeEquipProgress(m_prevOffHandEquipProgress, m_offHandEquipProgress, partial, 1.0f);

    // 优先使用上层传入的动画状态，便于未来接入完整客户端动画系统。
    if (context.mainHandEquipProgress > 0.0f || context.offHandEquipProgress > 0.0f) {
        mainEquipProgress = context.mainHandEquipProgress;
        offEquipProgress = context.offHandEquipProgress;
    }

    // 计算插值后的挥动进度
    f32 mainSwingProgress = context.mainHandSwingProgress;
    f32 offSwingProgress = context.offHandSwingProgress;
    if (mainSwingProgress <= 0.0f && offSwingProgress <= 0.0f) {
        const f32 fallbackSwing = lerp(m_prevSwingProgress, m_swingProgress, static_cast<f32>(context.partialTick));
        if (m_swingHand == Hand::MainHand) {
            mainSwingProgress = fallbackSwing;
        } else {
            offSwingProgress = fallbackSwing;
        }
    }

    // 获取手持物品
    ItemStack mainHandItem = _getHeldItem(player, Hand::MainHand);
    ItemStack offHandItem = _getHeldItem(player, Hand::OffHand);

    // 确定手臂姿态（复用上面已取的物品引用，避免每帧再深拷贝两次）
    ArmPose mainArmPose = _determineArmPose(player, Hand::MainHand, mainHandItem);
    ArmPose offArmPose = _determineArmPose(player, Hand::OffHand, offHandItem);

    // 更新模型手臂姿态
    m_model.setRightArmPose(primaryHand == HandSide::Right ? mainArmPose : offArmPose);
    m_model.setLeftArmPose(primaryHand == HandSide::Right ? offArmPose : mainArmPose);

    // D9: 评估本帧需要渲染的手（ItemInHandRenderer.evaluateWhichHandsToRender）。
    // 持弓/弩时的使用手与已装填弩副手抑制都由该函数决定；renderHand 内部据物品空态
    // 自行选择渲染手臂或物品。
    const HandRenderSelection handSelection = evaluateWhichHandsToRender(*player);
    const bool renderMainHand = shouldRenderHand(handSelection, Hand::MainHand);
    const bool renderOffHand = shouldRenderHand(handSelection, Hand::OffHand);

    const Vector3f cameraPos(static_cast<f32>(player->x()),
        static_cast<f32>(player->y() + player->eyeHeight()),
        static_cast<f32>(player->z()));

    const auto toModelMatrix = [](const Matrix4f& matrix) {
        std::array<f64, 16> modelMatrix{};
        for (i32 row = 0; row < 4; ++row) {
            for (i32 col = 0; col < 4; ++col) {
                modelMatrix[static_cast<size_t>(row * 4 + col)] =
                    static_cast<f64>(matrix.data[static_cast<size_t>(row * 4 + col)]);
            }
        }
        return modelMatrix;
    };

    const auto renderHand = [&](Hand hand, const ItemStack& heldItem, f32 equipProgress, f32 swingProgress) {
        const HandSide handSide = _resolveHandSide(hand, primaryHand);
        const ArmPose armPose = (hand == Hand::MainHand) ? mainArmPose : offArmPose;
        const f32 partial = static_cast<f32>(context.partialTick);

        MatrixStack baseStack;
        applyCameraAlignedBasis(baseStack, *player);

        // 伤害倾斜（GameRenderer.bobHurt）：相机对齐基之后、视野摇晃之前应用。
        // 从本地玩家读取 hurtTime/hurtDuration/hurtDir/deathTime，由服务端 hurt 动画包同步。
        computeDamageTilt(baseStack,
            partial,
            static_cast<f32>(player->hurtTime()),
            static_cast<f32>(player->hurtDuration()),
            player->getHurtDir(),
            context.damageTiltStrength,
            static_cast<f32>(player->deathTime()),
            player->isDead());

        // 视野摇晃作用到手部 PoseStack（GameRenderer.renderItemInHand：
        // 相机对齐基之后、渲染手部之前应用 bobView）。输入与相机 bobView 同源：
        // backwardsWalkDistance = -(moveDistanceWalked + delta*partial)，
        // interpolatedBob = lerp(prevCameraYaw, cameraYaw, partial)。
        if (context.viewBobbingEnabled) {
            const f32 walkDelta = player->moveDistanceWalked() - player->prevMoveDistanceWalked();
            const f32 backwardsWalk = -(player->moveDistanceWalked() + walkDelta * partial);
            const f32 interpolatedBob = lerp(player->prevCameraYaw(), player->cameraYaw(), partial);
            computeViewBobbing(baseStack, backwardsWalk, interpolatedBob);
        }

        // 手里为空时渲染手臂，非空时走物品分支（不额外绘制手臂网格）。
        if (heldItem.isEmpty()) {
            MatrixStack armStack = baseStack;
            renderPlayerArmTransform(armStack, handSide, equipProgress, swingProgress);

            _ensureArmMesh(hand, primaryHand);
            const bool isMainHand = hand == Hand::MainHand;
            const EntityMesh& armMesh = isMainHand ? m_mainHandArmMesh : m_offHandArmMesh;
            const bool armMeshValid = isMainHand ? m_mainHandArmMeshValid : m_offHandArmMeshValid;

            if (armMeshValid && armMesh.indexCount > 0) {
                const auto armModelMatrix = toModelMatrix(armStack.last());
                m_armPipeline->drawMesh(cmd, armMesh, armModelMatrix, cameraPos, 1.0);
            }
            return;
        }

        // 地图物品：渲染双手举起姿态，地图内容在GUI层渲染
        if (armPose == ArmPose::Map) {
            MatrixStack mapStack = baseStack;
            _renderMapFirstPerson(mapStack, heldItem, hand, partial, equipProgress, swingProgress, *player);

            _ensureArmMesh(hand, primaryHand);
            const bool isMainHand = hand == Hand::MainHand;
            const EntityMesh& armMesh = isMainHand ? m_mainHandArmMesh : m_offHandArmMesh;
            const bool armMeshValid = isMainHand ? m_mainHandArmMeshValid : m_offHandArmMeshValid;

            if (armMeshValid && armMesh.indexCount > 0) {
                const auto armModelMatrix = toModelMatrix(mapStack.last());
                m_armPipeline->drawMesh(cmd, armMesh, armModelMatrix, cameraPos, 1.0);
            }
            return;
        }

        if (m_itemPipeline == nullptr) {
            return;
        }

        _ensureItemMesh(hand, heldItem);
        const ItemMeshState& itemMeshState = m_itemMeshes[handIndex(hand)];
        if (!itemMeshState.valid || itemMeshState.mesh.indexCount == 0) {
            return;
        }

        MatrixStack itemStack = baseStack;
        // 传入真实的 isUsingItem/useCount/partialTicks，让 UseAction 变换（进食、拉弓、收纳袋等）生效
        // 对应 1.21.11 HeldItemRenderer.renderItem 中对 useItem 状态的判断
        const bool handIsUsingItem = player->isUsingItem() && player->getActiveHand() == hand;
        const i32 handUseCount = handIsUsingItem ? player->getItemInUseCount() : 0;
        _renderItemInHand(itemStack,
            player,
            heldItem,
            handSide,
            equipProgress,
            swingProgress,
            handIsUsingItem,
            handUseCount,
            static_cast<f32>(context.partialTick));

        m_itemPipeline->bind(cmd);
        vkCmdBindDescriptorSets(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_itemPipeline->pipelineLayout(),
            0,
            1,
            &cameraDescriptorSet,
            0,
            nullptr);

        // 方块物品 3D 网格 UV 基于 blocks atlas，绘制前切换管线图集，
        // 绘制后恢复为物品纹理图集（与第三人称 HeldBlockLayer 同模式）。
        const bool needAtlasSwitch = itemMeshState.isBlockItem && m_blockImageView != VK_NULL_HANDLE;
        if (needAtlasSwitch) {
            m_itemPipeline->setTextureAtlas(m_blockImageView, m_blockSampler);
        }
        m_itemPipeline->bindTextureDescriptor(cmd);
        const auto itemModelMatrix = toModelMatrix(itemStack.last());
        m_itemPipeline->drawMesh(cmd, itemMeshState.mesh, itemModelMatrix, cameraPos, 1.0);
        if (needAtlasSwitch && m_itemTextureAtlas != nullptr && m_itemTextureAtlas->isValid()) {
            m_itemPipeline->setTextureAtlas(m_itemTextureAtlas->imageView(), m_itemTextureAtlas->sampler());
        }

        // 恢复手臂管线，供后续手臂绘制继续使用。
        m_armPipeline->bind(cmd);
        vkCmdBindDescriptorSets(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_armPipeline->pipelineLayout(),
            0,
            1,
            &cameraDescriptorSet,
            0,
            nullptr);
        m_armPipeline->bindTextureDescriptor(cmd);
    };

    // 绑定管线
    m_armPipeline->bind(cmd);

    // 绑定相机描述符集
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_armPipeline->pipelineLayout(), 0, 1, &cameraDescriptorSet, 0, nullptr);

    // 绑定纹理描述符集
    m_armPipeline->bindTextureDescriptor(cmd);

    // 按槽位渲染，避免主手设置为左手时出现主副手物品错位。
    if (renderMainHand) {
        renderHand(Hand::MainHand, mainHandItem, mainEquipProgress, mainSwingProgress);
    }

    if (renderOffHand) {
        renderHand(Hand::OffHand, offHandItem, offEquipProgress, offSwingProgress);
    }
}

void FirstPersonRenderer::_renderItemInHand(
    MatrixStack& stack, Player* player, const ItemStack& itemStack, HandSide side, f32 equipProgress, f32 swingProgress)
{
    _renderItemInHand(stack, player, itemStack, side, equipProgress, swingProgress, false, 0, static_cast<f32>(PI));
}

void FirstPersonRenderer::_renderItemInHand(MatrixStack& stack,
    Player* player,
    const ItemStack& itemStack,
    HandSide side,
    f32 equipProgress,
    f32 swingProgress,
    bool isUsingItem,
    i32 useCount,
    f32 partialTicks)
{
    if (player == nullptr || itemStack.isEmpty()) {
        return;
    }

    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return;
    }

    const bool leftHanded = side == HandSide::Left;
    const TransformType transformType =
        leftHanded ? TransformType::FirstPersonLeftHand : TransformType::FirstPersonRightHand;
    const UseAction useAction = item->getUseAction(itemStack);

    // 弩：始终先施加通用基座，再按装填/已装填分支处理（CROSSBOW 分支）。
    if (useAction == UseAction::Crossbow) {
        applyItemArmTransform(stack, side, equipProgress);
        const bool isCharged = item::CrossbowItem::isCharged(itemStack);
        const bool isCharging = isUsingItem && useCount > 0 && !isCharged;
        const i32 useDuration = item->getUseDuration(itemStack);
        const i32 chargeDuration = item::CrossbowItem::getChargeTime(itemStack);
        const bool isMainHand = player->getActiveHand() == Hand::MainHand;
        applyCrossbowTransform(stack,
            partialTicks,
            side,
            useDuration,
            useCount,
            isCharging,
            isCharged,
            isMainHand,
            chargeDuration,
            swingProgress);
        m_itemInHandRenderer.applyTransform(stack, itemStack, transformType, leftHanded);
        return;
    }

    // 使用物品分支（非弩）：renderArmWithItem 的 isUsingItem 分支。
    if (isUsingItem && useCount > 0) {
        // EAT/DRINK 在 MC 中 hasCustomArmTransform=true，跳过通用基座（其 case 内会再施加）。
        const bool hasCustomArmTransform = (useAction == UseAction::Eat || useAction == UseAction::Drink);
        if (!hasCustomArmTransform) {
            applyItemArmTransform(stack, side, equipProgress);
        }

        const i32 useDuration = item->getUseDuration(itemStack);
        switch (useAction) {
            case UseAction::Eat:
            case UseAction::Drink:
                // 进食/饮用：先抖动变换，再补回通用基座（MC 在 case 内二次 applyItemArmTransform）。
                applyEatTransform(stack, partialTicks, side, useDuration, useCount);
                applyItemArmTransform(stack, side, equipProgress);
                break;
            case UseAction::Block:
                // 非盾格挡变换；盾牌由模型 display 自身处理（MC ShieldItem 跳过此变换）。
                if (dynamic_cast<const item::ShieldItem*>(item) == nullptr) {
                    applyBlockTransform(stack, side);
                }
                break;
            case UseAction::Bow:
                // 弓蓄力（含专属基座 translate+rotate）。
                applyBowTransform(stack, partialTicks, side, useDuration, useCount);
                break;
            case UseAction::Spear:
                // 三叉戟蓄力（项目 Spear 即 MC TRIDENT，含专属基座 translate+rotate）。
                applyTridentTransform(stack, partialTicks, side, useDuration, useCount);
                break;
            case UseAction::Brush:
                applyBrushTransform(stack, partialTicks, side, useCount);
                break;
            case UseAction::Bundle:
                // 收纳袋：仅挥动，无专属变换。
                swingArm(stack, side, swingProgress);
                break;
            default:
                // NONE/其他：仅有通用基座，无额外变换。
                break;
        }

        m_itemInHandRenderer.applyTransform(stack, itemStack, transformType, leftHanded);
        return;
    }

    // 三叉戟激流冲刺（auto-spin attack）：通用基座 + 冲刺变换。
    if (player->isSpinAttacking()) {
        applyItemArmTransform(stack, side, equipProgress);
        applyAutoSpinTransform(stack, side, equipProgress);
        m_itemInHandRenderer.applyTransform(stack, itemStack, transformType, leftHanded);
        return;
    }

    // 常规持物：通用基座 + 挥动 + display 变换。
    applyItemArmTransform(stack, side, equipProgress);
    swingArm(stack, side, swingProgress);
    m_itemInHandRenderer.applyTransform(stack, itemStack, transformType, leftHanded);
}

void FirstPersonRenderer::_renderMapFirstPerson(MatrixStack& stack,
    const ItemStack& mapStack,
    Hand hand,
    f32 pitch,
    f32 equipProgress,
    f32 swingProgress,
    const Player& player)
{
    (void)mapStack; // 地图内容（MapItemSavedData）由 GuiRenderer 在屏幕层渲染，此处仅对齐持握变换。

    // renderArmWithItem 的 MAP_ID 分支：
    //   主手持地图 + 副手为空 → renderTwoHandedMap（双手举起）；
    //   否则 → renderOneHandedMap（单手）。
    const bool twoHanded = (hand == Hand::MainHand) && player.getOffHandItem().isEmpty();
    const HandSide handSide = _resolveHandSide(hand, _getPrimaryHand(&player));

    if (twoHanded) {
        applyTwoHandedMapTransform(stack, pitch, equipProgress, swingProgress);
    } else {
        applyOneHandedMapTransform(stack, handSide, equipProgress, swingProgress);
    }
}

// ============================================================================
// 动画控制
// ============================================================================

void FirstPersonRenderer::resetEquippedProgress(Hand hand)
{
    if (hand == Hand::MainHand) {
        m_mainHandEquipProgress = 0.0f;
    } else {
        m_offHandEquipProgress = 0.0f;
    }
}

// ============================================================================
// GPU 资源管理
// ============================================================================

void FirstPersonRenderer::_invalidateArmMeshes()
{
    if (!m_armPipeline) {
        m_mainHandArmMeshValid = false;
        m_offHandArmMeshValid = false;
        return;
    }

    if (m_mainHandArmMeshValid) {
        m_armPipeline->destroyMesh(m_mainHandArmMesh);
        m_mainHandArmMeshValid = false;
    }

    if (m_offHandArmMeshValid) {
        m_armPipeline->destroyMesh(m_offHandArmMesh);
        m_offHandArmMeshValid = false;
    }
}

void FirstPersonRenderer::_invalidateItemMeshes()
{
    for (auto& itemMeshState : m_itemMeshes) {
        if (itemMeshState.valid) {
            _retireItemMesh(itemMeshState.mesh);
            itemMeshState.valid = false;
        }
        itemMeshState.itemId = std::numeric_limits<ItemId>::max();
    }
}

void FirstPersonRenderer::_cleanupRetiredItemMeshes()
{
    if (!m_itemPipeline) {
        m_retiredItemMeshes.clear();
        return;
    }

    auto it = m_retiredItemMeshes.begin();
    while (it != m_retiredItemMeshes.end()) {
        if (it->framesRemaining > 0) {
            --it->framesRemaining;
        }

        if (it->framesRemaining == 0) {
            m_itemPipeline->destroyMesh(it->mesh);
            it = m_retiredItemMeshes.erase(it);
        } else {
            ++it;
        }
    }
}

void FirstPersonRenderer::_retireItemMesh(EntityMesh& mesh)
{
    if (mesh.vertexBuffer == VK_NULL_HANDLE && mesh.vertexMemory == VK_NULL_HANDLE &&
        mesh.indexBuffer == VK_NULL_HANDLE && mesh.indexMemory == VK_NULL_HANDLE) {
        mesh = {};
        return;
    }

    m_retiredItemMeshes.push_back(RetiredItemMesh{mesh, m_itemMeshRetirementFrames});
    mesh = {};
}

void FirstPersonRenderer::_destroyItemMeshes()
{
    if (m_itemPipeline) {
        for (auto& itemMeshState : m_itemMeshes) {
            if (itemMeshState.valid) {
                m_itemPipeline->destroyMesh(itemMeshState.mesh);
            }
        }

        for (auto& retiredMesh : m_retiredItemMeshes) {
            m_itemPipeline->destroyMesh(retiredMesh.mesh);
        }
    }

    for (auto& itemMeshState : m_itemMeshes) {
        itemMeshState.mesh = {};
        itemMeshState.valid = false;
        itemMeshState.itemId = std::numeric_limits<ItemId>::max();
    }

    m_retiredItemMeshes.clear();
}

void FirstPersonRenderer::_ensureArmMesh(Hand hand, HandSide primaryHand)
{
    if (!m_armPipeline) {
        return;
    }

    const bool isMainHand = hand == Hand::MainHand;
    EntityMesh& armMesh = isMainHand ? m_mainHandArmMesh : m_offHandArmMesh;
    bool& armMeshValid = isMainHand ? m_mainHandArmMeshValid : m_offHandArmMeshValid;
    if (armMeshValid) {
        return;
    }

    m_cachedVertices.clear();
    m_cachedIndices.clear();

    const auto arm = _selectArmModel(hand, primaryHand);
    if (arm) {
        arm->generateMesh(m_cachedVertices, m_cachedIndices, 1.0 / 16.0);
    }

    _remapToPlayerSkinRegion(m_cachedVertices);

    if (m_cachedVertices.empty() || m_cachedIndices.empty()) {
        return;
    }

    auto result = m_armPipeline->createMesh(m_cachedVertices, m_cachedIndices);
    if (result.success()) {
        armMesh = std::move(result.value());
        armMeshValid = true;
    } else {
        spdlog::error("FirstPersonRenderer: Failed to create {} hand arm mesh: {}",
            isMainHand ? "main" : "off",
            result.error().message());
    }
}

void FirstPersonRenderer::_ensureItemMesh(Hand hand, const ItemStack& itemStack)
{
    if (!m_itemPipeline || itemStack.isEmpty()) {
        return;
    }

    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return;
    }

    const ItemId itemId = item->itemId();
    ItemMeshState& itemMeshState = m_itemMeshes[handIndex(hand)];

    // 缓存命中判定：itemId + 是否方块物品。bakeTransforms=false 时左右手原始几何相同，
    // 镜像由绘制阶段矩阵栈上的 applyTransform 施加，故无需纳入缓存键。
    const bool isBlockItem = (dynamic_cast<const BlockItem*>(item) != nullptr);
    if (itemMeshState.valid && itemMeshState.itemId == itemId && itemMeshState.isBlockItem == isBlockItem) {
        return;
    }

    if (itemMeshState.valid) {
        // 物品切换时将旧网格延后回收，避免上一帧仍在引用旧缓冲区。
        _retireItemMesh(itemMeshState.mesh);
        itemMeshState.valid = false;
        itemMeshState.itemId = std::numeric_limits<ItemId>::max();
        itemMeshState.isBlockItem = false;
    }

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    if (isBlockItem) {
        // 方块物品：构建带逐面纹理的 3D 方块网格（UV 基于方块纹理图集）。
        // 1.21.11 持方块物品渲染完整 3D 方块模型。纹理在方块纹理图集中，
        // 绘制阶段由 renderHand lambda 切换管线图集。
        const auto* blockItem = static_cast<const BlockItem*>(item);
        const BlockState& blockState = blockItem->block().defaultState();
        entity::util::BlockMeshBuilder::buildBlockMesh(blockState, vertices, indices);
    } else {
        // 普通物品：构建 3D 模型网格（原始几何，不烘焙 display/摄像机变换）。
        // display 变换由 _renderItemInHand 末尾的 applyTransform 在矩阵栈上单独施加，
        // 避免与 ItemMeshBuilder 内部烘焙双重施加。左手镜像由 transformType 决定。
        if (m_itemTextureAtlas == nullptr || !m_itemTextureAtlas->isValid()) {
            return;
        }
        // bakeTransforms=false 时 transformType 不影响输出（display 变换不烘焙），
        // 第一人称左右手共用同一份原始几何，镜像由绘制阶段矩阵栈施加。任意第一人称类型皆可。
        using entity::item::ItemTransformType;
        const ItemTransformType transformType = ItemTransformType::FirstPersonRightHand;
        auto [builtVertices, builtIndices] =
            entity::item::ItemMeshBuilder::buildHeldItemMesh(itemStack, transformType, false);
        vertices = std::move(builtVertices);
        indices = std::move(builtIndices);
    }

    if (vertices.empty() || indices.empty()) {
        return;
    }

    auto createResult = m_itemPipeline->createMesh(vertices, indices);
    if (createResult.failed()) {
        spdlog::warn("FirstPersonRenderer: Failed to create item mesh for {}", item->itemLocation().toString());
        return;
    }

    itemMeshState.mesh = std::move(createResult.value());
    itemMeshState.valid = true;
    itemMeshState.itemId = itemId;
    itemMeshState.isBlockItem = isBlockItem;
}

void FirstPersonRenderer::_remapToPlayerSkinRegion(std::vector<ModelVertex>& vertices) const
{
    if (vertices.empty() || m_entityTextureAtlas == nullptr || !m_entityTextureAtlas->isBuilt()) {
        return;
    }

    const mc::TextureRegion* region = m_entityTextureAtlas->getRegion(m_playerSkinLocation);
    if (region == nullptr) {
        const std::array<ResourceLocation, 4> fallbacks = {
            ResourceLocation("minecraft:textures/entity/player/slim/steve.png"),
            ResourceLocation("minecraft:textures/entity/player/wide/steve.png"),
            ResourceLocation("minecraft:textures/entity/player/slim/alex.png"),
            ResourceLocation("minecraft:textures/entity/steve.png")};

        for (const auto& location : fallbacks) {
            region = m_entityTextureAtlas->getRegion(location);
            if (region != nullptr) {
                break;
            }
        }
    }

    if (region == nullptr) {
        return;
    }

    const f64 du = region->u1 - region->u0;
    const f64 dv = region->v1 - region->v0;
    for (auto& vertex : vertices) {
        const f64 remappedU = region->u0 + static_cast<f64>(vertex.texCoord.x) * du;
        const f64 remappedV = region->v0 + static_cast<f64>(vertex.texCoord.y) * dv;
        vertex.texCoord.x = static_cast<f32>(remappedU);
        vertex.texCoord.y = static_cast<f32>(remappedV);
    }
}

// ============================================================================
// 工具方法
// ============================================================================

ArmPose FirstPersonRenderer::_determineArmPose(Player* player, Hand hand, const ItemStack& heldItem) const
{
    if (player == nullptr) {
        return ArmPose::Empty;
    }

    if (heldItem.isEmpty()) {
        return ArmPose::Empty;
    }

    // 获取物品
    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ArmPose::Item;
    }

    // 检查玩家是否正在使用物品
    const bool isUsingItem = player->isUsingItem();
    const Hand activeHand = player->getActiveHand();
    const bool isHandActive = isUsingItem && activeHand == hand;

    // 获取物品使用动作
    const UseAction useAction = item->getUseAction(heldItem);

    // 根据物品使用状态确定手臂姿态
    if (isHandActive) {
        switch (useAction) {
            case UseAction::Eat:
            case UseAction::Drink:
                return ArmPose::EatOrDrink;

            case UseAction::Block:
                return ArmPose::Block;

            case UseAction::Bow:
                return ArmPose::BowAndArrow;

            case UseAction::Spear:
                // Trident 是 Spear 的别名，不需要单独 case
                return ArmPose::ThrowSpear;

            case UseAction::Crossbow:
                return ArmPose::CrossbowCharge;

            case UseAction::Spyglass:
                // 原版望远镜第一人称走独立的 SpyglassItemInHandRenderer，不在此处施加姿态
                return ArmPose::Item;

            case UseAction::Bundle:
                // 收纳袋：使用动作类似饮用，复用 EatOrDrink 姿态
                return ArmPose::EatOrDrink;

            default:
                break;
        }
    }

    // 检查弩是否已装填
    if (dynamic_cast<const item::CrossbowItem*>(item) != nullptr) {
        if (item::CrossbowItem::isCharged(heldItem)) {
            return ArmPose::CrossbowHold;
        }
    }

    // 检查是否为地图
    if (item == Items::FILLED_MAP) {
        return ArmPose::Map;
    }

    // 默认持有物品姿态
    return ArmPose::Item;
}

HandSide FirstPersonRenderer::_getPrimaryHand(const Player* player)
{
    if (player == nullptr) {
        return HandSide::Right;
    }

    // 从 LivingEntity 获取主手设置
    return player->getPrimaryHand();
}

ItemStack FirstPersonRenderer::_getHeldItem(Player* player, Hand hand)
{
    if (player == nullptr) {
        return ItemStack();
    }

    // 从玩家背包获取手持物品
    PlayerInventory& inventory = player->inventory();
    if (hand == Hand::MainHand) {
        return inventory.getSelectedStack();
    } else {
        return inventory.getOffhandItem();
    }
}

HandSide FirstPersonRenderer::_resolveHandSide(Hand hand, HandSide primaryHand)
{
    if (hand == Hand::MainHand) {
        return primaryHand;
    }
    return primaryHand == HandSide::Right ? HandSide::Left : HandSide::Right;
}

std::shared_ptr<ModelRenderer> FirstPersonRenderer::_selectArmModel(Hand hand, HandSide primaryHand) const
{
    const HandSide handSide = _resolveHandSide(hand, primaryHand);
    return handSide == HandSide::Right ? m_model.rightArm() : m_model.leftArm();
}

} // namespace mc::client::renderer::trident::firstperson
