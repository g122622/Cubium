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

#include "ScoreboardDataManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/storage/ScoreboardSaveData.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "common/world/storage/db/ColumnFamilies.hpp"
#include "common/world/storage/db/RocksDBDatabase.hpp"
#include <cstddef>
#include <exception>
#include <ios>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::scoreboard {

// ========== 键生成 ==========

std::vector<u8> ScoreboardDataManager::makeObjectiveKey(const std::string& name)
{
    std::string key = std::string(KEY_PREFIX_OBJECTIVES) + name;
    return std::vector<u8>(key.begin(), key.end());
}

std::vector<u8> ScoreboardDataManager::makeScoreKey(const std::string& objectiveName, const std::string& playerName)
{
    std::string key = std::string(KEY_PREFIX_SCORES) + objectiveName + ":" + playerName;
    return std::vector<u8>(key.begin(), key.end());
}

std::vector<u8> ScoreboardDataManager::makeTeamKey(const std::string& name)
{
    std::string key = std::string(KEY_PREFIX_TEAMS) + name;
    return std::vector<u8>(key.begin(), key.end());
}

Result<std::pair<std::string, std::string>> ScoreboardDataManager::parseScoreKey(const std::vector<u8>& key)
{
    std::string keyStr(key.begin(), key.end());

    // 格式: "score:objective:player"
    const std::string prefix(KEY_PREFIX_SCORES);
    if (keyStr.size() <= prefix.size() || keyStr.substr(0, prefix.size()) != prefix) {
        return Error(ErrorCode::InvalidData, "Invalid score key format");
    }

    std::string rest = keyStr.substr(prefix.size());
    size_t colonPos = rest.find(':');
    if (colonPos == std::string::npos) {
        return Error(ErrorCode::InvalidData, "Invalid score key format: missing colon");
    }

    std::string objectiveName = rest.substr(0, colonPos);
    std::string playerName = rest.substr(colonPos + 1);

    return std::make_pair(objectiveName, playerName);
}

// ========== 辅助函数：NBT序列化 ==========

static Result<std::vector<u8>> serializeNbtToBytes(const nbt::tags::compound_tag& tag)
{
    std::ostringstream oss(std::ios::binary);
    oss << nbt::Contexts::java;
    nbt::operator<<(oss, tag);
    if (!oss) {
        return Error(ErrorCode::FileWriteFailed, "Failed to serialize NBT");
    }
    std::string str = oss.str();
    return std::vector<u8>(str.begin(), str.end());
}

static Result<nbt::tags::compound_tag> deserializeNbtFromBytes(const std::vector<u8>& data)
{
    try {
        std::istringstream iss(std::string(data.begin(), data.end()), std::ios::binary);
        iss >> nbt::Contexts::java;
        auto root = nbt::tags::compound_tag::read(iss);
        if (!root) {
            return Error(ErrorCode::InvalidData, "Failed to parse NBT");
        }
        return std::move(*root);
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to deserialize NBT: ") + e.what());
    }
}

// ========== 构造/析构 ==========

ScoreboardDataManager::ScoreboardDataManager(world::storage::SingleLevelStorageManager& storage)
    : m_storage(storage)
{}

ScoreboardDataManager::~ScoreboardDataManager() noexcept
{
    // 自动保存脏数据
    if (dirtyCount() > 0) {
        auto result = saveAllDirty();
        if (result.failed()) {
            spdlog::error("Failed to save dirty scoreboard data: {}", result.error().message());
        }
    }
}

// ========== 目标操作 ==========

Result<void> ScoreboardDataManager::saveObjective(const ScoreboardSaveData::ObjectiveData& objective)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 序列化
    auto serializeResult = serializeNbtToBytes(objective.toNbt());
    if (serializeResult.failed()) {
        return serializeResult.error();
    }

    // 写入数据库
    auto key = makeObjectiveKey(objective.name);
    auto putResult = m_storage._database()->put(world::storage::cf::SCOREBOARD, key, serializeResult.value(), true);
    if (putResult.failed()) {
        return putResult.error();
    }

    // 更新缓存
    m_objectiveCache[objective.name] = objective;
    m_dirtyObjectives.erase(objective.name);

    return {};
}

Result<std::optional<ScoreboardSaveData::ObjectiveData>> ScoreboardDataManager::loadObjective(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 检查缓存
    auto it = m_objectiveCache.find(name);
    if (it != m_objectiveCache.end()) {
        return it->second;
    }

    // 从数据库加载
    auto key = makeObjectiveKey(name);
    auto dataResult = m_storage._database()->get(world::storage::cf::SCOREBOARD, key);
    if (dataResult.failed()) {
        if (dataResult.error().code() == ErrorCode::NotFound) {
            return std::nullopt;
        }
        return dataResult.error();
    }

    if (dataResult.value().empty()) {
        return std::nullopt;
    }

    // 反序列化
    auto nbtResult = deserializeNbtFromBytes(dataResult.value());
    if (nbtResult.failed()) {
        return nbtResult.error();
    }

    auto objResult = ScoreboardSaveData::ObjectiveData::fromNbt(nbtResult.value());
    if (objResult.failed()) {
        return objResult.error();
    }

    // 缓存
    m_objectiveCache[name] = objResult.value();

    return objResult.value();
}

Result<void> ScoreboardDataManager::deleteObjective(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 从数据库删除
    auto key = makeObjectiveKey(name);
    auto delResult = m_storage._database()->del(world::storage::cf::SCOREBOARD, key);
    if (delResult.failed()) {
        return delResult.error();
    }

    // 从缓存删除
    m_objectiveCache.erase(name);
    m_dirtyObjectives.erase(name);

    // 级联删除该目标的所有分数
    // 1. 从缓存中移除该目标的所有分数
    m_scoreCache.erase(name);

    // 2. 从脏分数集合中移除该目标的所有脏分数条目
    std::erase_if(m_dirtyScores, [&name](const std::string& key) {
        // 脏分数键格式: "objectiveName:playerName"
        return key.size() > name.size() && key.substr(0, name.size()) == name && key[name.size()] == ':';
    });

    // 3. 从数据库中删除该目标的所有分数键（使用前缀迭代器）
    auto iter = m_storage._database()->newIterator(world::storage::cf::SCOREBOARD);
    if (iter) {
        const std::string scorePrefix(std::string(KEY_PREFIX_SCORES) + name + ":");
        std::vector<std::vector<u8>> keysToDelete;

        for (iter->Seek(scorePrefix); iter->Valid() && iter->key().starts_with(scorePrefix); iter->Next()) {
            keysToDelete.push_back(std::vector<u8>(iter->key().data(), iter->key().data() + iter->key().size()));
        }

        for (const auto& key : keysToDelete) {
            auto delResult = m_storage._database()->del(world::storage::cf::SCOREBOARD, key);
            if (delResult.failed()) {
                spdlog::warn("Failed to delete score for objective '{}': {}", name, delResult.error().message());
            }
        }
    }

    return {};
}

Result<std::vector<ScoreboardSaveData::ObjectiveData>> ScoreboardDataManager::loadAllObjectives()
{
    std::vector<ScoreboardSaveData::ObjectiveData> objectives;

    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 使用迭代器遍历所有目标
    auto iter = m_storage._database()->newIterator(world::storage::cf::SCOREBOARD);
    if (!iter) {
        return objectives;
    }

    const std::string prefix(KEY_PREFIX_OBJECTIVES);
    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        // 解析键
        std::string keyStr(iter->key().ToString());
        std::string name = keyStr.substr(prefix.size());

        // 检查缓存
        auto it = m_objectiveCache.find(name);
        if (it != m_objectiveCache.end()) {
            objectives.push_back(it->second);
            continue;
        }

        // 反序列化
        auto value = iter->value();
        std::vector<u8> data(value.data(), value.data() + value.size());
        auto nbtResult = deserializeNbtFromBytes(data);
        if (nbtResult.failed()) {
            spdlog::warn("Failed to deserialize objective {}: {}", name, nbtResult.error().message());
            continue;
        }

        auto objResult = ScoreboardSaveData::ObjectiveData::fromNbt(nbtResult.value());
        if (objResult.failed()) {
            spdlog::warn("Failed to parse objective {}: {}", name, objResult.error().message());
            continue;
        }

        // 缓存
        m_objectiveCache[name] = objResult.value();
        objectives.push_back(objResult.value());
    }

    return objectives;
}

// ========== 分数操作 ==========

Result<void> ScoreboardDataManager::saveScore(
    const std::string& objectiveName, const std::string& playerName, i32 score, bool locked)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 创建分数数据
    ScoreboardSaveData::ScoreData scoreData;
    scoreData.playerName = playerName;
    scoreData.objectiveName = objectiveName;
    scoreData.score = score;
    scoreData.locked = locked;

    // 序列化
    auto serializeResult = serializeNbtToBytes(scoreData.toNbt());
    if (serializeResult.failed()) {
        return serializeResult.error();
    }

    // 写入数据库
    auto key = makeScoreKey(objectiveName, playerName);
    auto putResult = m_storage._database()->put(world::storage::cf::SCOREBOARD, key, serializeResult.value(), true);
    if (putResult.failed()) {
        return putResult.error();
    }

    // 更新缓存
    m_scoreCache[objectiveName][playerName] = scoreData;
    m_dirtyScores.erase(objectiveName + ":" + playerName);

    return {};
}

Result<std::optional<ScoreboardSaveData::ScoreData>> ScoreboardDataManager::loadScore(
    const std::string& objectiveName, const std::string& playerName)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 检查缓存
    auto objIt = m_scoreCache.find(objectiveName);
    if (objIt != m_scoreCache.end()) {
        auto playerIt = objIt->second.find(playerName);
        if (playerIt != objIt->second.end()) {
            return playerIt->second;
        }
    }

    // 从数据库加载
    auto key = makeScoreKey(objectiveName, playerName);
    auto dataResult = m_storage._database()->get(world::storage::cf::SCOREBOARD, key);
    if (dataResult.failed()) {
        if (dataResult.error().code() == ErrorCode::NotFound) {
            return std::nullopt;
        }
        return dataResult.error();
    }

    if (dataResult.value().empty()) {
        return std::nullopt;
    }

    // 反序列化
    auto nbtResult = deserializeNbtFromBytes(dataResult.value());
    if (nbtResult.failed()) {
        return nbtResult.error();
    }

    auto scoreResult = ScoreboardSaveData::ScoreData::fromNbt(nbtResult.value());
    if (scoreResult.failed()) {
        return scoreResult.error();
    }

    // 缓存
    m_scoreCache[objectiveName][playerName] = scoreResult.value();

    return scoreResult.value();
}

Result<void> ScoreboardDataManager::deleteScore(const std::string& objectiveName, const std::string& playerName)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 从数据库删除
    auto key = makeScoreKey(objectiveName, playerName);
    auto delResult = m_storage._database()->del(world::storage::cf::SCOREBOARD, key);
    if (delResult.failed()) {
        return delResult.error();
    }

    // 从缓存删除
    auto objIt = m_scoreCache.find(objectiveName);
    if (objIt != m_scoreCache.end()) {
        objIt->second.erase(playerName);
    }
    m_dirtyScores.erase(objectiveName + ":" + playerName);

    return {};
}

Result<void> ScoreboardDataManager::deletePlayerScores(const std::string& playerName)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 遍历所有目标的分数
    for (auto& [objectiveName, scores] : m_scoreCache) {
        scores.erase(playerName);
        m_dirtyScores.erase(objectiveName + ":" + playerName);
    }

    // 注意：这里只更新缓存，实际的数据库删除需要在保存时处理
    // 或者通过迭代器遍历所有分数键进行删除

    // 使用迭代器遍历并删除
    auto iter = m_storage._database()->newIterator(world::storage::cf::SCOREBOARD);
    if (!iter) {
        return {};
    }

    const std::string prefix(KEY_PREFIX_SCORES);
    std::vector<std::vector<u8>> keysToDelete;

    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        auto parseResult = parseScoreKey(std::vector<u8>(iter->key().data(), iter->key().data() + iter->key().size()));
        if (parseResult.success() && parseResult.value().second == playerName) {
            keysToDelete.push_back(std::vector<u8>(iter->key().data(), iter->key().data() + iter->key().size()));
        }
    }

    // 批量删除
    for (const auto& key : keysToDelete) {
        auto delResult = m_storage._database()->del(world::storage::cf::SCOREBOARD, key);
        if (delResult.failed()) {
            spdlog::warn("Failed to delete score: {}", delResult.error().message());
        }
    }

    return {};
}

Result<std::vector<ScoreboardSaveData::ScoreData>> ScoreboardDataManager::loadScoresForObjective(
    const std::string& objectiveName)
{
    std::vector<ScoreboardSaveData::ScoreData> scores;

    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 检查缓存
    auto objIt = m_scoreCache.find(objectiveName);
    if (objIt != m_scoreCache.end()) {
        for (const auto& [playerName, scoreData] : objIt->second) {
            scores.push_back(scoreData);
        }
        return scores;
    }

    // 使用迭代器遍历
    auto iter = m_storage._database()->newIterator(world::storage::cf::SCOREBOARD);
    if (!iter) {
        return scores;
    }

    const std::string prefix(std::string(KEY_PREFIX_SCORES) + objectiveName + ":");
    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        auto value = iter->value();
        std::vector<u8> data(value.data(), value.data() + value.size());
        auto nbtResult = deserializeNbtFromBytes(data);
        if (nbtResult.failed()) {
            spdlog::warn("Failed to deserialize score: {}", nbtResult.error().message());
            continue;
        }

        auto scoreResult = ScoreboardSaveData::ScoreData::fromNbt(nbtResult.value());
        if (scoreResult.failed()) {
            spdlog::warn("Failed to parse score: {}", scoreResult.error().message());
            continue;
        }

        scores.push_back(scoreResult.value());
        // 缓存
        m_scoreCache[objectiveName][scoreResult.value().playerName] = scoreResult.value();
    }

    return scores;
}

// ========== 队伍操作 ==========

Result<void> ScoreboardDataManager::saveTeam(const ScoreboardSaveData::TeamData& team)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 序列化
    auto serializeResult = serializeNbtToBytes(team.toNbt());
    if (serializeResult.failed()) {
        return serializeResult.error();
    }

    // 写入数据库
    auto key = makeTeamKey(team.name);
    auto putResult = m_storage._database()->put(world::storage::cf::SCOREBOARD, key, serializeResult.value(), true);
    if (putResult.failed()) {
        return putResult.error();
    }

    // 更新缓存
    m_teamCache[team.name] = team;
    m_dirtyTeams.erase(team.name);

    return {};
}

Result<std::optional<ScoreboardSaveData::TeamData>> ScoreboardDataManager::loadTeam(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 检查缓存
    auto it = m_teamCache.find(name);
    if (it != m_teamCache.end()) {
        return it->second;
    }

    // 从数据库加载
    auto key = makeTeamKey(name);
    auto dataResult = m_storage._database()->get(world::storage::cf::SCOREBOARD, key);
    if (dataResult.failed()) {
        if (dataResult.error().code() == ErrorCode::NotFound) {
            return std::nullopt;
        }
        return dataResult.error();
    }

    if (dataResult.value().empty()) {
        return std::nullopt;
    }

    // 反序列化
    auto nbtResult = deserializeNbtFromBytes(dataResult.value());
    if (nbtResult.failed()) {
        return nbtResult.error();
    }

    auto teamResult = ScoreboardSaveData::TeamData::fromNbt(nbtResult.value());
    if (teamResult.failed()) {
        return teamResult.error();
    }

    // 缓存
    m_teamCache[name] = teamResult.value();

    return teamResult.value();
}

Result<void> ScoreboardDataManager::deleteTeam(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 从数据库删除
    auto key = makeTeamKey(name);
    auto delResult = m_storage._database()->del(world::storage::cf::SCOREBOARD, key);
    if (delResult.failed()) {
        return delResult.error();
    }

    // 从缓存删除
    m_teamCache.erase(name);
    m_dirtyTeams.erase(name);

    return {};
}

Result<std::vector<ScoreboardSaveData::TeamData>> ScoreboardDataManager::loadAllTeams()
{
    std::vector<ScoreboardSaveData::TeamData> teams;

    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 使用迭代器遍历所有队伍
    auto iter = m_storage._database()->newIterator(world::storage::cf::SCOREBOARD);
    if (!iter) {
        return teams;
    }

    const std::string prefix(KEY_PREFIX_TEAMS);
    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        // 解析键
        std::string keyStr(iter->key().ToString());
        std::string name = keyStr.substr(prefix.size());

        // 检查缓存
        auto it = m_teamCache.find(name);
        if (it != m_teamCache.end()) {
            teams.push_back(it->second);
            continue;
        }

        // 反序列化
        auto value = iter->value();
        std::vector<u8> data(value.data(), value.data() + value.size());
        auto nbtResult = deserializeNbtFromBytes(data);
        if (nbtResult.failed()) {
            spdlog::warn("Failed to deserialize team {}: {}", name, nbtResult.error().message());
            continue;
        }

        auto teamResult = ScoreboardSaveData::TeamData::fromNbt(nbtResult.value());
        if (teamResult.failed()) {
            spdlog::warn("Failed to parse team {}: {}", name, teamResult.error().message());
            continue;
        }

        // 缓存
        m_teamCache[name] = teamResult.value();
        teams.push_back(teamResult.value());
    }

    return teams;
}

// ========== 显示槽位操作 ==========

Result<void> ScoreboardDataManager::saveDisplaySlot(i32 slot, const std::string& objectiveName)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 更新缓存中的显示槽位
    bool found = false;
    for (auto& slotData : m_displaySlotCache) {
        if (slotData.slot == slot) {
            slotData.objectiveName = objectiveName;
            found = true;
            break;
        }
    }
    if (!found && !objectiveName.empty()) {
        ScoreboardSaveData::DisplaySlotData slotData;
        slotData.slot = slot;
        slotData.objectiveName = objectiveName;
        m_displaySlotCache.push_back(slotData);
    }

    m_dirtyDisplaySlots = true;

    // 序列化所有显示槽位
    nbt::tags::compound_tag root;
    auto slotsList = std::make_unique<nbt::tags::compound_list_tag>();
    slotsList->value.reserve(m_displaySlotCache.size());
    for (const auto& slotData : m_displaySlotCache) {
        slotsList->value.push_back(slotData.toNbt());
    }
    root.value.emplace("Slots", std::move(slotsList));

    auto serializeResult = serializeNbtToBytes(root);
    if (serializeResult.failed()) {
        return serializeResult.error();
    }

    // 写入数据库
    std::string keyStr(KEY_DISPLAY_SLOTS);
    std::vector<u8> key(keyStr.begin(), keyStr.end());
    auto putResult = m_storage._database()->put(world::storage::cf::SCOREBOARD, key, serializeResult.value(), true);
    if (putResult.failed()) {
        return putResult.error();
    }

    m_dirtyDisplaySlots = false;

    return {};
}

Result<std::vector<ScoreboardSaveData::DisplaySlotData>> ScoreboardDataManager::loadDisplaySlots()
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    if (m_displaySlotsLoaded) {
        return m_displaySlotCache;
    }

    // 从数据库加载
    std::string keyStr(KEY_DISPLAY_SLOTS);
    std::vector<u8> key(keyStr.begin(), keyStr.end());
    auto dataResult = m_storage._database()->get(world::storage::cf::SCOREBOARD, key);
    if (dataResult.failed()) {
        if (dataResult.error().code() == ErrorCode::NotFound) {
            m_displaySlotCache.clear();
            m_displaySlotsLoaded = true;
            return m_displaySlotCache;
        }
        return dataResult.error();
    }

    m_displaySlotCache.clear();

    if (dataResult.value().empty()) {
        m_displaySlotsLoaded = true;
        return m_displaySlotCache;
    }

    // 反序列化
    auto nbtResult = deserializeNbtFromBytes(dataResult.value());
    if (nbtResult.failed()) {
        m_displaySlotsLoaded = true;
        return m_displaySlotCache;
    }

    auto slotsIt = nbtResult.value().value.find("Slots");
    if (slotsIt != nbtResult.value().value.end()) {
        auto* slotsTag = dynamic_cast<const nbt::tags::compound_list_tag*>(slotsIt->second.get());
        if (slotsTag) {
            for (const auto& slotTag : slotsTag->value) {
                auto slotResult = ScoreboardSaveData::DisplaySlotData::fromNbt(slotTag);
                if (slotResult.success()) {
                    m_displaySlotCache.push_back(slotResult.value());
                }
            }
        }
    }

    m_displaySlotsLoaded = true;

    return m_displaySlotCache;
}

// ========== 批量操作 ==========

Result<void> ScoreboardDataManager::saveScoreboard(const Scoreboard& scoreboard)
{
    auto data = ScoreboardSaveData::fromScoreboard(scoreboard);

    // 保存所有目标
    for (const auto& obj : data.objectives()) {
        auto result = saveObjective(obj);
        if (result.failed()) {
            return result.error();
        }
    }

    // 保存所有分数
    for (const auto& score : data.scores()) {
        auto result = saveScore(score.objectiveName, score.playerName, score.score, score.locked);
        if (result.failed()) {
            return result.error();
        }
    }

    // 保存所有队伍
    for (const auto& team : data.teams()) {
        auto result = saveTeam(team);
        if (result.failed()) {
            return result.error();
        }
    }

    // 保存显示槽位
    for (const auto& slot : data.displaySlots()) {
        auto result = saveDisplaySlot(slot.slot, slot.objectiveName);
        if (result.failed()) {
            return result.error();
        }
    }

    return {};
}

Result<void> ScoreboardDataManager::loadScoreboard(Scoreboard& scoreboard)
{
    // 加载所有目标
    auto objectivesResult = loadAllObjectives();
    if (objectivesResult.failed()) {
        return objectivesResult.error();
    }

    // 加载所有队伍
    auto teamsResult = loadAllTeams();
    if (teamsResult.failed()) {
        return teamsResult.error();
    }

    // 加载显示槽位
    auto slotsResult = loadDisplaySlots();
    if (slotsResult.failed()) {
        return slotsResult.error();
    }

    // 构建持久化数据
    ScoreboardSaveData data;
    for (const auto& obj : objectivesResult.value()) {
        data.addObjective(obj);
    }
    for (const auto& team : teamsResult.value()) {
        data.addTeam(team);
    }
    for (const auto& slot : slotsResult.value()) {
        data.addDisplaySlot(slot);
    }

    // 加载所有目标的分数
    for (const auto& obj : objectivesResult.value()) {
        auto scoresResult = loadScoresForObjective(obj.name);
        if (scoresResult.success()) {
            for (const auto& score : scoresResult.value()) {
                data.addScore(score);
            }
        }
    }

    // 应用到记分板
    return data.applyToScoreboard(scoreboard);
}

Result<size_t> ScoreboardDataManager::saveAllDirty()
{
    size_t savedCount = 0;

    // 保存脏目标
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        for (const auto& name : m_dirtyObjectives) {
            auto it = m_objectiveCache.find(name);
            if (it != m_objectiveCache.end()) {
                auto result = saveObjective(it->second);
                if (result.success()) {
                    ++savedCount;
                }
            }
        }
        m_dirtyObjectives.clear();
    }

    // 保存脏分数
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        for (const auto& key : m_dirtyScores) {
            size_t colonPos = key.find(':');
            if (colonPos == std::string::npos) continue;

            std::string objectiveName = key.substr(0, colonPos);
            std::string playerName = key.substr(colonPos + 1);

            auto objIt = m_scoreCache.find(objectiveName);
            if (objIt != m_scoreCache.end()) {
                auto playerIt = objIt->second.find(playerName);
                if (playerIt != objIt->second.end()) {
                    auto result = saveScore(objectiveName, playerName, playerIt->second.score, playerIt->second.locked);
                    if (result.success()) {
                        ++savedCount;
                    }
                }
            }
        }
        m_dirtyScores.clear();
    }

    // 保存脏队伍
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        for (const auto& name : m_dirtyTeams) {
            auto it = m_teamCache.find(name);
            if (it != m_teamCache.end()) {
                auto result = saveTeam(it->second);
                if (result.success()) {
                    ++savedCount;
                }
            }
        }
        m_dirtyTeams.clear();
    }

    // 保存脏显示槽位
    if (m_dirtyDisplaySlots) {
        for (const auto& slot : m_displaySlotCache) {
            auto result = saveDisplaySlot(slot.slot, slot.objectiveName);
            if (result.success()) {
                ++savedCount;
            }
        }
        m_dirtyDisplaySlots = false;
    }

    return savedCount;
}

void ScoreboardDataManager::clearCache()
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_objectiveCache.clear();
    m_scoreCache.clear();
    m_teamCache.clear();
    m_displaySlotCache.clear();
    m_dirtyObjectives.clear();
    m_dirtyScores.clear();
    m_dirtyTeams.clear();
    m_dirtyDisplaySlots = false;
    m_displaySlotsLoaded = false;
}

size_t ScoreboardDataManager::cacheSize() const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    size_t size = m_objectiveCache.size() + m_teamCache.size() + m_displaySlotCache.size();
    for (const auto& [obj, scores] : m_scoreCache) {
        size += scores.size();
    }
    return size;
}

size_t ScoreboardDataManager::dirtyCount() const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_dirtyObjectives.size() + m_dirtyScores.size() + m_dirtyTeams.size() + (m_dirtyDisplaySlots ? 1 : 0);
}

} // namespace mc::scoreboard
