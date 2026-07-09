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

#include "world/blockentity/storage/ChestEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/container/ChestContainer.hpp"
#include "item/loot/LootTable.hpp"
#include "item/loot/context/LootContext.hpp"
#include "sound/SoundCategory.hpp"
#include "sound/SoundEvents.hpp"
#include "util/Direction.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/ChestBlock.hpp"
#include "world/gameevent/GameEvents.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include <cmath>

namespace mc {
namespace blockentity {

// ========== 常量定义 ==========

namespace {
/// 盖子动画速度（每tick变化量），参考 MC ChestLidController
constexpr f32 LID_OPEN_SPEED = 0.1f;
/// 音效音量
constexpr f32 CHEST_SOUND_VOLUME = 0.5f;
} // namespace

// ========== 构造函数 ==========

ChestEntity::ChestEntity(const BlockPos& pos)
    : ChestEntity(BlockEntityType::Chest, pos)
{}

ChestEntity::ChestEntity(BlockEntityType type, const BlockPos& pos)
    : LootableContainerBlockEntity(type, pos)
    , m_inventory(CHEST_SIZE, [this]() { setChanged(); })
{
    // 重新绑定战利品表延迟填充回调（this 指针已确定）。
    // 回调使 m_inventory 的 isEmpty/getItem/setItem/removeItem/removeItemNoUpdate/clear
    // 都自动触发 _unpackLootTable(nullptr)，与 MC Java 的
    // RandomizableContainerBlockEntity 行为一致。
    m_inventory.setLootUnpackCallback(_makeLootUnpackCallback());
}

ChestEntity::~ChestEntity() = default;

// ========== 移动操作 ==========

ChestEntity::ChestEntity(ChestEntity&& other) noexcept
    : LootableContainerBlockEntity(std::move(other))
    , m_inventory(std::move(other.m_inventory))
    , m_lidAngle(other.m_lidAngle)
    , m_prevLidAngle(other.m_prevLidAngle)
    , m_ticksSinceSync(other.m_ticksSinceSync)
{
    // 设置库存变更回调（this 指针变化，需重新绑定）
    m_inventory.setOnChanged([this]() { setChanged(); });
    // 重新绑定战利品表延迟填充回调（this 指针变化）
    m_inventory.setLootUnpackCallback(_makeLootUnpackCallback());
    // 重置源对象状态
    other.m_lidAngle = 0.0f;
    other.m_prevLidAngle = 0.0f;
    other.m_ticksSinceSync = 0;
}

ChestEntity& ChestEntity::operator=(ChestEntity&& other) noexcept
{
    if (this != &other) {
        LootableContainerBlockEntity::operator=(std::move(other));
        m_inventory = std::move(other.m_inventory);
        m_inventory.setOnChanged([this]() { setChanged(); });
        // 重新绑定战利品表延迟填充回调（this 指针变化）
        m_inventory.setLootUnpackCallback(_makeLootUnpackCallback());
        m_lidAngle = other.m_lidAngle;
        m_prevLidAngle = other.m_prevLidAngle;
        m_ticksSinceSync = other.m_ticksSinceSync;
        // 重置源对象状态
        other.m_lidAngle = 0.0f;
        other.m_prevLidAngle = 0.0f;
        other.m_ticksSinceSync = 0;
    }
    return *this;
}

// ========== 双箱相关 ==========

bool ChestEntity::isDoubleChest(IWorld& world) const
{
    return getConnectedChest(world) != nullptr;
}

ChestEntity* ChestEntity::getConnectedChest(IWorld& world) const
{
    // 获取当前方块的方块状态
    const BlockState* statePtr = world.getBlockState(m_pos);
    if (statePtr == nullptr) {
        return nullptr;
    }

    // 获取箱子类型
    BlockStateProperties::ChestType chestType = statePtr->get(BlockStateProperties::CHEST_TYPE());
    if (chestType == BlockStateProperties::ChestType::Single) {
        return nullptr; // 单箱，无连接
    }

    // 获取朝向
    Direction facing = statePtr->get(BlockStateProperties::HORIZONTAL_FACING());

    // 计算连接方向
    // LEFT 箱子向右连接（顺时针旋转），RIGHT 箱子向左连接（逆时针旋转）
    Direction connectDir = Direction::None;
    if (chestType == BlockStateProperties::ChestType::Left) {
        connectDir = Directions::rotateY(facing); // 左箱子向右连接
    } else if (chestType == BlockStateProperties::ChestType::Right) {
        connectDir = Directions::rotateYCCW(facing); // 右箱子向左连接
    }

    if (connectDir == Direction::None) {
        return nullptr;
    }

    // 获取相邻位置的方块实体
    BlockPos neighborPos = m_pos.offset(connectDir);
    BlockEntity* neighborEntity = world.getBlockEntity(neighborPos);
    if (neighborEntity == nullptr) {
        return nullptr;
    }

    // 检查是否是箱子实体
    BlockEntityType neighborType = neighborEntity->getType();
    if (neighborType != BlockEntityType::Chest && neighborType != BlockEntityType::TrappedChest) {
        return nullptr;
    }

    // 验证相邻箱子确实是连接的
    const BlockState* neighborStatePtr = world.getBlockState(neighborPos);
    if (neighborStatePtr == nullptr) {
        return nullptr;
    }

    BlockStateProperties::ChestType neighborChestType = neighborStatePtr->get(BlockStateProperties::CHEST_TYPE());
    if (neighborChestType == BlockStateProperties::ChestType::Single) {
        return nullptr; // 相邻的是单箱，不应该发生
    }

    // 验证连接方向是否正确
    Direction neighborFacing = neighborStatePtr->get(BlockStateProperties::HORIZONTAL_FACING());
    if (neighborFacing != facing) {
        return nullptr; // 朝向不同，不是有效连接
    }

    return static_cast<ChestEntity*>(neighborEntity);
}

std::unique_ptr<DoubleSidedInventory> ChestEntity::getDoubleInventory(IWorld& world)
{
    ChestEntity* connected = getConnectedChest(world);
    if (!connected) {
        return nullptr;
    }

    // 根据ChestType确定顺序
    // LEFT类型在左侧（上半部分），RIGHT类型在右侧（下半部分）
    const BlockState* statePtr = world.getBlockState(m_pos);
    if (statePtr == nullptr) {
        return nullptr;
    }

    BlockStateProperties::ChestType chestType = statePtr->get(BlockStateProperties::CHEST_TYPE());

    if (chestType == BlockStateProperties::ChestType::Left) {
        // 当前箱子是左半部分
        return std::make_unique<DoubleSidedInventory>(&m_inventory, &connected->m_inventory);
    } else {
        // 当前箱子是右半部分
        return std::make_unique<DoubleSidedInventory>(&connected->m_inventory, &m_inventory);
    }
}

// ========== 打开计数 ==========

void ChestEntity::openContainer(Player* player)
{
    // 记录边沿状态：当前是否为空（0个打开者）
    const bool wasEmpty = (m_openCount == 0);

    // 基类处理观察者检查和计数增加
    LootableContainerBlockEntity::openContainer(player);

    // 仅在首次打开（0->1）时触发音效和游戏事件（仅服务端）
    if (wasEmpty && m_openCount > 0 && m_world != nullptr && !m_world->isClientSide()) {
        _playSound(*m_world, true);
        m_world->gameEvent(gameevent::GameEvents::CONTAINER_OPEN, m_pos, gameevent::GameEvent::Context::of(player));
    }

    // 广播状态变化（红石信号、客户端同步等）
    if (m_world != nullptr) {
        broadcastChestState(*m_world, true);
    }
}

void ChestEntity::closeContainer(Player* player)
{
    // 记录关闭前的计数
    const i32 prevCount = m_openCount;

    // 基类处理计数减少
    ContainerBlockEntity::closeContainer(player);

    // 仅在最后一个关闭者（1->0）时触发音效和游戏事件（仅服务端）
    if (prevCount == 1 && m_openCount == 0 && m_world != nullptr && !m_world->isClientSide()) {
        _playSound(*m_world, false);
        m_world->gameEvent(gameevent::GameEvents::CONTAINER_CLOSE, m_pos, gameevent::GameEvent::Context::of(player));
    }

    // 广播状态变化
    if (m_world != nullptr) {
        broadcastChestState(*m_world, false);
    }
}

// ========== 红石比较器 ==========

i32 ChestEntity::getComparatorSignal(IWorld& world) const
{
    // 计算填充度
    i32 nonEmptySlots = 0;
    f32 fillRatio = 0.0f;
    i32 totalSlots = CHEST_SIZE;

    for (i32 i = 0; i < CHEST_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            ++nonEmptySlots;
            fillRatio += static_cast<f32>(stack.getCount()) / static_cast<f32>(stack.getMaxStackSize());
        }
    }

    // 如果是双箱，需要计算合并的信号
    ChestEntity* connected = getConnectedChest(world);
    if (connected != nullptr) {
        // 添加相邻箱子的数据
        for (i32 i = 0; i < CHEST_SIZE; ++i) {
            const ItemStack& stack = connected->m_inventory.getItem(i);
            if (!stack.isEmpty()) {
                ++nonEmptySlots;
                fillRatio += static_cast<f32>(stack.getCount()) / static_cast<f32>(stack.getMaxStackSize());
            }
        }
        totalSlots += CHEST_SIZE;
    }

    fillRatio /= static_cast<f32>(totalSlots);

    // 信号强度 = floor(填充度 * 14) + (非空槽位数 > 0 ? 1 : 0)
    i32 signal = static_cast<i32>(std::floor(fillRatio * 14.0f));
    if (nonEmptySlots > 0) {
        signal += 1;
    }

    return std::min(signal, 15);
}

// ========== Tick 更新 ==========

void ChestEntity::tick(IWorld& world)
{
    // 在tick开头更新prevLidAngle，确保每帧都能正确插值
    m_prevLidAngle = m_lidAngle;

    // 更新同步计数器
    ++m_ticksSinceSync;

    // 每 RECHECK_INTERVAL(5) ticks 重新检查打开者数量
    // 参考 MC ContainerOpenersCounter.recheckOpeners / CHECK_TICK_DELAY
    if (m_ticksSinceSync >= RECHECK_INTERVAL) {
        // 如果有打开者，重新检查附近玩家是否仍在使用此容器
        if (m_openCount > 0) {
            _recheckOpeners(world);
        }
    }

    // 每 SYNC_INTERVAL(200) ticks 进行完整的客户端同步
    // 参考 MC 每 200 ticks 调用 level.sendBlockUpdated()
    if (m_ticksSinceSync >= SYNC_INTERVAL) {
        m_ticksSinceSync = 0;
        // 通知客户端方块实体数据更新
        world.notifyBlockUpdate(m_pos);
    }

    // 只有在需要动画时才更新
    // 条件：(openCount == 0 && lidAngle > 0) || (openCount > 0 && lidAngle < 1)
    const bool needsAnimation = (m_openCount == 0 && m_lidAngle > 0.0f) || (m_openCount > 0 && m_lidAngle < 1.0f);

    if (needsAnimation) {
        const f32 prevAngle = m_lidAngle;

        if (m_openCount > 0) {
            // 打开动画
            m_lidAngle += LID_OPEN_SPEED;
            if (m_lidAngle > 1.0f) {
                m_lidAngle = 1.0f;
            }
        } else {
            // 关闭动画
            m_lidAngle -= LID_OPEN_SPEED;
            if (m_lidAngle < 0.0f) {
                m_lidAngle = 0.0f;
            }
        }

        // 打开音效：lidAngle从0变为正值时（已在openContainer中触发，此处不再播放）
        // 关闭音效：lidAngle从>=0.5变为<0.5时
        // 注：关闭音效已在 closeContainer 中边沿检测时触发，此处不再播放
        MC_UNUSED(prevAngle);
    }

    MC_UNUSED(world);
}

// ========== BlockEvent 同步 ==========

bool ChestEntity::triggerEvent(i32 id, i32 type)
{
    if (id == 1) {
        // 箱子开合动画事件：type > 0 表示打开，type == 0 表示关闭
        m_lidAngle = (type > 0) ? 1.0f : 0.0f;
        return true;
    }
    return false;
}

// ========== 序列化 ==========

bool ChestEntity::load(const nlohmann::json& data)
{
    if (!LootableContainerBlockEntity::load(data)) {
        return false;
    }

    m_inventory.clear();

    if (data.contains("Items") && data["Items"].is_array()) {
        const auto& items = data["Items"];
        for (const auto& itemJson : items) {
            if (!itemJson.is_object()) {
                continue;
            }

            const i32 slot = itemJson.value("Slot", -1);
            if (slot < 0 || slot >= CHEST_SIZE) {
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

void ChestEntity::save(nlohmann::json& data) const
{
    LootableContainerBlockEntity::save(data);

    nlohmann::json itemsJson = nlohmann::json::array();
    for (i32 i = 0; i < CHEST_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        nlohmann::json itemJson = stack.toJson();
        itemJson["Slot"] = i;
        itemsJson.push_back(std::move(itemJson));
    }
    data["Items"] = std::move(itemsJson);
}

// ========== NBT 序列化（结构模板 / 客户端同步）==========

bool ChestEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!LootableContainerBlockEntity::loadFromNBT(tag)) {
        return false;
    }

    // 仅在无未解包的战利品表时加载物品，与 MC Java 互斥语义一致
    if (!hasLootTable()) {
        loadItemsFromNBT(tag, m_inventory);
    }

    return true;
}

void ChestEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    LootableContainerBlockEntity::saveToNBT(tag);

    // 仅在无未解包的战利品表时保存物品，与 MC Java 互斥语义一致
    if (!hasLootTable()) {
        saveItemsToNBT(tag, m_inventory);
    }
}

std::unique_ptr<BlockEntity> ChestEntity::clone() const
{
    auto cloned = std::make_unique<ChestEntity>(getType(), m_pos);
    cloned->m_lidAngle = m_lidAngle;
    cloned->m_prevLidAngle = m_prevLidAngle;
    cloned->m_openCount = m_openCount;
    cloned->m_ticksSinceSync = m_ticksSinceSync;

    for (i32 slot = 0; slot < CHEST_SIZE; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            cloned->m_inventory.setItem(slot, stack.copy());
        }
    }

    nlohmann::json state;
    LockableBlockEntity::save(state);
    cloned->LockableBlockEntity::load(state);

    return cloned;
}

// ========== 受保护方法 ==========

void ChestEntity::broadcastChestState(IWorld& world, bool open)
{
    MC_UNUSED(open);

    // 发送 blockEvent 通知客户端同步盖子开合动画
    // 参考 MC ChestBlockEntity.signalOpenCount -> level.blockEvent(pos, block, 1, openCount)
    const BlockState* state = world.getBlockState(m_pos);
    if (state != nullptr) {
        world.blockEvent(m_pos, state->getBlock(), 1, m_openCount);
    }

    // 通知客户端方块实体数据更新
    world.notifyBlockUpdate(m_pos);

    if (state != nullptr) {
        world::redstone::RedstoneSystem::instance().updateNeighbors(world, m_pos, state->getBlockMutable());
        world::redstone::RedstoneSystem::instance().updateComparators(world, m_pos);
    }
}

// ========== 私有方法 ==========

void ChestEntity::_playSound(IWorld& world, bool open)
{
    // 仅服务端播放音效
    if (world.isClientSide()) {
        return;
    }

    // 参考 MC ChestBlockEntity.playSound:
    // LEFT 箱子不播放音效（由 RIGHT 箱子统一播放）
    // SINGLE 箱子在方块中心播放
    // RIGHT 箱子音效位置向 LEFT 方向偏移 0.5 格（双箱中心）
    const BlockState* state = world.getBlockState(m_pos);
    if (state == nullptr) {
        return;
    }

    BlockStateProperties::ChestType chestType = state->get(BlockStateProperties::CHEST_TYPE());

    // LEFT 箱子不播放音效（由 RIGHT 箱子统一在双箱中心播放）
    if (chestType == BlockStateProperties::ChestType::Left) {
        return;
    }

    // 计算音效位置：默认为方块中心
    f64 soundX = static_cast<f64>(m_pos.x) + 0.5;
    f64 soundY = static_cast<f64>(m_pos.y) + 0.5;
    f64 soundZ = static_cast<f64>(m_pos.z) + 0.5;

    // RIGHT 箱子向连接方向偏移 0.5 格，使音效在双箱中心播放
    if (chestType == BlockStateProperties::ChestType::Right) {
        Direction connectedDir = blocks::ChestBlock::getConnectedDirection(*state);
        soundX += static_cast<f64>(Directions::xOffset(connectedDir)) * 0.5;
        soundZ += static_cast<f64>(Directions::zOffset(connectedDir)) * 0.5;
    }

    // 音调随机化 0.9~1.0，参考 MC level.random.nextFloat() * 0.1F + 0.9F
    static thread_local math::Random sSoundRng(0);
    f32 pitch = sSoundRng.nextFloat() * 0.1f + 0.9f;

    // 从方块实例取得开合音效（普通箱子返回 BLOCK_CHEST_OPEN/CLOSE，
    // 铜箱子按氧化等级返回对应声音事件）
    // 对应 MC Java ChestBlockEntity.onOpen/onClose:
    //   if (state.getBlock() instanceof ChestBlock chestblock)
    //       playSound(level, pos, state, chestblock.getOpenChestSound());
    const Block& block = state->getBlock();
    const auto* chestBlock = dynamic_cast<const blocks::ChestBlock*>(&block);
    const ResourceLocation& soundEvent = (chestBlock != nullptr)
        ? (open ? chestBlock->getOpenSound() : chestBlock->getCloseSound())
        : (open ? SoundEvents::BLOCK_CHEST_OPEN : SoundEvents::BLOCK_CHEST_CLOSE);
    world.playSound(
        soundEvent, sound::SoundCategory::Blocks, Vector3(soundX, soundY, soundZ), CHEST_SOUND_VOLUME, pitch);
}

void ChestEntity::_recheckOpeners(IWorld& world)
{
    // 参考 MC ContainerOpenersCounter.recheckOpeners
    // 遍历附近玩家，检查哪些玩家仍在使用此容器

    // 搜索范围：MAX_ACCESS_DISTANCE（8格）
    const f32 maxDistSq = MAX_ACCESS_DISTANCE * MAX_ACCESS_DISTANCE;
    Vector3 centerPos = m_pos.center();

    i32 actualOpenCount = 0;

    // 获取附近所有实体并筛选玩家
    std::vector<Entity*> entities = world.getEntitiesInRange(centerPos, MAX_ACCESS_DISTANCE);
    for (Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }

        // 检查是否是玩家
        auto* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 排除旁观者
        if (player->isSpectator()) {
            continue;
        }

        // 检查玩家是否在访问范围内
        if (player->distanceSqTo(centerPos.x, centerPos.y, centerPos.z) > maxDistSq) {
            continue;
        }

        // 检查玩家当前打开的容器菜单是否包含此箱子
        // 参考 MC ChestBlockEntity.isOwnContainer:
        //   1. 玩家的 containerMenu 必须是 ChestMenu（对应本项目的 ChestContainer）
        //   2. ChestMenu 持有的底层容器（getContainer()）必须是此箱子的 m_inventory
        //   3. 对于双箱情况，底层容器是 DoubleSidedInventory，需检查 isPartOfLargeChest
        if (_isOwnContainer(*player)) {
            ++actualOpenCount;
        }
    }

    // 如果计数不匹配，修正
    if (actualOpenCount != m_openCount) {
        const bool wasOpen = (m_openCount > 0);
        const bool isOpen = (actualOpenCount > 0);

        // 状态从关闭变为打开
        if (isOpen && !wasOpen) {
            _playSound(world, true);
            world.gameEvent(gameevent::GameEvents::CONTAINER_OPEN, m_pos, nullptr);
        }
        // 状态从打开变为关闭
        else if (!isOpen && wasOpen) {
            _playSound(world, false);
            world.gameEvent(gameevent::GameEvents::CONTAINER_CLOSE, m_pos, nullptr);
        }

        m_openCount = actualOpenCount;

        // 广播状态变化
        broadcastChestState(world, actualOpenCount > 0);
    }
}

bool ChestEntity::_isOwnContainer(const Player& player) const
{
    // 参考 MC ChestBlockEntity.isOwnContainer:
    //   if (!(player.containerMenu instanceof ChestMenu)) return false;
    //   Container container = ((ChestMenu)player.containerMenu).getContainer();
    //   return container == ChestBlockEntity.this
    //       || container instanceof CompoundContainer
    //          && ((CompoundContainer)container).contains(ChestBlockEntity.this);

    const auto* menu = player.openContainerMenu();
    if (menu == nullptr) {
        return false;
    }

    // 检查菜单是否是 ChestContainer
    const auto* chestContainer = dynamic_cast<const ChestContainer*>(menu);
    if (chestContainer == nullptr) {
        return false;
    }

    // 获取 ChestContainer 持有的底层容器
    const IInventory* containerInventory = chestContainer->getChestInventory();
    if (containerInventory == nullptr) {
        return false;
    }

    // 单箱情况：底层容器直接是此箱子的 m_inventory
    if (containerInventory == &m_inventory) {
        return true;
    }

    // 双箱情况：底层容器是 DoubleSidedInventory，检查此箱子的 m_inventory 是否是其一部分
    const auto* doubleInv = dynamic_cast<const DoubleSidedInventory*>(containerInventory);
    if (doubleInv != nullptr) {
        return doubleInv->isPartOfLargeChest(&m_inventory);
    }

    return false;
}

} // namespace blockentity
} // namespace mc
