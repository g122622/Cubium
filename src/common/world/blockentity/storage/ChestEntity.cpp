#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/ChestBlock.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include "util/property/Properties.hpp"
#include <cmath>

namespace mc {
namespace blockentity {

// ========== 常量定义 ==========

namespace {
    /// 盖子动画速度（每tick变化量）
    constexpr f32 LID_OPEN_SPEED = 0.1f;
    /// 盖子动画关闭音效触发阈值
    constexpr f32 LID_CLOSE_SOUND_THRESHOLD = 0.5f;
    /// 同步间隔（ticks）
    constexpr i32 SYNC_INTERVAL = 200;
}

// ========== 构造函数 ==========

ChestEntity::ChestEntity(const BlockPos& pos)
    : ChestEntity(BlockEntityType::Chest, pos) {
}

ChestEntity::ChestEntity(BlockEntityType type, const BlockPos& pos)
    : LockableBlockEntity(type, pos)
    , m_inventory(CHEST_SIZE, [this]() { setChanged(); }) {
}

ChestEntity::~ChestEntity() = default;

// ========== 双箱相关 ==========

bool ChestEntity::isDoubleChest(IWorld& world) const {
    return getConnectedChest(world) != nullptr;
}

ChestEntity* ChestEntity::getConnectedChest(IWorld& world) const {
    // 获取当前方块的方块状态
    const BlockState* statePtr = world.getBlockState(m_pos);
    if (statePtr == nullptr) {
        return nullptr;
    }

    // 获取箱子类型
    BlockStateProperties::ChestType chestType = statePtr->get(BlockStateProperties::CHEST_TYPE());
    if (chestType == BlockStateProperties::ChestType::Single) {
        return nullptr;  // 单箱，无连接
    }

    // 获取朝向
    Direction facing = statePtr->get(BlockStateProperties::HORIZONTAL_FACING());

    // 计算连接方向
    // LEFT 箱子向右连接（顺时针旋转），RIGHT 箱子向左连接（逆时针旋转）
    Direction connectDir = Direction::None;
    if (chestType == BlockStateProperties::ChestType::Left) {
        connectDir = Directions::rotateY(facing);  // 左箱子向右连接
    } else if (chestType == BlockStateProperties::ChestType::Right) {
        connectDir = Directions::rotateYCCW(facing);  // 右箱子向左连接
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
        return nullptr;  // 相邻的是单箱，不应该发生
    }

    // 验证连接方向是否正确
    Direction neighborFacing = neighborStatePtr->get(BlockStateProperties::HORIZONTAL_FACING());
    if (neighborFacing != facing) {
        return nullptr;  // 朝向不同，不是有效连接
    }

    return static_cast<ChestEntity*>(neighborEntity);
}

std::unique_ptr<DoubleSidedInventory> ChestEntity::getDoubleInventory(IWorld& world) {
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

void ChestEntity::openContainer() {
    // MC 1.16.5: 观察者模式的玩家不计入打开数
    // 注：当前项目尚未实现观察者模式检查，待Player类添加isSpectator()后补充

    // MC 1.16.5: 负数保护
    if (m_openCount < 0) {
        m_openCount = 0;
    }

    ++m_openCount;

    if (m_world != nullptr) {
        broadcastChestState(*m_world, true);
    }
}

void ChestEntity::closeContainer() {
    // MC 1.16.5: 观察者模式的玩家不计入打开数
    // 注：当前项目尚未实现观察者模式检查，待Player类添加isSpectator()后补充

    --m_openCount;

    if (m_world != nullptr) {
        broadcastChestState(*m_world, false);
    }
}

// ========== 红石比较器 ==========

i32 ChestEntity::getComparatorSignal(IWorld& world) const {
    // 计算填充度
    i32 nonEmptySlots = 0;
    f32 fillRatio = 0.0f;
    i32 totalSlots = CHEST_SIZE;

    for (i32 i = 0; i < CHEST_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            ++nonEmptySlots;
            fillRatio += static_cast<f32>(stack.getCount()) /
                         static_cast<f32>(stack.getMaxStackSize());
        }
    }

    // 如果是双箱，需要计算合并的信号
    ChestEntity* connected = getConnectedChest(world);
    if (connected != nullptr) {
        // 添加相邻箱子的��据
        for (i32 i = 0; i < CHEST_SIZE; ++i) {
            const ItemStack& stack = connected->m_inventory.getItem(i);
            if (!stack.isEmpty()) {
                ++nonEmptySlots;
                fillRatio += static_cast<f32>(stack.getCount()) /
                             static_cast<f32>(stack.getMaxStackSize());
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

void ChestEntity::tick(IWorld& world) {
    // MC 1.16.5: 在tick开头更新prevLidAngle，确保每帧都能正确插值
    m_prevLidAngle = m_lidAngle;

    // 更新同步计数器
    ++m_ticksSinceSync;

    // 定期重新统计附近打开箱子的玩家数（MC每200ticks）
    // 注：原版calculatePlayersUsingSync在服务端执行，此处简化为方块状态同步
    if (m_ticksSinceSync >= SYNC_INTERVAL) {
        m_ticksSinceSync = 0;

        const BlockState* state = world.getBlockState(m_pos);
        if (state != nullptr) {
            world.setBlockState(m_pos, state, 3);
        }
    }

    // MC 1.16.5: 只有在需要动画时才更新
    // 条件：(openCount == 0 && lidAngle > 0) || (openCount > 0 && lidAngle < 1)
    const bool needsAnimation = (m_openCount == 0 && m_lidAngle > 0.0f) ||
                                 (m_openCount > 0 && m_lidAngle < 1.0f);

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

        // 打开音效：lidAngle从0变为正值时
        if (prevAngle == 0.0f && m_lidAngle > 0.0f) {
            playSound(world, true);
        }

        // 关闭音效：lidAngle从>=0.5变为<0.5时
        if (prevAngle >= LID_CLOSE_SOUND_THRESHOLD && m_lidAngle < LID_CLOSE_SOUND_THRESHOLD) {
            playSound(world, false);
        }
    }

    MC_UNUSED(world);
}

// ========== 序列化 ==========

bool ChestEntity::load(const nlohmann::json& data) {
    if (!LockableBlockEntity::load(data)) {
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

void ChestEntity::save(nlohmann::json& data) const {
    LockableBlockEntity::save(data);

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

std::unique_ptr<BlockEntity> ChestEntity::clone() const {
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

void ChestEntity::broadcastChestState(IWorld& world, bool open) {
    MC_UNUSED(open);

    const BlockState* state = world.getBlockState(m_pos);
    if (state == nullptr) {
        return;
    }

    world.setBlockState(m_pos, state, 3);

    const Block& sourceBlock = state->getBlock();
    world::redstone::RedstoneSystem::instance().updateNeighbors(
        world, m_pos, const_cast<Block&>(sourceBlock));
    world::redstone::RedstoneSystem::instance().updateComparators(world, m_pos);
}

void ChestEntity::playSound(IWorld& world, bool open) {
    // 当前 IWorld 尚未提供统一音效事件接口。
    // 先通过状态广播保持动画/红石一致，避免遗漏行为更新。
    broadcastChestState(world, open);
}

} // namespace blockentity
} // namespace mc

