#include "BeaconContainer.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/backend/java/mappings/JavaMobEffectIdMap.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {
class _BeaconPaymentSlot final : public Slot {
public:
    using Slot::Slot;
    bool mayPlace(const ItemStack& stack) const override
    {
        return item::tag::ItemTags::BEACON_PAYMENT_ITEMS().contains(stack);
    }
    i32 getMaxStackSize() const override { return 1; }
    i32 getMaxStackSize(const ItemStack&) const override { return 1; }
};

i32 _encodeEffect(const entity::effect::EffectType* effect)
{
    return effect == nullptr
        ? 0
        : static_cast<i32>(network::backend::java::JavaMobEffectIdMap::instance().toJavaRegistryId(*effect)) + 1;
}
} // namespace

BeaconContainer::BeaconContainer(
    ContainerId id, PlayerInventory* inventory, IWorld& world, blockentity::BeaconEntity& beacon)
    : AbstractContainerMenu(id, inventory)
    , m_payment(1)
    , m_world(world)
    , m_beacon(beacon)
    , m_position(beacon.getPos())
{
    addSlot(std::make_unique<_BeaconPaymentSlot>(&m_payment, 0, 136, 110));
    addPlayerInventorySlots(36, 137);
    addPlayerHotbarSlots(36, 195);
    trackInt([this] { return m_beacon.getLevel(); }, [this](i32 value) { m_beacon.setLevel(value); });
    trackInt([this] { return _encodeEffect(m_beacon.getPrimaryEffect()); },
        [this](i32 value) {
            const auto effect = value > 0 && value <= 40
                ? std::optional<entity::effect::EffectType>(
                      network::backend::java::JavaMobEffectIdMap::instance().fromJavaRegistryId(
                          static_cast<u32>(value - 1)))
                : std::nullopt;
            m_beacon.setPrimaryEffect(effect ? &*effect : nullptr);
        });
    trackInt([this] { return _encodeEffect(m_beacon.getSecondaryEffect()); },
        [this](i32 value) {
            const auto effect = value > 0 && value <= 40
                ? std::optional<entity::effect::EffectType>(
                      network::backend::java::JavaMobEffectIdMap::instance().fromJavaRegistryId(
                          static_cast<u32>(value - 1)))
                : std::nullopt;
            m_beacon.setSecondaryEffect(effect ? &*effect : nullptr);
        });
}

bool BeaconContainer::stillValid(const Player& player) const
{
    const auto* state = m_world.getBlockState(m_position);
    return player.world() == &m_world && isWithinDistance(player, m_position) && state != nullptr &&
        &state->getBlock() == VanillaBlocks::BEACON && m_world.getBlockEntity(m_position) == &m_beacon;
}

void BeaconContainer::removed(Player& player)
{
    AbstractContainerMenu::removed(player);
    const auto payment = m_payment.removeItemNoUpdate(0);
    if (!payment.isEmpty()) dropItem(payment, player, false);
}

ItemStack BeaconContainer::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;
    auto* slot = getSlot(slotIndex);
    if (slot == nullptr || !slot->hasItem()) return ItemStack::EMPTY;
    auto stack = slot->getItem();
    const auto original = stack.copy();
    if (slotIndex == 0) {
        if (!moveItemToRange(stack, 1, 37, true)) return ItemStack::EMPTY;
    } else if (!getSlot(0)->hasItem() && getSlot(0)->mayPlace(stack) && stack.getCount() == 1) {
        if (!moveItemToRange(stack, 0, 1, false)) return ItemStack::EMPTY;
    } else if (slotIndex < 28) {
        if (!moveItemToRange(stack, 28, 37, false)) return ItemStack::EMPTY;
    } else if (!moveItemToRange(stack, 1, 28, false))
        return ItemStack::EMPTY;
    slot->set(stack);
    return original;
}

bool BeaconContainer::updateEffects(
    std::optional<entity::effect::EffectType> primary, std::optional<entity::effect::EffectType> secondary)
{
    const auto payment = m_payment.getItem(0);
    if (payment.isEmpty() || !getSlot(0)->mayPlace(payment)) return false;
    if (primary && !m_beacon.canUseEffect(&*primary)) primary.reset();
    if (secondary && !m_beacon.canUseEffect(&*secondary)) secondary.reset();
    m_beacon.setPrimaryEffect(primary ? &*primary : nullptr);
    m_beacon.setSecondaryEffect(secondary ? &*secondary : nullptr);
    static_cast<void>(m_payment.removeItem(0, 1));
    m_beacon.setChanged();
    if (m_beacon.hasBeam())
        m_world.playSound(
            SoundEvents::BLOCK_BEACON_POWER_SELECT, sound::SoundCategory::Blocks, m_position.center(), 1.0f, 1.0f);
    detectAndSendChanges();
    return true;
}

} // namespace mc
