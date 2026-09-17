#pragma once

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp" // Rotation

#include <string>
#include <nlohmann/json_fwd.hpp> // nlohmann::json（前向声明，.cpp 含完整头）

namespace mc::test {

/**
 * @brief GameTest 测试数据 schema（注册期元数据）。
 *
 * 对齐 Java 1.21.11 `TestData`（10 字段）+ 增补基岩版 `padding`/`batchName`（见校正 2）。
 * 这是数据驱动测试定义的权威 schema：描述一个测试"用什么结构、跑多久、是否必需、如何旋转、
 * 重试几次"等注册期属性。运行期参数（坐标、每行测试数等）见 `TestParameters`。
 *
 * 字段语义：
 * - `environment`：环境定义的注册键名（如 `"default"`/`"gametest:day"`）。Java 用 `Holder<TestEnvironmentDefinition>`，
 *   此处用字符串键，由 runner 在运行期经 `EnvironmentRegistry` 解析为实际环境对象，避免 base/ 反向依赖 framework/。
 * - `structure`：结构资源位置（如 `"gametest:empty_3x3"`），由 `TemplateManager::getTemplate` 加载。
 * - `maxTicks`：测试体最长运行 tick 数（超时则 `ExecutionTimeout` 失败）。
 * - `setupTicks`：结构放置后的预热 tick 数（对齐 Java `startExecution(-(setupTicks+1))` 负值阶段）。
 * - `required`：是否必需（optional 测试失败不计入退出码）。
 * - `rotation`：结构旋转（对齐 `mc::Rotation`）。
 * - `manualOnly`：仅手动触发（不在 `runall` 中自动跑）。
 * - `maxAttempts`：最大尝试次数（flaky 测试重试）。
 * - `requiredSuccesses`：需达成的成功次数（flaky 测试判定）。
 * - `skyAccess`：结构上方是否留空给光照（Java 独有，影响屏障是否封顶）。
 * - `padding`：结构周边清理格数（基岩独有，`MinecraftStructurePlacer` 据此清理外围）。
 * - `batchName`：所属批次名（基岩独有，用于 `before`/`after` 批次回调分组）。
 * - `loadSpawnChunks`：是否强制加载结构周围 `SPAWN_DISTANCE_CHUNK(8)` 区块范围（项目独有）。
 *   GameTestServer 是无头门面，`SimulatedPlayer::spawn` 不走真实玩家的区块加载链路
 *   （`_loadPlayerChunks` 对 PlayerId=0 是 no-op），结构外区块不被加载。
 *   `NaturalSpawner._collectSpawnableChunks` 遍历玩家周围 8 区块内 `hasChunk` 的区块计数
 *   `spawnableChunkCount`，仅结构 footprint（3 区块）时 `cap=maxInstances*3/289=0`，
 *   `activeCategories.empty()` 早退，怪物/动物都不自然生成。开启此标志让
 *   `MinecraftStructurePlacer` 在放置结构时额外 force 玩家周围 8 区块（满载 289），
 *   使 `spawnableChunkCount` 对齐原版 `DistanceManager.getNaturalSpawnChunkCount`。
 *   副作用：force 的区块会 worldgen 出真实地形，`NaturalSpawner` 会在结构外黑暗洞穴
 *   自然生成怪物残留世界——故仅 NaturalSpawner 类测试启用，且须放独立 batch（避免与
 *   全维度 getEntities 查询测试并行污染）并区域限定计数。
 *
 * 支持 nlohmann::json 序列化：经 ADL 自由函数 `mc::test::to_json`/`from_json`（.cpp 实现），
 * nlohmann 经参数依赖查找自动发现。字段名对齐 Java codec：
 * environment/structure/max_ticks/setup_ticks/required/rotation/manual_only/max_attempts/
 * required_successes/sky_access/padding/batch_name/load_spawn_chunks。
 */
class TestData {
public:
    TestData() = default;

    [[nodiscard]] const std::string& environment() const noexcept { return m_environment; }
    [[nodiscard]] const std::string& structure() const noexcept { return m_structure; }
    [[nodiscard]] i32 maxTicks() const noexcept { return m_maxTicks; }
    [[nodiscard]] i32 setupTicks() const noexcept { return m_setupTicks; }
    [[nodiscard]] bool required() const noexcept { return m_required; }
    [[nodiscard]] Rotation rotation() const noexcept { return m_rotation; }
    [[nodiscard]] bool manualOnly() const noexcept { return m_manualOnly; }
    [[nodiscard]] i32 maxAttempts() const noexcept { return m_maxAttempts; }
    [[nodiscard]] i32 requiredSuccesses() const noexcept { return m_requiredSuccesses; }
    [[nodiscard]] bool skyAccess() const noexcept { return m_skyAccess; }
    [[nodiscard]] i32 padding() const noexcept { return m_padding; }
    [[nodiscard]] const std::string& batchName() const noexcept { return m_batchName; }
    [[nodiscard]] bool loadSpawnChunks() const noexcept { return m_loadSpawnChunks; }

    TestData& setEnvironment(std::string environment)
    {
        m_environment = std::move(environment);
        return *this;
    }
    TestData& setStructure(std::string structure)
    {
        m_structure = std::move(structure);
        return *this;
    }
    TestData& setMaxTicks(i32 maxTicks) noexcept
    {
        m_maxTicks = maxTicks;
        return *this;
    }
    TestData& setSetupTicks(i32 setupTicks) noexcept
    {
        m_setupTicks = setupTicks;
        return *this;
    }
    TestData& setRequired(bool required) noexcept
    {
        m_required = required;
        return *this;
    }
    TestData& setRotation(Rotation rotation) noexcept
    {
        m_rotation = rotation;
        return *this;
    }
    TestData& setManualOnly(bool manualOnly) noexcept
    {
        m_manualOnly = manualOnly;
        return *this;
    }
    TestData& setMaxAttempts(i32 maxAttempts) noexcept
    {
        m_maxAttempts = maxAttempts;
        return *this;
    }
    TestData& setRequiredSuccesses(i32 requiredSuccesses) noexcept
    {
        m_requiredSuccesses = requiredSuccesses;
        return *this;
    }
    TestData& setSkyAccess(bool skyAccess) noexcept
    {
        m_skyAccess = skyAccess;
        return *this;
    }
    TestData& setPadding(i32 padding) noexcept
    {
        m_padding = padding;
        return *this;
    }
    TestData& setBatchName(std::string batchName)
    {
        m_batchName = std::move(batchName);
        return *this;
    }
    TestData& setLoadSpawnChunks(bool loadSpawnChunks) noexcept
    {
        m_loadSpawnChunks = loadSpawnChunks;
        return *this;
    }

    /**
     * @brief 是否为 flaky 测试（需多次重试以达成 requiredSuccesses）。
     *
     * 对齐 Java `GameTestInfo.isFlaky()`。
     */
    [[nodiscard]] bool isFlaky() const noexcept { return m_maxAttempts > 1; }

private:
    std::string m_environment = "default";
    std::string m_structure;
    i32 m_maxTicks = 100;
    i32 m_setupTicks = 0;
    bool m_required = true;
    Rotation m_rotation = Rotation::None;
    bool m_manualOnly = false;
    i32 m_maxAttempts = 1;
    i32 m_requiredSuccesses = 1;
    bool m_skyAccess = false;
    i32 m_padding = 0;
    std::string m_batchName = "default";
    bool m_loadSpawnChunks = false;

    // ADL 序列化自由函数访问私有字段（.cpp 实现 to_json/from_json）。
    friend void to_json(nlohmann::json& j, const TestData& d);
    friend void from_json(const nlohmann::json& j, TestData& d);
};

} // namespace mc::test
