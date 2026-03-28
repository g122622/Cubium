#include "world/blockentity/storage/BarrelEntity.hpp"
#include "world/IWorld.hpp"
#include "item/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== BarrelEntity 实现 ==========

BarrelEntity::BarrelEntity(const BlockPos& pos)
    : LockableBlockEntity(BlockEntityType::Barrel, pos)
    , m_inventory(BARREL_SIZE) {
}

BarrelEntity::~BarrelEntity() = default;

void BarrelEntity::openContainer() {
    LockableBlockEntity::openContainer();
    m_openCount++;

    // 标记改变
    setChanged();
}

void BarrelEntity::closeContainer() {
    if (m_openCount > 0) {
        m_openCount--;
        setChanged();
    }
}

i32 BarrelEntity::getComparatorSignal(IWorld& world) const {
    MC_UNUSED(world);

    // 计算填充比例
    i32 filledSlots = 0;
    i32 totalCount = 0;

    for (i32 i = 0; i < BARREL_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            filledSlots++;
            totalCount += stack.getCount();
        }
    }

    if (filledSlots == 0) {
        return 0;
    }

    // 比较器信号计算公式
    // signal = (filledSlots / totalSlots) * 14 + (totalCount > 0 ? 1 : 0)
    f32 fillRatio = static_cast<f32>(filledSlots) / static_cast<f32>(BARREL_SIZE);
    return static_cast<i32>(fillRatio * 14.0f) + (totalCount > 0 ? 1 : 0);
}

void BarrelEntity::tick(IWorld& world) {
    m_ticksSinceSync++;

    // 定期同步打开状态
    if (m_ticksSinceSync >= 10) {
        m_ticksSinceSync = 0;
        // TODO: 同步到客户端
    }
}

void BarrelEntity::updateBlockState(IWorld& world, bool open) {
    // TODO: 更新方块的 OPEN 属性
    // BlockState state = world.getBlockState(m_pos);
    // if (state.hasProperty(BlockStateProperties::OPEN())) {
    //     world.setBlockState(m_pos, state.with(BlockStateProperties::OPEN(), open), 3);
    // }
    MC_UNUSED(world);
    MC_UNUSED(open);
}

bool BarrelEntity::load(const nlohmann::json& data) {
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    // 加载物品
    if (data.contains("items")) {
        m_inventory.load(data["items"]);
    }

    if (data.contains("open_count")) {
        m_openCount = data["open_count"].get<i32>();
    }

    return true;
}

void BarrelEntity::save(nlohmann::json& data) const {
    LockableBlockEntity::save(data);

    // 保存物品
    nlohmann::json itemsJson;
    m_inventory.save(itemsJson);
    data["items"] = itemsJson;

    data["open_count"] = m_openCount;
}

std::unique_ptr<BlockEntity> BarrelEntity::clone() const {
    auto clone = std::make_unique<BarrelEntity>(m_pos);
    clone->m_openCount = m_openCount;
    // TODO: 复制物品
    return clone;
}

} // namespace blockentity
} // namespace mc
