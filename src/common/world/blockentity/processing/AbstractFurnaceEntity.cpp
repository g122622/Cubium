#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "world/IWorld.hpp"
#include "world/World.hpp"
#include "world/block/BlockState.hpp"
#include "world/block/Block.hpp"
#include "util/assert/AssertAll.hpp"
#include <algorithm>

namespace mc {
namespace blockentity {

// ========== 燃烧时间表 ==========

namespace {
    // 燃料燃烧时间映射表（tick）
    // 参考: net.minecraft.tileentity.AbstractFurnaceTileEntity#getBurnTimes
    const std::unordered_map<u32, i32> BURN_TIMES = {
        // 煤炭类
        // { Items::COAL->getId(), 1600 },       // 煤炭
        // { Items::CHARCOAL->getId(), 1600 },   // 木炭
        // { Blocks::COAL_BLOCK->asItem()->getId(), 16000 }, // 煤炭块

        // 木制品
        // 木板、木头、木质工具等: 300 tick
        // 木棍: 100 tick
        // 树苗: 100 tick

        // 其他
        // 岩浆桶: 20000 tick
        // 烈焰棒: 2400 tick
        // 干海带块: 4001 tick
        // 竹子: 50 tick
    };
}

// ========== 构造函数 ==========

AbstractFurnaceEntity::AbstractFurnaceEntity(BlockEntityType type, const BlockPos& pos)
    : LockableBlockEntity(type, pos)
    , m_inventory([this]() { this->setChanged(); }) {
}

// ========== BlockEntity 接口 ==========

void AbstractFurnaceEntity::tick(World& world) {
    // 检查是否在燃烧
    bool wasBurning = isBurning();

    // 减少燃烧时间
    if (isBurning()) {
        --m_burnTime;
    }

    // 服务端逻辑
    // if (!world.isRemote()) { ... }

    // 获取输入物品和燃料
    ItemStack inputItem = m_inventory.getInputItem();
    ItemStack fuelItem = m_inventory.getFuelItem();

    // 检查是否可以燃烧/熔炼
    bool canSmeltNow = canSmelt(world);

    if (isBurning() || (!fuelItem.isEmpty() && canSmeltNow)) {
        // 如果没在燃烧且可以熔炼，尝试消耗燃料
        if (!isBurning() && canSmeltNow) {
            m_burnTimeTotal = getBurnTime(fuelItem);
            m_burnTime = m_burnTimeTotal;

            if (isBurning()) {
                // 消耗燃料
                burnFuel();
            }
        }

        // 如果在燃烧且可以熔炼，增加熔炼进度
        if (isBurning() && canSmeltNow) {
            ++m_cookTime;

            if (m_cookTime >= m_cookTimeTotal) {
                // 完成熔炼
                m_cookTime = 0;
                m_cookTimeTotal = getCookTime(world);
                smelt(world);
                setChanged();
            }
        } else if (!canSmeltNow) {
            // 不能熔炼时，进度回退
            m_cookTime = std::max(0, m_cookTime - 2);
        }
    } else if (!isBurning() && m_cookTime > 0) {
        // 不燃烧但有进度，进度回退
        m_cookTime = std::max(0, m_cookTime - 2);
    }

    // 如果燃烧状态改变，更新方块状态
    if (wasBurning != isBurning()) {
        updateBurnState(world);
        setChanged();
    }
}

// ========== 序列化 ==========

bool AbstractFurnaceEntity::load(const nlohmann::json& data) {
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    // 加载燃烧时间
    if (data.contains("BurnTime") && data["BurnTime"].is_number()) {
        m_burnTime = data["BurnTime"].get<i32>();
    }

    if (data.contains("CookTime") && data["CookTime"].is_number()) {
        m_cookTime = data["CookTime"].get<i32>();
    }

    if (data.contains("CookTimeTotal") && data["CookTimeTotal"].is_number()) {
        m_cookTimeTotal = data["CookTimeTotal"].get<i32>();
    }

    // 设置当前燃料的总燃烧时间
    m_burnTimeTotal = getBurnTime(m_inventory.getFuelItem());

    // TODO: 加载背包内容

    return true;
}

void AbstractFurnaceEntity::save(nlohmann::json& data) const {
    LockableBlockEntity::save(data);

    data["BurnTime"] = m_burnTime;
    data["CookTime"] = m_cookTime;
    data["CookTimeTotal"] = m_cookTimeTotal;

    // TODO: 保存背包内容
}

// ========== 熔炉状态 ==========

i32 AbstractFurnaceEntity::getComparatorSignal() const {
    // 计算填充度信号
    // 公式: signal = floor(fillRatio * 14) + (nonEmpty ? 1 : 0)
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

// ========== 静态方法 ==========

bool AbstractFurnaceEntity::isFuel(const ItemStack& stack) {
    return getBurnTime(stack) > 0;
}

i32 AbstractFurnaceEntity::getBurnTime(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return 0;
    }

    // 从燃烧时间表查找
    // const Item* item = stack.getItem();
    // if (item != nullptr) {
    //     auto it = BURN_TIMES.find(item->getId());
    //     if (it != BURN_TIMES.end()) {
    //         return it->second;
    //     }
    // }

    // TODO: 实现燃烧时间查询
    // 目前返回默认值
    return 0;
}

// ========== 保护方法 ==========

i32 AbstractFurnaceEntity::getCookTime(World& world) const {
    const crafting::SmeltingRecipe* recipe = getRecipe(world);
    if (recipe != nullptr) {
        return recipe->getCookTime();
    }
    return getDefaultCookTime();
}

bool AbstractFurnaceEntity::canSmelt(World& world) const {
    const ItemStack& input = m_inventory.getInputItem();
    if (input.isEmpty()) {
        return false;
    }

    // 获取熔炼配方
    const crafting::SmeltingRecipe* recipe = getRecipe(world);
    if (recipe == nullptr) {
        return false;
    }

    // 检查输出槽
    const ItemStack& output = m_inventory.getOutputItem();
    const ItemStack& result = recipe->getResult();

    if (output.isEmpty()) {
        return true;
    }

    // 检查是否可以堆叠
    if (!output.canStackWith(result)) {
        return false;
    }

    // 检查堆叠数量限制
    i32 resultCount = output.getCount() + result.getCount();
    return resultCount <= output.getMaxStackSize();
}

void AbstractFurnaceEntity::smelt(World& world) {
    if (!canSmelt(world)) {
        return;
    }

    const crafting::SmeltingRecipe* recipe = getRecipe(world);
    if (recipe == nullptr) {
        return;
    }

    ItemStack input = m_inventory.getInputItem();
    ItemStack result = recipe->getResult();

    // 减少输入
    input.shrink(1);
    m_inventory.setInputItem(input);

    // 增加输出
    ItemStack output = m_inventory.getOutputItem();
    if (output.isEmpty()) {
        m_inventory.setOutputItem(result.copy());
    } else {
        output.grow(result.getCount());
        m_inventory.setOutputItem(output);
    }

    // 特殊处理：湿海绵干燥
    // TODO: 实现湿海绵的特殊处理

    setChanged();
}

bool AbstractFurnaceEntity::burnFuel() {
    ItemStack fuel = m_inventory.getFuelItem();
    if (fuel.isEmpty()) {
        return false;
    }

    // 减少燃料
    fuel.shrink(1);

    // 如果燃料用完了，检查是否有容器物品（如岩浆桶）
    // TODO: 实现容器物品处理（岩浆桶 -> 空桶）

    m_inventory.setFuelItem(fuel);
    return true;
}

void AbstractFurnaceEntity::updateBurnState(World& world) {
    // 更新方块的 LIT 属性
    // const BlockState* state = world.getBlockState(getPos().x, getPos().y, getPos().z);
    // if (state != nullptr) {
    //     bool lit = isBurning();
    //     // 更新方块状态
    // }
}

const crafting::SmeltingRecipe* AbstractFurnaceEntity::getRecipe(World& world) const {
    const ItemStack& input = m_inventory.getInputItem();
    if (input.isEmpty()) {
        return nullptr;
    }

    // 从配方管理器获取熔炼配方
    // TODO: 实现配方管理器查询
    // return world.getRecipeManager().getSmeltingRecipe(input, getRecipeType());

    (void)world;
    return nullptr;
}

} // namespace blockentity
} // namespace mc
