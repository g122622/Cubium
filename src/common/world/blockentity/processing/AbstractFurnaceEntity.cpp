#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "item/Items.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/sound/SoundCategory.hpp"
#include "util/assert/AssertAll.hpp"
#include <algorithm>

namespace mc {
namespace blockentity {

namespace {

/// 火苗噼啪声概率 (MC 1.16.5: 1/20 per tick when burning)
constexpr i32 FIRE_CRACKLE_CHANCE = 20;

/**
 * @brief 获取指定物品的燃烧时间。
 *
 * 参考: MC 1.16.5 AbstractFurnaceTileEntity 燃烧时间表
 * 燃烧时间单位：tick
 *
 * 注意：部分物品（煤炭块、岩浆桶等）尚未在 Items 中注册，
 * 待相关物品添加后需补充。
 */
[[nodiscard]] i32 getBurnTimeByItem(const Item* item) {
        if (item == nullptr) {
            return 0;
        }

        // ========== 燃料（高燃烧值）==========
        // 烈焰棒: 2400 tick (120 秒)
        if (item == Items::BLAZE_ROD) {
            return 2400;
        }

        // ========== 煤炭类 ==========
        // 煤炭/木炭: 1600 tick (80 秒)
        if (item == Items::COAL || item == Items::CHARCOAL) {
            return 1600;
        }
        // TODO: 煤炭块 (COAL_BLOCK) - 16000 tick (800 秒) - 待物品注册

        // ========== 木头类 (300 tick = 15 秒) ==========
        // 原木
        if (item == Items::OAK_LOG || item == Items::SPRUCE_LOG ||
            item == Items::BIRCH_LOG || item == Items::JUNGLE_LOG ||
            item == Items::ACACIA_LOG || item == Items::DARK_OAK_LOG) {
            return 300;
        }
        // 木板
        if (item == Items::OAK_PLANKS || item == Items::SPRUCE_PLANKS ||
            item == Items::BIRCH_PLANKS || item == Items::JUNGLE_PLANKS ||
            item == Items::ACACIA_PLANKS || item == Items::DARK_OAK_PLANKS) {
            return 300;
        }
        // TODO: 木质楼梯、木质门、栅栏、书架、音符盒、箱子等 - 300 tick

        // ========== 木制工具 (200 tick = 10 秒) ==========
        if (item == Items::WOODEN_PICKAXE || item == Items::WOODEN_AXE ||
            item == Items::WOODEN_SHOVEL || item == Items::WOODEN_HOE ||
            item == Items::WOODEN_SWORD) {
            return 200;
        }

        // ========== 木棍、树苗、碗 (100 tick = 5 秒) ==========
        if (item == Items::STICK) {
            return 100;
        }
        // TODO: 树苗 (SAPLING) - 100 tick - 待物品注册
        // TODO: 碗 (BOWL) - 100 tick - 待物品注册

        // ========== 其他木制品 ==========
        // TODO: 木船 (BOAT) - 1200 tick (60 秒) - 待物品注册
        // TODO: 羊毛 (WOOL) - 100 tick - 待物品注册
        // TODO: 地毯 (CARPET) - 67 tick - 待物品注册
        // TODO: 竹子 (BAMBOO) - 50 tick - 待物品注册
        // TODO: 脚手架 (SCAFFOLDING) - 400 tick (20 秒) - 待物品注册
        // TODO: 干海带块 (DRIED_KELP_BLOCK) - 4001 tick - 待物品注册

        // ========== 特殊燃料 ==========
        // 岩浆桶: 20000 tick (1000 秒) - 燃烧后返回空桶
        // 参考: MC 1.16.5 AbstractFurnaceTileEntity.getBurnTimes() 第 97 行
        if (item == Items::LAVA_BUCKET) {
            return 20000;
        }

        return 0;
    }
}

AbstractFurnaceEntity::AbstractFurnaceEntity(BlockEntityType type, const BlockPos& pos)
    : LockableBlockEntity(type, pos)
    , m_inventory([this]() { this->BlockEntity::setChanged(); }) {
}

void AbstractFurnaceEntity::tick(IWorld& world) {
    bool wasBurning = isBurning();

    if (isBurning()) {
        --m_burnTime;

        // MC 1.16.5: 火苗噼啪声 - 燃烧时随机播放
        // 参考: AbstractFurnaceTileEntity.tick() 中 world.playSound 调用
        if (!world.isClientSide() && world.getRandom().nextInt(FIRE_CRACKLE_CHANCE) == 0) {
            ResourceLocation soundEvent = getFireCrackleSound();
            world.playSound(
                soundEvent,
                sound::SoundCategory::Blocks,
                m_pos.center(),
                1.0f,
                1.0f
            );
        }
    }

    ItemStack fuelItem = m_inventory.getFuelItem();

    const crafting::SmeltingRecipe* cachedRecipe = getRecipe(world);
    bool canSmeltNow = canSmeltWithRecipe(cachedRecipe);

    if (isBurning() || (!fuelItem.isEmpty() && canSmeltNow)) {
        if (!isBurning() && canSmeltNow) {
            m_burnTimeTotal = getBurnTimeForFuel(fuelItem);
            m_burnTime = m_burnTimeTotal;

            if (isBurning()) {
                burnFuel();
            }
        }

        if (isBurning() && canSmeltNow) {
            ++m_cookTime;

            if (m_cookTime >= m_cookTimeTotal) {
                m_cookTime = 0;
                m_cookTimeTotal = getCookTimeFromRecipe(cachedRecipe);
                smeltWithRecipe(cachedRecipe);
                BlockEntity::setChanged();
            }
        } else if (!canSmeltNow) {
            m_cookTime = std::max(0, m_cookTime - 2);
        }
    } else if (!isBurning() && m_cookTime > 0) {
        m_cookTime = std::max(0, m_cookTime - 2);
    }

    if (wasBurning != isBurning()) {
        updateBurnState(world);
        setChanged();
    }
}

bool AbstractFurnaceEntity::load(const nlohmann::json& data) {
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    if (data.contains("BurnTime") && data["BurnTime"].is_number()) {
        m_burnTime = data["BurnTime"].get<i32>();
    }

    if (data.contains("CookTime") && data["CookTime"].is_number()) {
        m_cookTime = data["CookTime"].get<i32>();
    }

    if (data.contains("CookTimeTotal") && data["CookTimeTotal"].is_number()) {
        m_cookTimeTotal = data["CookTimeTotal"].get<i32>();
    }

    if (data.contains("StoredExperience") && data["StoredExperience"].is_number()) {
        m_storedExperience = data["StoredExperience"].get<f32>();
    }

    m_burnTimeTotal = getBurnTimeForFuel(m_inventory.getFuelItem());

    if (data.contains("Items") && data["Items"].is_array()) {
        m_inventory.clear();
        for (const auto& itemJson : data["Items"]) {
            if (!itemJson.is_object()) {
                continue;
            }

            const i32 slot = itemJson.value("Slot", -1);
            if (slot < SLOT_INPUT || slot > SLOT_OUTPUT) {
                continue;
            }

            auto stackResult = ItemStack::fromJson(itemJson);
            if (!stackResult.success()) {
                continue;
            }

            m_inventory.setItem(slot, stackResult.value());
        }

        m_burnTimeTotal = getBurnTimeForFuel(m_inventory.getFuelItem());
    }

    return true;
}

void AbstractFurnaceEntity::save(nlohmann::json& data) const {
    LockableBlockEntity::save(data);

    data["BurnTime"] = m_burnTime;
    data["CookTime"] = m_cookTime;
    data["CookTimeTotal"] = m_cookTimeTotal;
    data["StoredExperience"] = m_storedExperience;

    nlohmann::json itemsJson = nlohmann::json::array();
    for (i32 slot = SLOT_INPUT; slot <= SLOT_OUTPUT; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (stack.isEmpty()) {
            continue;
        }

        nlohmann::json itemJson = stack.toJson();
        itemJson["Slot"] = slot;
        itemsJson.push_back(std::move(itemJson));
    }
    data["Items"] = std::move(itemsJson);
}

i32 AbstractFurnaceEntity::getComparatorSignal() const {
    i32 totalItems = 0;
    i32 totalSlots = 3;
    i32 maxPerSlot = 64;

    for (i32 i = 0; i < totalSlots; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            totalItems += stack.getCount();
        }
    }

    i32 maxItems = totalSlots * maxPerSlot;
    if (maxItems == 0) {
        return 0;
    }

    f32 fillRatio = static_cast<f32>(totalItems) / static_cast<f32>(maxItems);
    i32 signal = static_cast<i32>(fillRatio * 14.0f);

    if (totalItems > 0) {
        signal += 1;
    }

    return std::min(signal, 15);
}

bool AbstractFurnaceEntity::isFuel(const ItemStack& stack) {
    return getBurnTime(stack) > 0;
}

i32 AbstractFurnaceEntity::getBurnTime(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return 0;
    }

    return getBurnTimeByItem(stack.getItem());
}

i32 AbstractFurnaceEntity::getCookTime(IWorld& world) const {
    const crafting::SmeltingRecipe* recipe = getRecipe(world);
    return getCookTimeFromRecipe(recipe);
}

i32 AbstractFurnaceEntity::getCookTimeFromRecipe(const crafting::SmeltingRecipe* recipe) const {
    if (recipe != nullptr) {
        return recipe->getCookTime();
    }
    return getDefaultCookTime();
}

bool AbstractFurnaceEntity::canSmelt(IWorld& world) const {
    return canSmeltWithRecipe(getRecipe(world));
}

bool AbstractFurnaceEntity::canSmeltWithRecipe(const crafting::SmeltingRecipe* recipe) const {
    const ItemStack& input = m_inventory.getInputItem();
    if (input.isEmpty()) {
        return false;
    }

    if (recipe == nullptr) {
        return false;
    }

    const ItemStack& output = m_inventory.getOutputItem();
    const ItemStack& result = recipe->getResultItem();

    if (output.isEmpty()) {
        return true;
    }

    if (!output.canStackWith(result)) {
        return false;
    }

    i32 resultCount = output.getCount() + result.getCount();
    return resultCount <= output.getMaxStackSize();
}

void AbstractFurnaceEntity::smelt(IWorld& world) {
    smeltWithRecipe(getRecipe(world));
}

void AbstractFurnaceEntity::smeltWithRecipe(const crafting::SmeltingRecipe* recipe) {
    if (!canSmeltWithRecipe(recipe)) {
        return;
    }

    MC_ASSERT(recipe != nullptr);

    ItemStack input = m_inventory.getInputItem();
    ItemStack result = recipe->getResultItem().copy();

    input.shrink(1);
    m_inventory.setInputItem(input);

    ItemStack output = m_inventory.getOutputItem();
    if (output.isEmpty()) {
        m_inventory.setOutputItem(result.copy());
    } else {
        output.grow(result.getCount());
        m_inventory.setOutputItem(output);
    }

    // MC 1.16.5: 湿海绵干燥逻辑
    // 当输入是湿海绵且燃料槽有空桶时，将空桶变为水桶
    // 参考: AbstractFurnaceTileEntity.smelt() 第 310-313 行
    if (input.getItem() == Items::WET_SPONGE) {
        ItemStack fuelItem = m_inventory.getFuelItem();
        if (!fuelItem.isEmpty() && fuelItem.getItem() == Items::BUCKET) {
            m_inventory.setFuelItem(ItemStack(Items::WATER_BUCKET, 1));
        }
    }

    m_storedExperience += recipe->getExperience();

    setChanged();
}

bool AbstractFurnaceEntity::burnFuel() {
    ItemStack fuel = m_inventory.getFuelItem();
    if (fuel.isEmpty()) {
        return false;
    }

    // MC 1.16.5: 容器物品消耗逻辑
    // 参考: AbstractFurnaceTileEntity.tick() 第 234-243 行
    if (fuel.hasContainerItem()) {
        // 有容器物品的情况（如岩浆桶），直接用容器物品替换燃料槽
        m_inventory.setFuelItem(fuel.getContainerItem());
    } else {
        // 没有容器物品的情况，减少燃料数量
        fuel.shrink(1);
        m_inventory.setFuelItem(fuel);
    }

    return true;
}

void AbstractFurnaceEntity::updateBurnState(IWorld& world) {
    (void)world;
}

const crafting::SmeltingRecipe* AbstractFurnaceEntity::getRecipe(IWorld& world) const {
    (void)world;

    const ItemStack& input = m_inventory.getInputItem();
    if (input.isEmpty()) {
        return nullptr;
    }

    return crafting::RecipeManager::instance().getSmeltingRecipe(input, getRecipeType());
}

// ========== ISidedInventory 接口 ==========

std::vector<i32> AbstractFurnaceEntity::getSlotsForFace(Direction side) const {
    // 参考 MC 1.16.5 AbstractFurnaceTileEntity:
    // SLOTS_UP = new int[]{0}          - 上方只能访问输入槽
    // SLOTS_DOWN = new int[]{2, 1}     - 下方可以访问输出槽和燃料槽
    // SLOTS_HORIZONTAL = new int[]{1}  - 侧面只能访问燃料槽

    switch (side) {
        case Direction::Up:
            // 上方：输入槽
            return {SLOT_INPUT};
        case Direction::Down:
            // 下方：输出槽、燃料槽
            return {SLOT_OUTPUT, SLOT_FUEL};
        default:
            // 侧面（北、南、东、西）：燃料槽
            return {SLOT_FUEL};
    }
}

bool AbstractFurnaceEntity::isSlotAccessibleForDirection(i32 slot, Direction direction) const {
    const std::vector<i32> accessibleSlots = getSlotsForFace(direction);
    for (i32 accessibleSlot : accessibleSlots) {
        if (accessibleSlot == slot) {
            return true;
        }
    }
    return false;
}

bool AbstractFurnaceEntity::canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const {
    // MC 1.16.5: 参考 AbstractFurnaceTileEntity.isItemValidForSlot() 第 437-445 行
    // 和 canInsertItem() 第 347-349 行

    // 首先检查方向是否允许访问该槽位
    if (!isSlotAccessibleForDirection(slot, direction)) {
        return false;
    }

    // 根据槽位类型检查物品是否有效
    switch (slot) {
        case SLOT_INPUT:
            // 输入槽：接受任何物品（熔炼配方检查在 tick 中进行）
            return true;
        case SLOT_FUEL: {
            // 燃料槽：接受燃料
            if (isFuel(stack)) {
                return true;
            }
            // 或者如果燃料槽当前不是空桶则接受空桶（用于装岩浆桶燃烧后的空桶）
            // 参考: isFuel(stack) || stack.getItem() == Items.BUCKET && itemstack.getItem() != Items.BUCKET
            if (stack.getItem() == Items::BUCKET) {
                const ItemStack fuelItem = m_inventory.getFuelItem();
                return fuelItem.isEmpty() || fuelItem.getItem() != Items::BUCKET;
            }
            return false;
        }
        case SLOT_OUTPUT:
            // 输出槽：不允许插入（只能提取）
            return false;
        default:
            return false;
    }
}

bool AbstractFurnaceEntity::canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const {
    // MC 1.16.5: 参考 AbstractFurnaceTileEntity.canExtractItem() 第 354-363 行
    // 从下方提取燃料槽时，只允许提取空桶或水桶
    // 这是为了防止漏斗从燃料槽中提取部分使用的燃料（如岩浆桶）
    if (direction == Direction::Down && slot == SLOT_FUEL) {
        const Item* item = stack.getItem();
        // 只允许提取空桶和水桶（湿海绵干燥产生的水桶）
        return item == Items::BUCKET || item == Items::WATER_BUCKET;
    }

    return true;
}

} // namespace blockentity
} // namespace mc