#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include "world/blockentity/CraftingTableEntity.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

BlockEntityRegistry& BlockEntityRegistry::instance() {
    static BlockEntityRegistry instance;
    return instance;
}

void BlockEntityRegistry::registerType(BlockEntityType type, Factory factory) {
    MC_ASSERT(factory && "Factory cannot be null");
    m_factories[type] = std::move(factory);
}

void BlockEntityRegistry::registerBuiltinTypes() {
    // 注册工作台方块实体
    registerType(BlockEntityType::CraftingTable, [](const BlockPos& pos) {
        return std::make_unique<CraftingTableEntity>(pos);
    });

    // 其他方块实体类型将在后续实现中注册：
    // registerType(BlockEntityType::Chest, [](const BlockPos& pos) {
    //     return std::make_unique<ChestEntity>(pos);
    // });
    // registerType(BlockEntityType::Furnace, [](const BlockPos& pos) {
    //     return std::make_unique<FurnaceEntity>(pos);
    // });
    // registerType(BlockEntityType::Hopper, [](const BlockPos& pos) {
    //     return std::make_unique<HopperEntity>(pos);
    // });
    // ... 等等
}

std::unique_ptr<BlockEntity> BlockEntityRegistry::create(BlockEntityType type, const BlockPos& pos) const {
    auto it = m_factories.find(type);
    if (it == m_factories.end()) {
        return nullptr;
    }
    return it->second(pos);
}

std::unique_ptr<BlockEntity> BlockEntityRegistry::createFromJson(const nlohmann::json& data) const {
    // 解析方块实体类型ID
    if (!data.contains("id") || !data["id"].is_string()) {
        return nullptr;
    }

    ResourceLocation id(data["id"].get<String>());
    BlockEntityType type = blockEntityTypeFromId(id);

    if (type == BlockEntityType::Unknown) {
        return nullptr;
    }

    // 解析位置
    BlockPos pos(0, 0, 0);
    if (data.contains("x") && data["x"].is_number()) {
        pos.x = data["x"].get<i32>();
    }
    if (data.contains("y") && data["y"].is_number()) {
        pos.y = data["y"].get<i32>();
    }
    if (data.contains("z") && data["z"].is_number()) {
        pos.z = data["z"].get<i32>();
    }

    // 创建方块实体
    auto entity = create(type, pos);
    if (!entity) {
        return nullptr;
    }

    // 加载数据
    if (!entity->load(data)) {
        return nullptr;
    }

    return entity;
}

bool BlockEntityRegistry::hasType(BlockEntityType type) const {
    return m_factories.find(type) != m_factories.end();
}

} // namespace blockentity
} // namespace mc
