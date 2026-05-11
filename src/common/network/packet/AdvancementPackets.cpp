#include "AdvancementPackets.hpp"

namespace mc {

// ============================================================================
// AdvancementDisplayData 实现
// ============================================================================

void AdvancementDisplayData::serialize(network::PacketSerializer& ser) const {
    // 图标物品
    icon.serialize(ser);

    // 标题和描述（JSON字符串）
    ser.writeString(title);
    ser.writeString(description);

    // 框架类型
    ser.writeU8(static_cast<u8>(frame));

    // 标志位
    u8 flags = 0;
    if (showToast) flags |= 0x01;
    if (announceToChat) flags |= 0x02;
    if (hidden) flags |= 0x04;
    ser.writeU8(flags);

    // 背景纹理（仅根成就）
    bool hasBackground = background.has_value();
    ser.writeBool(hasBackground);
    if (hasBackground) {
        ser.writeString(background->toString());
    }
}

Result<AdvancementDisplayData> AdvancementDisplayData::deserialize(network::PacketDeserializer& deser) {
    AdvancementDisplayData data;

    // 图标物品
    auto iconResult = ItemStack::deserialize(deser);
    if (iconResult.failed()) {
        return iconResult.error();
    }
    data.icon = iconResult.value();

    // 标题和描述
    auto titleResult = deser.readString();
    if (titleResult.failed()) {
        return titleResult.error();
    }
    data.title = titleResult.value();

    auto descResult = deser.readString();
    if (descResult.failed()) {
        return descResult.error();
    }
    data.description = descResult.value();

    // 框架类型
    auto frameResult = deser.readU8();
    if (frameResult.failed()) {
        return frameResult.error();
    }
    data.frame = static_cast<advancement::AdvancementFrame>(frameResult.value());

    // 标志位
    auto flagsResult = deser.readU8();
    if (flagsResult.failed()) {
        return flagsResult.error();
    }
    u8 flags = flagsResult.value();
    data.showToast = (flags & 0x01) != 0;
    data.announceToChat = (flags & 0x02) != 0;
    data.hidden = (flags & 0x04) != 0;

    // 背景纹理
    auto hasBackgroundResult = deser.readBool();
    if (hasBackgroundResult.failed()) {
        return hasBackgroundResult.error();
    }
    if (hasBackgroundResult.value()) {
        auto bgResult = deser.readString();
        if (bgResult.failed()) {
            return bgResult.error();
        }
        data.background = ResourceLocation(bgResult.value());
    }

    return data;
}

// ============================================================================
// AdvancementRewardsData 实现
// ============================================================================

void AdvancementRewardsData::serialize(network::PacketSerializer& ser) const {
    // 经验值
    ser.writeVarUInt(experience);

    // 配方列表
    ser.writeVarUInt(static_cast<u32>(recipes.size()));
    for (const auto& recipe : recipes) {
        ser.writeString(recipe.toString());
    }

    // 战利品表列表
    ser.writeVarUInt(static_cast<u32>(loot.size()));
    for (const auto& lootTable : loot) {
        ser.writeString(lootTable.toString());
    }

    // 函数ID
    bool hasFunction = functionId.has_value();
    ser.writeBool(hasFunction);
    if (hasFunction) {
        ser.writeString(functionId->toString());
    }
}

Result<AdvancementRewardsData> AdvancementRewardsData::deserialize(network::PacketDeserializer& deser) {
    AdvancementRewardsData data;

    // 经验值
    auto expResult = deser.readVarUInt();
    if (expResult.failed()) {
        return expResult.error();
    }
    data.experience = expResult.value();

    // 配方列表
    auto recipeCountResult = deser.readVarUInt();
    if (recipeCountResult.failed()) {
        return recipeCountResult.error();
    }
    u32 recipeCount = recipeCountResult.value();
    data.recipes.reserve(recipeCount);
    for (u32 i = 0; i < recipeCount; ++i) {
        auto recipeResult = deser.readString();
        if (recipeResult.failed()) {
            return recipeResult.error();
        }
        data.recipes.emplace_back(recipeResult.value());
    }

    // 战利品表列表
    auto lootCountResult = deser.readVarUInt();
    if (lootCountResult.failed()) {
        return lootCountResult.error();
    }
    u32 lootCount = lootCountResult.value();
    data.loot.reserve(lootCount);
    for (u32 i = 0; i < lootCount; ++i) {
        auto lootResult = deser.readString();
        if (lootResult.failed()) {
            return lootResult.error();
        }
        data.loot.emplace_back(lootResult.value());
    }

    // 函数ID
    auto hasFunctionResult = deser.readBool();
    if (hasFunctionResult.failed()) {
        return hasFunctionResult.error();
    }
    if (hasFunctionResult.value()) {
        auto funcResult = deser.readString();
        if (funcResult.failed()) {
            return funcResult.error();
        }
        data.functionId = ResourceLocation(funcResult.value());
    }

    return data;
}

// ============================================================================
// AdvancementData 实现
// ============================================================================

void AdvancementData::serialize(network::PacketSerializer& ser) const {
    // 成就ID
    ser.writeString(id.toString());

    // 父成就
    bool hasParent = parent.has_value();
    ser.writeBool(hasParent);
    if (hasParent) {
        ser.writeString(parent->toString());
    }

    // 显示信息
    bool hasDisplay = display.has_value();
    ser.writeBool(hasDisplay);
    if (hasDisplay) {
        display->serialize(ser);
    }

    // 奖励
    bool hasRewards = rewards.has_value();
    ser.writeBool(hasRewards);
    if (hasRewards) {
        rewards->serialize(ser);
    }

    // 条件数量和条件列表
    ser.writeVarUInt(static_cast<u32>(criteria.size()));
    for (const auto& [name, triggerId] : criteria) {
        ser.writeString(name);
        ser.writeString(triggerId);
    }

    // 需求矩阵
    ser.writeVarUInt(static_cast<u32>(requirements.size()));
    for (const auto& group : requirements) {
        ser.writeVarUInt(static_cast<u32>(group.size()));
        for (const auto& criterion : group) {
            ser.writeString(criterion);
        }
    }
}

Result<AdvancementData> AdvancementData::deserialize(network::PacketDeserializer& deser) {
    AdvancementData data;

    // 成就ID
    auto idResult = deser.readString();
    if (idResult.failed()) {
        return idResult.error();
    }
    data.id = ResourceLocation(idResult.value());

    // 父成就
    auto hasParentResult = deser.readBool();
    if (hasParentResult.failed()) {
        return hasParentResult.error();
    }
    if (hasParentResult.value()) {
        auto parentResult = deser.readString();
        if (parentResult.failed()) {
            return parentResult.error();
        }
        data.parent = ResourceLocation(parentResult.value());
    }

    // 显示信息
    auto hasDisplayResult = deser.readBool();
    if (hasDisplayResult.failed()) {
        return hasDisplayResult.error();
    }
    if (hasDisplayResult.value()) {
        auto displayResult = AdvancementDisplayData::deserialize(deser);
        if (displayResult.failed()) {
            return displayResult.error();
        }
        data.display = displayResult.value();
    }

    // 奖励
    auto hasRewardsResult = deser.readBool();
    if (hasRewardsResult.failed()) {
        return hasRewardsResult.error();
    }
    if (hasRewardsResult.value()) {
        auto rewardsResult = AdvancementRewardsData::deserialize(deser);
        if (rewardsResult.failed()) {
            return rewardsResult.error();
        }
        data.rewards = rewardsResult.value();
    }

    // 条件列表
    auto criteriaCountResult = deser.readVarUInt();
    if (criteriaCountResult.failed()) {
        return criteriaCountResult.error();
    }
    u32 criteriaCount = criteriaCountResult.value();
    // Note: std::map doesn't have reserve(), using hint insertion for efficiency
    for (u32 i = 0; i < criteriaCount; ++i) {
        auto nameResult = deser.readString();
        if (nameResult.failed()) {
            return nameResult.error();
        }
        auto triggerResult = deser.readString();
        if (triggerResult.failed()) {
            return triggerResult.error();
        }
        data.criteria.emplace(nameResult.value(), triggerResult.value());
    }

    // 需求矩阵
    auto reqCountResult = deser.readVarUInt();
    if (reqCountResult.failed()) {
        return reqCountResult.error();
    }
    u32 reqCount = reqCountResult.value();
    data.requirements.reserve(reqCount);
    for (u32 i = 0; i < reqCount; ++i) {
        auto groupSizeResult = deser.readVarUInt();
        if (groupSizeResult.failed()) {
            return groupSizeResult.error();
        }
        u32 groupSize = groupSizeResult.value();
        std::vector<std::string> group;
        group.reserve(groupSize);
        for (u32 j = 0; j < groupSize; ++j) {
            auto criterionResult = deser.readString();
            if (criterionResult.failed()) {
                return criterionResult.error();
            }
            group.push_back(criterionResult.value());
        }
        data.requirements.push_back(std::move(group));
    }

    return data;
}

// ============================================================================
// CriterionProgressData 实现
// ============================================================================

void CriterionProgressData::serialize(network::PacketSerializer& ser) const {
    ser.writeString(criterionName);
    bool hasObtained = obtainedTime.has_value();
    ser.writeBool(hasObtained);
    if (hasObtained) {
        ser.writeI64(obtainedTime.value());
    }
}

Result<CriterionProgressData> CriterionProgressData::deserialize(network::PacketDeserializer& deser) {
    CriterionProgressData data;

    auto nameResult = deser.readString();
    if (nameResult.failed()) {
        return nameResult.error();
    }
    data.criterionName = nameResult.value();

    auto hasObtainedResult = deser.readBool();
    if (hasObtainedResult.failed()) {
        return hasObtainedResult.error();
    }
    if (hasObtainedResult.value()) {
        auto timeResult = deser.readI64();
        if (timeResult.failed()) {
            return timeResult.error();
        }
        data.obtainedTime = timeResult.value();
    }

    return data;
}

// ============================================================================
// AdvancementProgressData 实现
// ============================================================================

void AdvancementProgressData::serialize(network::PacketSerializer& ser) const {
    ser.writeString(advancementId.toString());

    ser.writeVarUInt(static_cast<u32>(criteria.size()));
    for (const auto& progress : criteria) {
        progress.serialize(ser);
    }
}

Result<AdvancementProgressData> AdvancementProgressData::deserialize(network::PacketDeserializer& deser) {
    AdvancementProgressData data;

    auto idResult = deser.readString();
    if (idResult.failed()) {
        return idResult.error();
    }
    data.advancementId = ResourceLocation(idResult.value());

    auto countResult = deser.readVarUInt();
    if (countResult.failed()) {
        return countResult.error();
    }
    u32 count = countResult.value();
    data.criteria.reserve(count);
    for (u32 i = 0; i < count; ++i) {
        auto progressResult = CriterionProgressData::deserialize(deser);
        if (progressResult.failed()) {
            return progressResult.error();
        }
        data.criteria.push_back(progressResult.value());
    }

    return data;
}

// ============================================================================
// AdvancementInfoPacket 实现
// ============================================================================

void AdvancementInfoPacket::serialize(network::PacketSerializer& ser) const {
    // 是否首次同步
    ser.writeBool(m_firstSync);

    // 要添加/更新的成就
    ser.writeVarUInt(static_cast<u32>(m_advancementsToAdd.size()));
    for (const auto& adv : m_advancementsToAdd) {
        adv.serialize(ser);
    }

    // 要移除的成就
    ser.writeVarUInt(static_cast<u32>(m_advancementsToRemove.size()));
    for (const auto& id : m_advancementsToRemove) {
        ser.writeString(id.toString());
    }

    // 进度更新
    ser.writeVarUInt(static_cast<u32>(m_progress.size()));
    for (const auto& [id, progress] : m_progress) {
        progress.serialize(ser);
    }
}

Result<AdvancementInfoPacket> AdvancementInfoPacket::deserialize(network::PacketDeserializer& deser) {
    AdvancementInfoPacket packet;

    // 是否首次同步
    auto firstSyncResult = deser.readBool();
    if (firstSyncResult.failed()) {
        return firstSyncResult.error();
    }
    packet.m_firstSync = firstSyncResult.value();

    // 要添加/更新的成就
    auto addCountResult = deser.readVarUInt();
    if (addCountResult.failed()) {
        return addCountResult.error();
    }
    u32 addCount = addCountResult.value();
    packet.m_advancementsToAdd.reserve(addCount);
    for (u32 i = 0; i < addCount; ++i) {
        auto advResult = AdvancementData::deserialize(deser);
        if (advResult.failed()) {
            return advResult.error();
        }
        packet.m_advancementsToAdd.push_back(advResult.value());
    }

    // 要移除的成就
    auto removeCountResult = deser.readVarUInt();
    if (removeCountResult.failed()) {
        return removeCountResult.error();
    }
    u32 removeCount = removeCountResult.value();
    for (u32 i = 0; i < removeCount; ++i) {
        auto idResult = deser.readString();
        if (idResult.failed()) {
            return idResult.error();
        }
        packet.m_advancementsToRemove.emplace(idResult.value());
    }

    // 进度更新
    auto progressCountResult = deser.readVarUInt();
    if (progressCountResult.failed()) {
        return progressCountResult.error();
    }
    u32 progressCount = progressCountResult.value();
    for (u32 i = 0; i < progressCount; ++i) {
        auto progressResult = AdvancementProgressData::deserialize(deser);
        if (progressResult.failed()) {
            return progressResult.error();
        }
        auto data = progressResult.value();
        packet.m_progress[data.advancementId] = std::move(data);
    }

    return packet;
}

// ============================================================================
// SelectAdvancementTabPacket 实现
// ============================================================================

void SelectAdvancementTabPacket::serialize(network::PacketSerializer& ser) const {
    bool hasTab = m_tab.has_value();
    ser.writeBool(hasTab);
    if (hasTab) {
        ser.writeString(m_tab->toString());
    }
}

Result<SelectAdvancementTabPacket> SelectAdvancementTabPacket::deserialize(network::PacketDeserializer& deser) {
    SelectAdvancementTabPacket packet;

    auto hasTabResult = deser.readBool();
    if (hasTabResult.failed()) {
        return hasTabResult.error();
    }
    if (hasTabResult.value()) {
        auto tabResult = deser.readString();
        if (tabResult.failed()) {
            return tabResult.error();
        }
        packet.m_tab = ResourceLocation(tabResult.value());
    }

    return packet;
}

// ============================================================================
// SeenAdvancementsPacket 实现
// ============================================================================

void SeenAdvancementsPacket::serialize(network::PacketSerializer& ser) const {
    ser.writeU8(static_cast<u8>(m_action));

    if (m_action == AdvancementAction::OpenedTab && m_tab.has_value()) {
        ser.writeString(m_tab->toString());
    }
}

Result<SeenAdvancementsPacket> SeenAdvancementsPacket::deserialize(network::PacketDeserializer& deser) {
    SeenAdvancementsPacket packet;

    auto actionResult = deser.readU8();
    if (actionResult.failed()) {
        return actionResult.error();
    }
    packet.m_action = static_cast<AdvancementAction>(actionResult.value());

    if (packet.m_action == AdvancementAction::OpenedTab) {
        auto tabResult = deser.readString();
        if (tabResult.failed()) {
            return tabResult.error();
        }
        packet.m_tab = ResourceLocation(tabResult.value());
    }

    return packet;
}

} // namespace mc
