/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PlayerSaveData.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include <cstdlib>
#include <exception>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <zconf.h>
#include <zlib.h>

namespace mc {
namespace world::storage {

// ============================================================================
// NBT 常量键名
// ============================================================================

namespace nbt_keys {
// 基本属性
constexpr const char* UUID = "UUID";
constexpr const char* UUID_MOST = "UUIDMost";
constexpr const char* UUID_LEAST = "UUIDLeast";
constexpr const char* NAME = "Name";

// 位置
constexpr const char* POS_X = "PosX";
constexpr const char* POS_Y = "PosY";
constexpr const char* POS_Z = "PosZ";
constexpr const char* POS = "Pos";
constexpr const char* ROTATION = "Rotation";
constexpr const char* YAW = "Yaw";
constexpr const char* PITCH = "Pitch";

// 维度
constexpr const char* DIMENSION = "Dimension";
constexpr const char* SPAWN_X = "SpawnX";
constexpr const char* SPAWN_Y = "SpawnY";
constexpr const char* SPAWN_Z = "SpawnZ";
constexpr const char* SPAWN_FORCED = "SpawnForced";
constexpr const char* SPAWN_DIM = "SpawnDimension";
constexpr const char* ENTERED_NETHER_POS = "EnteredNetherPosition";
constexpr const char* LAST_DEATH_LOCATION = "LastDeathLocation";
constexpr const char* LDL_DIMENSION = "dimension";
constexpr const char* LDL_POS = "pos";

// 游戏模式
constexpr const char* PLAYER_GAME_TYPE = "playerGameType";
constexpr const char* FLYING = "flying";

// 生命值
constexpr const char* HEALTH = "Health";
constexpr const char* MAX_HEALTH = "MaxHealth";
constexpr const char* ABSORPTION = "AbsorptionAmount";

// 饥饿值
constexpr const char* FOOD_LEVEL = "foodLevel";
constexpr const char* FOOD_SATURATION = "foodSaturationLevel";
constexpr const char* FOOD_EXHAUSTION = "foodExhaustionLevel";
constexpr const char* FOOD_TICK_TIMER = "foodTickTimer";

// 经验
constexpr const char* XP_LEVEL = "XpLevel";
constexpr const char* XP_PROGRESS = "XpP";
constexpr const char* XP_TOTAL = "XpTotal";
constexpr const char* XP_SEED = "XpSeed";

// 能力
constexpr const char* INVULNERABLE = "Invulnerable";
constexpr const char* MAY_FLY = "MayFly";
constexpr const char* FLY_SPEED = "flySpeed";
constexpr const char* WALK_SPEED = "walkSpeed";
constexpr const char* ABILITIES = "abilities";

// 背包
constexpr const char* INVENTORY = "Inventory";
constexpr const char* SELECTED_ITEM_SLOT = "SelectedItemSlot";
constexpr const char* CARRIED_ITEM = "CarriedItem";

// 效果
constexpr const char* ACTIVE_EFFECTS = "ActiveEffects";
constexpr const char* EFFECT_ID = "Id";
constexpr const char* EFFECT_AMPLIFIER = "Amplifier";
constexpr const char* EFFECT_DURATION = "Duration";
constexpr const char* EFFECT_AMBIENT = "Ambient";
constexpr const char* EFFECT_SHOW_PARTICLES = "ShowParticles";
constexpr const char* EFFECT_SHOW_ICON = "ShowIcon";

// 空气
constexpr const char* AIR = "Air";
constexpr const char* MAX_AIR = "MaxAir";

// 睡眠
constexpr const char* SLEEPING = "Sleeping";
constexpr const char* SLEEP_TIMER = "SleepTimer";
constexpr const char* BED_POSITION = "BedPosition";

// 其他
constexpr const char* ON_GROUND = "OnGround";
constexpr const char* SPRINTING = "Sprinting";
constexpr const char* SNEAKING = "Sneaking";
} // namespace nbt_keys

// ============================================================================
// 辅助函数：安全获取NBT值
// ============================================================================

namespace {

/**
 * @brief 安全获取 compound_tag 中的 int 值
 */
std::optional<i32> tryGetInt(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Int) {
        return dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
    }
    return std::nullopt;
}

/**
 * @brief 安全获取 compound_tag 中的 float 值
 */
std::optional<f32> tryGetFloat(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Float) {
        return dynamic_cast<const nbt::tags::float_tag&>(*it->second).value;
    }
    return std::nullopt;
}

/**
 * @brief 安全获取 compound_tag 中的 double 值
 */
std::optional<f64> tryGetDouble(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Double) {
        return dynamic_cast<const nbt::tags::double_tag&>(*it->second).value;
    }
    return std::nullopt;
}

/**
 * @brief 安全获取 compound_tag 中的 byte 值
 */
std::optional<i8> tryGetByte(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Byte) {
        return dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value;
    }
    return std::nullopt;
}

/**
 * @brief 安全获取 compound_tag 中的 string 值
 */
std::optional<std::string> tryGetString(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::String) {
        return dynamic_cast<const nbt::tags::string_tag&>(*it->second).value;
    }
    return std::nullopt;
}

/**
 * @brief 安全获取 compound_tag 中的 bool 值（存储为 byte）
 */
std::optional<bool> tryGetBool(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto byteVal = tryGetByte(tag, key);
    return byteVal.has_value() ? std::optional<bool>(*byteVal != 0) : std::nullopt;
}

/**
 * @brief 安全获取 compound_tag 中的 compound_tag 指针
 */
const nbt::tags::compound_tag* tryGetCompound(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Compound) {
        return &dynamic_cast<const nbt::tags::compound_tag&>(*it->second);
    }
    return nullptr;
}

/**
 * @brief 安全获取 compound_tag 中的 list_tag 指针
 */
const nbt::tags::list_tag* tryGetList(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::List) {
        return &dynamic_cast<const nbt::tags::list_tag&>(*it->second);
    }
    return nullptr;
}

} // anonymous namespace

// ============================================================================
// 序列化
// ============================================================================

nbt::tags::compound_tag PlayerSaveData::toNbt() const
{
    nbt::tags::compound_tag tag;

    // 基本信息
    tag.put(nbt_keys::UUID, uuid);
    tag.put(nbt_keys::NAME, username);

    // 位置
    {
        nbt::tags::double_list_tag posList;
        posList.value.reserve(3);
        posList.value.push_back(posX);
        posList.value.push_back(posY);
        posList.value.push_back(posZ);
        tag.value.emplace(nbt_keys::POS, std::make_unique<nbt::tags::double_list_tag>(std::move(posList)));
    }

    // 旋转
    {
        nbt::tags::float_list_tag rotList;
        rotList.value.reserve(2);
        rotList.value.push_back(yaw);
        rotList.value.push_back(pitch);
        tag.value.emplace(nbt_keys::ROTATION, std::make_unique<nbt::tags::float_list_tag>(std::move(rotList)));
    }

    // 维度
    tag.put(nbt_keys::DIMENSION, static_cast<i32>(dimension));

    // 重生点
    if (spawnPoint.has_value()) {
        tag.put(nbt_keys::SPAWN_X, spawnPoint->x());
        tag.put(nbt_keys::SPAWN_Y, spawnPoint->y());
        tag.put(nbt_keys::SPAWN_Z, spawnPoint->z());
        tag.put(nbt_keys::SPAWN_DIM, static_cast<i32>(spawnPoint->getDimensionId()));
        if (spawnForced) {
            tag.put(nbt_keys::SPAWN_FORCED, static_cast<i8>(1));
        }
    }

    // 进入下界位置
    if (enteredNetherPosition.has_value()) {
        auto netherPos = std::make_unique<nbt::tags::compound_tag>();
        netherPos->put("x", enteredNetherPosition->x);
        netherPos->put("y", enteredNetherPosition->y);
        netherPos->put("z", enteredNetherPosition->z);
        tag.value.emplace(nbt_keys::ENTERED_NETHER_POS, std::move(netherPos));
    }

    // 最后死亡位置
    if (lastDeathLocation.has_value()) {
        auto deathTag = std::make_unique<nbt::tags::compound_tag>();
        deathTag->put(nbt_keys::LDL_DIMENSION, std::string(dimensionIdToString(lastDeathLocation->getDimensionId())));
        entity::serialization::nbt_helper::putIntList(
            *deathTag, nbt_keys::LDL_POS, {lastDeathLocation->x(), lastDeathLocation->y(), lastDeathLocation->z()});
        tag.value.emplace(nbt_keys::LAST_DEATH_LOCATION, std::move(deathTag));
    }

    // 游戏模式
    tag.put(nbt_keys::PLAYER_GAME_TYPE, static_cast<i32>(gameMode));
    tag.put(nbt_keys::FLYING, static_cast<i8>(flying ? 1 : 0));

    // 生命值
    tag.put(nbt_keys::HEALTH, health);

    // 饥饿值
    tag.put(nbt_keys::FOOD_LEVEL, foodLevel);
    tag.put(nbt_keys::FOOD_SATURATION, saturationLevel);
    tag.put(nbt_keys::FOOD_EXHAUSTION, exhaustionLevel);
    tag.put(nbt_keys::FOOD_TICK_TIMER, foodTickTimer);

    // 经验
    tag.put(nbt_keys::XP_LEVEL, experienceLevel);
    tag.put(nbt_keys::XP_PROGRESS, experienceProgress);
    tag.put(nbt_keys::XP_TOTAL, totalExperience);
    tag.put(nbt_keys::XP_SEED, xpSeed);

    // 能力
    {
        auto abilities = std::make_unique<nbt::tags::compound_tag>();
        abilities->put(nbt_keys::INVULNERABLE, static_cast<i8>(invulnerable ? 1 : 0));
        abilities->put(nbt_keys::MAY_FLY, static_cast<i8>(canFly ? 1 : 0));
        abilities->put(nbt_keys::FLYING, static_cast<i8>(flying ? 1 : 0));
        abilities->put(nbt_keys::FLY_SPEED, flySpeed);
        abilities->put(nbt_keys::WALK_SPEED, walkSpeed);
        tag.value.emplace(nbt_keys::ABILITIES, std::move(abilities));
    }

    // 背包（MC 1.21.11 新格式：Inventory 仅存储快捷栏和主背包 Slot 0-35，
    // 护甲和副手通过 "equipment" 复合标签以枚举名独立存储）
    {
        auto inventoryList = std::make_unique<nbt::tags::compound_list_tag>();
        // 仅写入快捷栏和主背包（索引 0-35）
        constexpr i32 mainEnd = 35; // HOTBAR_SIZE(9) + MAIN_SIZE(27) - 1
        for (i32 i = 0; i <= mainEnd && i < static_cast<i32>(inventoryItems.size()); ++i) {
            const auto& itemOpt = inventoryItems[static_cast<size_t>(i)];
            if (!itemOpt.has_value() || itemOpt->isEmpty()) {
                continue;
            }

            nbt::tags::compound_tag itemTag;
            itemTag.put("Slot", static_cast<i8>(i));
            itemOpt->toNbt(itemTag);
            inventoryList->value.push_back(std::move(itemTag));
        }
        tag.value.emplace(nbt_keys::INVENTORY, std::move(inventoryList));
    }

    // equipment 复合标签（护甲 + 副手，以枚举名存储）
    // 参考: net.minecraft.world.entity.EntityEquipment.CODEC
    // 键名: "offhand", "feet", "legs", "chest", "head"
    // 空槽位不写入，MainHand 来自 Inventory 的选中快捷栏槽位
    {
        nbt::tags::compound_tag equipmentTag;

        // 副手（索引 40）
        if (inventoryItems.size() > 40) {
            const auto& offhand = inventoryItems[40];
            if (offhand.has_value() && !offhand->isEmpty()) {
                nbt::tags::compound_tag itemTag;
                offhand->toNbt(itemTag);
                equipmentTag.value.emplace("offhand", std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));
            }
        }

        // 护甲（索引 36-39: HEAD=36, CHEST=37, LEGS=38, FEET=39）
        constexpr struct {
            const char* name;
            size_t internalIndex;
        } armorSlots[] = {
            {"feet", 39},
            {"legs", 38},
            {"chest", 37},
            {"head", 36},
        };

        for (const auto& [name, idx] : armorSlots) {
            if (idx < inventoryItems.size()) {
                const auto& item = inventoryItems[idx];
                if (item.has_value() && !item->isEmpty()) {
                    nbt::tags::compound_tag itemTag;
                    item->toNbt(itemTag);
                    equipmentTag.value.emplace(name, std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));
                }
            }
        }

        if (!equipmentTag.value.empty()) {
            tag.value.emplace("equipment", std::make_unique<nbt::tags::compound_tag>(std::move(equipmentTag)));
        }
    }

    // 鼠标持有物品
    if (carriedItem.has_value() && !carriedItem->isEmpty()) {
        auto carriedTag = std::make_unique<nbt::tags::compound_tag>();
        carriedItem->toNbt(*carriedTag);
        tag.value.emplace(nbt_keys::CARRIED_ITEM, std::move(carriedTag));
    }

    tag.put(nbt_keys::SELECTED_ITEM_SLOT, selectedSlot);

    // 药水效果
    if (!effects.empty()) {
        auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();
        for (const auto& effect : effects) {
            nbt::tags::compound_tag effectTag;
            effect.toNbt(effectTag);
            effectsList->value.push_back(std::move(effectTag));
        }
        tag.value.emplace(nbt_keys::ACTIVE_EFFECTS, std::move(effectsList));
    }

    // 空气
    tag.put(nbt_keys::AIR, airSupply);
    tag.put(nbt_keys::MAX_AIR, maxAirSupply);

    // 睡眠
    tag.put(nbt_keys::SLEEPING, static_cast<i8>(sleeping ? 1 : 0));
    tag.put(nbt_keys::SLEEP_TIMER, sleepTimer);
    if (sleepingPosition.has_value()) {
        auto bedPos = std::make_unique<nbt::tags::compound_tag>();
        bedPos->put("x", sleepingPosition->x);
        bedPos->put("y", sleepingPosition->y);
        bedPos->put("z", sleepingPosition->z);
        tag.value.emplace(nbt_keys::BED_POSITION, std::move(bedPos));
    }

    // 其他状态
    tag.put(nbt_keys::ON_GROUND, static_cast<i8>(onGround ? 1 : 0));
    tag.put(nbt_keys::SPRINTING, static_cast<i8>(sprinting ? 1 : 0));
    tag.put(nbt_keys::SNEAKING, static_cast<i8>(sneaking ? 1 : 0));

    // 冲量上下文
    if (currentImpulseImpactPos.has_value()) {
        auto impulsePos = std::make_unique<nbt::tags::compound_tag>();
        impulsePos->put("x", static_cast<f64>(currentImpulseImpactPos->x));
        impulsePos->put("y", static_cast<f64>(currentImpulseImpactPos->y));
        impulsePos->put("z", static_cast<f64>(currentImpulseImpactPos->z));
        tag.value.emplace("current_explosion_impact_pos", std::move(impulsePos));
    }
    tag.put("ignore_fall_damage_from_current_explosion", static_cast<i8>(ignoreFallDamageFromCurrentImpulse ? 1 : 0));
    tag.put("current_impulse_context_reset_grace_time", currentImpulseContextResetGraceTime);

    return tag;
}

// ============================================================================
// 反序列化
// ============================================================================

Result<PlayerSaveData> PlayerSaveData::fromNbt(const nbt::tags::compound_tag& tag)
{
    PlayerSaveData data;

    // 基本信息
    if (auto uuidOpt = tryGetString(tag, nbt_keys::UUID)) {
        data.uuid = std::move(*uuidOpt);
    }
    if (auto nameOpt = tryGetString(tag, nbt_keys::NAME)) {
        data.username = std::move(*nameOpt);
    }

    // 位置
    if (auto* posList = tryGetList(tag, nbt_keys::POS)) {
        if (posList->element_id() == nbt::TagId::Double && posList->size() >= 3) {
            // 需要转换为具体的 double_list_tag
            auto& doubleList = dynamic_cast<const nbt::tags::double_list_tag&>(*posList);
            data.posX = doubleList.value[0];
            data.posY = doubleList.value[1];
            data.posZ = doubleList.value[2];
        }
    } else {
        // 兼容旧格式
        if (auto opt = tryGetDouble(tag, nbt_keys::POS_X)) data.posX = *opt;
        if (auto opt = tryGetDouble(tag, nbt_keys::POS_Y)) data.posY = *opt;
        if (auto opt = tryGetDouble(tag, nbt_keys::POS_Z)) data.posZ = *opt;
    }

    // 旋转
    if (auto* rotList = tryGetList(tag, nbt_keys::ROTATION)) {
        if (rotList->element_id() == nbt::TagId::Float && rotList->size() >= 2) {
            auto& floatList = dynamic_cast<const nbt::tags::float_list_tag&>(*rotList);
            data.yaw = floatList.value[0];
            data.pitch = floatList.value[1];
        }
    } else {
        if (auto opt = tryGetFloat(tag, nbt_keys::YAW)) data.yaw = *opt;
        if (auto opt = tryGetFloat(tag, nbt_keys::PITCH)) data.pitch = *opt;
    }

    // 维度
    if (auto opt = tryGetInt(tag, nbt_keys::DIMENSION)) {
        data.dimension = static_cast<DimensionId>(*opt);
    }

    // 重生点
    auto spawnX = tryGetInt(tag, nbt_keys::SPAWN_X);
    auto spawnY = tryGetInt(tag, nbt_keys::SPAWN_Y);
    auto spawnZ = tryGetInt(tag, nbt_keys::SPAWN_Z);
    if (spawnX.has_value() && spawnY.has_value() && spawnZ.has_value()) {
        GlobalPos spawn;
        spawn = GlobalPos(tryGetInt(tag, nbt_keys::SPAWN_DIM).value_or(0), // 默认主世界
            BlockPos(*spawnX, *spawnY, *spawnZ));
        data.spawnPoint = spawn;
        data.spawnForced = tryGetBool(tag, nbt_keys::SPAWN_FORCED).value_or(false);
    }

    // 进入下界位置
    if (auto* netherPos = tryGetCompound(tag, nbt_keys::ENTERED_NETHER_POS)) {
        auto x = tryGetDouble(*netherPos, "x");
        auto y = tryGetDouble(*netherPos, "y");
        auto z = tryGetDouble(*netherPos, "z");
        if (x.has_value() && y.has_value() && z.has_value()) {
            data.enteredNetherPosition = Vector3d(*x, *y, *z);
        }
    }

    // 最后死亡位置
    if (auto* deathTag = tryGetCompound(tag, nbt_keys::LAST_DEATH_LOCATION)) {
        auto dimStr = tryGetString(*deathTag, nbt_keys::LDL_DIMENSION);
        auto posList = entity::serialization::nbt_helper::getIntList(*deathTag, nbt_keys::LDL_POS);
        if (dimStr.has_value() && posList.size() >= 3) {
            DimensionId dim = dimensionNameToId(*dimStr);
            data.lastDeathLocation = GlobalPos(dim, BlockPos(posList[0], posList[1], posList[2]));
        } else {
            // 兼容整数维度格式
            auto dimInt = tryGetInt(*deathTag, nbt_keys::LDL_DIMENSION);
            if (dimInt.has_value() && posList.size() >= 3) {
                data.lastDeathLocation = GlobalPos(*dimInt, BlockPos(posList[0], posList[1], posList[2]));
            }
        }
    }

    // 游戏模式
    if (auto opt = tryGetInt(tag, nbt_keys::PLAYER_GAME_TYPE)) {
        data.gameMode = static_cast<GameMode>(*opt);
    }
    if (auto opt = tryGetBool(tag, nbt_keys::FLYING)) {
        data.flying = *opt;
    }

    // 生命值
    if (auto opt = tryGetFloat(tag, nbt_keys::HEALTH)) {
        data.health = *opt;
    }

    // 饥饿值
    if (auto opt = tryGetInt(tag, nbt_keys::FOOD_LEVEL)) data.foodLevel = *opt;
    if (auto opt = tryGetFloat(tag, nbt_keys::FOOD_SATURATION)) data.saturationLevel = *opt;
    if (auto opt = tryGetFloat(tag, nbt_keys::FOOD_EXHAUSTION)) data.exhaustionLevel = *opt;
    if (auto opt = tryGetInt(tag, nbt_keys::FOOD_TICK_TIMER)) data.foodTickTimer = *opt;

    // 经验
    if (auto opt = tryGetInt(tag, nbt_keys::XP_LEVEL)) data.experienceLevel = *opt;
    if (auto opt = tryGetFloat(tag, nbt_keys::XP_PROGRESS)) data.experienceProgress = *opt;
    if (auto opt = tryGetInt(tag, nbt_keys::XP_TOTAL)) data.totalExperience = *opt;
    if (auto opt = tryGetInt(tag, nbt_keys::XP_SEED)) {
        data.xpSeed = *opt;
    } else {
        // 如果没有种子，生成一个随机种子
        data.xpSeed = static_cast<i32>(std::rand());
    }

    // 能力
    if (auto* abilities = tryGetCompound(tag, nbt_keys::ABILITIES)) {
        if (auto opt = tryGetBool(*abilities, nbt_keys::INVULNERABLE)) data.invulnerable = *opt;
        if (auto opt = tryGetBool(*abilities, nbt_keys::MAY_FLY)) data.canFly = *opt;
        if (auto opt = tryGetBool(*abilities, nbt_keys::FLYING)) data.flying = *opt;
        if (auto opt = tryGetFloat(*abilities, nbt_keys::FLY_SPEED)) data.flySpeed = *opt;
        if (auto opt = tryGetFloat(*abilities, nbt_keys::WALK_SPEED)) data.walkSpeed = *opt;
    }

    // 背包（支持 MC 1.21.11 新格式和旧版格式）
    // 新格式：Inventory 仅包含 Slot 0-35，装备通过 "equipment" 复合标签读取
    // 旧格式：Inventory 包含所有槽位（0-40），护甲使用 Slot 100-103，副手使用 Slot -106
    {
        // 预分配槽位（41个槽位：0-8快捷栏，9-35主背包，36-39护甲，40副手）
        data.inventoryItems.resize(41);

        // 先读取 equipment 复合标签（新格式）
        bool hasEquipment = false;
        if (auto* equipmentTag = tryGetCompound(tag, "equipment")) {
            hasEquipment = true;
            // EquipmentSlot 名称到内部索引的映射
            constexpr struct {
                const char* name;
                size_t internalIndex;
            } slotMapping[] = {
                {"offhand", 40},
                {"feet", 39},
                {"legs", 38},
                {"chest", 37},
                {"head", 36},
            };

            for (const auto& [name, idx] : slotMapping) {
                if (auto* itemCompound = tryGetCompound(*equipmentTag, name)) {
                    auto stackResult = ItemStack::fromNbt(*itemCompound);
                    if (stackResult.success() && !stackResult.value().isEmpty()) {
                        data.inventoryItems[idx] = std::move(stackResult.value());
                    }
                }
            }
        }

        // 读取 Inventory 列表
        if (auto* invList = tryGetList(tag, nbt_keys::INVENTORY)) {
            if (invList->element_id() == nbt::TagId::Compound) {
                auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*invList);

                for (const auto& itemTag : compoundList.value) {
                    // 获取 NBT 槽位值
                    i8 nbtSlot = 0;
                    if (auto slotOpt = tryGetByte(itemTag, "Slot")) {
                        nbtSlot = *slotOpt;
                    } else {
                        continue;
                    }

                    // 将 NBT 槽位值转换为内部索引
                    i32 internalSlot = InventorySlots::fromNbtSlot(static_cast<i32>(nbtSlot));
                    if (internalSlot < 0 || internalSlot >= 41) {
                        continue; // 无效槽位，跳过
                    }

                    // 如果已有 equipment 字段，跳过护甲和副手槽位（它们已从 equipment 读取）
                    if (hasEquipment && internalSlot >= 36) {
                        continue;
                    }

                    // 从NBT恢复ItemStack
                    auto stackResult = ItemStack::fromNbt(itemTag);
                    if (stackResult.success() && !stackResult.value().isEmpty()) {
                        data.inventoryItems[static_cast<size_t>(internalSlot)] = std::move(stackResult.value());
                    }
                }
            }
        }
    }

    // 鼠标持有物品
    if (auto* carriedTag = tryGetCompound(tag, nbt_keys::CARRIED_ITEM)) {
        auto stackResult = ItemStack::fromNbt(*carriedTag);
        if (stackResult.success() && !stackResult.value().isEmpty()) {
            data.carriedItem = std::move(stackResult.value());
        }
    }

    if (auto opt = tryGetInt(tag, nbt_keys::SELECTED_ITEM_SLOT)) {
        data.selectedSlot = *opt;
    }

    // 药水效果
    if (auto* effectsList = tryGetList(tag, nbt_keys::ACTIVE_EFFECTS)) {
        if (effectsList->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*effectsList);
            data.effects.reserve(compoundList.value.size());
            for (const auto& effectTag : compoundList.value) {
                data.effects.push_back(entity::effect::EffectInstance::fromNbt(effectTag));
            }
        }
    }

    // 空气
    if (auto opt = tryGetInt(tag, nbt_keys::AIR)) data.airSupply = *opt;
    if (auto opt = tryGetInt(tag, nbt_keys::MAX_AIR)) data.maxAirSupply = *opt;

    // 睡眠
    if (auto opt = tryGetBool(tag, nbt_keys::SLEEPING)) data.sleeping = *opt;
    if (auto opt = tryGetInt(tag, nbt_keys::SLEEP_TIMER)) data.sleepTimer = *opt;
    if (auto* bedPos = tryGetCompound(tag, nbt_keys::BED_POSITION)) {
        auto x = tryGetInt(*bedPos, "x");
        auto y = tryGetInt(*bedPos, "y");
        auto z = tryGetInt(*bedPos, "z");
        if (x.has_value() && y.has_value() && z.has_value()) {
            data.sleepingPosition = BlockPos(*x, *y, *z);
        }
    }

    // 其他状态
    if (auto opt = tryGetBool(tag, nbt_keys::ON_GROUND)) data.onGround = *opt;
    if (auto opt = tryGetBool(tag, nbt_keys::SPRINTING)) data.sprinting = *opt;
    if (auto opt = tryGetBool(tag, nbt_keys::SNEAKING)) data.sneaking = *opt;

    // 冲量上下文
    if (auto* impulsePos = tryGetCompound(tag, "current_explosion_impact_pos")) {
        auto x = tryGetDouble(*impulsePos, "x");
        auto y = tryGetDouble(*impulsePos, "y");
        auto z = tryGetDouble(*impulsePos, "z");
        if (x.has_value() && y.has_value() && z.has_value()) {
            data.currentImpulseImpactPos = Vector3(static_cast<f32>(*x), static_cast<f32>(*y), static_cast<f32>(*z));
        }
    }
    if (auto opt = tryGetBool(tag, "ignore_fall_damage_from_current_explosion")) {
        data.ignoreFallDamageFromCurrentImpulse = *opt;
    }
    if (auto opt = tryGetInt(tag, "current_impulse_context_reset_grace_time")) {
        data.currentImpulseContextResetGraceTime = *opt;
    }

    return data;
}

// ============================================================================
// 二进制序列化
// ============================================================================

Result<std::vector<u8>> PlayerSaveData::serialize() const
{
    // 转换为NBT
    nbt::tags::compound_tag tag = toNbt();

    // 序列化NBT为二进制
    // 使用 nbt 命名空间中的 operator<<
    std::ostringstream oss;
    oss << nbt::contexts::java;
    nbt::operator<<(oss, tag);
    std::string nbtStr = oss.str();
    std::vector<u8> nbtData(nbtStr.begin(), nbtStr.end());

    // 使用 gzip 压缩玩家数据
    std::vector<u8> compressed;
    compressed.resize(nbtData.size() + 1024); // 预留压缩空间

    uLongf destLen = static_cast<uLongf>(compressed.size());
    int result =
        compress2(compressed.data(), &destLen, nbtData.data(), static_cast<uLong>(nbtData.size()), Z_BEST_COMPRESSION);

    if (result != Z_OK) {
        return Error(
            ErrorCode::CompressionFailed, fmt::format("Failed to compress player data: zlib error {}", result));
    }

    compressed.resize(destLen);
    return compressed;
}

Result<PlayerSaveData> PlayerSaveData::deserialize(const std::vector<u8>& data)
{
    // 解压数据
    std::vector<u8> decompressed;
    decompressed.resize(data.size() * 10); // 预估解压大小

    uLongf destLen = static_cast<uLongf>(decompressed.size());
    int result = uncompress(decompressed.data(), &destLen, data.data(), static_cast<uLong>(data.size()));

    if (result != Z_OK) {
        // 尝试不解压直接解析（可能是未压缩的数据）
        decompressed = data;
        destLen = static_cast<uLongf>(data.size());
    } else {
        decompressed.resize(destLen);
    }

    // 解析NBT
    try {
        std::istringstream iss(std::string(decompressed.begin(), decompressed.end()));
        iss >> nbt::contexts::java;
        auto root = nbt::tags::compound_tag::read(iss);
        if (!root) {
            return Error(ErrorCode::InvalidData, "Failed to parse player data NBT");
        }

        return fromNbt(*root);
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, fmt::format("Failed to deserialize player data: {}", e.what()));
    }
}

} // namespace world::storage
} // namespace mc
