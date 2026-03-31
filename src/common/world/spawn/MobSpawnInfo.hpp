#pragma once

#include "../../core/Types.hpp"
#include "../../entity/core/EntityClassification.hpp"
#include <vector>
#include <string>
#include <unordered_map>

namespace mc::world::spawn {

/**
 * @brief 生成成本
 *
 * 定义在特定位置生成实体的成本，
 * 用于限制高密度区域内的实体数量。
 *
 * 参考 MC 1.16.5 SpawnCosts
 *
 * 字段命名与MC原版对应：
 * - energy_budget: 能量预算，限制区域内的总生成成本
 * - charge: 单个实体的"充电"成本
 */
struct SpawnCosts {
    /// 能量预算（区域内允许的最大生成成本总和）
    /// 对应MC原版的 energy_budget
    f64 energyBudget = 0.0;

    /// 单个实体的充电成本（增加给区域的成本）
    /// 对应MC原版的 charge
    f64 charge = 0.0;

    SpawnCosts() = default;

    /**
     * @brief 构造生成成本
     * @param budget 能量预算（最大总成本）
     * @param chargePerEntity 每个实体的成本
     */
    SpawnCosts(f64 budget, f64 chargePerEntity)
        : energyBudget(budget), charge(chargePerEntity) {}

    /**
     * @brief 检查是否有效（有成本限制）
     */
    [[nodiscard]] bool isValid() const {
        return energyBudget > 0.0 && charge > 0.0;
    }
};

/**
 * @brief 生成权重条目
 *
 * 定义单个实体类型的生成配置。
 *
 * 参考 MC 1.16.5 MobSpawnInfo.SpawnCosts
 */
struct SpawnEntry {
    /// 实体类型ID（字符串标识符）
    String entityTypeId;

    /// 生成权重（越高越容易被选中）
    i32 weight = 0;

    /// 最小生成数量
    i32 minCount = 1;

    /// 最大生成数量
    i32 maxCount = 4;

    /// 生成成本（可选）
    SpawnCosts costs;

    SpawnEntry() = default;

    SpawnEntry(String typeId, i32 w, i32 minC, i32 maxC)
        : entityTypeId(std::move(typeId))
        , weight(w)
        , minCount(minC)
        , maxCount(maxC)
    {}

    SpawnEntry(String typeId, i32 w, i32 minC, i32 maxC, SpawnCosts c)
        : entityTypeId(std::move(typeId))
        , weight(w)
        , minCount(minC)
        , maxCount(maxC)
        , costs(c)
    {}
};

/**
 * @brief 生成分类信息
 *
 * 定义特定实体分类（怪物、动物、环境等）的生成配置。
 *
 * 参考 MC 1.16.5 MobSpawnInfo.Spawners
 */
struct SpawnCategory {
    /// 该分类的生成条目列表
    std::vector<SpawnEntry> entries;

    /// 该分类的最大实例数
    i32 maxInstances = 0;

    /// 是否启用
    bool enabled = true;

    SpawnCategory() = default;

    explicit SpawnCategory(i32 maxInst) : maxInstances(maxInst) {}

    void addEntry(const SpawnEntry& entry) {
        entries.push_back(entry);
    }

    void addEntry(SpawnEntry&& entry) {
        entries.push_back(std::move(entry));
    }

    /**
     * @brief 计算所有条目的总权重
     */
    [[nodiscard]] i32 getTotalWeight() const {
        i32 total = 0;
        for (const auto& entry : entries) {
            total += entry.weight;
        }
        return total;
    }
};

/**
 * @brief 生物群系实体生成信息
 *
 * 定义在特定生物群系中可以生成的实体类型及其配置。
 * 包含生成概率、各分类的生成列表、生成成本等完整信息。
 *
 * 参考 MC 1.16.5 MobSpawnInfo
 */
class MobSpawnInfo {
public:
    /**
     * @brief 构建器类
     *
     * 用于流式构建 MobSpawnInfo 对象
     */
    class Builder {
    public:
        Builder() = default;

        /**
         * @brief 添加生成条目
         * @param classification 实体分类
         * @param entry 生成条目
         * @return Builder引用
         */
        Builder& addSpawn(entity::EntityClassification classification, const SpawnEntry& entry) {
            switch (classification) {
                case entity::EntityClassification::Monster:
                    m_monsters.addEntry(entry);
                    break;
                case entity::EntityClassification::Creature:
                    m_creatures.addEntry(entry);
                    break;
                case entity::EntityClassification::Ambient:
                    m_ambient.addEntry(entry);
                    break;
                case entity::EntityClassification::WaterCreature:
                    m_waterCreatures.addEntry(entry);
                    break;
                case entity::EntityClassification::WaterAmbient:
                    m_waterAmbient.addEntry(entry);
                    break;
                case entity::EntityClassification::Misc:
                    m_misc.addEntry(entry);
                    break;
            }
            return *this;
        }

        /**
         * @brief 设置生成成本
         * @param entityTypeId 实体类型ID
         * @param costs 生成成本
         * @return Builder引用
         */
        Builder& setSpawnCost(const String& entityTypeId, const SpawnCosts& costs) {
            m_spawnCosts[entityTypeId] = costs;
            return *this;
        }

        /**
         * @brief 设置动物生成概率
         * @param probability 概率值（默认 0.1F）
         * @return Builder引用
         */
        Builder& setCreatureSpawnProbability(f32 probability) {
            m_creatureSpawnProbability = probability;
            return *this;
        }

        /**
         * @brief 设置为适合玩家生成
         * @return Builder引用
         */
        Builder& setPlayerSpawnFriendly() {
            m_playerSpawnFriendly = true;
            return *this;
        }

        /**
         * @brief 构建最终的 MobSpawnInfo
         */
        MobSpawnInfo build() const {
            MobSpawnInfo info;
            info.m_monsters = m_monsters;
            info.m_creatures = m_creatures;
            info.m_ambient = m_ambient;
            info.m_waterCreatures = m_waterCreatures;
            info.m_waterAmbient = m_waterAmbient;
            info.m_misc = m_misc;
            info.m_spawnCosts = m_spawnCosts;
            info.m_creatureSpawnProbability = m_creatureSpawnProbability;
            info.m_playerSpawnFriendly = m_playerSpawnFriendly;
            return info;
        }

    private:
        SpawnCategory m_monsters;
        SpawnCategory m_creatures;
        SpawnCategory m_ambient;
        SpawnCategory m_waterCreatures;
        SpawnCategory m_waterAmbient;
        SpawnCategory m_misc;
        std::unordered_map<String, SpawnCosts> m_spawnCosts;
        f32 m_creatureSpawnProbability = 0.1f;
        bool m_playerSpawnFriendly = false;
    };

    MobSpawnInfo() = default;

    // ========== 生成条目管理 ==========

    /**
     * @brief 添加怪物生成条目
     */
    void addMonsterSpawn(const SpawnEntry& entry) {
        m_monsters.addEntry(entry);
    }

    /**
     * @brief 添加动物生成条目
     */
    void addCreatureSpawn(const SpawnEntry& entry) {
        m_creatures.addEntry(entry);
    }

    /**
     * @brief 添加环境生物生成条目（蝙蝠等）
     */
    void addAmbientSpawn(const SpawnEntry& entry) {
        m_ambient.addEntry(entry);
    }

    /**
     * @brief 添加水生生物生成条目
     */
    void addWaterCreatureSpawn(const SpawnEntry& entry) {
        m_waterCreatures.addEntry(entry);
    }

    /**
     * @brief 添加水生环境生物生成条目（小鱼等）
     */
    void addWaterAmbientSpawn(const SpawnEntry& entry) {
        m_waterAmbient.addEntry(entry);
    }

    /**
     * @brief 添加其他生成条目
     */
    void addMiscSpawn(const SpawnEntry& entry) {
        m_misc.addEntry(entry);
    }

    /**
     * @brief 设置生成成本
     * @param entityTypeId 实体类型ID
     * @param costs 生成成本
     */
    void setSpawnCost(const String& entityTypeId, const SpawnCosts& costs) {
        m_spawnCosts[entityTypeId] = costs;
    }

    // ========== 获取生成条目 ==========

    /**
     * @brief 根据实体分类获取生成列表
     * @param classification 实体分类
     * @return 对应分类的生成列表
     *
     * 参考 MC 1.16.5 MobSpawnInfo.func_242559_a
     */
    [[nodiscard]] const std::vector<SpawnEntry>& getSpawns(
        entity::EntityClassification classification) const {
        switch (classification) {
            case entity::EntityClassification::Monster:
                return m_monsters.entries;
            case entity::EntityClassification::Creature:
                return m_creatures.entries;
            case entity::EntityClassification::Ambient:
                return m_ambient.entries;
            case entity::EntityClassification::WaterCreature:
                return m_waterCreatures.entries;
            case entity::EntityClassification::WaterAmbient:
                return m_waterAmbient.entries;
            case entity::EntityClassification::Misc:
            default:
                return m_misc.entries;
        }
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getMonsterSpawns() const {
        return m_monsters.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getCreatureSpawns() const {
        return m_creatures.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getAmbientSpawns() const {
        return m_ambient.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getWaterCreatureSpawns() const {
        return m_waterCreatures.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getWaterAmbientSpawns() const {
        return m_waterAmbient.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getMiscSpawns() const {
        return m_misc.entries;
    }

    /**
     * @brief 获取实体的生成成本
     * @param entityTypeId 实体类型ID
     * @return 生成成本指针，如果不存在返回 nullptr
     *
     * 参考 MC 1.16.5 MobSpawnInfo.func_242558_a
     */
    [[nodiscard]] const SpawnCosts* getSpawnCost(const String& entityTypeId) const {
        auto it = m_spawnCosts.find(entityTypeId);
        return it != m_spawnCosts.end() ? &it->second : nullptr;
    }

    // ========== 分类配置 ==========

    [[nodiscard]] i32 getMaxMonsterInstances() const { return m_monsters.maxInstances; }
    void setMaxMonsterInstances(i32 max) { m_monsters.maxInstances = max; }

    [[nodiscard]] i32 getMaxCreatureInstances() const { return m_creatures.maxInstances; }
    void setMaxCreatureInstances(i32 max) { m_creatures.maxInstances = max; }

    [[nodiscard]] i32 getMaxAmbientInstances() const { return m_ambient.maxInstances; }
    void setMaxAmbientInstances(i32 max) { m_ambient.maxInstances = max; }

    [[nodiscard]] i32 getMaxWaterCreatureInstances() const { return m_waterCreatures.maxInstances; }
    void setMaxWaterCreatureInstances(i32 max) { m_waterCreatures.maxInstances = max; }

    [[nodiscard]] i32 getMaxWaterAmbientInstances() const { return m_waterAmbient.maxInstances; }
    void setMaxWaterAmbientInstances(i32 max) { m_waterAmbient.maxInstances = max; }

    // ========== 生物群系特性 ==========

    /**
     * @brief 获取动物生成概率
     *
     * 参考 MC 1.16.5 MobSpawnInfo.func_242557_a
     * 这是区块生成时放置动物的基础概率
     * 默认值为 0.1F (10%)
     */
    [[nodiscard]] f32 getCreatureSpawnProbability() const {
        return m_creatureSpawnProbability;
    }

    /**
     * @brief 设置动物生成概率
     */
    void setCreatureSpawnProbability(f32 probability) {
        m_creatureSpawnProbability = probability;
    }

    /**
     * @brief 是否适合玩家生成
     *
     * 参考 MC 1.16.5 MobSpawnInfo.func_242562_b
     * 用于判断是否可以在该生物群系生成玩家
     */
    [[nodiscard]] bool isPlayerSpawnFriendly() const {
        return m_playerSpawnFriendly;
    }

    /**
     * @brief 设置是否适合玩家生成
     */
    void setPlayerSpawnFriendly(bool friendly) {
        m_playerSpawnFriendly = friendly;
    }

    // ========== 工厂方法 ==========

    /**
     * @brief 创建平原生物群系的生成信息
     */
    static MobSpawnInfo createPlains();

    /**
     * @brief 创建森林生物群系的生成信息
     */
    static MobSpawnInfo createForest();

    /**
     * @brief 创建沙漠生物群系的生成信息
     */
    static MobSpawnInfo createDesert();

    /**
     * @brief 创建海洋生物群系的生成信息
     */
    static MobSpawnInfo createOcean();

    /**
     * @brief 创建针叶林生物群系的生成信息
     */
    static MobSpawnInfo createTaiga();

    /**
     * @brief 创建丛林生物群系的生成信息
     */
    static MobSpawnInfo createJungle();

    /**
     * @brief 创建热带草原生物群系的生成信息
     */
    static MobSpawnInfo createSavanna();

    /**
     * @brief 创建沼泽生物群系的生成信息
     */
    static MobSpawnInfo createSwamp();

    /**
     * @brief 创建山地生物群系的生成信息
     */
    static MobSpawnInfo createMountains();

    /**
     * @brief 创建雪地生物群系的生成信息
     */
    static MobSpawnInfo createSnowy();

    /**
     * @brief 创建默认（空）生成信息
     */
    static MobSpawnInfo createEmpty();

    // ========================================================================
    // 下界生物群系生成信息
    // ========================================================================

    /**
     * @brief 创建下界荒地生物群系的生成信息
     * 生物：猪灵(15)、僵尸猪灵(5)、恶魂(10)、岩浆怪(3)、末影人(1)、炽足兽(60)
     */
    static MobSpawnInfo createNetherWastes();

    /**
     * @brief 创建灵魂沙谷生物群系的生成信息
     * 生物：骷髅(20)、恶魂(50)、末影人(4)、炽足兽(60)
     */
    static MobSpawnInfo createSoulSandValley();

    /**
     * @brief 创建绯红森林生物群系的生成信息
     * 生物：猪灵(15)、僵尸猪灵(1)、疣猪兽(9)、炽足兽(60)
     */
    static MobSpawnInfo createCrimsonForest();

    /**
     * @brief 创建诡异森林生物群系的生成信息
     * 生物：末影人(80)、炽足兽(60)
     */
    static MobSpawnInfo createWarpedForest();

    /**
     * @brief 创建玄武岩三角洲生物群系的生成信息
     * 生物：岩浆怪(80)、恶魂(2)、炽足兽(60)
     */
    static MobSpawnInfo createBasaltDeltas();

    // ========================================================================
    // 末地生物群系生成信息
    // ========================================================================

    /**
     * @brief 创建末地生物群系的生成信息
     * 生物：末影人(10)
     */
    static MobSpawnInfo createTheEnd();

private:
    SpawnCategory m_monsters;           // 怪物（僵尸、骷髅等）
    SpawnCategory m_creatures;          // 动物（猪、牛、羊等）
    SpawnCategory m_ambient;            // 环境生物（蝙蝠）
    SpawnCategory m_waterCreatures;     // 水生生物（鱿鱼、海豚）
    SpawnCategory m_waterAmbient;       // 水生环境生物（鱼）
    SpawnCategory m_misc;               // 其他

    /// 实体类型到生成成本的映射
    /// 参考 MC 1.16.5 MobSpawnInfo.field_242555_f
    std::unordered_map<String, SpawnCosts> m_spawnCosts;

    /// 动物生成概率
    /// 参考 MC 1.16.5 MobSpawnInfo.field_242553_d
    f32 m_creatureSpawnProbability = 0.1f;

    /// 是否适合玩家生成
    /// 参考 MC 1.16.5 MobSpawnInfo.field_242556_g
    bool m_playerSpawnFriendly = false;

    // 默认实例限制
    static constexpr i32 DEFAULT_MAX_MONSTERS = 70;
    static constexpr i32 DEFAULT_MAX_CREATURES = 10;
    static constexpr i32 DEFAULT_MAX_AMBIENT = 15;
    static constexpr i32 DEFAULT_MAX_WATER_CREATURES = 5;
    static constexpr i32 DEFAULT_MAX_WATER_AMBIENT = 20;
};

} // namespace mc::world::spawn
