#include "PlayerData.hpp"
#include <sstream>
#include <iomanip>

namespace mc::world::save::data {

// ========== SlotData ==========

std::unique_ptr<nbt::CompoundTag> SlotData::serialize() const {
    if (isEmpty()) {
        return nullptr;  // 空槽位不存储
    }

    auto nbt = std::make_unique<nbt::CompoundTag>();
    nbt->put("id", itemId);
    nbt->put("Count", static_cast<i8>(count));

    // MC 1.13+ 使用 Damage，旧版本可能不同
    // 对于没有损坏值的物品，可以省略

    if (tag) {
        nbt->put("tag", tag->copy());
    }

    return nbt;
}

Result<SlotData> SlotData::deserialize(const nbt::CompoundTag& nbt) {
    SlotData slot;

    if (nbt.has("id")) {
        auto* idTag = nbt.get_if<nbt::IntTag>("id");
        if (idTag) {
            slot.itemId = idTag->get();
        } else {
            // 兼容旧版本的字符串 ID
            auto* strTag = nbt.get_if<nbt::StringTag>("id");
            if (strTag) {
                // TODO: 从字符串 ID 解析
                slot.itemId = -1;
            }
        }
    }

    if (nbt.has("Count")) {
        auto* tag = nbt.get_if<nbt::ByteTag>("Count");
        if (tag) {
            slot.count = static_cast<i32>(tag->get());
        } else {
            auto* intTag = nbt.get_if<nbt::IntTag>("Count");
            if (intTag) {
                slot.count = intTag->get();
            }
        }
    }

    if (nbt.has("Damage")) {
        auto* tag = nbt.get_if<nbt::ShortTag>("Damage");
        if (tag) {
            slot.damage = static_cast<i32>(tag->get());
        }
    }

    if (nbt.has("tag")) {
        auto* tagCompound = nbt.get_if<nbt::CompoundTag>("tag");
        if (tagCompound) {
            slot.tag = tagCompound->copy();
        }
    }

    return slot;
}

// ========== AttributeData ==========

std::unique_ptr<nbt::CompoundTag> AttributeData::serialize() const {
    auto nbt = std::make_unique<nbt::CompoundTag>();
    nbt->put("Name", name);
    nbt->put("Base", baseValue);

    // 省略修饰符列表

    return nbt;
}

Result<AttributeData> AttributeData::deserialize(const nbt::CompoundTag& nbt) {
    AttributeData attr;

    if (nbt.has("Name")) {
        auto* tag = nbt.get_if<nbt::StringTag>("Name");
        if (tag) attr.name = tag->get();
    }

    if (nbt.has("Base")) {
        auto* tag = nbt.get_if<nbt::DoubleTag>("Base");
        if (tag) attr.baseValue = tag->get();
    }

    return attr;
}

// ========== EffectData ==========

std::unique_ptr<nbt::CompoundTag> EffectData::serialize() const {
    auto nbt = std::make_unique<nbt::CompoundTag>();
    nbt->put("Id", id);
    nbt->put("Amplifier", static_cast<i8>(amplifier));
    nbt->put("Duration", duration);
    nbt->put("Ambient", ambient);
    nbt->put("ShowParticles", showParticles);

    return nbt;
}

Result<EffectData> EffectData::deserialize(const nbt::CompoundTag& nbt) {
    EffectData effect;

    if (nbt.has("Id")) {
        auto* tag = nbt.get_if<nbt::StringTag>("Id");
        if (tag) effect.id = tag->get();
    } else if (nbt.has("EffectId")) {
        // 兼容旧版本的数字 ID
        auto* tag = nbt.get_if<nbt::ByteTag>("EffectId");
        if (tag) {
            // TODO: 从数字 ID 转换为字符串 ID
        }
    }

    if (nbt.has("Amplifier")) {
        auto* tag = nbt.get_if<nbt::ByteTag>("Amplifier");
        if (tag) effect.amplifier = static_cast<i32>(tag->get());
    }

    if (nbt.has("Duration")) {
        auto* tag = nbt.get_if<nbt::IntTag>("Duration");
        if (tag) effect.duration = tag->get();
    }

    if (nbt.has("Ambient")) {
        auto* tag = nbt.get_if<nbt::ByteTag>("Ambient");
        if (tag) effect.ambient = tag->get() != 0;
    }

    if (nbt.has("ShowParticles")) {
        auto* tag = nbt.get_if<nbt::ByteTag>("ShowParticles");
        if (tag) effect.showParticles = tag->get() != 0;
    }

    return effect;
}

// ========== PlayerData ==========

String PlayerData::uuidString() const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    oss << std::setw(8) << ((uuid >> 96) & 0xFFFFFFFF);
    oss << std::setw(8) << ((uuid >> 64) & 0xFFFFFFFF);
    oss << std::setw(8) << ((uuid >> 32) & 0xFFFFFFFF);
    oss << std::setw(8) << (uuid & 0xFFFFFFFF);
    return oss.str();
}

std::unique_ptr<PlayerData> PlayerData::fromPlayer(const mc::Player& player) {
    auto data = std::make_unique<PlayerData>();

    // 基本信息
    data->uuid = player.uuid();
    data->username = player.username();
    data->gameType = static_cast<GameType>(static_cast<i32>(player.gameMode()));

    // 位置
    data->dimension = player.dimension();
    data->posX = player.x();
    data->posY = player.y();
    data->posZ = player.z();
    data->yaw = player.yaw();
    data->pitch = player.pitch();

    // 速度
    data->motionX = player.velocityX();
    data->motionY = player.velocityY();
    data->motionZ = player.velocityZ();

    // 生命值
    data->health = player.health();
    data->maxHealth = player.maxHealth();
    data->absorptionAmount = player.absorptionAmount();

    // 食物
    const auto& food = player.foodStats();
    data->foodLevel = food.foodLevel;
    data->foodSaturation = food.saturationLevel;
    data->foodExhaustion = food.exhaustionLevel;
    data->foodTickTimer = food.foodTimer;

    // 经验
    data->xpLevel = player.experienceLevel();
    data->xpProgress = player.experienceProgress();
    data->xpTotal = player.totalExperience();
    data->xpSeed = player.xpSeed();

    // 能力
    const auto& abilities = player.abilities();
    data->invulnerable = abilities.invulnerable;
    data->flying = abilities.flying;
    data->canFly = abilities.canFly;
    data->creativeMode = abilities.creativeMode;
    data->allowEdit = abilities.allowEdit;
    data->flySpeed = abilities.flySpeed;
    data->walkSpeed = abilities.walkSpeed;

    // TODO: 物品栏、属性、效果

    data->dataVersion = 2586;

    return data;
}

void PlayerData::applyToPlayer(mc::Player& player) const {
    // 位置
    player.setPosition(posX, posY, posZ);
    player.setRotation(yaw, pitch);
    player.setDimension(dimension);

    // 速度
    player.setVelocity(motionX, motionY, motionZ);

    // 生命值
    player.setHealth(health);
    player.setAbsorptionAmount(absorptionAmount);

    // 食物
    auto& food = player.foodStats();
    food.foodLevel = foodLevel;
    food.saturationLevel = foodSaturation;
    food.exhaustionLevel = foodExhaustion;
    food.foodTimer = foodTickTimer;

    // 经验
    player.setExperienceLevel(xpLevel);

    // 能力
    auto& abilities = player.abilities();
    abilities.invulnerable = invulnerable;
    abilities.flying = flying;
    abilities.canFly = canFly;
    abilities.creativeMode = creativeMode;
    abilities.allowEdit = allowEdit;
    abilities.flySpeed = flySpeed;
    abilities.walkSpeed = walkSpeed;

    // TODO: 物品栏、属性、效果
}

// ========== 序列化 ==========

std::unique_ptr<nbt::CompoundTag> PlayerData::serialize() const {
    auto nbt = std::make_unique<nbt::CompoundTag>();

    // 数据版本
    nbt->put("DataVersion", dataVersion);

    // UUID
    auto uuidList = std::make_unique<nbt::IntArrayTag>();
    uuidList->push_back(static_cast<i32>((uuid >> 96) & 0xFFFFFFFF));
    uuidList->push_back(static_cast<i32>((uuid >> 64) & 0xFFFFFFFF));
    uuidList->push_back(static_cast<i32>((uuid >> 32) & 0xFFFFFFFF));
    uuidList->push_back(static_cast<i32>(uuid & 0xFFFFFFFF));
    nbt->put("UUID", std::move(uuidList));

    // 位置和旋转
    serializePosition(*nbt);
    serializeMotion(*nbt);
    serializeRotation(*nbt);

    // 维度
    nbt->put("Dimension", static_cast<i32>(dimension));

    // 生命值
    nbt->put("Health", health);
    nbt->put("AbsorptionAmount", absorptionAmount);
    nbt->put("FallDistance", fallDistance);

    // 食物
    serializeFood(*nbt);

    // 经验
    serializeExperience(*nbt);

    // 游戏模式
    nbt->put("playerGameType", static_cast<i32>(gameType));
    nbt->put("canBeKilledInstantly", canBeKilledInstantly);

    // 能力
    serializeAbilities(*nbt);

    // 物品栏
    serializeInventory(*nbt);

    // 属性
    serializeAttributes(*nbt);

    // 效果
    serializeEffects(*nbt);

    // 其他
    nbt->put("seenCredits", seenCredits);
    nbt->put("Score", score);

    // 用户名
    nbt->put("identifier", username);

    return nbt;
}

void PlayerData::serializePosition(nbt::CompoundTag& nbt) const {
    auto posList = std::make_unique<nbt::ListTag>();
    posList->push_back(std::make_unique<nbt::DoubleTag>(posX));
    posList->push_back(std::make_unique<nbt::DoubleTag>(posY));
    posList->push_back(std::make_unique<nbt::DoubleTag>(posZ));
    nbt.put("Pos", std::move(posList));
}

void PlayerData::serializeMotion(nbt::CompoundTag& nbt) const {
    auto motionList = std::make_unique<nbt::ListTag>();
    motionList->push_back(std::make_unique<nbt::DoubleTag>(motionX));
    motionList->push_back(std::make_unique<nbt::DoubleTag>(motionY));
    motionList->push_back(std::make_unique<nbt::DoubleTag>(motionZ));
    nbt.put("Motion", std::move(motionList));
}

void PlayerData::serializeRotation(nbt::CompoundTag& nbt) const {
    auto rotList = std::make_unique<nbt::ListTag>();
    rotList->push_back(std::make_unique<nbt::FloatTag>(yaw));
    rotList->push_back(std::make_unique<nbt::FloatTag>(pitch));
    nbt.put("Rotation", std::move(rotList));
}

void PlayerData::serializeFood(nbt::CompoundTag& nbt) const {
    nbt.put("foodLevel", foodLevel);
    nbt.put("foodSaturationLevel", foodSaturation);
    nbt.put("foodExhaustionLevel", foodExhaustion);
    nbt.put("foodTickTimer", foodTickTimer);
}

void PlayerData::serializeExperience(nbt::CompoundTag& nbt) const {
    nbt.put("XpLevel", xpLevel);
    nbt.put("XpTotal", xpTotal);
    nbt.put("XpP", xpProgress);
    nbt.put("XpSeed", xpSeed);
}

void PlayerData::serializeAbilities(nbt::CompoundTag& nbt) const {
    auto abilitiesNbt = std::make_unique<nbt::CompoundTag>();
    abilitiesNbt->put("invulnerable", invulnerable);
    abilitiesNbt->put("flying", flying);
    abilitiesNbt->put("mayfly", canFly);
    abilitiesNbt->put("instabuild", creativeMode);
    abilitiesNbt->put("mayBuild", allowEdit);
    abilitiesNbt->put("flySpeed", flySpeed);
    abilitiesNbt->put("walkSpeed", walkSpeed);
    nbt.put("abilities", std::move(abilitiesNbt));
}

void PlayerData::serializeInventory(nbt::CompoundTag& nbt) const {
    auto invList = std::make_unique<nbt::ListTag>();
    for (size_t i = 0; i < inventory.size(); ++i) {
        auto slotNbt = inventory[i].serialize();
        if (slotNbt) {
            slotNbt->put("Slot", static_cast<i8>(i));
            invList->push_back(std::move(slotNbt));
        }
    }
    nbt.put("Inventory", std::move(invList));

    // 末影箱
    auto enderList = std::make_unique<nbt::ListTag>();
    for (size_t i = 0; i < enderChest.size(); ++i) {
        auto slotNbt = enderChest[i].serialize();
        if (slotNbt) {
            slotNbt->put("Slot", static_cast<i8>(i));
            enderList->push_back(std::move(slotNbt));
        }
    }
    nbt.put("EnderItems", std::move(enderList));

    // 选中的槽位
    nbt.put("SelectedItemSlot", selectedSlot);
}

void PlayerData::serializeAttributes(nbt::CompoundTag& nbt) const {
    auto attrList = std::make_unique<nbt::ListTag>();
    for (const auto& attr : attributes) {
        attrList->push_back(attr.serialize());
    }
    nbt.put("Attributes", std::move(attrList));
}

void PlayerData::serializeEffects(nbt::CompoundTag& nbt) const {
    auto effectList = std::make_unique<nbt::ListTag>();
    for (const auto& effect : effects) {
        effectList->push_back(effect.serialize());
    }
    nbt.put("ActiveEffects", std::move(effectList));
}

// ========== 反序列化 ==========

Result<std::unique_ptr<PlayerData>> PlayerData::deserialize(const nbt::CompoundTag& nbt) {
    auto data = std::make_unique<PlayerData>();

    // 数据版本
    if (nbt.has("DataVersion")) {
        auto* tag = nbt.get_if<nbt::IntTag>("DataVersion");
        if (tag) data->dataVersion = tag->get();
    }

    // UUID
    if (nbt.has("UUID")) {
        auto* uuidArray = nbt.get_if<nbt::IntArrayTag>("UUID");
        if (uuidArray && uuidArray->size() >= 4) {
            u64 msb = (static_cast<u64>((*uuidArray)[0]) << 32) |
                       static_cast<u64>((*uuidArray)[1]);
            u64 lsb = (static_cast<u64>((*uuidArray)[2]) << 32) |
                       static_cast<u64>((*uuidArray)[3]);
            data->uuid = (msb << 64) | lsb;
        }
    } else if (nbt.has("UUIDMost") && nbt.has("UUIDLeast")) {
        // 兼容旧版本
        auto* mostTag = nbt.get_if<nbt::LongTag>("UUIDMost");
        auto* leastTag = nbt.get_if<nbt::LongTag>("UUIDLeast");
        if (mostTag && leastTag) {
            u64 msb = static_cast<u64>(mostTag->get());
            u64 lsb = static_cast<u64>(leastTag->get());
            data->uuid = (msb << 64) | lsb;
        }
    }

    // 位置和旋转
    deserializePosition(*data, nbt);
    deserializeMotion(*data, nbt);
    deserializeRotation(*data, nbt);

    // 维度
    if (nbt.has("Dimension")) {
        auto* tag = nbt.get_if<nbt::IntTag>("Dimension");
        if (tag) data->dimension = static_cast<DimensionId>(tag->get());
    }

    // 生命值
    if (nbt.has("Health")) {
        auto* tag = nbt.get_if<nbt::FloatTag>("Health");
        if (tag) data->health = tag->get();
    }
    if (nbt.has("AbsorptionAmount")) {
        auto* tag = nbt.get_if<nbt::FloatTag>("AbsorptionAmount");
        if (tag) data->absorptionAmount = tag->get();
    }
    if (nbt.has("FallDistance")) {
        auto* tag = nbt.get_if<nbt::FloatTag>("FallDistance");
        if (tag) data->fallDistance = tag->get();
    }

    // 食物
    deserializeFood(*data, nbt);

    // 经验
    deserializeExperience(*data, nbt);

    // 游戏模式
    if (nbt.has("playerGameType")) {
        auto* tag = nbt.get_if<nbt::IntTag>("playerGameType");
        if (tag) data->gameType = static_cast<GameType>(tag->get());
    }
    if (nbt.has("canBeKilledInstantly")) {
        auto* tag = nbt.get_if<nbt::ByteTag>("canBeKilledInstantly");
        if (tag) data->canBeKilledInstantly = tag->get() != 0;
    }

    // 能力
    deserializeAbilities(*data, nbt);

    // 物品栏
    deserializeInventory(*data, nbt);

    // 属性
    deserializeAttributes(*data, nbt);

    // 效果
    deserializeEffects(*data, nbt);

    // 其他
    if (nbt.has("seenCredits")) {
        auto* tag = nbt.get_if<nbt::ByteTag>("seenCredits");
        if (tag) data->seenCredits = tag->get() != 0;
    }
    if (nbt.has("Score")) {
        auto* tag = nbt.get_if<nbt::IntTag>("Score");
        if (tag) data->score = tag->get();
    }
    if (nbt.has("identifier")) {
        auto* tag = nbt.get_if<nbt::StringTag>("identifier");
        if (tag) data->username = tag->get();
    }

    return data;
}

void PlayerData::deserializePosition(PlayerData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("Pos")) {
        auto* posList = nbt.get_if<nbt::ListTag>("Pos");
        if (posList && posList->size() >= 3) {
            auto* x = posList->get_if<nbt::DoubleTag>(0);
            auto* y = posList->get_if<nbt::DoubleTag>(1);
            auto* z = posList->get_if<nbt::DoubleTag>(2);
            if (x) data.posX = x->get();
            if (y) data.posY = y->get();
            if (z) data.posZ = z->get();
        }
    }
}

void PlayerData::deserializeMotion(PlayerData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("Motion")) {
        auto* motionList = nbt.get_if<nbt::ListTag>("Motion");
        if (motionList && motionList->size() >= 3) {
            auto* x = motionList->get_if<nbt::DoubleTag>(0);
            auto* y = motionList->get_if<nbt::DoubleTag>(1);
            auto* z = motionList->get_if<nbt::DoubleTag>(2);
            if (x) data.motionX = x->get();
            if (y) data.motionY = y->get();
            if (z) data.motionZ = z->get();
        }
    }
}

void PlayerData::deserializeRotation(PlayerData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("Rotation")) {
        auto* rotList = nbt.get_if<nbt::ListTag>("Rotation");
        if (rotList && rotList->size() >= 2) {
            auto* yaw = rotList->get_if<nbt::FloatTag>(0);
            auto* pitch = rotList->get_if<nbt::FloatTag>(1);
            if (yaw) data.yaw = yaw->get();
            if (pitch) data.pitch = pitch->get();
        }
    }
}

void PlayerData::deserializeFood(PlayerData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("foodLevel")) {
        auto* tag = nbt.get_if<nbt::IntTag>("foodLevel");
        if (tag) data.foodLevel = tag->get();
    }
    if (nbt.has("foodSaturationLevel")) {
        auto* tag = nbt.get_if<nbt::FloatTag>("foodSaturationLevel");
        if (tag) data.foodSaturation = tag->get();
    }
    if (nbt.has("foodExhaustionLevel")) {
        auto* tag = nbt.get_if<nbt::FloatTag>("foodExhaustionLevel");
        if (tag) data.foodExhaustion = tag->get();
    }
    if (nbt.has("foodTickTimer")) {
        auto* tag = nbt.get_if<nbt::IntTag>("foodTickTimer");
        if (tag) data.foodTickTimer = tag->get();
    }
}

void PlayerData::deserializeExperience(PlayerData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("XpLevel")) {
        auto* tag = nbt.get_if<nbt::IntTag>("XpLevel");
        if (tag) data.xpLevel = tag->get();
    }
    if (nbt.has("XpTotal")) {
        auto* tag = nbt.get_if<nbt::IntTag>("XpTotal");
        if (tag) data.xpTotal = tag->get();
    }
    if (nbt.has("XpP")) {
        auto* tag = nbt.get_if<nbt::FloatTag>("XpP");
        if (tag) data.xpProgress = tag->get();
    }
    if (nbt.has("XpSeed")) {
        auto* tag = nbt.get_if<nbt::IntTag>("XpSeed");
        if (tag) data.xpSeed = tag->get();
    }
}

void PlayerData::deserializeAbilities(PlayerData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("abilities")) {
        auto* abilitiesNbt = nbt.get_if<nbt::CompoundTag>("abilities");
        if (abilitiesNbt) {
            if (abilitiesNbt->has("invulnerable")) {
                auto* tag = abilitiesNbt->get_if<nbt::ByteTag>("invulnerable");
                if (tag) data.invulnerable = tag->get() != 0;
            }
            if (abilitiesNbt->has("flying")) {
                auto* tag = abilitiesNbt->get_if<nbt::ByteTag>("flying");
                if (tag) data.flying = tag->get() != 0;
            }
            if (abilitiesNbt->has("mayfly")) {
                auto* tag = abilitiesNbt->get_if<nbt::ByteTag>("mayfly");
                if (tag) data.canFly = tag->get() != 0;
            }
            if (abilitiesNbt->has("instabuild")) {
                auto* tag = abilitiesNbt->get_if<nbt::ByteTag>("instabuild");
                if (tag) data.creativeMode = tag->get() != 0;
            }
            if (abilitiesNbt->has("mayBuild")) {
                auto* tag = abilitiesNbt->get_if<nbt::ByteTag>("mayBuild");
                if (tag) data.allowEdit = tag->get() != 0;
            }
            if (abilitiesNbt->has("flySpeed")) {
                auto* tag = abilitiesNbt->get_if<nbt::FloatTag>("flySpeed");
                if (tag) data.flySpeed = tag->get();
            }
            if (abilitiesNbt->has("walkSpeed")) {
                auto* tag = abilitiesNbt->get_if<nbt::FloatTag>("walkSpeed");
                if (tag) data.walkSpeed = tag->get();
            }
        }
    }
}

void PlayerData::deserializeInventory(PlayerData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("Inventory")) {
        auto* invList = nbt.get_if<nbt::ListTag>("Inventory");
        if (invList) {
            for (size_t i = 0; i < invList->size(); ++i) {
                auto* itemNbt = invList->get_if<nbt::CompoundTag>(i);
                if (itemNbt) {
                    auto slotResult = SlotData::deserialize(*itemNbt);
                    if (slotResult.success()) {
                        // 获取槽位索引
                        if (itemNbt->has("Slot")) {
                            auto* slotTag = itemNbt->get_if<nbt::ByteTag>("Slot");
                            if (slotTag) {
                                i32 slotIndex = static_cast<i32>(slotTag->get());
                                // 确保物品栏足够大
                                if (slotIndex >= static_cast<i32>(data.inventory.size())) {
                                    data.inventory.resize(slotIndex + 1);
                                }
                                data.inventory[slotIndex] = std::move(slotResult.value());
                            }
                        }
                    }
                }
            }
        }
    }

    if (nbt.has("EnderItems")) {
        auto* enderList = nbt.get_if<nbt::ListTag>("EnderItems");
        if (enderList) {
            for (size_t i = 0; i < enderList->size(); ++i) {
                auto* itemNbt = enderList->get_if<nbt::CompoundTag>(i);
                if (itemNbt) {
                    auto slotResult = SlotData::deserialize(*itemNbt);
                    if (slotResult.success()) {
                        if (itemNbt->has("Slot")) {
                            auto* slotTag = itemNbt->get_if<nbt::ByteTag>("Slot");
                            if (slotTag) {
                                i32 slotIndex = static_cast<i32>(slotTag->get());
                                if (slotIndex >= static_cast<i32>(data.enderChest.size())) {
                                    data.enderChest.resize(slotIndex + 1);
                                }
                                data.enderChest[slotIndex] = std::move(slotResult.value());
                            }
                        }
                    }
                }
            }
        }
    }

    if (nbt.has("SelectedItemSlot")) {
        auto* tag = nbt.get_if<nbt::IntTag>("SelectedItemSlot");
        if (tag) data.selectedSlot = tag->get();
    }
}

void PlayerData::deserializeAttributes(PlayerData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("Attributes")) {
        auto* attrList = nbt.get_if<nbt::ListTag>("Attributes");
        if (attrList) {
            for (size_t i = 0; i < attrList->size(); ++i) {
                auto* attrNbt = attrList->get_if<nbt::CompoundTag>(i);
                if (attrNbt) {
                    auto attrResult = AttributeData::deserialize(*attrNbt);
                    if (attrResult.success()) {
                        data.attributes.push_back(std::move(attrResult.value()));
                    }
                }
            }
        }
    }
}

void PlayerData::deserializeEffects(PlayerData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("ActiveEffects")) {
        auto* effectList = nbt.get_if<nbt::ListTag>("ActiveEffects");
        if (effectList) {
            for (size_t i = 0; i < effectList->size(); ++i) {
                auto* effectNbt = effectList->get_if<nbt::CompoundTag>(i);
                if (effectNbt) {
                    auto effectResult = EffectData::deserialize(*effectNbt);
                    if (effectResult.success()) {
                        data.effects.push_back(std::move(effectResult.value()));
                    }
                }
            }
        }
    }
}

} // namespace mc::world::save::data
