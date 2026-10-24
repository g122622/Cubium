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

#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::world::spawn {

/**
 * @brief 生成成本
 *
 * 定义在特定位置生成实体的成本，
 * 用于限制高密度区域内的实体数量。
 */
struct SpawnCosts {
    /// 能量预算（区域内允许的最大生成成本总和）
    f64 energyBudget = 0.0;

    /// 单个实体的充电成本（增加给区域的成本）
    f64 charge = 0.0;

    SpawnCosts() = default;

    /**
     * @brief 构造生成成本
     * @param budget 能量预算（最大总成本）
     * @param chargePerEntity 每个实体的成本
     */
    SpawnCosts(f64 budget, f64 chargePerEntity) noexcept
        : energyBudget(budget)
        , charge(chargePerEntity)
    {}

    /**
     * @brief 检查是否有效（有成本限制）
     */
    [[nodiscard]] bool isValid() const noexcept
    {
        return energyBudget > 0.0 && charge > 0.0;
    }
};

/**
 * @brief 生成权重条目
 *
 * 定义单个实体类型的生成配置。
 */
struct SpawnEntry {
    /// 实体类型ID（字符串标识符）
    std::string entityTypeId;

    /// 生成权重（越高越容易被选中）
    i32 weight = 0;

    /// 最小生成数量
    i32 minCount = 1;

    /// 最大生成数量
    i32 maxCount = 4;

    /// 生成成本（可选）
    SpawnCosts costs;

    SpawnEntry() = default;

    SpawnEntry(std::string typeId, i32 w, i32 minC, i32 maxC) noexcept
        : entityTypeId(std::move(typeId))
        , weight(w)
        , minCount(minC)
        , maxCount(maxC)
    {}

    SpawnEntry(std::string typeId, i32 w, i32 minC, i32 maxC, SpawnCosts c) noexcept
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
 */
struct SpawnCategory {
    /// 该分类的生成条目列表
    std::vector<SpawnEntry> entries;

    /// 该分类的最大实例数
    i32 maxInstances = 0;

    /// 是否启用
    bool enabled = true;

    SpawnCategory() = default;

    explicit SpawnCategory(i32 maxInst) noexcept
        : maxInstances(maxInst)
    {}

    void addEntry(const SpawnEntry& entry)
    {
        entries.push_back(entry);
    }

    void addEntry(SpawnEntry&& entry) noexcept
    {
        entries.push_back(std::move(entry));
    }

    /**
     * @brief 计算所有条目的总权重
     */
    [[nodiscard]] i32 getTotalWeight() const noexcept
    {
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
        Builder& addSpawn(entity::EntityClassification classification, const SpawnEntry& entry)
        {
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
                case entity::EntityClassification::Axolotls:
                    m_axolotls.addEntry(entry);
                    break;
                case entity::EntityClassification::UndergroundWaterCreature:
                    m_undergroundWaterCreatures.addEntry(entry);
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
        Builder& setSpawnCost(const std::string& entityTypeId, const SpawnCosts& costs)
        {
            m_spawnCosts[entityTypeId] = costs;
            return *this;
        }

        /**
         * @brief 设置动物生成概率
         * @param probability 概率值（默认 0.1F）
         * @return Builder引用
         */
        Builder& setCreatureSpawnProbability(f32 probability)
        {
            m_creatureSpawnProbability = probability;
            return *this;
        }

        /**
         * @brief 设置为适合玩家生成
         * @return Builder引用
         */
        Builder& setPlayerSpawnFriendly()
        {
            m_playerSpawnFriendly = true;
            return *this;
        }

        /**
         * @brief 构建最终的 MobSpawnInfo
         */
        MobSpawnInfo build() const
        {
            MobSpawnInfo info;
            info.m_monsters = m_monsters;
            info.m_creatures = m_creatures;
            info.m_ambient = m_ambient;
            info.m_axolotls = m_axolotls;
            info.m_undergroundWaterCreatures = m_undergroundWaterCreatures;
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
        SpawnCategory m_axolotls;
        SpawnCategory m_undergroundWaterCreatures;
        SpawnCategory m_waterCreatures;
        SpawnCategory m_waterAmbient;
        SpawnCategory m_misc;
        std::unordered_map<std::string, SpawnCosts> m_spawnCosts;
        f32 m_creatureSpawnProbability = 0.1f;
        bool m_playerSpawnFriendly = false;
    };

    MobSpawnInfo() = default;

    // ========== 生成条目管理 ==========

    /**
     * @brief 添加怪物生成条目
     */
    void addMonsterSpawn(const SpawnEntry& entry)
    {
        m_monsters.addEntry(entry);
    }

    /**
     * @brief 添加动物生成条目
     */
    void addCreatureSpawn(const SpawnEntry& entry)
    {
        m_creatures.addEntry(entry);
    }

    /**
     * @brief 添加环境生物生成条目（蝙蝠等）
     */
    void addAmbientSpawn(const SpawnEntry& entry)
    {
        m_ambient.addEntry(entry);
    }

    /**
     * @brief 添加美西螈生成条目
     */
    void addAxolotlSpawn(const SpawnEntry& entry)
    {
        m_axolotls.addEntry(entry);
    }

    /**
     * @brief 添加地下水生生物生成条目（发光鱿鱼等）
     */
    void addUndergroundWaterCreatureSpawn(const SpawnEntry& entry)
    {
        m_undergroundWaterCreatures.addEntry(entry);
    }

    /**
     * @brief 添加水生生物生成条目
     */
    void addWaterCreatureSpawn(const SpawnEntry& entry)
    {
        m_waterCreatures.addEntry(entry);
    }

    /**
     * @brief 添加水生环境生物生成条目（小鱼等）
     */
    void addWaterAmbientSpawn(const SpawnEntry& entry)
    {
        m_waterAmbient.addEntry(entry);
    }

    /**
     * @brief 添加其他生成条目
     */
    void addMiscSpawn(const SpawnEntry& entry)
    {
        m_misc.addEntry(entry);
    }

    /**
     * @brief 设置生成成本
     * @param entityTypeId 实体类型ID
     * @param costs 生成成本
     */
    void setSpawnCost(const std::string& entityTypeId, const SpawnCosts& costs)
    {
        m_spawnCosts[entityTypeId] = costs;
    }

    // ========== 获取生成条目 ==========

    /**
     * @brief 根据实体分类获取生成列表
     * @param classification 实体分类
     * @return 对应分类的生成列表
     */
    [[nodiscard]] const std::vector<SpawnEntry>& getSpawns(entity::EntityClassification classification) const
    {
        switch (classification) {
            case entity::EntityClassification::Monster:
                return m_monsters.entries;
            case entity::EntityClassification::Creature:
                return m_creatures.entries;
            case entity::EntityClassification::Ambient:
                return m_ambient.entries;
            case entity::EntityClassification::Axolotls:
                return m_axolotls.entries;
            case entity::EntityClassification::UndergroundWaterCreature:
                return m_undergroundWaterCreatures.entries;
            case entity::EntityClassification::WaterCreature:
                return m_waterCreatures.entries;
            case entity::EntityClassification::WaterAmbient:
                return m_waterAmbient.entries;
            case entity::EntityClassification::Misc:
            default:
                return m_misc.entries;
        }
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getMonsterSpawns() const noexcept
    {
        return m_monsters.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getCreatureSpawns() const noexcept
    {
        return m_creatures.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getAmbientSpawns() const noexcept
    {
        return m_ambient.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getAxolotlSpawns() const noexcept
    {
        return m_axolotls.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getUndergroundWaterCreatureSpawns() const noexcept
    {
        return m_undergroundWaterCreatures.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getWaterCreatureSpawns() const noexcept
    {
        return m_waterCreatures.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getWaterAmbientSpawns() const noexcept
    {
        return m_waterAmbient.entries;
    }

    [[nodiscard]] const std::vector<SpawnEntry>& getMiscSpawns() const noexcept
    {
        return m_misc.entries;
    }

    /**
     * @brief 获取实体的生成成本
     * @param entityTypeId 实体类型ID
     * @return 生成成本指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const SpawnCosts* getSpawnCost(const std::string& entityTypeId) const
    {
        auto it = m_spawnCosts.find(entityTypeId);
        return it != m_spawnCosts.end() ? &it->second : nullptr;
    }

    // ========== 分类配置 ==========

    [[nodiscard]] i32 getMaxMonsterInstances() const noexcept
    {
        return m_monsters.maxInstances;
    }

    void setMaxMonsterInstances(i32 max) noexcept
    {
        m_monsters.maxInstances = max;
    }

    [[nodiscard]] i32 getMaxCreatureInstances() const noexcept
    {
        return m_creatures.maxInstances;
    }

    void setMaxCreatureInstances(i32 max) noexcept
    {
        m_creatures.maxInstances = max;
    }

    [[nodiscard]] i32 getMaxAmbientInstances() const noexcept
    {
        return m_ambient.maxInstances;
    }

    void setMaxAmbientInstances(i32 max) noexcept
    {
        m_ambient.maxInstances = max;
    }

    [[nodiscard]] i32 getMaxAxolotlInstances() const noexcept
    {
        return m_axolotls.maxInstances;
    }

    void setMaxAxolotlInstances(i32 max) noexcept
    {
        m_axolotls.maxInstances = max;
    }

    [[nodiscard]] i32 getMaxUndergroundWaterCreatureInstances() const noexcept
    {
        return m_undergroundWaterCreatures.maxInstances;
    }

    void setMaxUndergroundWaterCreatureInstances(i32 max) noexcept
    {
        m_undergroundWaterCreatures.maxInstances = max;
    }

    [[nodiscard]] i32 getMaxWaterCreatureInstances() const noexcept
    {
        return m_waterCreatures.maxInstances;
    }

    void setMaxWaterCreatureInstances(i32 max) noexcept
    {
        m_waterCreatures.maxInstances = max;
    }

    [[nodiscard]] i32 getMaxWaterAmbientInstances() const noexcept
    {
        return m_waterAmbient.maxInstances;
    }

    void setMaxWaterAmbientInstances(i32 max) noexcept
    {
        m_waterAmbient.maxInstances = max;
    }

    // ========== 生物群系特性 ==========

    /**
     * @brief 获取动物生成概率
     *
     * 这是区块生成时放置动物的基础概率
     * 默认值为 0.1F (10%)
     */
    [[nodiscard]] f32 getCreatureSpawnProbability() const noexcept
    {
        return m_creatureSpawnProbability;
    }

    /**
     * @brief 设置动物生成概率
     */
    void setCreatureSpawnProbability(f32 probability) noexcept
    {
        m_creatureSpawnProbability = probability;
    }

    /**
     * @brief 是否适合玩家生成
     *
     * 用于判断是否可以在该生物群系生成玩家
     */
    [[nodiscard]] bool isPlayerSpawnFriendly() const noexcept
    {
        return m_playerSpawnFriendly;
    }

    /**
     * @brief 设置是否适合玩家生成
     */
    void setPlayerSpawnFriendly(bool friendly) noexcept
    {
        m_playerSpawnFriendly = friendly;
    }

    // ========== 工厂方法 ==========

    // 默认实例限制常量（公开以便工厂实现复用）
    // 取值对齐原版 MobCategory：MONSTER 70、CREATURE 10、AMBIENT 15、AXOLOTLS 5、
    // UNDERGROUND_WATER_CREATURE 5、WATER_CREATURE 5、WATER_AMBIENT 20（均为每区块上限）。
    static constexpr i32 DEFAULT_MAX_MONSTERS = 70;
    static constexpr i32 DEFAULT_MAX_CREATURES = 10;
    static constexpr i32 DEFAULT_MAX_AMBIENT = 15;
    static constexpr i32 DEFAULT_MAX_AXOLOTLS = 5;
    static constexpr i32 DEFAULT_MAX_UNDERGROUND_WATER_CREATURES = 5;
    static constexpr i32 DEFAULT_MAX_WATER_CREATURES = 5;
    static constexpr i32 DEFAULT_MAX_WATER_AMBIENT = 20;

    /**
     * @brief 创建平原生物群系的生成信息
     */
    static MobSpawnInfo createPlains();

    /**
     * @brief 创建森林生物群系的生成信息
     */
    static MobSpawnInfo createForest();

    /**
     * @brief 创建花森林生物群系的生成信息
     * 动物：farmAnimals + 兔子(4, 2-3)；其余为 commonSpawns
     */
    static MobSpawnInfo createFlowerForest();

    /**
     * @brief 创建桦木森林/原始桦木森林生物群系的生成信息
     * 动物：farmAnimals；其余为 commonSpawns（无狼、无兔）
     */
    static MobSpawnInfo createBirchForest();

    /**
     * @brief 创建深色森林生物群系的生成信息
     * 动物：farmAnimals；其余为 commonSpawns
     */
    static MobSpawnInfo createDarkForest();

    /**
     * @brief 创建苍园生物群系的生成信息
     * 不生成动物，仅 commonSpawns（对应 OverworldBiomes.darkForest(paleGarden=true)）
     */
    static MobSpawnInfo createPaleGarden();

    /**
     * @brief 创建恶地/侵蚀恶地生物群系的生成信息
     * 动物：farmAnimals；其余为 commonSpawns；creatureGenerationProbability 0.03
     */
    static MobSpawnInfo createBadlands();

    /**
     * @brief 创建疏林恶地生物群系的生成信息
     * 在恶地基础上追加狼(2, 4-8)，creatureGenerationProbability 0.04
     */
    static MobSpawnInfo createWoodedBadlands();

    /**
     * @brief 创建雪坡生物群系的生成信息
     * 动物：兔子(4, 2-3)；其余为 commonSpawns
     */
    static MobSpawnInfo createSnowySlopes();

    /**
     * @brief 创建蘑菇岛生物群系的生成信息
     * 动物：哞菇(8, 4-8)；环境生物：蝙蝠；地下水生生物：发光鱿鱼
     */
    static MobSpawnInfo createMushroomFields();

    /**
     * @brief 创建石岸生物群系的生成信息
     * 无海龟，仅 commonSpawns
     */
    static MobSpawnInfo createStoneShore();

    /**
     * @brief 创建冻河生物群系的生成信息
     * 与河流一致，但溺尸权重降为 1
     */
    static MobSpawnInfo createFrozenRiver();

    /**
     * @brief 创建红树林沼泽生物群系的生成信息
     * 沼泽刷怪表（无 farmAnimals）+ 热带鱼(25, 8-8)
     */
    static MobSpawnInfo createMangroveSwamp();

    /**
     * @brief 创建沙漠生物群系的生成信息
     */
    static MobSpawnInfo createDesert();

    /**
     * @brief 创建海洋生物群系的生成信息
     */
    static MobSpawnInfo createOcean();

    /**
     * @brief 创建暖水海洋生物群系的生成信息
     */
    static MobSpawnInfo createWarmOcean();

    /**
     * @brief 创建温水海洋生物群系的生成信息
     */
    static MobSpawnInfo createLukewarmOcean();

    /**
     * @brief 创建深海温水海洋生物群系的生成信息
     */
    static MobSpawnInfo createDeepLukewarmOcean();

    /**
     * @brief 创建冷水海洋生物群系的生成信息
     */
    static MobSpawnInfo createColdOcean();

    /**
     * @brief 创建冰冻海洋生物群系的生成信息
     */
    static MobSpawnInfo createFrozenOcean();

    /**
     * @brief 创建深海生物群系的生成信息
     */
    static MobSpawnInfo createDeepOcean();

    /**
     * @brief 创建深海暖水海洋生物群系的生成信息
     */
    static MobSpawnInfo createDeepWarmOcean();

    /**
     * @brief 创建针叶林生物群系的生成信息
     */
    static MobSpawnInfo createTaiga();

    /**
     * @brief 创建丛林生物群系的生成信息
     */
    static MobSpawnInfo createJungle();

    /**
     * @brief 创建稀疏丛林（旧名 JungleEdge）生物群系的生成信息
     */
    static MobSpawnInfo createSparseJungle();

    /**
     * @brief 创建竹林生物群系的生成信息
     */
    static MobSpawnInfo createBambooJungle();

    /**
     * @brief 创建热带草原系生物群系的生成信息（对应 OverworldBiomes.savanna）
     * @param llamaAndWolves true 时为热带草原高原（追加 llama(8,4,4) 与 wolf(8,4,8)），
     *                       false 时为普通/破碎热带草原
     */
    static MobSpawnInfo createSavanna(bool llamaAndWolves = false);

    /**
     * @brief 创建热带草原高原生物群系的生成信息
     */
    static MobSpawnInfo createSavannaPlateau();

    /**
     * @brief 创建破碎热带草原生物群系的生成信息
     */
    static MobSpawnInfo createShatteredSavanna();

    /**
     * @brief 创建沼泽生物群系的生成信息
     */
    static MobSpawnInfo createSwamp();

    /**
     * @brief 创建山地生物群系的生成信息
     */
    static MobSpawnInfo createMountains();

    /**
     * @brief 创建带森林的疾风丘陵生物群系的生成信息
     * 与疾风丘陵（createMountains）一致，仅地形生成不同
     */
    static MobSpawnInfo createWindsweptForest();

    /**
     * @brief 创建雪地生物群系的生成信息
     */
    static MobSpawnInfo createSnowy();

    /**
     * @brief 创建积雪沙滩生物群系的生成信息
     */
    static MobSpawnInfo createSnowyBeach();

    /**
     * @brief 创建沙滩生物群系的生成信息
     * 生物：海龟(5, 2-5)；其余为 commonSpawns
     */
    static MobSpawnInfo createBeach();

    /**
     * @brief 创建河流生物群系的生成信息
     * 水生生物：鱿鱼(2, 1-4)；水生环境生物：鲑鱼(5, 1-5)；怪物：溺尸(100, 1) + commonSpawns
     */
    static MobSpawnInfo createRiver();

    /**
     * @brief 创建草甸生物群系的生成信息
     * 生物：驴(1, 1-2)、兔子(2, 2-6)、羊(2, 2-4)；其余为 commonSpawns
     */
    static MobSpawnInfo createMeadow();

    /**
     * @brief 创建樱花树林生物群系的生成信息
     * 生物：猪(1, 1-2)、兔子(2, 2-6)、羊(2, 2-4)；其余为 commonSpawns
     */
    static MobSpawnInfo createCherryGrove();

    /**
     * @brief 创建雪林（Grove）生物群系的生成信息
     * 生物：狼(1, 1)、兔子(8, 2-3)、狐狸(4, 2-4)；其余为 commonSpawns
     */
    static MobSpawnInfo createGrove();

    /**
     * @brief 创建尖峭山峰类生物群系（尖峭峰/冰封峰/裸岩峰）的生成信息
     * 山羊尚未实现，当前仅有 commonSpawns
     */
    static MobSpawnInfo createGoatPeaks();

    /**
     * @brief 创建溶洞生物群系的生成信息
     * 怪物：commonSpawns + 溺尸(95, 4-4)
     */
    static MobSpawnInfo createDripstoneCaves();

    /**
     * @brief 创建默认（空）生成信息
     */
    static MobSpawnInfo createEmpty();

    // ========================================================================
    // 下界生物群系生成信息
    // ========================================================================

    /**
     * @brief 创建下界荒地生物群系的生成信息
     * 怪物：恶魂(50, 4-4)、僵尸猪灵(100, 4-4)、岩浆怪(2, 4-4)、末影人(1, 4-4)、猪灵(15, 4-4)；动物：炽足兽(60, 1-2)
     */
    static MobSpawnInfo createNetherWastes();

    /**
     * @brief 创建灵魂沙谷生物群系的生成信息
     * 怪物：骷髅(20, 5-5)、恶魂(50, 4-4)、末影人(1, 4-4)；动物：炽足兽(60, 1-2)；生成成本 energyBudget 0.7 / charge
     * 0.15
     */
    static MobSpawnInfo createSoulSandValley();

    /**
     * @brief 创建绯红森林生物群系的生成信息
     * 怪物：僵尸猪灵(1, 2-4)、疣猪兽(9, 3-4)、猪灵(5, 3-4)；动物：炽足兽(60, 1-2)
     */
    static MobSpawnInfo createCrimsonForest();

    /**
     * @brief 创建诡异森林生物群系的生成信息
     * 怪物：末影人(1, 4-4)；动物：炽足兽(60, 1-2)；生成成本 energyBudget 1.0 / charge 0.12
     */
    static MobSpawnInfo createWarpedForest();

    /**
     * @brief 创建玄武岩三角洲生物群系的生成信息
     * 怪物：恶魂(40, 1-1)、岩浆怪(100, 2-5)；动物：炽足兽(60, 1-2)
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

    // ========================================================================
    // 洞穴生物群系生成信息
    // ========================================================================

    /**
     * @brief 创建繁茂洞穴生物群系的生成信息
     * 水生生物：美西螈(10, 4-6)
     * 水生环境生物：热带鱼(25, 8)
     * 怪物：普通洞穴怪物
     */
    static MobSpawnInfo createLushCaves();

private:
    SpawnCategory m_monsters;                  // 怪物（僵尸、骷髅等）
    SpawnCategory m_creatures;                 // 动物（猪、牛、羊等）
    SpawnCategory m_ambient;                   // 环境生物（蝙蝠）
    SpawnCategory m_axolotls;                  // 美西螈（独立分类）
    SpawnCategory m_undergroundWaterCreatures; // 地下水生生物（发光鱿鱼）
    SpawnCategory m_waterCreatures;            // 水生生物（鱿鱼、海豚）
    SpawnCategory m_waterAmbient;              // 水生环境生物（鱼）
    SpawnCategory m_misc;                      // 其他

    /// 实体类型到生成成本的映射
    std::unordered_map<std::string, SpawnCosts> m_spawnCosts;

    /// 动物生成概率
    f32 m_creatureSpawnProbability = 0.1f;

    /// 是否适合玩家生成
    bool m_playerSpawnFriendly = false;
};

} // namespace mc::world::spawn
