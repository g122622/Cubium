#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockState.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/ChestBlock.hpp"
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
    /// 盖子关闭音效事件ID
    constexpr i32 SOUND_EVENT_CLOSE = 1012;
    /// 盖子打开音效事件ID
    constexpr i32 SOUND_EVENT_OPEN = 1006;
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

bool ChestEntity::isDoubleChest(World& world) const {
    // TODO: 检查方块状态中的ChestType属性
    // 如果是LEFT或RIGHT，则为双箱
    return getConnectedChest(world) != nullptr;
}

ChestEntity* ChestEntity::getConnectedChest(World& world) const {
    // TODO: 根据方块状态中的ChestType和FACING属性
    // 确定相邻箱子的位置，然后获取其方块实体

    // 临时返回nullptr，待ChestBlock实现后完善
    (void)world;
    return nullptr;
}

std::unique_ptr<DoubleSidedInventory> ChestEntity::getDoubleInventory(World& world) {
    ChestEntity* connected = getConnectedChest(world);
    if (!connected) {
        return nullptr;
    }

    // 根据ChestType确定顺序
    // LEFT类型在左侧（上半部分），RIGHT类型在右侧（下半部分）
    // TODO: 根据实际方块状态确定顺序

    return std::make_unique<DoubleSidedInventory>(
        &m_inventory,
        &connected->m_inventory
    );
}

// ========== 打开计数 ==========

void ChestEntity::openContainer() {
    // 增加计数
    ++m_openCount;

    // 广播状态变化
    // TODO: 实现World广播
    // broadcastChestState(world, true);
}

void ChestEntity::closeContainer() {
    // 减少计数（不低于0）
    if (m_openCount > 0) {
        --m_openCount;
    }

    // 广播状态变化
    // TODO: 实现World广播
    // broadcastChestState(world, false);
}

// ========== 红石比较器 ==========

i32 ChestEntity::getComparatorSignal(IWorld& world) const {
    // 计算填充度
    i32 nonEmptySlots = 0;
    f32 fillRatio = 0.0f;

    for (i32 i = 0; i < CHEST_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            ++nonEmptySlots;
            fillRatio += static_cast<f32>(stack.getCount()) /
                         static_cast<f32>(stack.getMaxStackSize());
        }
    }

    fillRatio /= static_cast<f32>(CHEST_SIZE);

    // 信号强度 = floor(填充度 * 14) + (非空槽位数 > 0 ? 1 : 0)
    i32 signal = static_cast<i32>(std::floor(fillRatio * 14.0f));
    if (nonEmptySlots > 0) {
        signal += 1;
    }

    // 如果是双箱，需要计算合并的信号
    if (isDoubleChest(world)) {
        // TODO: 获取相邻箱子并合并计算
        // ChestEntity* connected = getConnectedChest(world);
        // 需要合并两个箱子的填充度
    }

    return std::min(signal, 15);
}

// ========== 动画 ==========

void ChestEntity::updateLidAnimation(f32 partialTick) {
    m_prevLidAngle = m_lidAngle;

    if (m_openCount > 0 && m_lidAngle < 1.0f) {
        // 打开动画
        m_lidAngle += LID_OPEN_SPEED;
        if (m_lidAngle > 1.0f) {
            m_lidAngle = 1.0f;
        }
    } else if (m_openCount == 0 && m_lidAngle > 0.0f) {
        // 关闭动画
        m_lidAngle -= LID_OPEN_SPEED;
        if (m_lidAngle < 0.0f) {
            m_lidAngle = 0.0f;
        }
    }
}

// ========== Tick 更新 ==========

void ChestEntity::tick(World& world) {
    // 更新同步计数器
    ++m_ticksSinceSync;

    // 定期同步打开计数（服务端）
    // TODO: 实现定期同步逻辑
    // if (!world.isRemote() && m_ticksSinceSync % SYNC_INTERVAL == 0) {
    //     m_openCount = calculatePlayersUsingSync(world);
    // }

    // 更新盖子动画
    f32 prevAngle = m_lidAngle;

    // 打开时播放音效
    if (m_openCount > 0 && m_lidAngle == 0.0f) {
        playSound(world, true);
    }

    // 更新动画
    if (m_openCount > 0) {
        m_lidAngle += LID_OPEN_SPEED;
    } else {
        m_lidAngle -= LID_OPEN_SPEED;
    }

    // 限制范围
    if (m_lidAngle > 1.0f) {
        m_lidAngle = 1.0f;
    } else if (m_lidAngle < 0.0f) {
        m_lidAngle = 0.0f;
    }

    // 关闭到一半时播放音效
    if (m_lidAngle < LID_CLOSE_SOUND_THRESHOLD && prevAngle >= LID_CLOSE_SOUND_THRESHOLD) {
        playSound(world, false);
    }

    (void)world; // 暂时避免未使用警告
}

// ========== 序列化 ==========

bool ChestEntity::load(const nlohmann::json& data) {
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    // 加载物品
    if (data.contains("Items") && data["Items"].is_array()) {
        const auto& items = data["Items"];
        for (const auto& itemJson : items) {
            // TODO: 实现ItemStack从JSON加载
            // i32 slot = itemJson.value("Slot", 0);
            // ItemStack stack = ItemStack::fromJson(itemJson);
            // m_inventory.setItem(slot, stack);
        }
    }

    return true;
}

void ChestEntity::save(nlohmann::json& data) const {
    LockableBlockEntity::save(data);

    // 保存物品
    nlohmann::json itemsJson = nlohmann::json::array();
    for (i32 i = 0; i < CHEST_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            nlohmann::json itemJson;
            itemJson["Slot"] = i;
            // TODO: 保存ItemStack数据
            // stack.save(itemJson);
            itemsJson.push_back(itemJson);
        }
    }
    data["Items"] = itemsJson;
}

std::unique_ptr<BlockEntity> ChestEntity::clone() const {
    auto cloned = std::make_unique<ChestEntity>(m_pos);
    // TODO: 复制物品数据
    return cloned;
}

// ========== 受保护方法 ==========

void ChestEntity::broadcastChestState(World& world, bool open) {
    // TODO: 实现World广播
    // world.addBlockEvent(m_pos, getBlockState()->getBlock(), 1, m_openCount);
    // world.notifyNeighborsOfStateChange(m_pos, getBlockState()->getBlock());

    (void)world;
    (void)open;
}

void ChestEntity::playSound(World& world, bool open) {
    // TODO: 实现音效播放
    // 只在RIGHT类型或SINGLE类型播放音效
    // if (isLeftPartOfDoubleChest()) {
    //     return; // 左半部分不播放
    // }

    (void)world;
    (void)open;
}

} // namespace blockentity
} // namespace mc
