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
#include "ItemInHandRenderer.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::firstperson {

using namespace mc::math;

namespace {

[[nodiscard]] std::size_t handIndex(Hand hand)
{
    return hand == Hand::MainHand ? 0u : 1u;
}

} // namespace

// ============================================================================
// 常量定义
// ============================================================================

/// 手臂在屏幕侧边的偏移
static constexpr f32 SIDE_OFFSET_X = 0.56f;
static constexpr f32 SIDE_OFFSET_Y = -0.52f;
static constexpr f32 SIDE_OFFSET_Z = -0.72f;

/// 装备动画速度
static constexpr f32 EQUIP_SPEED = 0.4f;

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
 * 严格按 MC 1.16.5 的相机前向定义构造基向量：
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
        sampleCount);

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
        sampleCount);

    if (result.failed()) {
        destroy();
        return result.error();
    }

    // 设置手臂纹理图集
    if (entityTextureAtlas && entityTextureAtlas->isBuilt()) {
        m_armPipeline->setTextureAtlas(entityTextureAtlas->imageView(), entityTextureAtlas->sampler());
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
    destroyItemMeshes();
    invalidateArmMeshes();

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

void FirstPersonRenderer::setItemTextureAtlas(const mc::client::ItemTextureAtlas* itemTextureAtlas)
{
    m_itemTextureAtlas = itemTextureAtlas;

    // 图集切换后强制重建手持物品网格，避免旧 UV 映射失效。
    invalidateItemMeshes();

    if (m_itemPipeline && m_itemTextureAtlas && m_itemTextureAtlas->isValid()) {
        m_itemPipeline->setTextureAtlas(m_itemTextureAtlas->imageView(), m_itemTextureAtlas->sampler());
    }
}

void FirstPersonRenderer::setPlayerSkinLocation(const ResourceLocation& playerSkinLocation)
{
    m_playerSkinLocation = playerSkinLocation;

    // 皮肤切换后强制重建手臂网格，确保 UV 立即生效。
    invalidateArmMeshes();

    if (m_armPipeline && m_entityTextureAtlas && m_entityTextureAtlas->isBuilt()) {
        m_armPipeline->setTextureAtlas(m_entityTextureAtlas->imageView(), m_entityTextureAtlas->sampler());
    }
}

// ============================================================================
// 每帧更新
// ============================================================================

void FirstPersonRenderer::tick(Player* player)
{
    // 保存上一帧的装备进度
    m_prevMainHandEquipProgress = m_mainHandEquipProgress;
    m_prevOffHandEquipProgress = m_offHandEquipProgress;

    // 获取当前手持物品
    ItemStack mainHandItem = player != nullptr ? getHeldItem(player, Hand::MainHand) : ItemStack();
    ItemStack offHandItem = player != nullptr ? getHeldItem(player, Hand::OffHand) : ItemStack();

    // 检测物品切换
    const bool mainHandChanged = !(m_prevMainHandItem == mainHandItem);
    const bool offHandChanged = !(m_prevOffHandItem == offHandItem);

    // 物品切换时重置装备进度
    if (mainHandChanged) {
        m_mainHandEquipProgress = 0.0f;
        m_prevMainHandItem = mainHandItem;
    }
    if (offHandChanged) {
        m_offHandEquipProgress = 0.0f;
        m_prevOffHandItem = offHandItem;
    }

    // 获取攻击冷却进度
    f32 cooldownStrength = 1.0f;
    if (player != nullptr) {
        cooldownStrength = player->getCooledAttackStrength(1.0f);
    }

    // 装备进度向 1.0 靠拢（使用攻击冷却影响速度）
    // MC 1.16.5: equippedProgress += clamp((!requip ? f^3 : 0) - equippedProgress, -0.4, 0.4)
    const f32 mainTarget = cooldownStrength * cooldownStrength * cooldownStrength; // f^3
    const f32 offTarget = 1.0f;

    m_mainHandEquipProgress += clamp(mainTarget - m_mainHandEquipProgress, -0.4f, 0.4f);
    m_offHandEquipProgress += clamp(offTarget - m_offHandEquipProgress, -0.4f, 0.4f);

    // 限制范围
    m_mainHandEquipProgress = clamp(m_mainHandEquipProgress, 0.0f, 1.0f);
    m_offHandEquipProgress = clamp(m_offHandEquipProgress, 0.0f, 1.0f);

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

    cleanupRetiredItemMeshes();

    if (context.player == nullptr || m_armPipeline == nullptr) {
        return;
    }

    Player* player = context.player;

    // 获取玩家主手设置
    HandSide primaryHand = getPrimaryHand(player);

    // 计算插值后的装备进度
    f32 mainEquipProgress =
        lerp(m_prevMainHandEquipProgress, m_mainHandEquipProgress, static_cast<f32>(context.partialTick));
    f32 offEquipProgress =
        lerp(m_prevOffHandEquipProgress, m_offHandEquipProgress, static_cast<f32>(context.partialTick));

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
    ItemStack mainHandItem = getHeldItem(player, Hand::MainHand);
    ItemStack offHandItem = getHeldItem(player, Hand::OffHand);

    // 确定手臂姿态
    ArmPose mainArmPose = determineArmPose(player, Hand::MainHand);
    ArmPose offArmPose = determineArmPose(player, Hand::OffHand);

    // 更新模型手臂姿态
    m_model.setRightArmPose(primaryHand == HandSide::Right ? mainArmPose : offArmPose);
    m_model.setLeftArmPose(primaryHand == HandSide::Right ? offArmPose : mainArmPose);

    // 检查是否应该渲染主手
    bool renderMainHand = true;
    bool renderOffHand = true;

    // 当使用双手物品时，副手不渲染
    if (isTwoHanded(mainArmPose)) {
        renderOffHand = false;
    }

    // 与原版一致：副手为空时默认不渲染副手模型。
    if (offHandItem.isEmpty()) {
        renderOffHand = false;
    }

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
        const HandSide handSide = resolveHandSide(hand, primaryHand);

        MatrixStack baseStack;
        applyCameraAlignedBasis(baseStack, *player);

        // 对齐 MC 1.16.5：手里为空时渲染手臂，非空时走物品分支（不额外绘制手臂网格）。
        if (heldItem.isEmpty()) {
            MatrixStack armStack = baseStack;
            renderArmFirstPerson(armStack, handSide, equipProgress, swingProgress);

            ensureArmMesh(hand, primaryHand);
            const bool isMainHand = hand == Hand::MainHand;
            const EntityMesh& armMesh = isMainHand ? m_mainHandArmMesh : m_offHandArmMesh;
            const bool armMeshValid = isMainHand ? m_mainHandArmMeshValid : m_offHandArmMeshValid;

            if (armMeshValid && armMesh.indexCount > 0) {
                const auto armModelMatrix = toModelMatrix(armStack.last());
                m_armPipeline->drawMesh(cmd, armMesh, armModelMatrix, cameraPos, 1.0);
            }
            return;
        }

        if (m_itemTextureAtlas == nullptr || !m_itemTextureAtlas->isValid() || m_itemPipeline == nullptr) {
            return;
        }

        ensureItemMesh(hand, heldItem);
        const ItemMeshState& itemMeshState = m_itemMeshes[handIndex(hand)];
        if (!itemMeshState.valid || itemMeshState.mesh.indexCount == 0) {
            return;
        }

        MatrixStack itemStack = baseStack;
        renderItemInHand(itemStack, player, heldItem, handSide, equipProgress, swingProgress);

        m_itemPipeline->bind(cmd);
        vkCmdBindDescriptorSets(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_itemPipeline->pipelineLayout(),
            0,
            1,
            &cameraDescriptorSet,
            0,
            nullptr);
        m_itemPipeline->bindTextureDescriptor(cmd);
        const auto itemModelMatrix = toModelMatrix(itemStack.last());
        m_itemPipeline->drawMesh(cmd, itemMeshState.mesh, itemModelMatrix, cameraPos, 1.0);

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

void FirstPersonRenderer::renderArmFirstPerson(
    MatrixStack& matrixStack, HandSide side, f32 equipProgress, f32 swingProgress)
{
    const f32 sideSign = side == HandSide::Right ? 1.0f : -1.0f;

    // 对齐 MC 1.16.5 FirstPersonRenderer::renderArmFirstPerson。
    const f32 sqrtSwing = std::sqrt(swingProgress);
    const f32 swingSin = std::sin(sqrtSwing * static_cast<f32>(PI));

    const f32 translateX = sideSign * (-0.3f * swingSin + 0.64000005f);
    const f32 translateY = 0.4f * std::sin(sqrtSwing * static_cast<f32>(PI) * 2.0f) - 0.6f + equipProgress * -0.6f;
    const f32 translateZ = -0.4f * std::sin(swingProgress * static_cast<f32>(PI)) - 0.71999997f;
    matrixStack.translate(translateX, translateY, translateZ);

    matrixStack.rotateY(sideSign * 45.0f);

    const f32 swingSquareSin = std::sin(swingProgress * swingProgress * static_cast<f32>(PI));
    matrixStack.rotateY(sideSign * swingSin * 70.0f);
    matrixStack.rotateZ(sideSign * swingSquareSin * -20.0f);

    matrixStack.translate(sideSign * -1.0f, 3.6f, 3.5f);
    matrixStack.rotateZ(sideSign * 120.0f);
    matrixStack.rotateX(200.0f);
    matrixStack.rotateY(sideSign * -135.0f);
    matrixStack.translate(sideSign * 5.6f, 0.0f, 0.0f);
}

void FirstPersonRenderer::renderItemInHand(
    MatrixStack& stack, Player* player, const ItemStack& itemStack, HandSide side, f32 equipProgress, f32 swingProgress)
{
    renderItemInHand(stack, player, itemStack, side, equipProgress, swingProgress, false, 0, static_cast<f32>(PI));
}

void FirstPersonRenderer::renderItemInHand(MatrixStack& stack,
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

    const f32 sideSign = side == HandSide::Right ? 1.0f : -1.0f;
    const bool leftHanded = side == HandSide::Left;

    // 对齐 MC 1.16.5 常规物品分支（非蓄力、非特殊动作）。
    const f32 sqrtSwing = std::sqrt(swingProgress);
    const f32 offsetX = -0.4f * std::sin(sqrtSwing * static_cast<f32>(PI));
    const f32 offsetY = 0.2f * std::sin(sqrtSwing * static_cast<f32>(PI) * 2.0f);
    const f32 offsetZ = -0.2f * std::sin(swingProgress * static_cast<f32>(PI));
    stack.translate(sideSign * offsetX, offsetY, offsetZ);

    transformSideFirstPerson(stack, side, equipProgress);
    transformFirstPerson(stack, side, swingProgress);

    // 应用物品模型变换（从物品 JSON 模型获取自定义变换）
    m_itemInHandRenderer.applyTransform(stack,
        itemStack,
        leftHanded ? TransformType::FirstPersonLeftHand : TransformType::FirstPersonRightHand,
        leftHanded);

    // 根据物品使用动作应用额外变换
    if (isUsingItem && itemStack.getItem() != nullptr) {
        const UseAction useAction = itemStack.getItem()->getUseAction(itemStack);

        switch (useAction) {
            case UseAction::Eat:
            case UseAction::Drink:
                // 食物/药水：进食动画
                transformEatOrDrink(stack, partialTicks, side, itemStack, useCount);
                break;

            case UseAction::Block:
                // 盾牌：格挡动画（已由 transformSideFirstPerson 处理）
                break;

            case UseAction::Bow:
                // 弓：拉弓动画
                transformBow(stack, partialTicks, side, useCount);
                break;

            case UseAction::Spear:
                // 三叉戟：投掷动画（Trident 是 Spear 的别名）
                transformSpear(stack, partialTicks, side, useCount);
                break;

            case UseAction::Crossbow:
                // 弩：装填动画
                transformCrossbow(stack, partialTicks, side, useCount, false);
                break;

            default:
                break;
        }
    }
}

void FirstPersonRenderer::renderMapFirstPerson(
    MatrixStack& stack, const ItemStack& mapStack, f32 pitch, f32 equipProgress, f32 swingProgress)
{
    // TODO: 实现地图渲染
    (void)stack;
    (void)mapStack;
    (void)pitch;
    (void)equipProgress;
    (void)swingProgress;
}

// ============================================================================
// 变换方法
// ============================================================================

void FirstPersonRenderer::transformSideFirstPerson(MatrixStack& stack, HandSide side, f32 equipProgress)
{
    const f32 sideSign = side == HandSide::Right ? 1.0f : -1.0f;

    stack.translate(sideSign * SIDE_OFFSET_X, SIDE_OFFSET_Y + equipProgress * -0.6f, SIDE_OFFSET_Z);
}

void FirstPersonRenderer::transformFirstPerson(MatrixStack& stack, HandSide side, f32 swingProgress)
{
    const f32 sideSign = side == HandSide::Right ? 1.0f : -1.0f;

    const f32 swing = std::sin(swingProgress * swingProgress * static_cast<f32>(PI));
    const f32 swingSqrt = std::sin(std::sqrt(swingProgress) * static_cast<f32>(PI));

    stack.rotateY(sideSign * (45.0f + swing * -20.0f));
    stack.rotateZ(sideSign * swingSqrt * -20.0f);
    stack.rotateX(swingSqrt * -80.0f);
    stack.rotateY(sideSign * -45.0f);
}

void FirstPersonRenderer::transformEatOrDrink(
    MatrixStack& matrixStack, f32 partialTicks, HandSide side, const ItemStack& item, i32 useCount)
{
    const Item* itemPtr = item.getItem();
    if (itemPtr == nullptr) {
        return;
    }

    const i32 useDuration = itemPtr->getUseDuration(item);
    if (useDuration <= 0) {
        return;
    }

    // MC 1.16.5: f = useDuration - useCount + partialTicks + 1
    // useCount 是剩余使用时间
    const f32 f = static_cast<f32>(useDuration - useCount) + partialTicks + 1.0f;
    const f32 f1 = f / static_cast<f32>(useDuration);

    // 在进度小于 80% 时添加轻微抖动
    if (f1 < 0.8f) {
        const f32 f2 = std::abs(std::cos(f / 4.0f * static_cast<f32>(PI)) * 0.1f);
        matrixStack.translate(0.0f, f2, 0.0f);
    }

    // 使用进度快速接近完成（27次方使曲线变陡）
    const f32 f3 = 1.0f - static_cast<f32>(std::pow(static_cast<f64>(f1), 27.0));
    const f32 sideSign = side == HandSide::Right ? 1.0f : -1.0f;

    // 移动物品到嘴边
    matrixStack.translate(sideSign * f3 * 0.6f, f3 * -0.5f, f3 * 0.0f);

    // 旋转物品
    matrixStack.rotateY(sideSign * f3 * 90.0f);
    matrixStack.rotateX(f3 * 10.0f);
    matrixStack.rotateZ(sideSign * f3 * 30.0f);
}

void FirstPersonRenderer::transformBow(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useCount)
{
    // MC 1.16.5: FirstPersonRenderer BOW 分支
    const f32 sideSign = side == HandSide::Right ? 1.0f : -1.0f;

    // 弓的默认使用时间是 72000 ticks
    // f8 = useDuration - useCount + partialTicks + 1
    const f32 f8 = static_cast<f32>(72000 - useCount) + partialTicks + 1.0f;

    // f12 = f8 / 20, 然后用公式 (f12^2 + f12*2) / 3
    f32 f12 = f8 / 20.0f;
    f12 = (f12 * f12 + f12 * 2.0f) / 3.0f;
    if (f12 > 1.0f) {
        f12 = 1.0f;
    }

    // 超过 10% 进度后有轻微震动
    if (f12 > 0.1f) {
        const f32 f15 = std::sin((f8 - 0.1f) * 1.3f);
        const f32 f18 = f12 - 0.1f;
        const f32 f20 = f15 * f18;
        stack.translate(f20 * 0.0f, f20 * 0.004f, f20 * 0.0f);
    }

    // 蓄力时弓向前移动并放大
    stack.translate(f12 * 0.0f, f12 * 0.0f, f12 * 0.04f);
    stack.scale(1.0f, 1.0f, 1.0f + f12 * 0.2f);
    stack.rotateY(-sideSign * 45.0f);
}

void FirstPersonRenderer::transformSpear(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useCount)
{
    // MC 1.16.5: FirstPersonRenderer SPEAR 分支
    const f32 sideSign = side == HandSide::Right ? 1.0f : -1.0f;

    // 三叉戟蓄力需要 10 ticks
    const f32 f7 = static_cast<f32>(72000 - useCount) + partialTicks + 1.0f;
    f32 f11 = f7 / 10.0f;
    if (f11 > 1.0f) {
        f11 = 1.0f;
    }

    // 超过 10% 进度后有轻微震动
    if (f11 > 0.1f) {
        const f32 f14 = std::sin((f7 - 0.1f) * 1.3f);
        const f32 f17 = f11 - 0.1f;
        const f32 f19 = f14 * f17;
        stack.translate(f19 * 0.0f, f19 * 0.004f, f19 * 0.0f);
    }

    // 蓄力时三叉戟向前移动并放大
    stack.translate(0.0f, 0.0f, f11 * 0.2f);
    stack.scale(1.0f, 1.0f, 1.0f + f11 * 0.2f);
    stack.rotateY(-sideSign * 45.0f);
}

void FirstPersonRenderer::transformCrossbow(
    MatrixStack& stack, f32 partialTicks, HandSide side, i32 useCount, bool isCharged)
{
    // MC 1.16.5: FirstPersonRenderer CROSSBOW 分支
    const f32 sideSign = side == HandSide::Right ? 1.0f : -1.0f;

    if (isCharged) {
        // 已装填状态：弩有轻微偏移
        stack.translate(sideSign * -0.641864f, 0.0f, 0.0f);
        stack.rotateY(sideSign * 10.0f);
    } else {
        // 装填中
        // 获取装填时间（考虑快速装填附魔）
        // 简化：假设基础 25 ticks
        const f32 f9 = static_cast<f32>(25 - useCount) + partialTicks + 1.0f;
        f32 f13 = f9 / 25.0f;
        if (f13 > 1.0f) {
            f13 = 1.0f;
        }

        // 超过 10% 进度后有轻微震动
        if (f13 > 0.1f) {
            const f32 f16 = std::sin((f9 - 0.1f) * 1.3f);
            const f32 f3 = f13 - 0.1f;
            const f32 f4 = f16 * f3;
            stack.translate(f4 * 0.0f, f4 * 0.004f, f4 * 0.0f);
        }

        // 装填时弩向前移动并放大
        stack.translate(f13 * 0.0f, f13 * 0.0f, f13 * 0.04f);
        stack.scale(1.0f, 1.0f, 1.0f + f13 * 0.2f);
        stack.rotateY(-sideSign * 45.0f);
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

void FirstPersonRenderer::invalidateArmMeshes()
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

void FirstPersonRenderer::invalidateItemMeshes()
{
    for (auto& itemMeshState : m_itemMeshes) {
        if (itemMeshState.valid) {
            retireItemMesh(itemMeshState.mesh);
            itemMeshState.valid = false;
        }
        itemMeshState.itemId = std::numeric_limits<ItemId>::max();
    }
}

void FirstPersonRenderer::cleanupRetiredItemMeshes()
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

void FirstPersonRenderer::retireItemMesh(EntityMesh& mesh)
{
    if (mesh.vertexBuffer == VK_NULL_HANDLE && mesh.vertexMemory == VK_NULL_HANDLE &&
        mesh.indexBuffer == VK_NULL_HANDLE && mesh.indexMemory == VK_NULL_HANDLE) {
        mesh = {};
        return;
    }

    m_retiredItemMeshes.push_back(RetiredItemMesh{mesh, m_itemMeshRetirementFrames});
    mesh = {};
}

void FirstPersonRenderer::destroyItemMeshes()
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

void FirstPersonRenderer::ensureArmMesh(Hand hand, HandSide primaryHand)
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

    const auto arm = selectArmModel(hand, primaryHand);
    if (arm) {
        arm->generateMesh(m_cachedVertices, m_cachedIndices, 1.0 / 16.0);
    }

    remapToPlayerSkinRegion(m_cachedVertices);

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

void FirstPersonRenderer::ensureItemMesh(Hand hand, const ItemStack& itemStack)
{
    if (!m_itemPipeline || !m_itemTextureAtlas || itemStack.isEmpty()) {
        return;
    }

    ItemMeshState& itemMeshState = m_itemMeshes[handIndex(hand)];

    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return;
    }

    const ItemId itemId = item->itemId();
    if (itemMeshState.valid && itemMeshState.itemId == itemId) {
        return;
    }

    const TextureRegion* region = m_itemTextureAtlas->getItemTexture(itemId);
    if (region == nullptr) {
        const ResourceLocation& itemLocation = item->itemLocation();
        const ResourceLocation itemPath(itemLocation.namespace_(), "item/" + itemLocation.path());
        region = m_itemTextureAtlas->getItemTexture(itemPath);
    }
    if (region == nullptr) {
        const ResourceLocation& itemLocation = item->itemLocation();
        const ResourceLocation fullPath(itemLocation.namespace_(), "textures/item/" + itemLocation.path());
        region = m_itemTextureAtlas->getItemTexture(fullPath);
    }

    if (region == nullptr) {
        return;
    }

    if (itemMeshState.valid) {
        // 物品切换时将旧网格延后回收，避免上一帧仍在引用旧缓冲区。
        retireItemMesh(itemMeshState.mesh);
        itemMeshState.valid = false;
        itemMeshState.itemId = std::numeric_limits<ItemId>::max();
    }

    constexpr f64 halfSize = 0.14f;
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    vertices.reserve(8);
    indices.reserve(12);

    // 正面
    vertices.emplace_back(-halfSize, -halfSize, 0.0f, region->u0, region->v1, 0.0f, 0.0f, 1.0f);
    vertices.emplace_back(-halfSize, halfSize, 0.0f, region->u0, region->v0, 0.0f, 0.0f, 1.0f);
    vertices.emplace_back(halfSize, halfSize, 0.0f, region->u1, region->v0, 0.0f, 0.0f, 1.0f);
    vertices.emplace_back(halfSize, -halfSize, 0.0f, region->u1, region->v1, 0.0f, 0.0f, 1.0f);
    // 背面
    vertices.emplace_back(halfSize, -halfSize, 0.0f, region->u0, region->v1, 0.0f, 0.0f, -1.0f);
    vertices.emplace_back(halfSize, halfSize, 0.0f, region->u0, region->v0, 0.0f, 0.0f, -1.0f);
    vertices.emplace_back(-halfSize, halfSize, 0.0f, region->u1, region->v0, 0.0f, 0.0f, -1.0f);
    vertices.emplace_back(-halfSize, -halfSize, 0.0f, region->u1, region->v1, 0.0f, 0.0f, -1.0f);

    indices = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};

    auto createResult = m_itemPipeline->createMesh(vertices, indices);
    if (createResult.failed()) {
        spdlog::warn("FirstPersonRenderer: Failed to create item mesh for {}", item->itemLocation().toString());
        return;
    }

    itemMeshState.mesh = std::move(createResult.value());
    itemMeshState.valid = true;
    itemMeshState.itemId = itemId;
}

void FirstPersonRenderer::remapToPlayerSkinRegion(std::vector<ModelVertex>& vertices) const
{
    if (vertices.empty() || m_entityTextureAtlas == nullptr || !m_entityTextureAtlas->isBuilt()) {
        return;
    }

    const mc::TextureRegion* region = m_entityTextureAtlas->getRegion(m_playerSkinLocation);
    if (region == nullptr) {
        const std::array<ResourceLocation, 4> fallbacks = {ResourceLocation("minecraft:textures/entity/steve.png"),
            ResourceLocation("minecraft:textures/entity/player/wide/steve.png"),
            ResourceLocation("minecraft:textures/entity/alex.png"),
            ResourceLocation("minecraft:textures/entity/player/slim/alex.png")};

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

f32 FirstPersonRenderer::getSwingProgress(f32 partialTicks, Player* player, Hand hand) const
{
    if (player == nullptr) {
        return 0.0f;
    }

    // 只有正在挥动的手才返回进度
    if (player->isSwingInProgress() && player->swingingHand() == hand) {
        const f32 prevSwing = player->prevSwingProgress();
        const f32 currSwing = player->swingProgress();
        return prevSwing + (currSwing - prevSwing) * partialTicks;
    }

    return 0.0f;
}

ArmPose FirstPersonRenderer::determineArmPose(Player* player, Hand hand) const
{
    if (player == nullptr) {
        return ArmPose::Empty;
    }

    // 获取手持物品
    ItemStack heldItem = getHeldItem(player, hand);

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
                // 望远镜暂不实现特殊姿态
                return ArmPose::Item;

            default:
                break;
        }
    }

    // 检查弩是否已装填
    // MC 1.16.5: 如果物品是弩且已装填，显示 CrossbowHold 姿态
    if (dynamic_cast<const item::CrossbowItem*>(item) != nullptr) {
        if (item::CrossbowItem::isCharged(heldItem)) {
            return ArmPose::CrossbowHold;
        }
    }

    // 检查是否为地图
    // TODO: 实现地图检测

    // 默认持有物品姿态
    return ArmPose::Item;
}

HandSide FirstPersonRenderer::getPrimaryHand(Player* player)
{
    if (player == nullptr) {
        return HandSide::Right;
    }

    // 从 LivingEntity 获取主手设置
    return player->getPrimaryHand();
}

ItemStack FirstPersonRenderer::getHeldItem(Player* player, Hand hand)
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

HandSide FirstPersonRenderer::resolveHandSide(Hand hand, HandSide primaryHand)
{
    if (hand == Hand::MainHand) {
        return primaryHand;
    }
    return primaryHand == HandSide::Right ? HandSide::Left : HandSide::Right;
}

std::shared_ptr<ModelRenderer> FirstPersonRenderer::selectArmModel(Hand hand, HandSide primaryHand) const
{
    const HandSide handSide = resolveHandSide(hand, primaryHand);
    return handSide == HandSide::Right ? m_model.rightArm() : m_model.leftArm();
}

} // namespace mc::client::renderer::trident::firstperson
