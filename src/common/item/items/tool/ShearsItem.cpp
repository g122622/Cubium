#include "ShearsItem.hpp"
#include "../../../core/Types.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/item/ItemEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/interfaces/IShearable.hpp"
#include "../../../entity/utils/ItemDropHelper.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockTags.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../core/ActionResult.hpp"

namespace mc {
namespace item {
namespace tool {

ShearsItem::ShearsItem(ItemProperties properties)
    : Item(std::move(properties))
{}

f32 ShearsItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const
{
    (void)stack;

    // MC 1.16.5: 对蜘蛛网和树叶返回 15.0
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return 15.0f;
    }

    // 使用 BlockTags 检查树叶
    if (BlockTags::LEAVES().contains(state)) {
        return 15.0f;
    }

    // MC 1.16.5: 对羊毛返回 5.0
    if (BlockTags::WOOL().contains(state)) {
        return 5.0f;
    }

    // 其他方块返回基础速度
    return 1.0f;
}

bool ShearsItem::canHarvestBlock(const BlockState& state) const
{
    // MC 1.16.5: 剪刀可以采集蜘蛛网、红石线、绊线
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return true;
    }
    if (VanillaBlocks::REDSTONE_WIRE && &state.owner() == VanillaBlocks::REDSTONE_WIRE) {
        return true;
    }
    if (VanillaBlocks::TRIPWIRE && &state.owner() == VanillaBlocks::TRIPWIRE) {
        return true;
    }

    // 其他方块使用默认采集规则
    return state.getHarvestTool() == TOOL_TYPE_NONE;
}

bool ShearsItem::onBlockDestroyed(
    ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& entity)
{
    (void)world;
    (void)pos;
    (void)entity;

    // MC 1.16.5: 以下方块不消耗耐久（参考 ShearsItem.java:26）
    // 树叶、蛛网、草、蕨、枯萎灌木、藤蔓、绊线、羊毛
    if (BlockTags::LEAVES().contains(state)) {
        return true; // 树叶不消耗耐久
    }
    if (BlockTags::WOOL().contains(state)) {
        return true; // 羊毛不消耗耐久
    }

    // 检查特定方块
    const Block& block = state.owner();
    if (VanillaBlocks::COBWEB && &block == VanillaBlocks::COBWEB) {
        return true; // 蛛网不消耗耐久
    }
    if (VanillaBlocks::SHORT_GRASS && &block == VanillaBlocks::SHORT_GRASS) {
        return true; // 草不消耗耐久
    }
    if (VanillaBlocks::FERN && &block == VanillaBlocks::FERN) {
        return true; // 蕨不消耗耐久
    }
    if (VanillaBlocks::DEAD_BUSH && &block == VanillaBlocks::DEAD_BUSH) {
        return true; // 枯萎灌木不消耗耐久
    }
    if (VanillaBlocks::VINE && &block == VanillaBlocks::VINE) {
        return true; // 藤蔓不消耗耐久
    }
    if (VanillaBlocks::TRIPWIRE && &block == VanillaBlocks::TRIPWIRE) {
        return true; // 绊线不消耗耐久
    }

    // MC 1.16.5: 火方块不消耗耐久（参考 ShearsItem.java:20）
    if (BlockTags::FIRE().contains(state)) {
        return true;
    }

    // 其他硬度>0的方块消耗耐久
    if (state.hardness() > 0.0f) {
        stack.attemptDamageItem(1);
    }
    return true;
}

bool ShearsItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    MC_UNUSED(hand);

    // MC 1.16.5: ShearsItem.itemInteractionForEntity()
    // 剪刀可以剪羊毛、雪傀儡的南瓜、哞菇的蘑菇

    // 检查实体是否实现 IShearable 接口
    auto* shearable = dynamic_cast<entity::IShearable*>(&target);
    if (shearable == nullptr) {
        return false;
    }

    // 检查是否可以被剪
    if (!shearable->isShearable()) {
        return false;
    }

    // 执行剪毛
    std::vector<ItemStack> drops = shearable->shear(&player);

    // 在世界中生成掉落物
    IWorld* world = target.world();
    if (world != nullptr && !drops.empty()) {
        math::Random& rng = world->getRandom();

        for (auto& drop : drops) {
            if (!drop.isEmpty()) {
                // 使用 ItemDropHelper 统一生成物品实体
                // 参考 MC 1.16.5 ShearsItem.itemInteractionForEntity()
                ItemDropHelper::spawnItemEntity(world,
                    drop,
                    target.x(),
                    target.y() + 0.5,
                    target.z(),
                    rng,
                    ItemDropHelper::DEFAULT_PICKUP_DELAY,
                    player.uuid());
            }
        }
    }

    // 消耗剪刀耐久（非创造模式）
    if (!player.isCreative()) {
        stack.attemptDamageItem(1);
    }

    return true;
}

} // namespace tool
} // namespace item
} // namespace mc
