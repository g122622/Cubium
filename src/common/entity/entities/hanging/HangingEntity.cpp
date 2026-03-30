#include "HangingEntity.hpp"
#include "../../world/IWorld.hpp"
#include "../../entities/player/Player.hpp"
#include "../../entities/item/ItemEntity.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ==================== HangingEntity ====================

HangingEntity::HangingEntity()
    : Entity()
{
    setMaxHealth(1.0f);
}

HangingEntity::HangingEntity(BlockPos pos, Direction direction)
    : Entity()
    , m_hangingPos(pos)
    , m_direction(direction)
{
    setMaxHealth(1.0f);
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

void HangingEntity::onEntityCollision(Entity& other) {
    // 悬挂实体不能被推动
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

    // 检查范围内是否有足够空间
    // TODO: 根据宽高检查方块

    return true;
}

bool HangingEntity::canPlaceOn() const {
    // TODO: 检查背后的方块是否可以支撑
    // BlockState* block = world->getBlockState(m_hangingPos);
    // return block && block->isSolid();
    return true;
}

void HangingEntity::onAttacked(Entity* attacker, f32 damage) {
    // 悬挂实体一击即碎
    dropItem();
    remove();
}

void HangingEntity::updateBoundingBox() {
    // 根据方向和尺寸更新边界框
    f32 width = static_cast<f32>(getWidth());
    f32 height = static_cast<f32>(getHeight());

    // 设置位置
    setPosition(
        static_cast<f64>(m_hangingPos.x()) + 0.5,
        static_cast<f64>(m_hangingPos.y()) + 0.5,
        static_cast<f64>(m_hangingPos.z()) + 0.5
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
    // TODO: 生成画作物品
    // ItemEntity* item = spawnItem(Items::PAINTING);
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

    // 物品展示框的额外逻辑
    if (m_item) {
        // 检查物品是否有效
        if (!m_item->isAlive()) {
            m_item = nullptr;
        }
    }
}

void ItemFrameEntity::dropItem() {
    // 掉落展示框物品
    // ItemEntity* frame = spawnItem(Items::ITEM_FRAME);

    // 掉落展示的物品
    if (m_item) {
        // spawnItem(m_item->getItemStack());
        m_item = nullptr;
    }
}

void ItemFrameEntity::setItem(const ItemEntity& item) {
    // TODO: 复制物品到展示框
    // m_item = item.copy();
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
    // TODO: 掉落拴绳物品
    // ItemEntity* item = spawnItem(Items::LEAD);
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
