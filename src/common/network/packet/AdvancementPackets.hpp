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

#pragma once

#include "../../advancement/AdvancementFrame.hpp"
#include "../../core/Result.hpp"
#include "../../core/Types.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "PacketSerializer.hpp"
#include <map>
#include <optional>
#include <set>
#include <vector>

// Forward declarations
namespace mc::text {
class ITextComponent;
}

namespace mc::advancement {
class Advancement;
class AdvancementProgress;
class AdvancementDisplay;
class AdvancementRewards;
struct Criterion;
} // namespace mc::advancement

namespace mc {

// ============================================================================
// 成就显示信息同步数据
// ============================================================================

/**
 * @brief 成就显示信息同步数据
 *
 * 用于网络同步的成就显示信息。
 */
struct AdvancementDisplayData {
    ItemStack icon;
    std::string title;       // JSON序列化的文本组件
    std::string description; // JSON序列化的文本组件
    advancement::AdvancementFrame frame = advancement::AdvancementFrame::Task;
    bool showToast = true;
    bool announceToChat = true;
    bool hidden = false;
    std::optional<ResourceLocation> background;

    // 序列化
    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<AdvancementDisplayData> deserialize(network::PacketDeserializer& deser);
};

// ============================================================================
// 成就奖励同步数据
// ============================================================================

/**
 * @brief 成就奖励同步数据
 *
 * 用于网络同步的成就奖励信息。
 */
struct AdvancementRewardsData {
    u32 experience = 0;
    std::vector<ResourceLocation> recipes;
    std::vector<ResourceLocation> loot;
    std::optional<ResourceLocation> functionId;

    // 序列化
    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<AdvancementRewardsData> deserialize(network::PacketDeserializer& deser);
};

// ============================================================================
// 成就同步数据
// ============================================================================

/**
 * @brief 成就同步数据
 *
 * 用于网络同步的单个成就信息。
 */
struct AdvancementData {
    ResourceLocation id;
    std::optional<ResourceLocation> parent;
    std::optional<AdvancementDisplayData> display;
    std::optional<AdvancementRewardsData> rewards;
    std::map<std::string, std::string> criteria; // criterionName -> triggerId
    std::vector<std::vector<std::string>> requirements;

    // 序列化
    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<AdvancementData> deserialize(network::PacketDeserializer& deser);
};

// ============================================================================
// 条件进度同步数据
// ============================================================================

/**
 * @brief 条件进度同步数据
 *
 * 用于网络同步的条件进度信息。
 */
struct CriterionProgressData {
    std::string criterionName;
    std::optional<i64> obtainedTime; // 毫秒时间戳

    // 序列化
    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<CriterionProgressData> deserialize(network::PacketDeserializer& deser);
};

// ============================================================================
// 成就进度同步数据
// ============================================================================

/**
 * @brief 成就进度同步数据
 *
 * 用于网络同步的成就进度信息。
 */
struct AdvancementProgressData {
    ResourceLocation advancementId;
    std::vector<CriterionProgressData> criteria;

    // 序列化
    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<AdvancementProgressData> deserialize(network::PacketDeserializer& deser);
};

// ============================================================================
// 成就信息同步包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 成就信息同步包
 *
 * 同步成就定义和进度到客户端。
 * 参考: MC 1.16.5 SPacketAdvancementInfo
 */
class AdvancementInfoPacket {
public:
    AdvancementInfoPacket() = default;

    // Getters
    [[nodiscard]] bool firstSync() const { return m_firstSync; }
    [[nodiscard]] const std::vector<AdvancementData>& advancementsToAdd() const { return m_advancementsToAdd; }
    [[nodiscard]] const std::set<ResourceLocation>& advancementsToRemove() const { return m_advancementsToRemove; }
    [[nodiscard]] const std::map<ResourceLocation, AdvancementProgressData>& progress() const { return m_progress; }

    // Setters
    void setFirstSync(bool first) { m_firstSync = first; }
    void addAdvancement(const AdvancementData& data) { m_advancementsToAdd.push_back(data); }
    void removeAdvancement(const ResourceLocation& id) { m_advancementsToRemove.insert(id); }
    void setProgress(const ResourceLocation& id, const AdvancementProgressData& data) { m_progress[id] = data; }
    void setAdvancementsToAdd(std::vector<AdvancementData> data) { m_advancementsToAdd = std::move(data); }
    void setAdvancementsToRemove(std::set<ResourceLocation> data) { m_advancementsToRemove = std::move(data); }
    void setProgress(std::map<ResourceLocation, AdvancementProgressData> data) { m_progress = std::move(data); }

    // 序列化
    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<AdvancementInfoPacket> deserialize(network::PacketDeserializer& deser);

private:
    bool m_firstSync = false;
    std::vector<AdvancementData> m_advancementsToAdd;
    std::set<ResourceLocation> m_advancementsToRemove;
    std::map<ResourceLocation, AdvancementProgressData> m_progress;
};

// ============================================================================
// 成就标签页选择包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 成就标签页选择包
 *
 * 服务端通知客户端选中的成就标签页。
 * 参考: MC 1.16.5 SPacketSelectAdvancementsTab
 */
class SelectAdvancementTabPacket {
public:
    SelectAdvancementTabPacket() = default;

    /**
     * @brief 构造标签页选择包
     * @param tab 选中的标签页ID（可选，空表示关闭）
     */
    explicit SelectAdvancementTabPacket(const std::optional<ResourceLocation>& tab)
        : m_tab(tab)
    {}

    // Getters
    [[nodiscard]] const std::optional<ResourceLocation>& tab() const { return m_tab; }
    [[nodiscard]] bool hasTab() const { return m_tab.has_value(); }

    // Setters
    void setTab(const std::optional<ResourceLocation>& tab) { m_tab = tab; }
    void clearTab() { m_tab = std::nullopt; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<SelectAdvancementTabPacket> deserialize(network::PacketDeserializer& deser);

private:
    std::optional<ResourceLocation> m_tab;
};

// ============================================================================
// 成就界面操作包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 成就界面操作类型
 */
enum class AdvancementAction : u8 {
    OpenedTab = 0,   // 打开标签页
    ClosedScreen = 1 // 关闭界面
};

/**
 * @brief 成就界面操作包
 *
 * 客户端通知服务端成就界面操作。
 * 参考: MC 1.16.5 CPacketSeenAdvancements
 */
class SeenAdvancementsPacket {
public:
    SeenAdvancementsPacket() = default;

    /**
     * @brief 构造打开标签页操作
     */
    static SeenAdvancementsPacket openedTab(const ResourceLocation& tab)
    {
        SeenAdvancementsPacket packet;
        packet.m_action = AdvancementAction::OpenedTab;
        packet.m_tab = tab;
        return packet;
    }

    /**
     * @brief 构造关闭界面操作
     */
    static SeenAdvancementsPacket closedScreen()
    {
        SeenAdvancementsPacket packet;
        packet.m_action = AdvancementAction::ClosedScreen;
        return packet;
    }

    // Getters
    [[nodiscard]] AdvancementAction action() const { return m_action; }
    [[nodiscard]] const std::optional<ResourceLocation>& tab() const { return m_tab; }
    [[nodiscard]] bool isOpenedTab() const { return m_action == AdvancementAction::OpenedTab; }
    [[nodiscard]] bool isClosedScreen() const { return m_action == AdvancementAction::ClosedScreen; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<SeenAdvancementsPacket> deserialize(network::PacketDeserializer& deser);

private:
    AdvancementAction m_action = AdvancementAction::ClosedScreen;
    std::optional<ResourceLocation> m_tab;
};

} // namespace mc
