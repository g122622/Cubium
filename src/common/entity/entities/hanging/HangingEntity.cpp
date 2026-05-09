#include "HangingEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../util/Direction.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../entity/utils/ItemDropHelper.hpp"
#include "../player/Player.hpp"
#include "../item/ItemEntity.hpp"
#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include <cmath>
#include <algorithm>

namespace mc {
namespace entity {

// ==================== HangingEntity ====================

HangingEntity::HangingEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

HangingEntity::HangingEntity(BlockPos pos, Direction direction)
    : Entity(LegacyEntityType::Unknown, EntityId(0))
    , m_hangingPos(pos)
    , m_direction(direction)
{
    updateBoundingBox();
}

void HangingEntity::tick() {
    Entity::tick();

    // 定期检查悬挂位置是否有效
    if (++m_checkInterval >= CHECK_INTERVAL) {
        m_checkInterval = 0;
        if (!isValidPosition()) {
            dropItem();
            remove();
        }
    }
}

void HangingEntity::setHangingPosition(BlockPos pos, Direction direction) {
    m_hangingPos = pos;
    m_direction = direction;
    updateBoundingBox();
}

bool HangingEntity::isValidPosition() const {
    if (!canPlaceOn()) {
        return false;
    }
    return true;
}

bool HangingEntity::canPlaceOn() const {
    // MC 1.16.5: AbstractDecorationEntity.canPlaceOn()
    // 检查背后的方块是否可以支撑悬挂实体
    if (m_world == nullptr) {
        return false;
    }

    // 获取悬挂方向对应的 MC Direction
    // HangingEntity::Direction: SOUTH=0, WEST=1, NORTH=2, EAST=3
    // 悬挂实体面向 SOUTH 时，背面是 NORTH，需要检查 NORTH 面是否可依附
    mc::Direction attachDir;
    switch (m_direction) {
        case Direction::SOUTH:
            attachDir = mc::Direction::North;  // 面向南方，背面是北方
            break;
        case Direction::WEST:
            attachDir = mc::Direction::East;   // 面向西方，背面是东方
            break;
        case Direction::NORTH:
            attachDir = mc::Direction::South;  // 面向北方，背面是南方
            break;
        case Direction::EAST:
            attachDir = mc::Direction::West;   // 面向东方，背面是西方
            break;
        default:
            return false;
    }

    // 计算支撑方块的位置（悬挂位置的背后）
    BlockPos attachPos = m_hangingPos.offset(attachDir);

    // 检查支撑方块是否有足够的固体面
    // 使用 Direction::opposite 获取我们面对的方向
    mc::Direction solidCheckDir = Directions::opposite(attachDir);
    return Block::hasEnoughSolidSide(*m_world, attachPos, solidCheckDir);
}

void HangingEntity::onAttacked(Entity* attacker, f32 damage) {
    dropItem();
    remove();
}

void HangingEntity::updateBoundingBox() {
    // 根据方向和尺寸更新边界框
    // 设置位置
    setPosition(
        static_cast<f64>(m_hangingPos.x) + 0.5,
        static_cast<f64>(m_hangingPos.y) + 0.5,
        static_cast<f64>(m_hangingPos.z) + 0.5
    );

    // 设置朝向
    switch (m_direction) {
        case Direction::SOUTH: setRotation(0.0f, 0.0f); break;
        case Direction::WEST: setRotation(90.0f, 0.0f); break;
        case Direction::NORTH: setRotation(180.0f, 0.0f); break;
        case Direction::EAST: setRotation(270.0f, 0.0f); break;
    }
}

// ==================== PaintingEntity ====================

const std::vector<PaintingEntity::PaintingType> PaintingEntity::PAINTING_TYPES = {
    {"Kebab", 1, 1}, {"Aztec", 1, 1}, {"Alban", 1, 1}, {"Aztec2", 1, 1},
    {"Bomb", 1, 1}, {"Plant", 1, 1}, {"Wasteland", 1, 1}, {"Pool", 2, 1},
    {"Courbet", 2, 1}, {"Sea", 2, 1}, {"Sunset", 2, 1}, {"Creebet", 2, 1},
    {"Wanderer", 1, 2}, {"Graham", 1, 2}, {"Match", 2, 2}, {"Bust", 2, 2},
    {"Stage", 2, 2}, {"Void", 2, 2}, {"SkullAndRoses", 2, 2}, {"Wither", 2, 2},
    {"Fighters", 4, 2}, {"Skeleton", 4, 3}, {"DonkeyKong", 4, 3},
    {"Pointer", 4, 4}, {"Pigscene", 4, 4}, {"BurningSkull", 4, 4},
    {"Skeleton2", 3, 4}, {"Bust2", 3, 4}
};

PaintingEntity::PaintingEntity()
    : HangingEntity()
{
}

PaintingEntity::PaintingEntity(BlockPos pos, Direction direction, const std::string& motive)
    : HangingEntity(pos, direction)
{
    setMotive(motive);
}

void PaintingEntity::dropItem() {
    // MC 1.16.5: PaintingEntity.dropItem()
    // 生成画作物品
    if (m_world == nullptr) {
        return;
    }

    // 创建画作物品堆
    if (Items::PAINTING != nullptr) {
        ItemStack stack(*Items::PAINTING, 1);
        math::Random& rng = m_world->getRandom();
        ItemDropHelper::spawnItemEntity(
            m_world,
            stack,
            x(), y(), z(),
            rng,
            ItemDropHelper::DEFAULT_PICKUP_DELAY
        );
    }
}

i32 PaintingEntity::getWidth() const {
    for (const auto& type : PAINTING_TYPES) {
        if (type.name == m_motive) {
            return type.width;
        }
    }
    return 1;
}

i32 PaintingEntity::getHeight() const {
    for (const auto& type : PAINTING_TYPES) {
        if (type.name == m_motive) {
            return type.height;
        }
    }
    return 1;
}

void PaintingEntity::setMotive(const std::string& motive) {
    m_motive = motive;
    updateBoundingBox();
}

// ==================== ItemFrameEntity ====================

ItemFrameEntity::ItemFrameEntity()
    : HangingEntity()
{
}

ItemFrameEntity::ItemFrameEntity(BlockPos pos, Direction direction)
    : HangingEntity(pos, direction)
{
}

void ItemFrameEntity::tick() {
    HangingEntity::tick();

    if (m_item) {
        if (!m_item->isAlive()) {
            m_item = nullptr;
        }
    }
}

void ItemFrameEntity::dropItem() {
    // 掉落展示框物品
    if (m_item) {
        m_item = nullptr;
    }
}

void ItemFrameEntity::setItem(const ItemEntity& item) {
    m_rotation = 0;
}

void ItemFrameEntity::setItemRotation(i32 rotation) {
    m_rotation = rotation % 8;
}

void ItemFrameEntity::rotateItem() {
    m_rotation = (m_rotation + 1) % 8;
}

// ==================== LeashKnotEntity ====================

LeashKnotEntity::LeashKnotEntity()
    : HangingEntity()
{
}

LeashKnotEntity::LeashKnotEntity(BlockPos pos, Direction direction)
    : HangingEntity(pos, direction)
{
}

void LeashKnotEntity::tick() {
    HangingEntity::tick();

    // 检查绑定的实体
    for (auto it = m_leashedEntities.begin(); it != m_leashedEntities.end(); ) {
        if (!(*it)->isAlive()) {
            it = m_leashedEntities.erase(it);
        } else {
            ++it;
        }
    }

    // 如果没有绑定的实体，移除自己
    if (m_leashedEntities.empty()) {
        dropItem();
        remove();
    }
}

void LeashKnotEntity::dropItem() {
    // MC 1.16.5: LeashKnotEntity.dropItem()
    // 掉落拴绳物品
    if (m_world == nullptr) {
        return;
    }

    // 创建拴绳物品堆
    if (Items::LEAD != nullptr) {
        ItemStack stack(*Items::LEAD, 1);
        math::Random& rng = m_world->getRandom();
        ItemDropHelper::spawnItemEntity(
            m_world,
            stack,
            x(), y(), z(),
            rng,
            ItemDropHelper::DEFAULT_PICKUP_DELAY
        );
    }
}

void LeashKnotEntity::attachLeash(Entity* entity) {
    if (entity && std::find(m_leashedEntities.begin(), m_leashedEntities.end(), entity) == m_leashedEntities.end()) {
        m_leashedEntities.push_back(entity);
    }
}

void LeashKnotEntity::detachLeash(Entity* entity) {
    auto it = std::find(m_leashedEntities.begin(), m_leashedEntities.end(), entity);
    if (it != m_leashedEntities.end()) {
        m_leashedEntities.erase(it);
    }
}

} // namespace entity
} // namespace mc
