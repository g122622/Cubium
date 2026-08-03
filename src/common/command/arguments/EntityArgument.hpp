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

#include "ArgumentType.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/StringReader.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
class Entity;
class Player;
class ServerPlayer;

namespace command {

/**
 * @brief 实体选择器类型。
 */
enum class EntitySelectorType { SinglePlayer, AllPlayers, AllEntities, RandomPlayer, Self };

/**
 * @brief 实体选择器排序方式。
 */
enum class EntitySelectorSort {
    Nearest,  // 按距离近到远
    Furthest, // 按距离远到近
    Random,   // 随机排序
    Arbitrary // 原始顺序
};

/**
 * @brief 浮点数范围边界。
 *
 * 用于 distance 参数。
 */
class FloatRange {
public:
    FloatRange()
        : m_min(std::nullopt)
        , m_max(std::nullopt)
    {}

    void setMin(f32 min) { m_min = min; }
    void setMax(f32 max) { m_max = max; }

    [[nodiscard]] bool hasMin() const { return m_min.has_value(); }
    [[nodiscard]] bool hasMax() const { return m_max.has_value(); }
    [[nodiscard]] f32 getMin() const { return m_min.value_or(0.0f); }
    [[nodiscard]] f32 getMax() const { return m_max.value_or(std::numeric_limits<f32>::max()); }

    [[nodiscard]] bool isUnbounded() const { return !m_min.has_value() && !m_max.has_value(); }

    [[nodiscard]] bool test(f32 value) const
    {
        if (m_min.has_value() && value < m_min.value()) return false;
        if (m_max.has_value() && value > m_max.value()) return false;
        return true;
    }

    [[nodiscard]] bool testSquared(f32 valueSq) const
    {
        if (m_min.has_value() && valueSq < m_min.value() * m_min.value()) return false;
        if (m_max.has_value() && valueSq > m_max.value() * m_max.value()) return false;
        return true;
    }

    /**
     * @brief 测试角度值是否在范围内（处理角度环绕）
     *
     * 角度范围需要特殊处理，因为角度在 -180 到 180 度之间环绕。
     * 例如，范围 [170..-170] 表示接近正北方向（跨越 180/-180 边界）。
     *
     * @param value 角度值（度），将被规范化到 [-180, 180)
     * @return true 如果角度在范围内
     */
    [[nodiscard]] bool testAngle(f32 value) const noexcept;

private:
    std::optional<f32> m_min;
    std::optional<f32> m_max;
};

/**
 * @brief 整数范围边界。
 *
 * 用于 level 参数。
 */
class IntRange {
public:
    IntRange()
        : m_min(std::nullopt)
        , m_max(std::nullopt)
    {}

    void setMin(i32 min) { m_min = min; }
    void setMax(i32 max) { m_max = max; }

    [[nodiscard]] bool hasMin() const { return m_min.has_value(); }
    [[nodiscard]] bool hasMax() const { return m_max.has_value(); }
    [[nodiscard]] i32 getMin() const { return m_min.value_or(0); }
    [[nodiscard]] i32 getMax() const { return m_max.value_or(std::numeric_limits<i32>::max()); }

    [[nodiscard]] bool isUnbounded() const { return !m_min.has_value() && !m_max.has_value(); }

    [[nodiscard]] bool test(i32 value) const
    {
        if (m_min.has_value() && value < m_min.value()) return false;
        if (m_max.has_value() && value > m_max.value()) return false;
        return true;
    }

private:
    std::optional<i32> m_min;
    std::optional<i32> m_max;
};

/**
 * @brief 实体选择器。
 *
 * 封装命令层需要的实体选择条件。
 */
class EntitySelector {
public:
    EntitySelector() = default;

    explicit EntitySelector(EntitySelectorType type)
        : m_type(type)
    {}

    // ========== 基础属性 ==========

    [[nodiscard]] EntitySelectorType type() const noexcept { return m_type; }
    void setType(EntitySelectorType type) { m_type = type; }

    [[nodiscard]] i32 limit() const noexcept { return m_limit; }
    void setLimit(i32 limit) { m_limit = limit; }

    [[nodiscard]] bool isSelf() const noexcept { return m_isSelf; }
    void setSelf(bool self) { m_isSelf = self; }

    [[nodiscard]] bool includesNonPlayers() const noexcept { return m_includesNonPlayers; }
    void setIncludesNonPlayers(bool includes) { m_includesNonPlayers = includes; }

    [[nodiscard]] bool isSingle() const noexcept { return m_single; }
    void setSingle(bool single) { m_single = single; }

    [[nodiscard]] const std::string& username() const noexcept { return m_username; }
    [[nodiscard]] bool hasUsername() const noexcept { return !m_username.empty(); }
    void setUsername(const std::string& username) { m_username = username; }

    // ========== 新增参数 ==========

    [[nodiscard]] const std::string& usernameNegated() const noexcept { return m_usernameNegated; }
    [[nodiscard]] bool hasUsernameNegated() const noexcept { return !m_usernameNegated.empty(); }
    void setUsernameNegated(const std::string& name) { m_usernameNegated = name; }

    [[nodiscard]] const FloatRange& distance() const noexcept { return m_distance; }
    FloatRange& distance() { return m_distance; }

    [[nodiscard]] const IntRange& level() const noexcept { return m_level; }
    IntRange& level() { return m_level; }

    [[nodiscard]] bool hasX() const noexcept { return m_x.has_value(); }
    [[nodiscard]] bool hasY() const noexcept { return m_y.has_value(); }
    [[nodiscard]] bool hasZ() const noexcept { return m_z.has_value(); }
    [[nodiscard]] f32 getX() const { return m_x.value_or(0.0f); }
    [[nodiscard]] f32 getY() const { return m_y.value_or(0.0f); }
    [[nodiscard]] f32 getZ() const { return m_z.value_or(0.0f); }
    void setX(f32 x) { m_x = x; }
    void setY(f32 y) { m_y = y; }
    void setZ(f32 z) { m_z = z; }

    [[nodiscard]] bool hasDx() const noexcept { return m_dx.has_value(); }
    [[nodiscard]] bool hasDy() const noexcept { return m_dy.has_value(); }
    [[nodiscard]] bool hasDz() const noexcept { return m_dz.has_value(); }
    [[nodiscard]] f32 getDx() const { return m_dx.value_or(0.0f); }
    [[nodiscard]] f32 getDy() const { return m_dy.value_or(0.0f); }
    [[nodiscard]] f32 getDz() const { return m_dz.value_or(0.0f); }
    void setDx(f32 dx) { m_dx = dx; }
    void setDy(f32 dy) { m_dy = dy; }
    void setDz(f32 dz) { m_dz = dz; }

    /**
     * @brief 判断是否存在体积过滤条件（dx/dy/dz）。
     */
    [[nodiscard]] bool hasVolume() const noexcept { return hasDx() || hasDy() || hasDz(); }

    /**
     * @brief 根据 dx/dy/dz 参数构造选择器的相对 AABB。
     *
     * 遵循 MC 原版 EntitySelectorParser.createAabb 逻辑：
     * - 负值 delta 赋给 min 侧，正值 delta 赋给 max 侧
     * - max 侧额外加 1.0（确保选择体积至少包含 1 格）
     * - 如果没有任何 dx/dy/dz 但 distance 有最大值，则从最大距离构造立方体
     *
     * @return 相对坐标下的 AABB，如果没有体积约束则返回 std::nullopt
     */
    [[nodiscard]] std::optional<AxisAlignedBB> createAabb() const
    {
        if (hasVolume()) {
            f32 dx = hasDx() ? getDx() : 0.0f;
            f32 dy = hasDy() ? getDy() : 0.0f;
            f32 dz = hasDz() ? getDz() : 0.0f;

            f32 minX = (dx < 0.0f) ? dx : 0.0f;
            f32 minY = (dy < 0.0f) ? dy : 0.0f;
            f32 minZ = (dz < 0.0f) ? dz : 0.0f;
            f32 maxX = (dx < 0.0f) ? 0.0f : dx;
            f32 maxY = (dy < 0.0f) ? 0.0f : dy;
            f32 maxZ = (dz < 0.0f) ? 0.0f : dz;

            // MC 原版行为：max 侧额外加 1.0
            maxX += 1.0f;
            maxY += 1.0f;
            maxZ += 1.0f;

            return AxisAlignedBB(minX, minY, minZ, maxX, maxY, maxZ);
        }

        if (!m_distance.isUnbounded() && m_distance.hasMax()) {
            // MC 原版行为：当没有 dx/dy/dz 但有 distance 最大值时，
            // 从最大距离构造立方体 AABB
            f32 maxDist = m_distance.getMax();
            return AxisAlignedBB(-maxDist, -maxDist, -maxDist, maxDist + 1.0f, maxDist + 1.0f, maxDist + 1.0f);
        }

        return std::nullopt;
    }

    [[nodiscard]] EntitySelectorSort sort() const noexcept { return m_sort; }
    void setSort(EntitySelectorSort sort) { m_sort = sort; }

    [[nodiscard]] const std::string& entityType() const noexcept { return m_entityType; }
    [[nodiscard]] bool hasEntityType() const noexcept { return !m_entityType.empty(); }
    [[nodiscard]] bool entityTypeNegated() const noexcept { return m_entityTypeNegated; }
    void setEntityType(const std::string& type, bool negated = false)
    {
        m_entityType = type;
        m_entityTypeNegated = negated;
    }

    [[nodiscard]] const std::vector<std::string>& tags() const noexcept { return m_tags; }
    [[nodiscard]] const std::vector<std::string>& tagsNegated() const noexcept { return m_tagsNegated; }
    void addTag(const std::string& tag, bool negated = false)
    {
        if (negated) {
            m_tagsNegated.push_back(tag);
        } else {
            m_tags.push_back(tag);
        }
    }

    [[nodiscard]] const std::string& gameMode() const noexcept { return m_gameMode; }
    [[nodiscard]] bool hasGameMode() const noexcept { return !m_gameMode.empty(); }
    [[nodiscard]] bool gameModeNegated() const noexcept { return m_gameModeNegated; }
    void setGameMode(const std::string& mode, bool negated = false)
    {
        m_gameMode = mode;
        m_gameModeNegated = negated;
    }

    [[nodiscard]] const std::string& team() const noexcept { return m_team; }
    [[nodiscard]] bool hasTeam() const noexcept { return !m_team.empty(); }
    [[nodiscard]] bool teamNegated() const noexcept { return m_teamNegated; }
    void setTeam(const std::string& team, bool negated = false)
    {
        m_team = team;
        m_teamNegated = negated;
    }

    // ========== 旋转角度范围 ==========

    /**
     * @brief 获取俯仰角范围（x_rotation）
     *
     * x_rotation 用于筛选实体的俯仰角度（pitch）。
     * 范围：-90 到 90 度（-90 为直视下方，90 为直视上方）
     */
    [[nodiscard]] const FloatRange& xRotation() const noexcept { return m_xRotation; }
    FloatRange& xRotation() { return m_xRotation; }

    /**
     * @brief 获取偏航角范围（y_rotation）
     *
     * y_rotation 用于筛选实体的偏航角度（yaw）。
     * 范围：-180 到 180 度
     * 注意：角度范围测试会处理 -180/180 边界环绕问题
     */
    [[nodiscard]] const FloatRange& yRotation() const noexcept { return m_yRotation; }
    FloatRange& yRotation() { return m_yRotation; }

    [[nodiscard]] bool currentWorldOnly() const noexcept { return m_currentWorldOnly; }
    void setCurrentWorldOnly(bool currentWorldOnly) { m_currentWorldOnly = currentWorldOnly; }

    // ========== 记分板分数条件 ==========

    /**
     * @brief 获取记分板分数条件。
     *
     * 返回格式: {目标名称: 分数范围}
     * 示例: scores={deaths=1..5,kills=10..} 会返回 {"deaths": IntRange(1, unbounded), "kills": IntRange(10, unbounded)}
     */
    [[nodiscard]] const std::map<std::string, IntRange>& scoreConditions() const noexcept { return m_scores; }
    void addScoreCondition(const std::string& objective, const IntRange& range) { m_scores[objective] = range; }
    [[nodiscard]] bool hasScoreConditions() const noexcept { return !m_scores.empty(); }

    // ========== 进度条件 ==========

    /**
     * @brief 进度条件结构。
     *
     * 存储单个进度的匹配条件：
     * - 如果 isComplete 有值，检查进度是否完成
     * - 如果 criteriaConditions 不为空，检查各个准则的完成状态
     */
    struct AdvancementCondition {
        std::optional<bool> isComplete;                 // 整体完成状态检查
        std::map<std::string, bool> criteriaConditions; // 准则完成状态检查

        [[nodiscard]] bool hasCondition() const noexcept
        {
            return isComplete.has_value() || !criteriaConditions.empty();
        }
    };

    /**
     * @brief 获取进度条件。
     *
     * 返回格式: {进度ID: 条件}
     * 示例: advancements={minecraft:story/root=true} 或 advancements={minecraft:story/root={criteria=true}}
     */
    [[nodiscard]] const std::map<ResourceLocation, AdvancementCondition>& advancementConditions() const noexcept
    {
        return m_advancements;
    }
    void addAdvancementCondition(const ResourceLocation& advancement, const AdvancementCondition& condition)
    {
        m_advancements[advancement] = condition;
    }
    [[nodiscard]] bool hasAdvancementConditions() const noexcept { return !m_advancements.empty(); }

    // ========== NBT 条件 ==========

    /**
     * @brief NBT 条件结构。
     *
     * 存储 NBT 标签匹配条件，支持取反。
     */
    struct NbtCondition {
        std::shared_ptr<nbt::tags::compound_tag> nbt; // NBT 数据
        bool negated = false;                         // 是否取反

        [[nodiscard]] bool hasCondition() const noexcept { return nbt != nullptr; }
    };

    [[nodiscard]] const NbtCondition& nbtCondition() const noexcept { return m_nbt; }
    void setNbtCondition(const NbtCondition& condition) { m_nbt = condition; }
    [[nodiscard]] bool hasNbtCondition() const noexcept { return m_nbt.hasCondition(); }

    // ========== 谓词条件 ==========

    /**
     * @brief 谓词条件结构。
     *
     * 存储战利品表谓词引用，支持取反。
     */
    struct PredicateCondition {
        ResourceLocation predicate; // 谓词 ID
        bool negated = false;       // 是否取反

        [[nodiscard]] bool hasCondition() const noexcept { return predicate.isValid() && !predicate.path().empty(); }
    };

    [[nodiscard]] const PredicateCondition& predicateCondition() const noexcept { return m_predicate; }
    void setPredicateCondition(const PredicateCondition& condition) { m_predicate = condition; }
    [[nodiscard]] bool hasPredicateCondition() const noexcept { return m_predicate.hasCondition(); }

    // ========== 静态工厂方法 ==========

    [[nodiscard]] static EntitySelector self()
    {
        EntitySelector selector(EntitySelectorType::Self);
        selector.m_isSelf = true;
        selector.m_single = true;
        return selector;
    }

    [[nodiscard]] static EntitySelector nearestPlayer()
    {
        EntitySelector selector(EntitySelectorType::SinglePlayer);
        selector.m_single = true;
        selector.m_limit = 1;
        selector.m_sort = EntitySelectorSort::Nearest;
        return selector;
    }

    [[nodiscard]] static EntitySelector allPlayers()
    {
        EntitySelector selector(EntitySelectorType::AllPlayers);
        selector.m_single = false;
        return selector;
    }

    [[nodiscard]] static EntitySelector allEntities()
    {
        EntitySelector selector(EntitySelectorType::AllEntities);
        selector.m_single = false;
        selector.m_includesNonPlayers = true;
        return selector;
    }

    [[nodiscard]] static EntitySelector randomPlayer()
    {
        EntitySelector selector(EntitySelectorType::RandomPlayer);
        selector.m_single = true;
        selector.m_limit = 1;
        selector.m_sort = EntitySelectorSort::Random;
        return selector;
    }

    [[nodiscard]] static EntitySelector byUsername(const std::string& username)
    {
        EntitySelector selector(EntitySelectorType::SinglePlayer);
        selector.m_username = username;
        selector.m_single = true;
        selector.m_limit = 1;
        return selector;
    }

private:
    EntitySelectorType m_type = EntitySelectorType::SinglePlayer;
    i32 m_limit = std::numeric_limits<i32>::max();
    bool m_isSelf = false;
    bool m_includesNonPlayers = false;
    bool m_single = true;
    bool m_currentWorldOnly = false;
    std::string m_username;
    std::string m_usernameNegated; // name=!xxx

    // 新增参数
    FloatRange m_distance;
    IntRange m_level;
    std::optional<f32> m_x, m_y, m_z;    // 坐标偏移
    std::optional<f32> m_dx, m_dy, m_dz; // 体积尺寸
    EntitySelectorSort m_sort = EntitySelectorSort::Arbitrary;
    std::string m_entityType;
    bool m_entityTypeNegated = false;
    std::vector<std::string> m_tags;
    std::vector<std::string> m_tagsNegated;
    std::string m_gameMode;
    bool m_gameModeNegated = false;
    std::string m_team;
    bool m_teamNegated = false;
    FloatRange m_xRotation; // 俯仰角范围（pitch，-90 到 90 度）
    FloatRange m_yRotation; // 偏航角范围（yaw，-180 到 180 度）

    // 新增条件存储
    std::map<std::string, IntRange> m_scores;                        // 记分板分数条件
    std::map<ResourceLocation, AdvancementCondition> m_advancements; // 进度条件
    NbtCondition m_nbt;                                              // NBT 条件
    PredicateCondition m_predicate;                                  // 谓词条件
};

/**
 * @brief 实体参数类型。
 *
 * 支持完整的选择器语法。
 */
class EntityArgumentType : public ArgumentType<EntitySelector> {
public:
    enum class Mode { SingleEntity, MultipleEntities, SinglePlayer, MultiplePlayers };

    explicit EntityArgumentType(Mode mode = Mode::SinglePlayer)
        : m_mode(mode)
    {}

    [[nodiscard]] EntitySelector parse(StringReader& reader) override;

    [[nodiscard]] std::string getTypeName() const override
    {
        switch (m_mode) {
            case Mode::SingleEntity:
                return "entity";
            case Mode::MultipleEntities:
                return "entities";
            case Mode::SinglePlayer:
                return "player";
            case Mode::MultiplePlayers:
                return "players";
        }
        return "entity";
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"Player", "0123", "@p", "@a", "@e", "@r", "@s"};
    }

    /// 序列化 entity 参数的 properties（供 clientbound/minecraft:commands 二进制编码）。
    /// 对齐 Java 1.21.11 minecraft:entity ArgumentType 的 flags 字节：bit0=single, bit1=playersOnly。
    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"single", isSingle()}, {"playersOnly", isPlayersOnly()}};
    }

    [[nodiscard]] static std::shared_ptr<EntityArgumentType> entity()
    {
        return std::make_shared<EntityArgumentType>(Mode::SingleEntity);
    }

    [[nodiscard]] static std::shared_ptr<EntityArgumentType> entities()
    {
        return std::make_shared<EntityArgumentType>(Mode::MultipleEntities);
    }

    [[nodiscard]] static std::shared_ptr<EntityArgumentType> player()
    {
        return std::make_shared<EntityArgumentType>(Mode::SinglePlayer);
    }

    [[nodiscard]] static std::shared_ptr<EntityArgumentType> players()
    {
        return std::make_shared<EntityArgumentType>(Mode::MultiplePlayers);
    }

    [[nodiscard]] bool isSingle() const noexcept
    {
        return m_mode == Mode::SingleEntity || m_mode == Mode::SinglePlayer;
    }

    [[nodiscard]] bool isPlayersOnly() const noexcept
    {
        return m_mode == Mode::SinglePlayer || m_mode == Mode::MultiplePlayers;
    }

    [[nodiscard]] Mode mode() const noexcept { return m_mode; }

private:
    [[nodiscard]] EntitySelector _parseSelector(StringReader& reader, i32 start);
    void _parseSelectorArguments(StringReader& reader, EntitySelector& selector);
    void _applySelectorArgument(
        EntitySelector& selector, const std::string& name, const std::string& value, i32 cursor);
    void _validateSelector(const EntitySelector& selector, i32 start);

    [[nodiscard]] static std::string _readSelectorArgumentToken(StringReader& reader);
    [[nodiscard]] static std::string _readScoresKey(StringReader& reader);
    [[nodiscard]] static std::string _readAdvancementKey(StringReader& reader);
    [[nodiscard]] static std::string _readCriteriaKey(StringReader& reader);
    [[nodiscard]] static bool _shouldInvertValue(StringReader& reader);
    [[nodiscard]] FloatRange _parseFloatRange(StringReader& reader);
    [[nodiscard]] IntRange _parseIntRange(StringReader& reader);

    Mode m_mode;
};

} // namespace command
} // namespace mc
