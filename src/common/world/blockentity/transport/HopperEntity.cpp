#include "world/blockentity/transport/HopperEntity.hpp"
#include "entity/entities/item/ItemEntity.hpp"
#include "entity/inventory/ISidedInventory.hpp"
#include "entity/inventory/ISidedInventoryProvider.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/blocks/HopperBlock.hpp"
#include "world/blockentity/transport/IHopper.hpp"
#include <algorithm>

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 判断漏斗方块状态是否处于启用状态。
 *
 * @param world 世界接口。
 * @param pos 漏斗方块位置。
 * @return `true` 表示漏斗可工作；`false` 表示被红石禁用。
 *
 * @note 如果当前位置不存在方块状态，或状态不包含 `ENABLED` 属性，则按“启用”处理，
 *       以保持和现有兼容逻辑一致，避免在非漏斗状态下误禁用传输。
 */
[[nodiscard]] bool isHopperEnabledAt(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return true;
    }

    if (!state->hasProperty(BlockStateProperties::ENABLED())) {
        return true;
    }

    return state->get(BlockStateProperties::ENABLED());
}

} // namespace

// ========== 构造函数 ==========

HopperEntity::HopperEntity(const BlockPos& pos)
    : LockableBlockEntity(BlockEntityType::Hopper, pos)
    , m_inventory(HOPPER_SIZE, [this]() { this->setChanged(); })
    , m_transferCooldown(-1)
{}

// ========== BlockEntity 接口 ==========

void HopperEntity::tick(IWorld& world)
{
    // 只在第一次 tick 时设置 world 指针
    if (m_world == nullptr) {
        m_world = &world;
    }

    // 客户端不执行传输逻辑
    // 注意：IWorld 没有 isClientSide() 方法，这里假设服务端执行
    // 实际使用时需要检查 world.isClientSide()

    // 减少冷却
    if (m_transferCooldown > 0) {
        --m_transferCooldown;
    }

    // 记录游戏时间
    m_tickedGameTime = world.currentTick();

    // 如果正在冷却中，跳过
    if (isOnTransferCooldown()) {
        return;
    }

    if (!isHopperEnabledAt(world, getPos())) {
        return;
    }

    // 重置冷却为0（允许传输）
    setTransferCooldown(0);

    // 更新漏斗状态
    updateHopper([&]() { return pullItems(*this); });
}

std::unique_ptr<BlockEntity> HopperEntity::clone() const
{
    auto cloned = std::make_unique<HopperEntity>(m_pos);
    cloned->m_transferCooldown = m_transferCooldown;
    cloned->m_tickedGameTime = m_tickedGameTime;
    for (i32 slot = 0; slot < HOPPER_SIZE; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            cloned->m_inventory.setItem(slot, stack.copy());
        }
    }
    return cloned;
}

// ========== 序列化 ==========

bool HopperEntity::load(const nlohmann::json& data)
{
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    // 加载传输冷却
    if (data.contains("TransferCooldown") && data["TransferCooldown"].is_number()) {
        m_transferCooldown = data["TransferCooldown"].get<i32>();
    }

    // 加载背包内容
    if (data.contains("Items") && data["Items"].is_array()) {
        m_inventory.clear();
        const auto& items = data["Items"];
        for (const auto& itemJson : items) {
            if (!itemJson.is_object()) {
                continue;
            }

            const i32 slot = itemJson.value("Slot", -1);
            if (slot < 0 || slot >= HOPPER_SIZE) {
                continue;
            }

            auto stackResult = ItemStack::fromJson(itemJson);
            if (!stackResult.success()) {
                continue;
            }

            m_inventory.setItem(slot, stackResult.value());
        }
    }

    return true;
}

void HopperEntity::save(nlohmann::json& data) const
{
    LockableBlockEntity::save(data);

    // 保存传输冷却
    data["TransferCooldown"] = m_transferCooldown;

    // 保存背包内容
    nlohmann::json itemsJson = nlohmann::json::array();
    for (i32 slot = 0; slot < HOPPER_SIZE; ++slot) {
        const ItemStack& stack = m_inventory.getItem(slot);
        if (stack.isEmpty()) {
            continue;
        }

        nlohmann::json itemJson = stack.toJson();
        itemJson["Slot"] = slot;
        itemsJson.push_back(std::move(itemJson));
    }
    data["Items"] = std::move(itemsJson);
}

// ========== 漏斗特定方法 ==========

bool HopperEntity::isFull() const
{
    return isInventoryFull(&m_inventory, Direction::None);
}

void HopperEntity::setTransferCooldown(i32 cooldown)
{
    m_transferCooldown = cooldown;
}

Direction HopperEntity::getOutputDirection() const
{
    // 从方块状态获取输出方向
    // 漏斗的 FACING 属性表示输出方向（不能向上）
    if (m_world != nullptr) {
        const BlockState* state = m_world->getBlockState(getPos());
        if (state != nullptr && state->hasProperty(BlockStateProperties::FACING_EXCEPT_UP())) {
            return state->get(BlockStateProperties::FACING_EXCEPT_UP());
        }
    }
    // 默认向下
    return Direction::Down;
}

// ========== 静态工具方法 ==========

bool HopperEntity::pullItems(IHopper& hopper)
{
    // 尝试从上方容器拉取物品
    IInventory* sourceInventory = getSourceInventory(hopper);

    if (sourceInventory != nullptr) {
        // 从容器拉取
        Direction direction = Direction::Down;

        // 检查源容器是否为空
        if (isInventoryEmpty(sourceInventory, direction)) {
            return false;
        }

        // 检查是否为 ISidedInventory
        ISidedInventory* sidedInventory = dynamic_cast<ISidedInventory*>(sourceInventory);
        if (sidedInventory != nullptr) {
            // 使用 ISidedInventory 的槽位访问
            const std::vector<i32> slots = sidedInventory->getSlotsForFace(direction);
            for (i32 slot : slots) {
                const ItemStack& stack = sourceInventory->getItem(slot);
                if (!stack.isEmpty()) {
                    if (pullItemFromSlot(hopper, sourceInventory, slot, direction)) {
                        return true;
                    }
                }
            }
            return false;
        }

        // 非 ISidedInventory：遍历所有槽位
        for (i32 slot = 0; slot < sourceInventory->getContainerSize(); ++slot) {
            const ItemStack& stack = sourceInventory->getItem(slot);
            if (!stack.isEmpty()) {
                if (pullItemFromSlot(hopper, sourceInventory, slot, direction)) {
                    return true;
                }
            }
        }
        return false;
    }

    // 尝试从物品实体拉取
    std::vector<ItemEntity*> items = getCaptureItems(hopper);
    for (ItemEntity* item : items) {
        // 获取漏斗的背包
        IInventory* hopperInventory = nullptr;
        // 通过 dynamic_cast 获取 IInventory 接口
        hopperInventory = dynamic_cast<IInventory*>(&hopper);
        if (captureItem(hopperInventory, item)) {
            return true;
        }
    }

    return false;
}

bool HopperEntity::captureItem(IInventory* inventory, ItemEntity* itemEntity)
{
    if (inventory == nullptr || itemEntity == nullptr) {
        return false;
    }

    ItemStack stack = itemEntity->getItemStack().copy();
    ItemStack remaining = putStackInInventoryAllSlots(nullptr, inventory, stack, Direction::None);

    if (remaining.isEmpty()) {
        // 物品完全被捕获，移除实体
        itemEntity->remove();
        return true;
    } else {
        // 部分物品被捕获，更新实体物品数量
        itemEntity->setItemStack(remaining);
        return false; // MC 1.16.5: 部分捕获时返回 false
    }
}

IInventory* HopperEntity::getInventoryAtPosition(IWorld* world, const BlockPos& pos)
{
    if (world == nullptr) {
        return nullptr;
    }

    // 获取方块状态
    const BlockState* blockState = world->getBlockState(pos);

    // MC 1.16.5: 首先检查方块是否实现 ISidedInventoryProvider
    // 例如堆肥桶（ComposterBlock）实现此接口
    if (blockState != nullptr) {
        // 注意：getBlock() 返回 const Block&，但 createInventory 需要非 const
        // MC 1.16.5 中 block 本身是 final 的，不会修改 block 状态
        Block& block = const_cast<Block&>(blockState->getBlock());
        ISidedInventoryProvider* provider = dynamic_cast<ISidedInventoryProvider*>(&block);
        if (provider != nullptr) {
            std::unique_ptr<ISidedInventory> inventory = provider->createInventory(*blockState, *world, pos);
            if (inventory != nullptr) {
                // 注意：这里返回原始指针，调用者需要注意生命周期
                // 当前实现假设 inventory 的生命周期由调用者管理
                return inventory.release();
            }
        }
    }

    // 尝试获取方块实体
    BlockEntity* blockEntity = world->getBlockEntity(pos);
    if (blockEntity != nullptr) {
        IInventory* inventory = dynamic_cast<IInventory*>(blockEntity);
        if (inventory != nullptr) {
            return inventory;
        }
    }

    // 兼容当前架构：若位置本身没有方块实体容器，则再尝试实体容器（如矿车容器）。
    const AxisAlignedBB lookupBox(static_cast<f32>(pos.x),
        static_cast<f32>(pos.y),
        static_cast<f32>(pos.z),
        static_cast<f32>(pos.x + 1),
        static_cast<f32>(pos.y + 1),
        static_cast<f32>(pos.z + 1));

    const std::vector<Entity*> entities = world->getEntitiesInAABB(lookupBox, nullptr);
    for (Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }

        IInventory* inventory = dynamic_cast<IInventory*>(entity);
        if (inventory != nullptr) {
            return inventory;
        }
    }

    return nullptr;
}

IInventory* HopperEntity::getSourceInventory(IHopper& hopper)
{
    // 获取漏斗上方一格的位置
    BlockPos pos = hopper.getHopperPos().up();
    return getInventoryAtPosition(hopper.getWorld(), pos);
}

std::vector<ItemEntity*> HopperEntity::getCaptureItems(IHopper& hopper)
{
    std::vector<ItemEntity*> result;

    IWorld* world = hopper.getWorld();
    if (world == nullptr) {
        return result;
    }

    // 获取收集区域
    AxisAlignedBB collectionArea = IHopper::getCollectionArea(hopper);

    // 获取区域内的所有实体
    std::vector<Entity*> entities = world->getEntitiesInAABB(collectionArea, nullptr);

    // 过滤出物品实体
    for (Entity* entity : entities) {
        if (entity != nullptr) {
            // 检查是否为 ItemEntity
            // 使用 dynamic_cast 安全检查
            ItemEntity* itemEntity = dynamic_cast<ItemEntity*>(entity);
            if (itemEntity != nullptr && itemEntity->canBePickedUp()) {
                result.push_back(itemEntity);
            }
        }
    }

    return result;
}

ItemStack HopperEntity::putStackInInventoryAllSlots(
    IInventory* source, IInventory* destination, const ItemStack& stack, Direction direction)
{

    if (destination == nullptr || stack.isEmpty()) {
        return stack;
    }

    ItemStack remaining = stack;

    // 检查是否为 ISidedInventory
    ISidedInventory* sidedInventory = dynamic_cast<ISidedInventory*>(destination);
    if (sidedInventory != nullptr && direction != Direction::None) {
        // 使用 ISidedInventory 的槽位访问
        const std::vector<i32> slots = sidedInventory->getSlotsForFace(direction);
        for (i32 slot : slots) {
            if (remaining.isEmpty()) {
                break;
            }
            remaining = insertStack(source, destination, remaining, slot, direction);
        }
    } else {
        // 非 ISidedInventory：遍历所有槽位
        for (i32 slot = 0; slot < destination->getContainerSize() && !remaining.isEmpty(); ++slot) {
            remaining = insertStack(source, destination, remaining, slot, direction);
        }
    }

    return remaining;
}

// ========== 私有方法 ==========

bool HopperEntity::updateHopper(std::function<bool()> pullFunc)
{
    if (m_world == nullptr) {
        return false;
    }

    // 检查是否正在冷却
    if (isOnTransferCooldown()) {
        return false;
    }

    if (!isHopperEnabledAt(*m_world, getPos())) {
        return false;
    }

    bool transferred = false;

    // 首先尝试输出物品（优先级高于拉取）
    if (!isEmpty()) {
        transferred = transferItemsOut();
    }

    // 然后尝试拉取物品
    if (!isFull()) {
        transferred |= pullFunc();
    }

    if (transferred) {
        setTransferCooldown(TRANSFER_COOLDOWN);
        setChanged();
        return true;
    }

    return false;
}

bool HopperEntity::transferItemsOut()
{
    // 获取输出目标容器
    IInventory* targetInventory = getInventoryForHopperTransfer();
    if (targetInventory == nullptr) {
        return false;
    }

    // 获取输出方向（漏斗朝向的反方向）
    Direction outputDir = getOutputDirection();
    Direction insertDir = Directions::opposite(outputDir);

    // 检查目标容器是否已满
    if (isInventoryFull(targetInventory, insertDir)) {
        return false;
    }

    // 遍历漏斗槽位，尝试输出物品
    for (i32 slot = 0; slot < HOPPER_SIZE; ++slot) {
        const ItemStack& stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            // 复制一份用于尝试插入
            ItemStack stackCopy = stack.copy();

            // 从漏斗移除1个物品
            ItemStack extracted = m_inventory.removeItem(slot, 1);

            // 尝试插入目标容器
            ItemStack remaining = putStackInInventoryAllSlots(&m_inventory, targetInventory, extracted, insertDir);

            if (remaining.isEmpty()) {
                // 成功输出
                targetInventory->setChanged();
                return true;
            }

            // 输出失败，恢复物品
            m_inventory.setItem(slot, stackCopy);
        }
    }

    return false;
}

IInventory* HopperEntity::getInventoryForHopperTransfer()
{
    if (m_world == nullptr) {
        return nullptr;
    }

    // 获取漏斗输出方向对应的方块位置
    Direction outputDir = getOutputDirection();
    BlockPos targetPos = getPos().offset(outputDir);

    return getInventoryAtPosition(m_world, targetPos);
}

bool HopperEntity::isInventoryFull(const IInventory* inventory, Direction side)
{
    if (inventory == nullptr) {
        return true;
    }

    // 检查是否为 ISidedInventory
    const ISidedInventory* sidedInventory = dynamic_cast<const ISidedInventory*>(inventory);
    if (sidedInventory != nullptr) {
        // 使用 ISidedInventory 的槽位访问
        const std::vector<i32> slots = sidedInventory->getSlotsForFace(side);
        for (i32 slot : slots) {
            const ItemStack& stack = inventory->getItem(slot);
            if (stack.isEmpty() || stack.getCount() < stack.getMaxStackSize()) {
                return false;
            }
        }
        return true;
    }

    // 非 ISidedInventory：检查所有槽位
    for (i32 slot = 0; slot < inventory->getContainerSize(); ++slot) {
        const ItemStack& stack = inventory->getItem(slot);
        if (stack.isEmpty() || stack.getCount() < stack.getMaxStackSize()) {
            return false;
        }
    }

    return true;
}

bool HopperEntity::isInventoryEmpty(const IInventory* inventory, Direction side)
{
    if (inventory == nullptr) {
        return true;
    }

    // 检查是否为 ISidedInventory
    const ISidedInventory* sidedInventory = dynamic_cast<const ISidedInventory*>(inventory);
    if (sidedInventory != nullptr) {
        // 使用 ISidedInventory 的槽位访问
        const std::vector<i32> slots = sidedInventory->getSlotsForFace(side);
        for (i32 slot : slots) {
            if (!inventory->getItem(slot).isEmpty()) {
                return false;
            }
        }
        return true;
    }

    // 非 ISidedInventory：检查所有槽位
    for (i32 slot = 0; slot < inventory->getContainerSize(); ++slot) {
        if (!inventory->getItem(slot).isEmpty()) {
            return false;
        }
    }

    return true;
}

bool HopperEntity::pullItemFromSlot(IHopper& hopper, IInventory* inventory, i32 slotIndex, Direction direction)
{

    if (inventory == nullptr) {
        return false;
    }

    const ItemStack& stack = inventory->getItem(slotIndex);
    if (stack.isEmpty()) {
        return false;
    }

    // 检查是否可以从该槽位提取
    if (!canExtractItemFromSlot(inventory, stack, slotIndex, direction)) {
        return false;
    }

    // 复制一份用于尝试插入
    ItemStack stackCopy = stack.copy();

    // 从源容器移除1个物品
    ItemStack extracted = inventory->removeItem(slotIndex, 1);

    // 尝试插入漏斗
    IInventory* hopperInventory = dynamic_cast<IInventory*>(&hopper);
    if (hopperInventory == nullptr) {
        // 恢复物品
        inventory->setItem(slotIndex, stackCopy);
        return false;
    }

    ItemStack remaining = putStackInInventoryAllSlots(inventory, hopperInventory, extracted, Direction::None);

    if (remaining.isEmpty()) {
        // 成功拉取
        inventory->setChanged();
        return true;
    }

    // 拉取失败，恢复物品
    inventory->setItem(slotIndex, stackCopy);
    return false;
}

ItemStack HopperEntity::insertStack(
    IInventory* source, IInventory* destination, const ItemStack& stack, i32 slotIndex, Direction direction)
{

    if (destination == nullptr || stack.isEmpty()) {
        return stack;
    }

    const ItemStack& existingStack = destination->getItem(slotIndex);

    // 检查是否可以插入该槽位
    if (!canInsertItemInSlot(destination, stack, slotIndex, direction)) {
        return stack;
    }

    bool inserted = false;
    ItemStack remaining = stack; // 初始化为原始物品
    bool wasEmpty = destination->isEmpty();

    if (existingStack.isEmpty()) {
        // 空槽位，直接放入
        destination->setItem(slotIndex, stack);
        remaining = ItemStack::EMPTY; // 全部插入，无剩余
        inserted = true;
    } else if (canCombine(existingStack, stack)) {
        // 可合并的物品，尝试堆叠
        i32 availableSpace = existingStack.getMaxStackSize() - existingStack.getCount();
        i32 toInsert = std::min(stack.getCount(), availableSpace);

        if (toInsert > 0) {
            // 创建合并后的物品堆
            ItemStack merged = existingStack.copy();
            merged.grow(toInsert);
            destination->setItem(slotIndex, merged);

            // 计算剩余物品
            remaining = stack.copy();
            remaining.shrink(toInsert);
            inserted = true;
        }
    }

    if (inserted) {
        // MC 1.16.5 漏斗链优化：如果目标是漏斗且为空
        // 减少冷却时间，但需要检查游戏时间
        if (wasEmpty && destination != nullptr) {
            HopperEntity* targetHopper = dynamic_cast<HopperEntity*>(destination);
            if (targetHopper != nullptr && !targetHopper->mayTransfer()) {
                i32 cooldownReduction = 0;
                // 检查源是否也是漏斗
                if (source != nullptr) {
                    HopperEntity* sourceHopper = dynamic_cast<HopperEntity*>(source);
                    if (sourceHopper != nullptr) {
                        // 只有当目标漏斗的游戏时间 >= 源漏斗的游戏时间时才减少冷却
                        if (targetHopper->m_tickedGameTime >= sourceHopper->m_tickedGameTime) {
                            cooldownReduction = 1;
                        }
                    }
                }
                targetHopper->setTransferCooldown(TRANSFER_COOLDOWN - cooldownReduction);
            }
        }

        destination->setChanged();
    }

    return remaining;
}

bool HopperEntity::canInsertItemInSlot(
    const IInventory* inventory, const ItemStack& stack, i32 slotIndex, Direction direction)
{

    if (inventory == nullptr) {
        return false;
    }

    // 检查槽位是否接受该物品
    if (!inventory->canPlaceItem(slotIndex, stack)) {
        return false;
    }

    // 检查是否为 ISidedInventory
    const ISidedInventory* sidedInventory = dynamic_cast<const ISidedInventory*>(inventory);
    if (sidedInventory != nullptr && direction != Direction::None) {
        return sidedInventory->canInsertItem(slotIndex, stack, direction);
    }

    return true;
}

bool HopperEntity::canExtractItemFromSlot(
    const IInventory* inventory, const ItemStack& stack, i32 slotIndex, Direction direction)
{

    if (inventory == nullptr) {
        return false;
    }

    MC_UNUSED(stack);
    MC_UNUSED(slotIndex);

    // 检查是否为 ISidedInventory
    const ISidedInventory* sidedInventory = dynamic_cast<const ISidedInventory*>(inventory);
    if (sidedInventory != nullptr) {
        return sidedInventory->canExtractItem(slotIndex, stack, direction);
    }

    return true;
}

bool HopperEntity::canCombine(const ItemStack& stack1, const ItemStack& stack2)
{
    return stack1.canMergeWith(stack2);
}

void HopperEntity::onEntityCollision(IWorld& world, Entity* entity)
{
    if (entity == nullptr) {
        return;
    }

    // 检查是否为物品实体
    ItemEntity* itemEntity = dynamic_cast<ItemEntity*>(entity);
    if (itemEntity == nullptr) {
        return;
    }

    // 检查物品是否在收集区域内
    AxisAlignedBB collectionArea = IHopper::getCollectionArea(*this);
    if (!collectionArea.contains(entity->position())) {
        return;
    }

    // 尝试捕获物品
    updateHopper([&]() { return captureItem(&m_inventory, itemEntity); });
}

} // namespace blockentity
} // namespace mc
