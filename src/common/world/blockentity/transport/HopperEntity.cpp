#include "world/blockentity/transport/HopperEntity.hpp"
#include "world/blockentity/transport/IHopper.hpp"
#include "world/World.hpp"
#include "entity/ItemEntity.hpp"
#include "world/block/BlockState.hpp"
#include "world/block/blocks/HopperBlock.hpp"
#include "util/assert/AssertAll.hpp"
#include <algorithm>

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

HopperEntity::HopperEntity(const BlockPos& pos)
    : LockableBlockEntity(BlockEntityType::Hopper, pos)
    , m_inventory(HOPPER_SIZE, [this]() { this->setChanged(); })
    , m_transferCooldown(-1) {
}

// ========== BlockEntity 接口 ==========

void HopperEntity::tick(World& world) {
    m_world = &world;

    // 客户端不执行传输逻辑
    // 注意：IWorld 没有 isRemote() 方法，这里假设服务端执行
    // 实际使用时需要检查 world.isRemote()

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

    // 重置冷却为0（允许传输）
    setTransferCooldown(0);

    // 检查漏斗是否被红石禁用
    // 这需要从方块状态获取 ENABLED 属性
    // 暂时假设漏斗总是启用的
    // TODO: 从 world 获取 BlockState 检查 ENABLED 属性

    // 更新漏斗状态
    updateHopper([&]() {
        return pullItems(*this);
    });
}

std::unique_ptr<BlockEntity> HopperEntity::clone() const {
    auto cloned = std::make_unique<HopperEntity>(m_pos);
    // TODO: 复制物品数据
    return cloned;
}

// ========== 序列化 ==========

bool HopperEntity::load(const nlohmann::json& data) {
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    // 加载传输冷却
    if (data.contains("TransferCooldown") && data["TransferCooldown"].is_number()) {
        m_transferCooldown = data["TransferCooldown"].get<i32>();
    }

    // 加载背包内容
    // TODO: 从 JSON 加载 ItemStack

    return true;
}

void HopperEntity::save(nlohmann::json& data) const {
    LockableBlockEntity::save(data);

    // 保存传输冷却
    data["TransferCooldown"] = m_transferCooldown;

    // 保存背包内容
    // TODO: 保存 ItemStack 到 JSON
}

// ========== 漏斗特定方法 ==========

bool HopperEntity::isFull() const {
    for (i32 i = 0; i < HOPPER_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (stack.isEmpty() || stack.getCount() < stack.getMaxStackSize()) {
            return false;
        }
    }
    return true;
}

void HopperEntity::setTransferCooldown(i32 cooldown) {
    m_transferCooldown = cooldown;
}

// ========== 静态工具方法 ==========

bool HopperEntity::pullItems(IHopper& hopper) {
    // 尝试从上方容器拉取物品
    IInventory* sourceInventory = getSourceInventory(hopper);

    if (sourceInventory != nullptr) {
        // 从容器拉取
        Direction direction = Direction::Down;

        // 检查源容器是否为空
        if (isInventoryEmpty(sourceInventory, direction)) {
            return false;
        }

        // 遍历源容器的槽位，尝试拉取物品
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
        if (captureItem(hopper.getWorld() ? hopper.getWorld()->getInventory() : nullptr, item)) {
            // 注意：hopper 本身是一个 IInventory，但我们需要传入正确的背包
            // 这里需要更复杂的处理
            return true;
        }
    }

    return false;
}

bool HopperEntity::captureItem(IInventory* inventory, ItemEntity* itemEntity) {
    if (inventory == nullptr || itemEntity == nullptr) {
        return false;
    }

    ItemStack stack = itemEntity->getItemStack().copy();
    ItemStack remaining = putStackInInventoryAllSlots(nullptr, inventory, stack, Direction::None);

    if (remaining.isEmpty()) {
        // 物品完全被捕获，移除实体
        // itemEntity->remove();  // 需要调用实体的移除方法
        return true;
    } else {
        // 部分物品被捕获，更新实体物品数量
        itemEntity->setItemStack(remaining);
        return false;
    }
}

IInventory* HopperEntity::getInventoryAtPosition(IWorld* world, const BlockPos& pos) {
    if (world == nullptr) {
        return nullptr;
    }

    // 尝试获取方块实体
    BlockEntity* blockEntity = world->getBlockEntity(pos);
    if (blockEntity != nullptr) {
        IInventory* inventory = dynamic_cast<IInventory*>(blockEntity);
        if (inventory != nullptr) {
            return inventory;
        }
    }

    // TODO: 检查方块是否实现 ISidedInventoryProvider
    // TODO: 检查该位置的实体是否有背包（如漏斗矿车）

    return nullptr;
}

IInventory* HopperEntity::getSourceInventory(IHopper& hopper) {
    // 获取漏斗上方一格的位置
    BlockPos pos = hopper.getHopperPos().up();
    return getInventoryAtPosition(hopper.getWorld(), pos);
}

std::vector<ItemEntity*> HopperEntity::getCaptureItems(IHopper& hopper) {
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
    IInventory* source,
    IInventory* destination,
    const ItemStack& stack,
    Direction direction) {

    if (destination == nullptr || stack.isEmpty()) {
        return stack;
    }

    ItemStack remaining = stack;

    // 遍历目标容器的所有槽位，尝试插入
    for (i32 slot = 0; slot < destination->getContainerSize() && !remaining.isEmpty(); ++slot) {
        remaining = insertStack(source, destination, remaining, slot, direction);
    }

    return remaining;
}

// ========== 私有方法 ==========

bool HopperEntity::updateHopper(std::function<bool()> pullFunc) {
    if (m_world == nullptr) {
        return false;
    }

    // 检查是否正在冷却
    if (isOnTransferCooldown()) {
        return false;
    }

    // TODO: 检查漏斗是否被红石禁用
    // 需要从 BlockState 获取 ENABLED 属性

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

bool HopperEntity::transferItemsOut() {
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
            ItemStack remaining = putStackInInventoryAllSlots(
                this, targetInventory, extracted, insertDir);

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

IInventory* HopperEntity::getInventoryForHopperTransfer() {
    if (m_world == nullptr) {
        return nullptr;
    }

    // 获取漏斗输出方向对应的方块位置
    Direction outputDir = getOutputDirection();
    BlockPos targetPos = getPos().offset(outputDir);

    return getInventoryAtPosition(m_world, targetPos);
}

bool HopperEntity::isInventoryFull(const IInventory* inventory, Direction side) {
    if (inventory == nullptr) {
        return true;
    }

    // TODO: 处理 ISidedInventory（侧面有不同槽位的容器）
    // 目前假设所有槽位都可访问

    for (i32 slot = 0; slot < inventory->getContainerSize(); ++slot) {
        const ItemStack& stack = inventory->getItem(slot);
        if (stack.isEmpty() || stack.getCount() < stack.getMaxStackSize()) {
            return false;
        }
    }

    return true;
}

bool HopperEntity::isInventoryEmpty(const IInventory* inventory, Direction side) {
    if (inventory == nullptr) {
        return true;
    }

    // TODO: 处理 ISidedInventory
    // 目前假设所有槽位都可访问

    for (i32 slot = 0; slot < inventory->getContainerSize(); ++slot) {
        if (!inventory->getItem(slot).isEmpty()) {
            return false;
        }
    }

    return true;
}

bool HopperEntity::pullItemFromSlot(
    IHopper& hopper,
    IInventory* inventory,
    i32 slotIndex,
    Direction direction) {

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

    ItemStack remaining = putStackInInventoryAllSlots(
        inventory, hopperInventory, extracted, Direction::None);

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
    IInventory* source,
    IInventory* destination,
    const ItemStack& stack,
    i32 slotIndex,
    Direction direction) {

    if (destination == nullptr || stack.isEmpty()) {
        return stack;
    }

    const ItemStack& existingStack = destination->getItem(slotIndex);

    // 检查是否可以插入该槽位
    if (!canInsertItemInSlot(destination, stack, slotIndex, direction)) {
        return stack;
    }

    bool inserted = false;
    bool wasEmpty = destination->isEmpty();

    if (existingStack.isEmpty()) {
        // 空槽位，直接放入
        destination->setItem(slotIndex, stack);
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

            // 返回剩余物品
            ItemStack remaining = stack.copy();
            remaining.shrink(toInsert);
            inserted = true;
        }
    }

    if (inserted) {
        // 漏斗链优化：如果目标是漏斗且为空
        // 减少冷却时间
        if (wasEmpty && source != nullptr) {
            // TODO: 检查目标是否是漏斗
            // HopperEntity* targetHopper = dynamic_cast<HopperEntity*>(destination);
            // if (targetHopper != nullptr && !targetHopper->mayTransfer()) {
            //     漏斗链优化
            // }
        }

        destination->setChanged();
    }

    return stack;  // 返回原始栈，调用者需要处理剩余
}

bool HopperEntity::canInsertItemInSlot(
    const IInventory* inventory,
    const ItemStack& stack,
    i32 slotIndex,
    Direction direction) {

    if (inventory == nullptr) {
        return false;
    }

    // 检查槽位是否接受该物品
    if (!inventory->canPlaceItem(slotIndex, stack)) {
        return false;
    }

    // TODO: 处理 ISidedInventory（侧面有不同槽位的容器）
    // 目前假设所有槽位都可从任意方向插入

    return true;
}

bool HopperEntity::canExtractItemFromSlot(
    const IInventory* inventory,
    const ItemStack& stack,
    i32 slotIndex,
    Direction direction) {

    if (inventory == nullptr) {
        return false;
    }

    // TODO: 处理 ISidedInventory（侧面有不同槽位的容器）
    // 目前假设所有槽位都可从任意方向提取

    return true;
}

bool HopperEntity::canCombine(const ItemStack& stack1, const ItemStack& stack2) {
    if (stack1.getItem() != stack2.getItem()) {
        return false;
    }

    if (stack1.getDamage() != stack2.getDamage()) {
        return false;
    }

    // 检查数量是否超过最大堆叠
    if (stack1.getCount() > stack1.getMaxStackSize()) {
        return false;
    }

    // 检查NBT标签是否相同
    // TODO: 实现 ItemStack 的 NBT 标签比较
    // return ItemStack::areItemStackTagsEqual(stack1, stack2);

    return true;
}

void HopperEntity::onEntityCollision(IWorld& world, Entity* entity) {
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
    updateHopper([&]() {
        return captureItem(&m_inventory, itemEntity);
    });
}

} // namespace blockentity
} // namespace mc
