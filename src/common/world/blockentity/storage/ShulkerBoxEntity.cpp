#include "world/blockentity/storage/ShulkerBoxEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "item/Items.hpp"
#include "util/property/Properties.hpp"
#include "util/Direction.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/core/Entity.hpp"
#include "util/AxisAlignedBB.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/math/MathConstants.hpp"

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 构建潜影盒打开时的实体阻挡检测区域。
 *
 * 与 Java 版行为一致：检测方块正上方 1 格体积内是否有实体。
 */
[[nodiscard]] AxisAlignedBB makeShulkerOpenSpaceBox(const BlockPos& pos) {
    return AxisAlignedBB(
        static_cast<f32>(pos.x),
        static_cast<f32>(pos.y + 1),
        static_cast<f32>(pos.z),
        static_cast<f32>(pos.x + 1),
        static_cast<f32>(pos.y + 2),
        static_cast<f32>(pos.z + 1));
}

/**
 * @brief 计算潜影盒打开时的碰撞盒。
 *
 * 参考: ShulkerBoxTileEntity.getBoundingBox(Direction)
 */
[[nodiscard]] AxisAlignedBB getShulkerBoundingBox(const BlockPos& pos, Direction facing, f32 progress) {
    // 基础碰撞盒为方块本身
    f32 minX = static_cast<f32>(pos.x);
    f32 minY = static_cast<f32>(pos.y);
    f32 minZ = static_cast<f32>(pos.z);
    f32 maxX = static_cast<f32>(pos.x + 1);
    f32 maxY = static_cast<f32>(pos.y + 1);
    f32 maxZ = static_cast<f32>(pos.z + 1);

    // 根据朝向和进度扩展碰撞盒
    f32 expand = 0.5f * progress;
    minX += static_cast<f32>(Directions::xOffset(facing)) * expand;
    minY += static_cast<f32>(Directions::yOffset(facing)) * expand;
    minZ += static_cast<f32>(Directions::zOffset(facing)) * expand;
    maxX += static_cast<f32>(Directions::xOffset(facing)) * expand;
    maxY += static_cast<f32>(Directions::yOffset(facing)) * expand;
    maxZ += static_cast<f32>(Directions::zOffset(facing)) * expand;

    return AxisAlignedBB(minX, minY, minZ, maxX, maxY, maxZ);
}

/**
 * @brief 计算实体推动区域。
 *
 * 参考: ShulkerBoxTileEntity.getTopBoundingBox(Direction)
 */
[[nodiscard]] AxisAlignedBB getTopBoundingBox(const BlockPos& pos, Direction facing) {
    // 获取相反方向的边界盒
    Direction opposite = Directions::opposite(facing);

    // 基础碰撞盒
    AxisAlignedBB box(
        static_cast<f32>(pos.x),
        static_cast<f32>(pos.y),
        static_cast<f32>(pos.z),
        static_cast<f32>(pos.x + 1),
        static_cast<f32>(pos.y + 1),
        static_cast<f32>(pos.z + 1));

    // 向朝向方向扩展后收缩
    box = box.expand(
        static_cast<f32>(Directions::xOffset(facing)) * 0.5f,
        static_cast<f32>(Directions::yOffset(facing)) * 0.5f,
        static_cast<f32>(Directions::zOffset(facing)) * 0.5f);

    // 收缩朝向反方向的边界（使用 expand 的反向）
    box = box.expand(
        -static_cast<f32>(Directions::xOffset(opposite)),
        -static_cast<f32>(Directions::yOffset(opposite)),
        -static_cast<f32>(Directions::zOffset(opposite)));

    return box;
}

} // namespace

// ========== ShulkerBoxEntity 实现 ==========

ShulkerBoxEntity::ShulkerBoxEntity(const BlockPos& pos)
    : LootableContainerBlockEntity(BlockEntityType::ShulkerBox, pos)
    , m_inventory(SHULKER_BOX_SIZE) {
}

ShulkerBoxEntity::~ShulkerBoxEntity() = default;

f32 ShulkerBoxEntity::getProgress(f32 partialTick) const {
    return m_prevProgress + (m_progress - m_prevProgress) * partialTick;
}

void ShulkerBoxEntity::openContainer(Player* player) {
    // 触发战利品表填充
    fillWithLoot(player);

    // 检查锁定状态
    if (player != nullptr && !LootableContainerBlockEntity::canOpen(player, ItemStack())) {
        return;
    }

    // 基类已处理观察者检查和负数保护
    LootableContainerBlockEntity::openContainer(player);

    if (m_openCount == 1) {
        m_animationStatus = AnimationStatus::Opening;
    }
    LootableContainerBlockEntity::setChanged();
}

void ShulkerBoxEntity::closeContainer(Player* player) {
    // 基类已处理观察者检查
    LootableContainerBlockEntity::closeContainer(player);

    if (m_openCount == 0) {
        m_animationStatus = AnimationStatus::Closing;
    }
    LootableContainerBlockEntity::setChanged();
}

bool ShulkerBoxEntity::canOpen(IWorld& world) const {
    return checkCanOpen(world);
}

void ShulkerBoxEntity::tick(IWorld& world) {
    // 缓存朝向（仅首次或朝向未初始化时）
    if (m_cachedFacing == Direction::None) {
        cacheFacing(world);
    }

    // MC 1.16.5: 先更新动画状态
    updateAnimation(0.0f);

    // MC 1.16.5: 在 Opening 或 Closing 状态时推动实体
    if (m_animationStatus == AnimationStatus::Opening ||
        m_animationStatus == AnimationStatus::Closing) {
        moveCollidedEntities(world, m_cachedFacing);
    }
}

void ShulkerBoxEntity::updateAnimation(f32 partialTick) {
    MC_UNUSED(partialTick);

    m_prevProgress = m_progress;

    switch (m_animationStatus) {
        case AnimationStatus::Opening:
            m_progress += 0.1f;
            if (m_progress >= 1.0f) {
                m_progress = 1.0f;
                m_animationStatus = AnimationStatus::Opened;
            }
            break;

        case AnimationStatus::Closing:
            m_progress -= 0.1f;
            if (m_progress <= 0.0f) {
                m_progress = 0.0f;
                m_animationStatus = AnimationStatus::Closed;
            }
            break;

        case AnimationStatus::Opened:
            m_progress = 1.0f;
            break;

        case AnimationStatus::Closed:
        default:
            m_progress = 0.0f;
            break;
    }
}

void ShulkerBoxEntity::moveCollidedEntities(IWorld& world, Direction facing) {
    if (facing == Direction::None) {
        return;
    }

    // 获取推动区域
    AxisAlignedBB pushBox = getTopBoundingBox(m_pos, facing);
    std::vector<Entity*> entities = world.getEntitiesInAABB(pushBox, nullptr);

    if (entities.empty()) {
        return;
    }

    // 计算推动方向和距离
    for (Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }

        const AxisAlignedBB entityBox = entity->boundingBox();

        // 计算推动距离
        f32 dx = 0.0f;
        f32 dy = 0.0f;
        f32 dz = 0.0f;

        switch (Directions::getAxis(facing)) {
            case Axis::X: {
                if (Directions::getAxisDirection(facing) == AxisDirection::Positive) {
                    dx = pushBox.maxX - entityBox.minX;
                } else {
                    dx = entityBox.maxX - pushBox.minX;
                }
                dx += 0.01f;
                break;
            }
            case Axis::Y: {
                if (Directions::getAxisDirection(facing) == AxisDirection::Positive) {
                    dy = pushBox.maxY - entityBox.minY;
                } else {
                    dy = entityBox.maxY - pushBox.minY;
                }
                dy += 0.01f;
                break;
            }
            case Axis::Z: {
                if (Directions::getAxisDirection(facing) == AxisDirection::Positive) {
                    dz = pushBox.maxZ - entityBox.minZ;
                } else {
                    dz = entityBox.maxZ - pushBox.minZ;
                }
                dz += 0.01f;
                break;
            }
        }

        // 推动实体
        entity->move(
            dx * static_cast<f32>(Directions::xOffset(facing)),
            dy * static_cast<f32>(Directions::yOffset(facing)),
            dz * static_cast<f32>(Directions::zOffset(facing))
        );
    }
}

void ShulkerBoxEntity::cacheFacing(IWorld& world) {
    const BlockState* state = world.getBlockState(m_pos);
    if (state == nullptr) {
        return;
    }

    if (state->hasProperty(BlockStateProperties::FACING())) {
        m_cachedFacing = state->get(BlockStateProperties::FACING());
    }
}

bool ShulkerBoxEntity::checkCanOpen(IWorld& world) const {
    const AxisAlignedBB openSpace = makeShulkerOpenSpaceBox(m_pos);
    const std::vector<Entity*> collidingEntities = world.getEntitiesInAABB(openSpace, nullptr);
    return collidingEntities.empty();
}

void ShulkerBoxEntity::fillWithLoot(Player* player) {
    // 需要世界引用来获取服务器
    if (m_world == nullptr) {
        return;
    }

    // 获取战利品表管理器（需要从服务器获取）
    // 注：目前简化实现，待服务器基础设施完善后改进
    // MC 1.16.5: this.world.getServer().getLootTableManager()
    MC_UNUSED(player);

    // 如果没有战利品表，不执行
    if (!hasLootTable()) {
        return;
    }

    // TODO: 需要从服务器获取 LootTableManager
    // 目前使用 fillWithLootFromTable 作为占位符
    // 当 LootTableManager 可用时，应该调用：
    // fillWithLootFromTable(lootTableManager, player);
}

bool ShulkerBoxEntity::load(const nlohmann::json& data) {
    if (!LootableContainerBlockEntity::load(data)) {
        return false;
    }

    // 加载物品
    if (data.contains("items")) {
        m_inventory.load(data["items"]);
    }

    return true;
}

void ShulkerBoxEntity::save(nlohmann::json& data) const {
    LootableContainerBlockEntity::save(data);

    // 保存物品
    nlohmann::json itemsJson;
    m_inventory.save(itemsJson);
    data["items"] = itemsJson;
}

std::unique_ptr<BlockEntity> ShulkerBoxEntity::clone() const {
    auto clone = std::make_unique<ShulkerBoxEntity>(m_pos);
    clone->m_animationStatus = m_animationStatus;
    clone->m_progress = m_progress;
    clone->m_prevProgress = m_prevProgress;
    clone->m_openCount = m_openCount;
    for (i32 slot = 0; slot < SHULKER_BOX_SIZE; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            clone->m_inventory.setItem(slot, stack.copy());
        }
    }
    return clone;
}

// ========== ISidedInventory 接口实现 ==========

std::vector<i32> ShulkerBoxEntity::getSlotsForFace(Direction side) const {
    MC_UNUSED(side);
    // MC 1.16.5: 潜影盒可以从任意方向访问所有槽位
    // SLOTS = IntStream.range(0, 27).toArray()
    // 使用静态常量避免每次调用都分配 vector
    static const std::vector<i32> allSlots = []() {
        std::vector<i32> slots;
        slots.reserve(SHULKER_BOX_SIZE);
        for (i32 i = 0; i < SHULKER_BOX_SIZE; ++i) {
            slots.push_back(i);
        }
        return slots;
    }();
    return allSlots;
}

bool ShulkerBoxEntity::canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const {
    MC_UNUSED(slot);
    MC_UNUSED(direction);

    // MC 1.16.5: 潜影盒不能插入另一个潜影盒（防止递归）
    // return !(Block.getBlockFromItem(itemStackIn.getItem()) instanceof ShulkerBoxBlock);
    if (stack.isEmpty()) {
        return false;
    }

    // TODO: 当 ShulkerBoxBlock 实现后，检查物品是否为潜影盒方块
    // const Item* item = stack.getItem();
    // if (item != nullptr) {
    //     const Block* block = item->getBlock();
    //     if (block != nullptr && dynamic_cast<const ShulkerBoxBlock*>(block) != nullptr) {
    //         return false;
    //     }
    // }

    return true;
}

bool ShulkerBoxEntity::canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const {
    MC_UNUSED(slot);
    MC_UNUSED(stack);
    MC_UNUSED(direction);
    // MC 1.16.5: 潜影盒可以从任意方向提取任意物品
    return true;
}

} // namespace blockentity
} // namespace mc
