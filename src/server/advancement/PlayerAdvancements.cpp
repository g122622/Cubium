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

#include "PlayerAdvancements.hpp"
#include "common/advancement/Advancement.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/AdvancementProgress.hpp"
#include "common/advancement/AdvancementRewards.hpp"
#include "common/advancement/AdvancementVisibilityEvaluator.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/function/FunctionManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc::server {

PlayerAdvancements::PlayerAdvancements(PlayerId playerId)
    : m_playerId(playerId)
{}

PlayerAdvancements::~PlayerAdvancements() noexcept = default;

bool PlayerAdvancements::grantCriterion(mc::advancement::AdvancementPtr advancement, const std::string& criterion)
{
    if (!advancement) {
        return false;
    }

    bool isNewProgress = false;
    auto it = m_progress.find(advancement);
    if (it == m_progress.end()) {
        // 创建新的进度
        auto [inserted, success] = m_progress.emplace(advancement, mc::advancement::AdvancementProgress(advancement));
        if (!success) {
            return false;
        }
        it = inserted;
        isNewProgress = true;

        // 新进度条目：先为该成就的所有条件注册触发器监听
        // grantCriterion 后续会注销已完成条件的监听器
        registerListeners(advancement);
    }

    bool wasDone = it->second.isDone();
    bool changed = it->second.grantCriterion(criterion);
    if (changed) {
        m_progressChanged.insert(advancement);

        // 如果成就刚完成，发放奖励
        if (!wasDone && it->second.isDone()) {
            spdlog::info("Player {} completed advancement: {}", m_playerId, advancement->getId().toString());

            // 发放奖励
            const auto& rewards = advancement->getRewards();
            if (rewards.has_value() && !rewards->isEmpty()) {
                _grantRewards(*rewards);
            }

            // 成就完成后注销所有监听器
            unregisterListeners(advancement);
        } else {
            // 条件完成但成就未完成，注销该条件的监听器
            const auto& criteria = advancement->getCriteria();
            auto criterionIt = criteria.find(criterion);
            if (criterionIt != criteria.end()) {
                const auto triggerId = criterionIt->second.getTrigger();
                auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
                if (triggerBase != nullptr) {
                    triggerBase->removeListenerForCriterion(*this, advancement, criterion);
                }
            }
        }

        // 更新可见性
        _ensureVisibility(advancement);
    } else if (isNewProgress) {
        // 新进度但条件授予无变化（条件已满足），注销不必要的监听器
        unregisterListeners(advancement);
    }

    return changed;
}

bool PlayerAdvancements::revokeCriterion(mc::advancement::AdvancementPtr advancement, const std::string& criterion)
{
    if (!advancement) {
        return false;
    }

    auto it = m_progress.find(advancement);
    if (it == m_progress.end()) {
        return false;
    }

    bool wasDone = it->second.isDone();
    bool changed = it->second.revokeCriterion(criterion);
    if (changed) {
        m_progressChanged.insert(advancement);
        _ensureVisibility(advancement);

        // 如果撤销条件导致已完成→未完成状态变化，需要重新注册监听器
        if (wasDone && !it->second.isDone()) {
            // 成就从已完成变为未完成，重新注册所有未完成条件的监听器
            registerListeners(advancement);
        } else {
            // 仅撤销单个条件，为该条件重新注册监听器
            const auto& criteria = advancement->getCriteria();
            auto criterionIt = criteria.find(criterion);
            if (criterionIt != criteria.end()) {
                const auto triggerId = criterionIt->second.getTrigger();
                auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
                if (triggerBase != nullptr) {
                    const auto& instance = criterionIt->second.getTriggerInstance();
                    if (instance != nullptr) {
                        triggerBase->addListenerForCriterion(*this, advancement, criterion, instance);
                    }
                }
            }
        }
    }

    return changed;
}

bool PlayerAdvancements::grantAllCriteria(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return false;
    }

    bool anyChanged = false;
    for (const auto& [criterionName, _] : advancement->getCriteria()) {
        if (grantCriterion(advancement, criterionName)) {
            anyChanged = true;
        }
    }
    return anyChanged;
}

bool PlayerAdvancements::revokeAllCriteria(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return false;
    }

    bool anyChanged = false;
    for (const auto& [criterionName, _] : advancement->getCriteria()) {
        if (revokeCriterion(advancement, criterionName)) {
            anyChanged = true;
        }
    }
    return anyChanged;
}

mc::advancement::AdvancementProgress* PlayerAdvancements::getProgress(mc::advancement::AdvancementPtr advancement)
{
    auto it = m_progress.find(advancement);
    return it != m_progress.end() ? &it->second : nullptr;
}

const mc::advancement::AdvancementProgress* PlayerAdvancements::getProgress(
    mc::advancement::AdvancementPtr advancement) const
{
    auto it = m_progress.find(advancement);
    return it != m_progress.end() ? &it->second : nullptr;
}

bool PlayerAdvancements::isDone(mc::advancement::AdvancementPtr advancement) const
{
    const auto* progress = getProgress(advancement);
    return progress != nullptr && progress->isDone();
}

bool PlayerAdvancements::hasProgress(mc::advancement::AdvancementPtr advancement) const
{
    const auto* progress = getProgress(advancement);
    return progress != nullptr && progress->hasProgress();
}

bool PlayerAdvancements::isVisible(mc::advancement::AdvancementPtr advancement) const
{
    return m_visible.count(advancement) > 0;
}

const std::set<mc::advancement::AdvancementPtr>& PlayerAdvancements::getVisibleAdvancements() const
{
    return m_visible;
}

const std::set<mc::advancement::AdvancementPtr>& PlayerAdvancements::getProgressChangedAdvancements() const
{
    return m_progressChanged;
}

void PlayerAdvancements::clearProgressChanged()
{
    m_progressChanged.clear();
}

void PlayerAdvancements::onAdvancementsReloaded(mc::advancement::AdvancementManager& manager)
{
    // 缓存 manager 引用，用于后续可见性评估
    m_manager = &manager;

    // 1. 保存当前进度的序列化数据
    nlohmann::json savedProgress = toJson();

    // 2. 注销所有旧的监听器
    for (const auto& [advancement, progress] : m_progress) {
        unregisterListeners(advancement);
    }

    // 3. 清空进度和可见性
    m_progress.clear();
    m_visible.clear();
    m_progressChanged.clear();
    m_visibilityChanged.clear();

    // 4. 从持久化数据恢复进度
    loadFromJson(savedProgress, manager);

    // 5. 初始化新成就（reload 后可能新增了成就定义）
    flushAdvancements(manager);
}

void PlayerAdvancements::flushAdvancements(mc::advancement::AdvancementManager& manager)
{
    // 缓存 manager 引用，用于后续可见性评估
    m_manager = &manager;
    // 遍历成就管理器中所有已注册的成就
    manager.forEach([this](mc::advancement::AdvancementPtr advancement) {
        // 跳过已有进度的成就（loadFromJson 已为它们注册监听器）
        auto it = m_progress.find(advancement);
        if (it != m_progress.end()) {
            // 已有进度，为未完成的成就注册监听器
            if (!it->second.isDone()) {
                registerListeners(advancement);
            }
        } else {
            // 没有进度的新成就，创建空的进度条目并注册监听器
            m_progress.emplace(advancement, mc::advancement::AdvancementProgress(advancement));
            registerListeners(advancement);
        }

        // 检查可见性
        _ensureVisibility(advancement);

        return true; // 继续遍历
    });
}

bool PlayerAdvancements::loadFromJson(const nlohmann::json& json, mc::advancement::AdvancementManager& manager)
{
    if (!json.is_object()) {
        return false;
    }

    // 解析进度
    if (json.contains("advancements")) {
        for (const auto& [advId, progressJson] : json["advancements"].items()) {
            auto advancement = manager.get(mc::ResourceLocation(advId));
            if (!advancement) {
                spdlog::warn("Unknown advancement in player data: {}", advId);
                continue;
            }

            auto progressResult = mc::advancement::AdvancementProgress::fromJson(progressJson, advancement);
            if (progressResult.failed()) {
                spdlog::warn(
                    "Failed to parse advancement progress for {}: {}", advId, progressResult.error().message());
                continue;
            }

            m_progress.emplace(advancement, std::move(progressResult.value()));
        }
    }

    // 更新可见性
    _updateVisibility(&manager);

    // 为所有已加载的成就注册监听器
    for (const auto& [advancement, progress] : m_progress) {
        if (!progress.isDone()) {
            registerListeners(advancement);
        }
    }

    return true;
}

nlohmann::json PlayerAdvancements::toJson() const
{
    nlohmann::json json;

    // 保存进度
    nlohmann::json advancements;
    for (const auto& [advancement, progress] : m_progress) {
        if (progress.hasProgress()) {
            advancements[advancement->getId().toString()] = progress.toJson();
        }
    }
    json["advancements"] = std::move(advancements);

    return json;
}

void PlayerAdvancements::registerListeners(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return;
    }

    // 仅当成就未完成时才注册监听器
    auto it = m_progress.find(advancement);
    if (it != m_progress.end() && it->second.isDone()) {
        return;
    }

    // 为每个未完成的条件注册触发器监听
    for (const auto& [criterionName, criterion] : advancement->getCriteria()) {
        // 跳过已完成的条件
        if (it != m_progress.end()) {
            const auto* criterionProgress = it->second.getCriterion(criterionName);
            if (criterionProgress != nullptr && criterionProgress->isObtained()) {
                continue;
            }
        }

        const auto triggerId = criterion.getTrigger();
        auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
        if (triggerBase == nullptr) {
            spdlog::warn(
                "Unknown trigger: {} for advancement: {}", triggerId.toString(), advancement->getId().toString());
            continue;
        }

        // 获取触发器实例
        const auto& instance = criterion.getTriggerInstance();
        if (instance == nullptr) {
            continue;
        }

        // 通过类型擦除接口注册监听器
        triggerBase->addListenerForCriterion(*this, advancement, criterionName, instance);
    }
}

void PlayerAdvancements::unregisterListeners(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return;
    }

    // 为每个已完成条件或成就已完成时注销监听器
    auto it = m_progress.find(advancement);
    const bool advancementDone = it != m_progress.end() && it->second.isDone();

    for (const auto& [criterionName, criterion] : advancement->getCriteria()) {
        // 跳过未完成的条件（除非整个成就已完成）
        if (!advancementDone && it != m_progress.end()) {
            const auto* criterionProgress = it->second.getCriterion(criterionName);
            if (criterionProgress == nullptr || !criterionProgress->isObtained()) {
                continue;
            }
        }

        const auto triggerId = criterion.getTrigger();
        auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
        if (triggerBase == nullptr) {
            continue;
        }

        // 通过类型擦除接口移除监听器
        triggerBase->removeListenerForCriterion(*this, advancement, criterionName);
    }
}

void PlayerAdvancements::_ensureVisibility(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return;
    }

    // 单个成就的状态变化可能级联影响子成就的可见性，
    // 因此需要重新评估整棵成就树。
    if (m_manager != nullptr) {
        // 记录旧可见性集合
        std::set<mc::advancement::AdvancementPtr> oldVisible = m_visible;

        // 通过 manager 向上找到根节点，然后从根开始重新评估可见性
        mc::advancement::AdvancementVisibilityEvaluator::evaluateVisibilityFromNode(
            advancement,
            *m_manager,
            [this](mc::advancement::Advancement::Ptr adv) { return isDone(adv); },
            [this](mc::advancement::Advancement::Ptr adv, bool visible) {
                if (visible) {
                    m_visible.insert(adv);
                } else {
                    m_visible.erase(adv);
                }
            });

        // 计算可见性变化
        for (const auto& adv : m_visible) {
            if (oldVisible.count(adv) == 0) {
                m_visibilityChanged.insert(adv);
            }
        }
        for (const auto& adv : oldVisible) {
            if (m_visible.count(adv) == 0) {
                m_visibilityChanged.insert(adv);
            }
        }
    } else {
        // 没有 manager 时，使用简化的单成就评估
        bool wasVisible = m_visible.count(advancement) > 0;
        bool shouldShow = this->_shouldShow(advancement);

        if (shouldShow != wasVisible) {
            if (shouldShow) {
                m_visible.insert(advancement);
            } else {
                m_visible.erase(advancement);
            }
            m_visibilityChanged.insert(advancement);
        }
    }
}

void PlayerAdvancements::_updateVisibility(mc::advancement::AdvancementManager* manager)
{
    m_visible.clear();

    if (manager == nullptr) {
        return;
    }

    // 使用 AdvancementVisibilityEvaluator 从每棵树的根开始评估可见性
    for (const auto& root : manager->getRoots()) {
        mc::advancement::AdvancementVisibilityEvaluator::evaluateVisibility(
            root,
            [this](mc::advancement::Advancement::Ptr adv) { return isDone(adv); },
            [this](mc::advancement::Advancement::Ptr adv, bool visible) {
                if (visible) {
                    m_visible.insert(adv);
                }
            });
    }

    m_visibilityChanged.clear();
}

bool PlayerAdvancements::_shouldShow(
    mc::advancement::AdvancementPtr advancement, mc::advancement::AdvancementManager* manager) const
{
    if (!advancement) {
        return false;
    }

    // MC 原版规则：可见性判定仅基于 isDone，不使用 hasProgress。
    // hasProgress（部分完成）在 MC Java 中不作为可见性判据，
    // 这与 AdvancementVisibilityEvaluator 的行为一致。

    // 已完成的成就始终可见
    if (isDone(advancement)) {
        return true;
    }

    // 无 display 的成就始终不可见（技术成就，如配方解锁）
    // 注意：AdvancementVisibilityEvaluator 中 anyChildDone 会将无 display 节点
    // 标记为可见，但客户端/UI 层应进一步过滤不渲染无 display 的成就。
    // _shouldShow 作为简化回退路径，对无 display 节点直接返回不可见。
    const auto& display = advancement->getDisplay();
    if (!display.has_value()) {
        return false;
    }

    // 隐藏成就（hidden=true）在完成前不可见
    if (display->isHidden()) {
        return false;
    }

    // 非隐藏且未完成的成就：检查祖先链的可见性
    // 如果没有 manager 则无法查找父成就，默认不可见
    // （根成就未完成且无已完成祖先 -> 不可见）
    if (manager == nullptr) {
        return false;
    }

    // 向上回溯 VISIBILITY_DEPTH 层祖先，检查是否有已完成的祖先
    // MC 原版规则：未完成的非隐藏成就，只有在 VISIBILITY_DEPTH 层内有已完成的祖先时才可见
    mc::advancement::Advancement::Ptr current = advancement;
    for (i32 depth = 0; depth < mc::advancement::AdvancementVisibilityEvaluator::VISIBILITY_DEPTH; ++depth) {
        const auto& parentId = current->getParent();
        if (!parentId.has_value()) {
            // 已到达根成就，根成就是否可见取决于自身
            // 根成就没有父节点，如果它未完成且非隐藏，则不可见（因为没有已完成的祖先）
            return false;
        }

        mc::advancement::Advancement::Ptr parent = manager->get(parentId.value());
        if (!parent) {
            // 父成就不存在（引用错误），不可见
            return false;
        }

        // 检查父成就的状态
        if (isDone(parent)) {
            // 父成就已完成 -> 当前成就可见
            return true;
        }

        const auto& parentDisplay = parent->getDisplay();
        if (!parentDisplay.has_value()) {
            // 父成就无 display -> 阻断可见性传播（隐藏的技术成就）
            return false;
        }

        if (parentDisplay->isHidden() && !isDone(parent)) {
            // 父成就隐藏且未完成 -> 阻断可见性传播
            return false;
        }

        // 父成就非隐藏且未完成，继续向上查找
        current = parent;
    }

    // 已回溯 VISIBILITY_DEPTH 层，没有找到已完成的祖先
    return false;
}

void PlayerAdvancements::_grantRewards(const mc::advancement::AdvancementRewards& rewards)
{
    // 发放经验值
    if (rewards.getExperience() > 0 && m_player != nullptr) {
        m_player->addExperience(static_cast<i32>(rewards.getExperience()));
    }

    // 解锁配方
    if (!rewards.getRecipes().empty() && m_player != nullptr) {
        m_player->unlockRecipes(rewards.getRecipes());
    }

    // 发放战利品表奖励
    if (!rewards.getLoot().empty() && m_player != nullptr) {
        auto* server = m_player->getServer();
        if (server != nullptr) {
            auto& lootTableManager = server->lootTableManager();
            auto* world = m_player->getWorld();
            if (world != nullptr) {
                for (const auto& lootTableId : rewards.getLoot()) {
                    const auto* table = lootTableManager.getTable(lootTableId.toString());
                    if (table == nullptr) {
                        spdlog::warn("Advancement rewards: loot table {} not found", lootTableId.toString());
                        continue;
                    }

                    // 构建战利品上下文
                    math::Random rng(static_cast<u64>(world->seed()) ^
                        static_cast<u64>(std::hash<std::string>{}(lootTableId.toString())));
                    auto context =
                        loot::LootContextBuilder(*world)
                            .withRandom(rng)
                            .withLootTableResolver(
                                [&lootTableManager](const std::string& id) -> const loot::LootTable* {
                                    return lootTableManager.getTable(id);
                                })
                            .withPredicateResolver(
                                [&lootTableManager](const std::string& id) -> const loot::LootCondition* {
                                    return lootTableManager.getPredicate(id);
                                })
                            .withNullableParameter(loot::LootParams::THIS_ENTITY, static_cast<Entity*>(m_player))
                            .withNullableParameter(loot::LootParams::KILLER_PLAYER, static_cast<Player*>(m_player))
                            .build(loot::LootParameterSets::chest());

                    if (context != nullptr) {
                        auto items = table->generate(*context);
                        auto& inventory = m_player->inventory();
                        for (auto& item : items) {
                            if (item.isEmpty()) {
                                continue;
                            }
                            i32 notAdded = inventory.add(item);
                            if (notAdded > 0) {
                                // 背包已满，将剩余物品掉落在玩家位置
                                ItemStack dropStack = item.copy();
                                dropStack.setCount(notAdded);
                                ItemDropHelper::spawnItemEntity(world,
                                    dropStack,
                                    m_player->x(),
                                    m_player->y() + 0.5,
                                    m_player->z(),
                                    rng,
                                    ItemDropHelper::DEFAULT_PICKUP_DELAY,
                                    m_player->uuid());
                            }
                        }
                    }
                }
            }
        }
    }

    // 执行函数奖励
    if (rewards.getFunction().has_value()) {
        auto* server = m_player ? m_player->getServer() : nullptr;
        if (server != nullptr) {
            auto& functionManager = server->functionManager();
            const auto& functionId = rewards.getFunction().value();
            if (functionManager.hasFunction(functionId)) {
                // 使用游戏循环命令源（权限等级2，抑制输出）
                command::ServerCommandSource gameLoopSource(
                    server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "");
                auto result = functionManager.execute(functionId, gameLoopSource);
                spdlog::debug("Advancement rewards: executed function '{}' ({} commands succeeded, {} failed)",
                    functionId.toString(),
                    result.successCount,
                    result.failureCount);
            } else {
                spdlog::warn("Advancement rewards: function '{}' not found", functionId.toString());
            }
        } else {
            spdlog::warn("Advancement rewards: cannot execute function '{}' - server not available",
                rewards.getFunction()->toString());
        }
    }
}

} // namespace mc::server
