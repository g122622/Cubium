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

// ============================================================================
// 结构（structure）parity 测试：Cubium 生成的结构起点 vs 原版 1.21.11 真实存档
//
// 素材：tests/unit/testdata/worlds/java-anvil-1.21.11-village/
//   种子   level.dat 的 WorldGenSettings.seed = -3361685360695458093
//   维度   minecraft:overworld（noise 生成器，settings=minecraft:overworld）
//   特征   出生点周围是村庄：4 个 minecraft:village_plains 结构起点
//          （区块 (-9,-9) / (-9,0) / (0,-9) / (0,0)，构件数 89 / 78 / 179 / 212）
//
// 与 JavaAnvilWorldGenParityTest 的分工：那个套件比对的是**逐方块/逐群系**的最终产物，
// 用来衡量整条生成管线的总差距；本套件比对的是**结构起点本身**——哪些区块生成了哪个
// 结构、装配出了哪些构件、每个构件的模板与包围盒。结构是"先选点、再装配、后再写方块"
// 的三段式流程，逐方块对比只能看到最后一段的后果，本套件能直接把偏差定位到
// 「选点错 / 装配错 / 放置错」中的哪一段。
//
// ---------------------------------------------------------------------------
// ground truth 的来源：直接读原版区块 NBT 的 structures.starts
// ---------------------------------------------------------------------------
// 原版把每个区块的结构起点写进区块 NBT 的 `structures.starts` 复合体，每个结构记录
// ChunkX/ChunkZ 与 Children（每个构件一条）：pool_element.location（模板路径）、
// rotation、PosX/PosY/PosZ（模板原点）、ground_level_delta、BB（包围盒）。
//
// 项目的读取链（JavaColumnReader → ChunkData）**不解析这一节**——ChunkData 上没有任何
// 结构字段，POI 也没有读取器。因此本套件用 RegionFile::readChunkData 直接取原始 NBT，
// 再用 mc::nbt 解析（这一步复用 JavaRealWorld::parseRoot 剥根前缀）。这样做的额外好处
// 是：ground truth 与项目自己"怎么读存档"完全解耦，读取链的缺陷不会污染本套件的结论
// （JavaAnvilWorldGenParityTest 曾因读取链缺陷得到过完全失真的 parity 数字）。
//
// 注意 `structures.starts` 里的结构起点是**落盘时刻**的状态，与 InhabitedTime 无关：
// 区块被 tick 不会新增/删除结构起点（结构起点在 STRUCTURE_STARTS 阶段一次性写入），
// 故本套件不受"区块是否被玩家改动过"的影响，无需像逐方块对比那样筛选纯净区块。
//
// ---------------------------------------------------------------------------
// 本套件测量的是"结构起点"这一独立阶段，因此**顺序无关**
// ---------------------------------------------------------------------------
// STRUCTURE_STARTS 只依赖本区块自身（ChunkStatus 依赖半径 0），不读写邻区块，故
// 逐区块生成结构起点的结果与区块处理顺序无关——这正是 JavaAnvilWorldGenParityTest
// 文件头记录的"顺序敏感性"问题的反面。本套件因此可以放心地按任意顺序扫描区块。
//
// ---------------------------------------------------------------------------
// 用例分两类
// ---------------------------------------------------------------------------
// 【基线】断言素材与 ground truth 自身完整（种子、村庄起点集合、构件字段齐全）。
//   素材被替换、结构注册表未加载、原版 NBT 结构变了，都会立刻暴露。
// 【门禁】断言 Cubium 的结构起点与原版逐项相等，当前**失败**——它是 parity 收敛的
//   进度表，也是防止退化的门禁：任何让某一环劣化的改动都会立刻 FAIL。
// ============================================================================

#include "common/TestDataDir.hpp"
#include "common/WorldGenRegistryFixture.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/JavaBiomeRegistryIdMap.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/storage/JavaRealWorldFixture.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/feature/template/TemplateManagerHostBinding.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/storage/backend/JavaAnvilBackend.hpp"
#include "server/world/storage/core/SaveFormat.hpp"
#include "server/world/storage/reader/java/RegionFile.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fmt/format.h>

#include <gtest/gtest.h>

using namespace mc;

namespace {

/// 素材目录（相对 tests/unit/testdata）
constexpr const char* kWorldRelPath = "worlds/java-anvil-1.21.11-village";

/// level.dat 的 WorldGenSettings.seed（Java 存档中为 Long，可能为负）
constexpr i64 kSeed = -3361685360695458093LL;

/**
 * 扫描邻域的半径（区块）：
 *
 * 结构集 villages 的放置间距为 34 区块，村庄落在候选区块上后还会向四周展开到
 * max_distance_from_center=80 格。取 12 足以覆盖"村庄本应出现在邻域却没有出现"
 * 与"不该出现却出现了"两类偏差——这两类都是选点环节（isStructureChunk / 结构集内
 * 加权选择 / 生物群系过滤）的错误，与装配环节无关。
 */
constexpr i32 kScanMargin = 12;

/**
 * @brief 原版存档中的一个构件（structure piece）
 *
 * 字段与原版 NBT `structures.starts.<结构>.Children[i]` 一一对应。
 * 两侧都归一到本结构后逐条比较。
 */
/**
 * @brief 原版存档中的一个 JigsawJunction
 *
 * 构件侧的 `junctions` 记录两类连接：**父构件接出本构件**时写入的那一条
 * （source 为父构件连接点的位置），以及**本构件接出各子构件**时逐条写入的
 * （source 为父侧的连接面）。因此把整个结构起点的全部 junction 汇总后逐条比对，
 * 等价于比对"结构里到底接出了哪些父子边"——比构件列表更细，
 * 能直接暴露"某条边多接/少接"，而不必先猜是哪一步装配决策出的问题。
 */
struct JunctionView {
    i32 sourceX = 0;
    i32 sourceGroundY = 0;
    i32 sourceZ = 0;
    i32 deltaY = 0;
    std::string destProjection;

    /// 排序键：与 JigsawJunction 的相等语义不同，这里把 sourceGroundY 也计入
    [[nodiscard]] auto sortKey() const
    {
        return std::make_tuple(sourceX, sourceGroundY, sourceZ, deltaY, destProjection);
    }

    [[nodiscard]] std::string text() const
    {
        return fmt::format(
            "({},{}) groundY={} deltaY={} dest={}", sourceX, sourceZ, sourceGroundY, deltaY, destProjection);
    }
};

struct PieceView {
    i32 minX = 0;
    i32 minY = 0;
    i32 minZ = 0;
    i32 maxX = 0;
    i32 maxY = 0;
    i32 maxZ = 0;
    i32 posX = 0;
    i32 posY = 0;
    i32 posZ = 0;
    i32 groundLevelDelta = 0;
    std::string rotation;
    std::string templateLocation;
    std::vector<JunctionView> junctions;

    /// 排序键：不含 origin（Cubium 侧不保存模板原点，只保存包围盒）
    [[nodiscard]] auto sortKey() const
    {
        return std::make_tuple(minX, minY, minZ, maxX, maxY, maxZ, rotation, templateLocation);
    }

    [[nodiscard]] std::string boxText() const
    {
        return fmt::format("[{},{},{},{},{},{}]", minX, minY, minZ, maxX, maxY, maxZ);
    }

    [[nodiscard]] std::string text() const
    {
        return fmt::format("bb={} rot={} gd={} origin=({},{},{}) template={}",
            boxText(),
            rotation,
            groundLevelDelta,
            posX,
            posY,
            posZ,
            templateLocation);
    }
};

/// 原版存档中的一个结构起点
struct StartView {
    ChunkCoord cx = 0;
    ChunkCoord cz = 0;
    std::string structureId;
    std::vector<PieceView> pieces;
};

/// 把 (cx, cz) 打包成单个 i64 作为容器键
constexpr i64 packChunkKey(ChunkCoord cx, ChunkCoord cz)
{
    return (static_cast<i64>(cx) << 32) | static_cast<u32>(cz);
}

/**
 * @brief 读取 NBT 中的整型数组（BB 一律是 IntArray；容错接受 IntList）
 */
bool readIntArray(const nbt::tags::tag& value, std::vector<i32>& out)
{
    if (value.id() == nbt::TagId::IntArray) {
        out = dynamic_cast<const nbt::tags::intarray_tag&>(value).value;
        return true;
    }
    if (value.id() == nbt::TagId::List) {
        const auto& list = dynamic_cast<const nbt::tags::list_tag&>(value);
        if (list.element_id() == nbt::TagId::Int) {
            out = dynamic_cast<const nbt::tags::int_list_tag&>(list).value;
            return true;
        }
    }
    return false;
}

/// 取子复合标签；不存在返回 nullptr
const nbt::tags::compound_tag* findCompound(const nbt::tags::compound_tag& parent, const std::string& name)
{
    const auto iter = parent.value.find(name);
    if (iter == parent.value.end() || iter->second == nullptr) {
        return nullptr;
    }
    return dynamic_cast<const nbt::tags::compound_tag*>(iter->second.get());
}

/// 取字符串字段；不存在返回空串
std::string findString(const nbt::tags::compound_tag& parent, const std::string& name)
{
    const auto iter = parent.value.find(name);
    if (iter == parent.value.end() || iter->second == nullptr || iter->second->id() != nbt::TagId::String) {
        return {};
    }
    return dynamic_cast<const nbt::tags::string_tag&>(*iter->second).value;
}

/// 取整型字段；不存在返回 fallback
i32 findInt(const nbt::tags::compound_tag& parent, const std::string& name, i32 fallback = 0)
{
    const auto iter = parent.value.find(name);
    if (iter == parent.value.end() || iter->second == nullptr || iter->second->id() != nbt::TagId::Int) {
        return fallback;
    }
    return dynamic_cast<const nbt::tags::int_tag&>(*iter->second).value;
}

/**
 * @brief 从原版区块根 NBT 中抽取该区块的全部结构起点
 *
 * 结构字段缺失（未生成任何结构）不是错误——绝大多数区块都没有结构起点。
 */
void collectStartsFromRoot(
    const nbt::tags::compound_tag& root, ChunkCoord cx, ChunkCoord cz, std::vector<StartView>& out)
{
    const nbt::tags::compound_tag* structures = findCompound(root, "structures");
    if (structures == nullptr) {
        return;
    }
    const nbt::tags::compound_tag* starts = findCompound(*structures, "starts");
    if (starts == nullptr) {
        return;
    }

    for (const auto& [structureId, startTag] : starts->value) {
        const auto* start = dynamic_cast<const nbt::tags::compound_tag*>(startTag.get());
        if (start == nullptr) {
            continue;
        }
        StartView view;
        view.cx = cx;
        view.cz = cz;
        view.structureId = structureId;

        const auto childrenIter = start->value.find("Children");
        if (childrenIter != start->value.end() && childrenIter->second != nullptr &&
            childrenIter->second->id() == nbt::TagId::List) {
            const auto& children = dynamic_cast<const nbt::tags::compound_list_tag&>(*childrenIter->second);
            view.pieces.reserve(children.value.size());
            for (const auto& child : children.value) {
                PieceView piece;
                std::vector<i32> box;
                const auto boxIter = child.value.find("BB");
                if (boxIter != child.value.end() && boxIter->second != nullptr && readIntArray(*boxIter->second, box) &&
                    box.size() >= 6) {
                    piece.minX = box[0];
                    piece.minY = box[1];
                    piece.minZ = box[2];
                    piece.maxX = box[3];
                    piece.maxY = box[4];
                    piece.maxZ = box[5];
                }
                piece.posX = findInt(child, "PosX");
                piece.posY = findInt(child, "PosY");
                piece.posZ = findInt(child, "PosZ");
                piece.groundLevelDelta = findInt(child, "ground_level_delta");
                piece.rotation = findString(child, "rotation");
                if (const auto* element = findCompound(child, "pool_element"); element != nullptr) {
                    // 单件/legacy 单件元素记模板路径；地物元素记地物 id；其余退化为元素类型名
                    // （Cubium 侧对应 templateLocation()：模板路径 / 地物 id / 空）
                    piece.templateLocation = findString(*element, "location");
                    if (piece.templateLocation.empty()) {
                        piece.templateLocation = findString(*element, "feature");
                    }
                    if (piece.templateLocation.empty()) {
                        piece.templateLocation = findString(*element, "element_type");
                    }
                }
                // junctions：父构件接出本构件的那一条 + 本构件接出各子构件的若干条
                const auto junctionIter = child.value.find("junctions");
                if (junctionIter != child.value.end() && junctionIter->second != nullptr &&
                    junctionIter->second->id() == nbt::TagId::List) {
                    const auto& junctions = dynamic_cast<const nbt::tags::compound_list_tag&>(*junctionIter->second);
                    piece.junctions.reserve(junctions.value.size());
                    for (const auto& junction : junctions.value) {
                        JunctionView jv;
                        jv.sourceX = findInt(junction, "source_x");
                        jv.sourceGroundY = findInt(junction, "source_ground_y");
                        jv.sourceZ = findInt(junction, "source_z");
                        jv.deltaY = findInt(junction, "delta_y");
                        jv.destProjection = findString(junction, "dest_proj");
                        piece.junctions.push_back(std::move(jv));
                    }
                }
                view.pieces.push_back(std::move(piece));
            }
        }
        out.push_back(std::move(view));
    }
}

/**
 * @brief 扫描素材 region/ 下所有区块，抽取全部结构起点
 */
std::vector<StartView> readJavaStarts(const std::filesystem::path& worldDir)
{
    std::vector<StartView> starts;
    const std::filesystem::path regionDir = worldDir / "region";
    if (!std::filesystem::exists(regionDir)) {
        return starts;
    }

    for (const auto& entry : std::filesystem::directory_iterator(regionDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        i32 regionX = 0;
        i32 regionZ = 0;
        if (std::sscanf(name.c_str(), "r.%d.%d.mca", &regionX, &regionZ) != 2) {
            continue;
        }

        world::storage::reader::java::RegionFile region(entry.path());
        if (!region.open().success()) {
            continue;
        }
        for (const auto& [localX, localZ] : region.listChunks()) {
            auto raw = region.readChunkData(localX, localZ);
            if (raw.failed()) {
                continue;
            }
            auto root = test::JavaRealWorld::parseRoot(raw.value());
            if (!root) {
                continue;
            }
            collectStartsFromRoot(*root, regionX * 32 + localX, regionZ * 32 + localZ, starts);
        }
    }
    return starts;
}

/// 旋转枚举 → 原版 NBT 的旋转名（结构装配全程不镜像，只比较旋转）
std::string_view rotationName(Rotation rotation)
{
    switch (rotation) {
        case Rotation::None:
            return "NONE";
        case Rotation::Clockwise90:
            return "CLOCKWISE_90";
        case Rotation::Clockwise180:
            return "CLOCKWISE_180";
        case Rotation::CounterClockwise90:
            return "COUNTERCLOCKWISE_90";
    }
    return "NONE";
}

/**
 * @brief 模板来源：数据包仓库
 *
 * 【生命周期是本测试的关键】TemplateManager 只持**非拥有**指针（见其头文件注释），
 * 仓库必须活到最后一个模板被使用；`loadVanillaWorldGenRegistries()` 内部自己新建的仓库
 * 在函数返回时即析构，不能充当模板来源。
 */
resource::DataPackRepository& templateDataPackRepository()
{
    static const std::unique_ptr<resource::DataPackRepository> repository = [] {
        auto repo = std::make_unique<resource::DataPackRepository>();
        const auto scanResult = repo->scanDirectory(GameDirectory::defaultDirectory().dataPacksDir());
        MC_ASSERT_RELEASE_MSG(scanResult.success() && scanResult.value() > 0, "data packs unavailable");
        return repo;
    }();
    return *repository;
}

/**
 * @brief 模板管理器绑定令牌（进程内保持存活）
 *
 * 【为何必须绑定】村庄是 jigsaw 结构，构件来自 `.nbt` 模板。模板来源未绑定时，
 * JigsawPiece 的模板加载静默失败（joints 为空、size 为 0），装配退化为"起始点上一个
 * 退化构件 + 一圈石头砖兜底方块"，且**不产生任何报错**——测出来的"结构差异"会变成
 * 装置缺陷而非被测系统的缺陷。
 *
 * 【为何是进程级而非用例级】模板池（TemplatePoolRegistry）在加载期就把模板读进
 * SingleJigsawPiece，晚绑定无法补救（clone 只复制 joints）。因此绑定必须早于
 * loadVanillaWorldGenRegistries()，且必须活到所有用例结束。
 */
world::gen::feature::template_::TemplateManagerHostBinding& templateBinding()
{
    static const std::unique_ptr<world::gen::feature::template_::TemplateManagerHostBinding> binding = [] {
        auto token = std::make_unique<world::gen::feature::template_::TemplateManagerHostBinding>(
            world::gen::jigsaw::JigsawAssembler::getTemplateManager());
        token->bindDataPackRepository(templateDataPackRepository());
        return token;
    }();
    return *binding;
}

/**
 * @brief 结构起点 parity 测试夹具
 */
class JavaAnvilVillageStructureParityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
        // 原版区块的群系按名解析依赖这张表（JavaBiomeMapper 委托它）
        ASSERT_TRUE(world::biome::JavaBiomeRegistryIdMap::instance().initialize().success())
            << "JavaBiomeRegistryIdMap 初始化失败";

        // 【顺序不可交换】模板来源必须先于世界生成注册表加载：数据包里的 template_pool JSON
        // 在加载期就构造 SingleJigsawPiece 并立刻读取 `.nbt` 模板，晚绑定会让所有
        // jigsaw 构件的 joints 与 size 恒为空/0，装配静默退化。
        (void)templateBinding();

        ASSERT_TRUE(mc::test::loadVanillaWorldGenRegistries())
            << "数据包缺失，无法加载世界生成注册表：" << GameDirectory::defaultDirectory().dataPacksDir().string();
    }

    void SetUp() override
    {
        const std::filesystem::path worldDir = mc::test::testDataPath(kWorldRelPath);
        ASSERT_TRUE(std::filesystem::exists(worldDir / "level.dat")) << "测试素材缺失：" << worldDir.string();
        ASSERT_TRUE(std::filesystem::exists(worldDir / "region")) << "测试素材损坏：缺少 region/ 目录";

        world::storage::SaveFormatInfo info;
        info.format = world::storage::SaveFormat::JavaAnvil;
        info.formatName = "Java 1.21.11";
        info.dataVersion = 4671;
        info.readonly = true;

        auto openResult = m_backend.open(worldDir, info);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        // 种子从素材自身读出（而不是硬编码），保证"生成器配置"与"ground truth"同源；
        // 素材被替换时 GetGroundTruthIsIntact 会先失败，避免拿错种子的结论误导排查。
        auto levelData = m_backend.loadLevelData();
        ASSERT_TRUE(levelData.success()) << levelData.error().message();
        m_seed = static_cast<i64>(levelData.value().summary.seed);

        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, static_cast<u64>(m_seed));
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        m_generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
    }

    /**
     * @brief 生成单个区块的结构起点并返回该区块的 ChunkPrimer
     *
     * 只跑 STRUCTURE_STARTS 一个阶段：该阶段只写本区块（ChunkStatus 依赖半径为 0、
     * 实现里也不读 region），因此一个只含本区块的 region 即已足够，无需把邻域推进到
     * 任何状态——这让"结构起点 parity"可以逐区块独立测量、与处理顺序无关。
     *
     * @return 该区块的 primer；其生命周期由 m_primers 持有
     */
    [[nodiscard]] world::chunk::ChunkPrimer* generateStarts(ChunkCoord cx, ChunkCoord cz)
    {
        auto primer = std::make_unique<world::chunk::ChunkPrimer>(cx, cz);
        world::chunk::ChunkPrimer* raw = primer.get();

        std::vector<IChunk*> chunkPtrs;
        chunkPtrs.push_back(raw);
        WorldGenRegion region(cx, cz, 0, std::move(chunkPtrs));
        m_generator->generateStructureStarts(region, *raw);

        m_primers.push_back(std::move(primer));
        return raw;
    }

    /// primer 转成与 Java 侧同构的视图（仅取两侧都有的字段）
    static std::vector<PieceView> toPieceViews(const world::gen::structure::StructureStart& start)
    {
        std::vector<PieceView> views;
        views.reserve(start.pieceCount());
        for (const auto& piece : start.pieces()) {
            PieceView view;
            const auto box = piece->getBoundingBox();
            view.minX = box.minX();
            view.minY = box.minY();
            view.minZ = box.minZ();
            view.maxX = box.maxX();
            view.maxY = box.maxY();
            view.maxZ = box.maxZ();
            view.groundLevelDelta = piece->getGroundLevelDelta();
            view.rotation = std::string(rotationName(piece->getRotation()));
            view.templateLocation = std::string(piece->templateLocation());
            // junction：与 Java 侧同构——父构件接出本构件的那一条，加上本构件接出各子构件的若干条
            for (const auto& junction : piece->getJunctions()) {
                JunctionView jv;
                jv.sourceX = junction.getSourceX();
                jv.sourceGroundY = junction.getSourceGroundY();
                jv.sourceZ = junction.getSourceZ();
                jv.deltaY = junction.getDeltaY();
                jv.destProjection =
                    (junction.getDestProjection() == world::gen::jigsaw::JigsawPlacementBehaviour::Rigid)
                    ? "rigid"
                    : "terrain_matching";
                view.junctions.push_back(std::move(jv));
            }
            views.push_back(std::move(view));
        }
        // **不排序**：两侧的构件列表都是"放置顺序"（原版 StructureStart.Children 的写入顺序，
        // 也是本实现追加到 StructureStart 的顺序），逐位对比能直接指出**第一个分歧的构件**；
        // 排序后对比则只能看出"集合不同"，无法定位分歧从哪一步开始。
        return views;
    }

    /**
     * @brief 逐位对照两侧构件列表（均为放置顺序），并打印前若干个
     *
     * 打印策略：先报"第一个分歧的序号"，再围绕该序号打印上下文；两侧数量不等时把多出来的
     * 尾部也打印出来。这样一次运行就能看出分歧是"起始构件选错"（序号 0）、"某一步随机数
     * 错位"（中间某序号）还是"展开提前终止"（尾部少了若干）。
     */
    static void printPieceDiff(
        const std::string& title, const std::vector<PieceView>& javaViews, const std::vector<PieceView>& cubiumViews)
    {
        const size_t common = std::min(javaViews.size(), cubiumViews.size());
        size_t firstDiff = common;
        for (size_t i = 0; i < common; ++i) {
            if (javaViews[i].sortKey() != cubiumViews[i].sortKey()) {
                firstDiff = i;
                break;
            }
        }

        std::printf("[STRUCT-PARITY] %s 原版 %zu 个构件 / Cubium %zu 个构件，逐位相同 %zu 个，首个分歧序号 %s\n",
            title.c_str(),
            javaViews.size(),
            cubiumViews.size(),
            firstDiff,
            firstDiff == common ? "无（前缀完全一致）" : std::to_string(firstDiff).c_str());

        // 默认只打印分歧点附近若干行；设 MC_STRUCT_PARITY_FULL=1 时打印两侧完整列表，
        // 供离线逐位比对（排查过程中反复使用，避免每次排查都要改代码重编译）。
        static const size_t kContext = std::getenv("MC_STRUCT_PARITY_FULL") != nullptr ? 4096 : 6;
        // 分歧构件上把两侧的 junction 一并打出：junction 是"哪条父子边"的唯一记录，
        // 直接指出该构件多接/少接了哪些子构件，不必再去猜是哪一步装配决策出的问题。
        if (firstDiff < common) {
            const auto printJunctions = [firstDiff](const char* side, const PieceView& view) {
                std::printf(
                    "[STRUCT-JUNC]   第 %zu 号构件（%s）junction %zu 条：", firstDiff, side, view.junctions.size());
                for (const auto& junction : view.junctions) {
                    std::printf(" %s", junction.text().c_str());
                }
                std::printf("\n");
            };
            printJunctions("原版", javaViews[firstDiff]);
            printJunctions("Cubium", cubiumViews[firstDiff]);
        }

        const size_t begin = firstDiff > kContext ? firstDiff - kContext : 0;
        const size_t end = std::min(common, firstDiff + kContext);
        for (size_t i = begin; i < end; ++i) {
            const bool same = javaViews[i].sortKey() == cubiumViews[i].sortKey();
            std::printf("[STRUCT-PARITY]   [%3zu]%s 原版   %s\n", i, same ? " " : "!", javaViews[i].text().c_str());
            if (!same) {
                std::printf("[STRUCT-PARITY]   [%3zu]  Cubium %s\n", i, cubiumViews[i].text().c_str());
            }
        }
        const size_t longer = std::max(javaViews.size(), cubiumViews.size());
        const size_t tailBegin = std::max(end, common);
        for (size_t i = tailBegin; i < longer && i < tailBegin + kContext; ++i) {
            if (i < javaViews.size()) {
                std::printf("[STRUCT-PARITY]   [%3zu] - 原版   %s\n", i, javaViews[i].text().c_str());
            } else {
                std::printf("[STRUCT-PARITY]   [%3zu] + Cubium %s\n", i, cubiumViews[i].text().c_str());
            }
        }
    }

    /**
     * @brief 汇总整个结构起点的 junction 做多重集比对
     *
     * 每条 junction 对应一条"父构件接出子构件"的边。两侧多重集相同 ⟺ 两侧接出的父子边
     * 完全相同；差异项直接指出"多接/少接了哪条边"（位置 + 地面高度 + 高度偏移 + 目标投影），
     * 是比构件列表更细的对照信号——构件列表只能看出"集合不同"，而这里能直接定位到连接面。
     */
    static void printJunctionDiff(
        const std::string& title, const std::vector<PieceView>& javaViews, const std::vector<PieceView>& cubiumViews)
    {
        const auto collect = [](const std::vector<PieceView>& views) {
            std::map<std::string, i32> counts;
            for (const auto& view : views) {
                for (const auto& junction : view.junctions) {
                    counts[junction.text()] += 1;
                }
            }
            return counts;
        };
        const auto javaCounts = collect(javaViews);
        const auto cubiumCounts = collect(cubiumViews);
        i32 javaTotal = 0;
        i32 cubiumTotal = 0;
        for (const auto& [key, count] : javaCounts) {
            javaTotal += count;
        }
        for (const auto& [key, count] : cubiumCounts) {
            cubiumTotal += count;
        }
        std::printf("[STRUCT-JUNC] %s 原版 junction %d 条 / Cubium %d 条\n", title.c_str(), javaTotal, cubiumTotal);

        constexpr i32 kMaxShown = 12;
        i32 shown = 0;
        for (const auto& [key, count] : javaCounts) {
            const auto iter = cubiumCounts.find(key);
            const i32 cubiumCount = (iter == cubiumCounts.end()) ? 0 : iter->second;
            if (count != cubiumCount && shown < kMaxShown) {
                std::printf("[STRUCT-JUNC]   仅原版多出 %d 条：%s\n", count - cubiumCount, key.c_str());
                ++shown;
            }
        }
        for (const auto& [key, count] : cubiumCounts) {
            const auto iter = javaCounts.find(key);
            const i32 javaCount = (iter == javaCounts.end()) ? 0 : iter->second;
            if (count != javaCount && shown < 2 * kMaxShown) {
                std::printf("[STRUCT-JUNC]   Cubium 多出 %d 条：%s\n", count - javaCount, key.c_str());
                ++shown;
            }
        }
    }

    /// 素材中的全部结构起点（只解析一次）
    [[nodiscard]] const std::vector<StartView>& javaStarts()
    {
        static const std::vector<StartView> starts = readJavaStarts(mc::test::testDataPath(kWorldRelPath));
        return starts;
    }

    /// 生成器使用的种子（从素材 level.dat 读出）
    i64 m_seed = kSeed;

    world::storage::JavaAnvilBackend m_backend;
    std::unique_ptr<NoiseChunkGenerator> m_generator;
    /// generateStarts 产生的 primer（保持存活，供调用方读结构起点）
    std::vector<std::unique_ptr<world::chunk::ChunkPrimer>> m_primers;
};

// ============================================================================
// 【基线】用例：当前应全部通过
// ============================================================================

/**
 * 素材与 ground truth 自身完整：种子、村庄起点集合、构件字段都必须可读且非空。
 *
 * 这是其余所有对比的前提。任一项不成立都说明"素材换了 / 解析器坏了 / 原版 NBT 结构变了"，
 * 此时其余用例的数字没有意义，必须先在这里失败。
 */
TEST_F(JavaAnvilVillageStructureParityTest, GetGroundTruthIsIntact)
{
    EXPECT_EQ(m_seed, kSeed) << "素材的种子与用例记录的常量不符：素材已被替换，其余结论需重测";

    const std::vector<StartView>& starts = javaStarts();
    ASSERT_FALSE(starts.empty()) << "素材中没有任何结构起点：解析失败或素材不合格";

    // 村庄起点：素材的关键特征，缺失则本套件失去测量对象
    std::vector<StartView> villages;
    for (const auto& start : starts) {
        if (start.structureId.starts_with("minecraft:village")) {
            villages.push_back(start);
        }
    }
    ASSERT_FALSE(villages.empty()) << "素材中没有任何村庄结构起点";

    std::set<i64> villageChunks;
    for (const auto& village : villages) {
        villageChunks.insert(packChunkKey(village.cx, village.cz));
        EXPECT_FALSE(village.pieces.empty()) << "村庄 (" << village.cx << "," << village.cz << ") 没有任何构件";
        for (const auto& piece : village.pieces) {
            EXPECT_FALSE(piece.templateLocation.empty())
                << "村庄 (" << village.cx << "," << village.cz << ") 的构件缺少模板路径（pool_element 解析失败）";
            EXPECT_FALSE(piece.rotation.empty())
                << "村庄 (" << village.cx << "," << village.cz << ") 的构件缺少 rotation";
            // 单方块构件（村民/动物）本身就是 1x1x1，故三个方向都只断言"不反向"
            EXPECT_LE(piece.minX, piece.maxX) << "构件包围盒 X 反向，BB 解析可能出错";
            EXPECT_LE(piece.minY, piece.maxY) << "构件包围盒 Y 反向，BB 解析可能出错";
            EXPECT_LE(piece.minZ, piece.maxZ) << "构件包围盒 Z 反向，BB 解析可能出错";
        }
    }

    // 打印 ground truth 概览：目标集合是运行时从素材推出的，打印出来便于比对诊断
    std::printf("[STRUCT-PARITY] 素材共 %zu 个结构起点，其中村庄 %zu 个，位于区块：", starts.size(), villages.size());
    for (const i64 key : villageChunks) {
        std::printf(" (%d,%d)", static_cast<i32>(key >> 32), static_cast<i32>(static_cast<u32>(key)));
    }
    std::printf("\n");
    for (const auto& village : villages) {
        std::printf("[STRUCT-PARITY]   村庄 (%d,%d) 结构=%s 构件数=%zu\n",
            village.cx,
            village.cz,
            village.structureId.c_str(),
            village.pieces.size());
    }
}

/**
 * 装置自检：jigsaw 装配链路的外部依赖（模板来源、模板池）确实就位且可用。
 *
 * 结构与方块生成不同，它依赖两个**外部装置**且失效时全程静默：
 *   1. 模板来源（TemplateManager）未绑定 → 模板加载失败 → 构件 joints/size 恒为空/0
 *      → 装配退化为"起始点一个退化构件 + 一圈石头砖兜底方块"，不报错；
 *   2. 模板池（TemplatePoolRegistry）来自数据包 JSON，未加载则整池为空。
 * 两者都会让 parity 测出来的是"装置缺陷"而不是"被测系统的缺陷"。本用例把这类
 * 环境问题和生成结果问题分开，避免在错误的结论上排查生成算法。
 */
TEST_F(JavaAnvilVillageStructureParityTest, JigsawHarnessIsWired)
{
    using world::gen::jigsaw::JigsawAssembler;
    using world::gen::jigsaw::TemplatePoolRegistry;

    const ResourceLocation fountain =
        ResourceLocation::parse("minecraft:village/plains/town_centers/plains_fountain_01");
    const auto* templ = JigsawAssembler::getTemplateManager().getTemplate(fountain);
    ASSERT_NE(templ, nullptr) << "村庄模板未加载：模板来源未绑定，或模板路径映射有误";
    std::printf("[STRUCT-PARITY] 装置自检：模板 %s size=(%d,%d,%d) jigsaw 方块 %zu 个\n",
        fountain.toString().c_str(),
        templ->getSize().x,
        templ->getSize().y,
        templ->getSize().z,
        templ->getJigsawBlocks().size());
    EXPECT_GT(templ->getJigsawBlocks().size(), 0u)
        << "模板里没有任何 jigsaw 方块：模板解析有误（连接点为空将使装配无法展开）";

    const auto* pool =
        TemplatePoolRegistry::instance().getPool(ResourceLocation::parse("minecraft:village/plains/town_centers"));
    ASSERT_NE(pool, nullptr) << "村庄起始模板池未加载：结构模板池数据包加载失败";
    EXPECT_FALSE(pool->isEmpty()) << "村庄起始模板池为空";

    math::Random rng(0ULL);
    const auto pieces = pool->getShuffledPieces(rng);
    std::printf("[STRUCT-PARITY] 装置自检：起始池 %s 展开 %zu 个候选（总权重 %zu）\n",
        pool->getName().toString().c_str(),
        pieces.size(),
        pool->getTotalWeight());
    ASSERT_FALSE(pieces.empty());
    for (size_t i = 0; i < std::min<size_t>(pieces.size(), 4); ++i) {
        std::printf("[STRUCT-PARITY]     候选 %s size=(%d,%d,%d) joints=%zu\n",
            pieces[i]->getName().c_str(),
            pieces[i]->getSize().x,
            pieces[i]->getSize().y,
            pieces[i]->getSize().z,
            pieces[i]->getJoints().size());
    }
    EXPECT_FALSE(pieces[0]->getName().empty()) << "候选拼图块没有名字，构件无法追溯到模板";
    EXPECT_GT(pieces[0]->getJoints().size(), 0u) << "候选拼图块没有连接点，装配无法展开";

    // use_expansion_hack 的净空估算读的是"池内元素在无旋转下的最大 Y 跨度"，它参与碰撞判定，
    // 因此其数值必须与数据包里的模板尺寸一致（含回退池）。数值取自 json 权重与 nbt 尺寸，
    // 是唯一能发现"某模板未加载导致尺寸退化为 0"这类静默故障的检查。
    struct PoolSpan {
        const char* pool;
        i32 expected;
    };
    constexpr PoolSpan kExpectedSpans[] = {
        {"minecraft:village/plains/streets", 2},
        {"minecraft:village/plains/houses", 12},
        {"minecraft:village/plains/decor", 4},
        {"minecraft:village/plains/villagers", 3},
        {"minecraft:village/common/cats", 3},
        {"minecraft:village/plains/terminators", 2},
    };
    // 【模板静默加载失败的检测】模板加载失败时，SingleJigsawPiece 仍会记录模板路径（名字非空），
    // 但尺寸退化为 (0,0,0)、连接点为空。后果是**静默**的：尺寸为 0 的构件包围盒退化为一个点，
    // 碰撞检测几乎恒过，于是它会在原版放不下的位置被放下；连接点为 0 又少消耗 (m-1) 次
    // "连接点洗牌"的随机数。两者都会让装配结果偏离原版而无任何日志。
    // 地物池元素（feature_pool_element）尺寸本就为 0，须排除。
    constexpr const char* kPoolsToAudit[] = {
        "minecraft:village/plains/streets",
        "minecraft:village/plains/houses",
        "minecraft:village/plains/decor",
        "minecraft:village/plains/villagers",
        "minecraft:village/plains/town_centers",
        "minecraft:village/common/cats",
        "minecraft:village/common/animals",
        "minecraft:village/plains/terminators",
    };
    for (const char* poolName : kPoolsToAudit) {
        const auto* auditPool = TemplatePoolRegistry::instance().getPool(ResourceLocation::parse(poolName));
        ASSERT_NE(auditPool, nullptr) << "模板池未加载：" << poolName;
        math::Random auditRng(0ULL);
        const auto audited = auditPool->getShuffledPieces(auditRng);
        i64 auditedReal = 0;
        for (const auto* piece : audited) {
            if (piece == nullptr || piece->isEmpty()) {
                continue;
            }
            if (piece->getTypeName() == "feature_pool_element") {
                continue; // 地物元素不加载模板，尺寸恒为 0
            }
            ++auditedReal;
            const BlockPos size = piece->getSize();
            EXPECT_GT(static_cast<i64>(size.x) * size.y * size.z, 0)
                << poolName << " 中的 " << piece->getName() << " 尺寸为 0：模板未加载成功";
            EXPECT_GT(piece->getJoints().size(), 0u)
                << poolName << " 中的 " << piece->getName() << " 没有连接点：模板未加载成功";
            // 【多调色板模板】原版取"按位置种子选出的调色板"的连接点，本实现固定读第一个调色板。
            // 若村庄模板存在多调色板，两者读到的连接点集合会不同，进而使连接点洗牌消耗的
            // 随机数次数不同——这是"装配结果不同但不报错"的一条隐蔽路径，故显式挡住。
            const world::gen::feature::template_::Template* pieceTemplate =
                world::gen::jigsaw::JigsawAssembler::getTemplateManager().getTemplate(
                    ResourceLocation::parse(piece->getName()));
            ASSERT_NE(pieceTemplate, nullptr) << piece->getName() << " 的模板无法按名字取回";
            EXPECT_EQ(pieceTemplate->getPaletteCount(), 1)
                << piece->getName() << " 是多调色板模板：本实现只读第一个调色板，连接点集合可能不同";
        }
        std::printf("[STRUCT-PARITY] 装置自检：池 %-40s 展开 %zu 项（其中模板件 %lld 项已审计）\n",
            poolName,
            audited.size(),
            static_cast<long long>(auditedReal));
        EXPECT_GT(auditedReal, 0) << poolName << " 没有可审计的模板件，自检形同虚设";
    }

    // 连接点的**顺序**必须与模板 NBT 中 jigsaw 方块的出现顺序一致：
    // 原版与本案都对连接点列表做 Fisher-Yates，输入顺序不同则同一随机数会产出不同排列——
    // 这会改变父块连接点的处理次序（进而改变"父块内部接合点"共享的可放置空间的演化），
    // 却**不改变随机数消耗次数**，因此是一种"前若干构件完全一致、之后突然发散"的隐蔽偏差。
    {
        const auto* orderPool =
            TemplatePoolRegistry::instance().getPool(ResourceLocation::parse("minecraft:village/plains/streets"));
        ASSERT_NE(orderPool, nullptr);
        math::Random orderRng(0ULL);
        for (const auto* piece : orderPool->getShuffledPieces(orderRng)) {
            if (piece == nullptr || piece->isEmpty() ||
                piece->getName() != "minecraft:village/plains/streets/straight_01") {
                continue;
            }
            std::string order;
            for (const auto& joint : piece->getJoints()) {
                order += fmt::format("({},{},{}) ", joint.sourcePos.x, joint.sourcePos.y, joint.sourcePos.z);
            }
            // 期望值取自模板 .nbt 中 jigsaw 方块的出现顺序
            EXPECT_EQ(order, "(2,0,13) (4,0,7) (11,0,6) (11,0,13) (12,0,4) (7,1,0) (7,1,15) ")
                << "straight_01 的连接点顺序与模板 NBT 不一致：连接点洗牌的输入顺序变了";
            break;
        }
    }

    for (const auto& expected : kExpectedSpans) {
        const auto* spanPool = TemplatePoolRegistry::instance().getPool(ResourceLocation::parse(expected.pool));
        ASSERT_NE(spanPool, nullptr) << "模板池未加载：" << expected.pool;
        const i32 actual = spanPool->getMaxYSpan();
        std::printf("[STRUCT-PARITY] 装置自检：池 %-40s 最大 Y 跨度 = %d（期望 %d）\n",
            expected.pool,
            actual,
            expected.expected);
        EXPECT_EQ(actual, expected.expected) << expected.pool << " 的最大 Y 跨度与数据包不符";
    }
}

/**
 * Cubium 生成的结构起点自洽：结构 id 已注册、构件数非零、包围盒非退化。
 *
 * 不依赖原版存档，纯校验生成产物的结构性正确性。构件数为 0 或包围盒退化为一个点，
 * 是"模板未加载"这类装置缺陷的典型症状，必须与"装配结果与原版不同"区分开。
 */
TEST_F(JavaAnvilVillageStructureParityTest, GeneratedVillageStartsAreStructurallySound)
{
    const std::vector<StartView>& starts = javaStarts();
    std::set<i64> villageChunks;
    for (const auto& start : starts) {
        if (start.structureId == "minecraft:village_plains") {
            villageChunks.insert(packChunkKey(start.cx, start.cz));
        }
    }
    ASSERT_FALSE(villageChunks.empty()) << "素材中没有 village_plains 起点，无法校验生成产物";

    for (const i64 key : villageChunks) {
        const ChunkCoord cx = static_cast<i32>(key >> 32);
        const ChunkCoord cz = static_cast<i32>(static_cast<u32>(key));

        world::chunk::ChunkPrimer* primer = generateStarts(cx, cz);
        ASSERT_NE(primer, nullptr);
        const auto* start = primer->getStructureStart(ResourceLocation::parse("minecraft:village_plains"));
        ASSERT_NE(start, nullptr) << "区块 (" << cx << "," << cz << ") 没有生成 village_plains 结构起点";
        EXPECT_TRUE(start->isValid()) << "区块 (" << cx << "," << cz << ") 的村庄结构起点为空";

        const auto box = start->getBoundingBox();
        EXPECT_LT(box.minX(), box.maxX()) << "村庄包围盒退化，模板很可能未加载";
        for (const auto& piece : start->pieces()) {
            EXPECT_FALSE(piece->templateLocation().empty())
                << "区块 (" << cx << "," << cz << ") 的构件没有模板路径，模板未加载";
        }
    }
}

// ============================================================================
// 【门禁】用例：与原版逐项相等，当前失败，parity 达成后自动转绿
// ============================================================================

/**
 * 结构起点的**分布**与原版一致：扫描邻域内，每个区块有哪些种类的结构起点。
 *
 * 这一层只关心"谁在哪个区块生成了"，不关心装配出了什么，因此它单独衡量选点环节：
 *   - 候选区块判定（RandomSpreadStructurePlacement.isStructureChunk：setLargeFeatureWithSalt
 *     低 24 位是否为 0、频率缩减、排斥区）
 *   - 结构集内多条目加权选择（含失败回退重抽）
 *   - 候选点的生物群系过滤
 *
 * 【当前状态】通过。该用例曾捕获过一个真实缺陷：村庄的生物群系校验在"区块中心、
 * y=0"处采样，而原版在**候选生成点**（起始块中心、投影后的地面线高度）采样——
 * y=0 的四分坐标落到洞穴/深板岩群系上，使 4 个村庄里有 3 个被判为"群系不符"
 * 而根本不生成。改用候选点采样后 4/4 全部生成，且扫描邻域内无多生成。
 *
 * 该用例同时是"选点环节"的回归门禁：它会把任何使村庄出现在错误区块（或消失）的改动
 * 立刻暴露出来，且不依赖装配环节（装配差异由 VillagePiecesMatchJavaSave 衡量）。
 */
TEST_F(JavaAnvilVillageStructureParityTest, StructureStartDistributionMatchesJavaSave)
{
    const std::vector<StartView>& starts = javaStarts();
    ASSERT_FALSE(starts.empty());

    // 扫描窗口：由素材里的村庄起点包围盒外扩 kScanMargin 推出（运行时确定，不硬编码）
    ChunkCoord minX = 0;
    ChunkCoord maxX = 0;
    ChunkCoord minZ = 0;
    ChunkCoord maxZ = 0;
    bool first = true;
    for (const auto& start : starts) {
        if (!start.structureId.starts_with("minecraft:village")) {
            continue;
        }
        if (first) {
            minX = maxX = start.cx;
            minZ = maxZ = start.cz;
            first = false;
            continue;
        }
        minX = std::min(minX, start.cx);
        maxX = std::max(maxX, start.cx);
        minZ = std::min(minZ, start.cz);
        maxZ = std::max(maxZ, start.cz);
    }
    ASSERT_FALSE(first) << "素材里没有村庄起点";
    minX -= kScanMargin;
    maxX += kScanMargin;
    minZ -= kScanMargin;
    maxZ += kScanMargin;

    // 原版侧：区块 → 村庄结构 id 集合
    std::map<i64, std::set<std::string>> javaByChunk;
    for (const auto& start : starts) {
        if (start.structureId.starts_with("minecraft:village")) {
            javaByChunk[packChunkKey(start.cx, start.cz)].insert(start.structureId);
        }
    }

    i64 matchedChunks = 0;
    i64 missingChunks = 0;
    i64 spuriousChunks = 0;
    i64 wrongIdChunks = 0;
    for (ChunkCoord cx = minX; cx <= maxX; ++cx) {
        for (ChunkCoord cz = minZ; cz <= maxZ; ++cz) {
            const i64 key = packChunkKey(cx, cz);
            world::chunk::ChunkPrimer* primer = generateStarts(cx, cz);
            ASSERT_NE(primer, nullptr);

            std::set<std::string> cubiumIds;
            for (const auto& [structureId, start] : primer->structureStarts()) {
                if (start && start->isValid() && structureId.toString().starts_with("minecraft:village")) {
                    cubiumIds.insert(structureId.toString());
                }
            }

            const auto javaIter = javaByChunk.find(key);
            const std::set<std::string> javaIds =
                javaIter != javaByChunk.end() ? javaIter->second : std::set<std::string>();

            if (javaIds == cubiumIds) {
                ++matchedChunks;
                continue;
            }
            if (javaIds.empty()) {
                ++spuriousChunks;
                std::printf("[STRUCT-PARITY] (%d,%d) 原版无村庄起点，Cubium 生成了：", cx, cz);
                for (const auto& id : cubiumIds) {
                    std::printf(" %s", id.c_str());
                }
                std::printf("\n");
            } else if (cubiumIds.empty()) {
                ++missingChunks;
                std::printf("[STRUCT-PARITY] (%d,%d) 原版有村庄起点，Cubium 没有\n", cx, cz);
            } else {
                ++wrongIdChunks;
                std::printf("[STRUCT-PARITY] (%d,%d) 村庄结构 id 不一致：原版", cx, cz);
                for (const auto& id : javaIds) {
                    std::printf(" %s", id.c_str());
                }
                std::printf(" / Cubium");
                for (const auto& id : cubiumIds) {
                    std::printf(" %s", id.c_str());
                }
                std::printf("\n");
            }
        }
    }

    std::printf("[STRUCT-PARITY] 扫描 (%d,%d)..(%d,%d)：一致 %lld，原版有而 Cubium 无 %lld，"
                "原版无而 Cubium 有 %lld，id 不符 %lld\n",
        minX,
        minZ,
        maxX,
        maxZ,
        static_cast<long long>(matchedChunks),
        static_cast<long long>(missingChunks),
        static_cast<long long>(spuriousChunks),
        static_cast<long long>(wrongIdChunks));

    EXPECT_EQ(missingChunks, 0) << "有村庄起点未被 Cubium 生成";
    EXPECT_EQ(spuriousChunks, 0) << "Cubium 生成了原版没有的村庄起点";
    EXPECT_EQ(wrongIdChunks, 0) << "村庄结构 id 与原版不一致";
}

/**
 * 村庄的**构件装配**与原版一致：构件数、每个构件的模板路径、旋转、地面高度偏移、包围盒。
 *
 * 这一层固定住选点（只对素材里确实有村庄的区块比较），单独衡量装配环节：
 *   - 起始构件选择与旋转的随机数消耗顺序
 *   - 起始构件的 X/Z（原版取区块最小角，无随机）与 Y 投影（project_start_to_heightmap）
 *   - jigsaw 展开：连接点匹配、深度门控、freeShape 裁剪、use_expansion_hack
 *
 * 【当前状态】4 个村庄中 3 个**逐构件完全一致**，第 4 个在前 82 个构件上一致：
 *
 *   区块        原版构件数   Cubium 构件数   逐位相同
 *   (-9,-9)        89            89           89（完全一致）
 *   (-9,0)         78            78           78（完全一致）
 *   (0,-9)        179           179          179（完全一致）
 *   (0,0)         212           205           82
 *
 * 逐个构件的模板路径、旋转、地面高度偏移、包围盒都已核对（此前 `rotation` 恒为 None，
 * 是因为适配器没有把装配得到的旋转写回 StructurePiece 基类字段，现已修复）。
 *
 * 【第 4 个村庄剩余差距的定位】(0,0) 的分歧可以精确描述为：**同一连接点上，候选池的
 * 洗牌结果不同**。具体地，父构件 `streets/straight_01 (9,62,0) rot=CCW90` 的连接面
 * `(22,63,-2)` 上，本实现接受洗牌后首个可附着的候选 `flower_plain`（原版在该连接点上
 * 一个构件都没放），随后同一父构件的连接面 `(15,63,-11)` 上，本实现又取到 `flower_plain`
 * 而原版取到 `pile_hay`。两侧的**候选多重集完全相同**（decor 池展开 7 条：lamp×2、oak、
 * flower_plain、pile_hay、empty×2，与数据包权重逐项一致），而 flower_plain / pile_hay /
 * oak 三个地物元素的虚拟连接点与包围盒完全相同（都是 (0,0,0) 上的单点），因此"取到哪一个"
 * 完全由洗牌后的先后次序决定 —— **候选内容相同而次序不同，等价于随机数状态不同**。
 *
 * 【决定性判据：empty 是否挡在被接受构件之前】`empty_pool_element` 会**立即终止候选枚举**
 * （原版 `element == EmptyPoolElement.INSTANCE → break`，本实现 `isEmpty() → break`），因此
 * "原版接受的构件 x 是否排在本实现序列的 empty 之后"可直接判定两侧洗牌结果是否一致。
 * 实测 (0,0) 村庄有**两处**违反：
 *   - 父构件 #53 `straight_01 (9,62,0) rot=CCW90` 的连接面 `(15,63,-11)`：原版接受
 *     `pile_hay`（本实现序列第 6 位），但本实现序列第 2、3 位就是 empty；
 *   - 父构件 #55 同型构件的连接面 `(12,0,4)`：原版接受 `flower_plain`（第 5 位），
 *     本实现序列第 3、4 位是 empty。
 * 两侧的候选**多重集**完全一致（decor 池展开 7 条与数据包权重逐项相同），且
 * flower_plain / pile_hay / oak 三个地物元素的虚拟连接点与包围盒完全相同（(0,0,0) 上的单点），
 * 因此"取到哪一个"只由洗牌次序决定 —— 次序不同即**随机数状态不同**。
 *
 * 【随机数消耗已逐位核验为忠实复刻】已用一份独立于本实现的 Python 模型（按原版算法书写：
 * LegacyRandom 的 LCG 与 `nextLong` 的低位符号扩展、`setLargeFeatureSeed`、池的权重展开、
 * Fisher-Yates、`Rotation.getShuffled`、父/子连接点洗牌、每个候选 3 次旋转洗牌 + 每旋转
 * (m-1) 次连接点洗牌、遇 empty 终止）与实现内记录的消耗计数逐连接点比对：
 *   - 首次比对必须修正 Python 侧 `nextLong` 的"低 32 位符号扩展"语义后才逐位吻合；
 *   - 修正后，4 个村庄的**全部**连接点消耗一致（残余报告经查是并发日志被截断造成的假象）。
 * 也就是说：**在本实现当前的决策序列下，随机数消耗与原版算法完全一致**，问题不在消耗本身，
 * 而在某处**决策**（碰撞判定）与之不同，进而使消耗错位。
 *
 * 已排除的因素：池的展开顺序与权重（模板加载期逐个打印，与数据包 JSON 顺序逐项一致）、
 * 池内最大 Y 跨度（use_expansion_hack 的输入）、全部 982 个已加载模板的连接点数
 * （与 .nbt 的 jigsaw 方块数逐一比对一致）、jigsaw NBT 与方块状态的一致性（105 万个方块中
 * 无一处"有 jigsaw NBT 但状态不是 jigsaw"）、`Rotation` 枚举顺序（NONE/CW90/CW180/CCW90）、
 * Fisher-Yates 实现、`SequencedPriorityIterator` 语义、池/回退池查找失败分支（实测无 null）、
 * `insideParent` 的 `contains` 边界语义（两端均含）、`Shapes::create` 的 ArrayVoxelShape 退化分支
 * （与原版 `Shapes.create` 的 findBits<0 分支同构）。
 *
 * 尚未排除：空池（0 元素且非 `Pools.EMPTY`）时原版会**整连接点跳过**（不洗回退池）而本实现
 * 仍会洗回退池——本素材上未观察到该情形的实际触发。
 *
 * ── 分歧点已定位到单个连接点（2026-09 结论，尚未修复）──────────────────────────────
 * 原版 #53 = `streets/straight_01 @ (9,62,0) rot=COUNTERCLOCKWISE_90`。其连接面
 * `(22,63,-2)`（连接点方块在 `(22,62,-2)`，朝向 up ⟹ 连接面在其上方一格）上：
 *   - 两侧候选序列相同，都以 `feature_pool_element`（decor 池的 flower_plain）开头；
 *   - 父构件包围盒同为 `[9,62,-15,24,67,0]`，连接面在该盒**内部** ⟹ 原版应走
 *     `flag1 == true` 分支，用 `localFree = Shapes.create(AABB.of(父包围盒))` 参与碰撞；
 *   - 候选是地物（`feature_pool_element`，`getSize()` 为 `Vec3i.ZERO`）⟹ 包围盒是
 *     `(22,63,-2)` 处的 1×1×1，完整落在父盒内部；
 *   - ⟹ `Shapes.joinIsNotEmpty(localFree, 候选盒.deflate(0.25), ONLY_SECOND)` 应为 false
 *     （不碰撞）⟹ **按对原版 `JigsawPlacement.Placer.tryPlacingChildren` 的逐行推导，
 *     原版应当在此放置该地物**。
 * 但存档的 `Children` 与 `junctions` 都表明原版在此**没有**放置任何子构件：
 * `Children[53]` 的 `junctions` 只有 2 条，按"每条 junction 对应一条父子边"计，
 * 它在本实现之外**一个子构件都没有接出**（另一条是父构件给它的）。
 *
 * 本实现多放这一个地物后随机数错位，于是**同一连接点** `(15,63,-11)` 上本实现选到
 * `flower_plain`、原版选到 `pile_hay`，构件列表自第 82 号起全程错位（212 vs 205）。
 *
 * ⟹ 这是一个**硬矛盾**：静态推导说原版应当接受、存档说原版没有接受。矛盾的出口只可能是
 * 对原版该函数的某处理解有偏差。剩余可疑点（按可疑度排序）：
 *   1. `Shapes.create(AABB)` 在**绝对坐标**下的 `ArrayVoxelShape` 回退路径，以及
 *      `Shapes.joinIsNotEmpty` 在该路径下的 `optimize()` 行为——`Shapes.create` 对
 *      `findBits` 失败时用 `DiscreteVoxelShape.box(1,1,1)` + 原始 min/max 作坐标数组，
 *      两端是否等价需要单独构造用例判定（注意本素材里父盒跨度 16×6×16、候选盒 0.5³，
 *      两者坐标系完全不同）；
 *   2. `localFree`（即 `MutableObject<VoxelShape>`）在"父块自身包围盒"与"父块逐个子构件扣减后的
 *      剩余空间"之间的生效范围。
 * 建议下一步先写一个直接针对第 1 点的单元测试：构造"大盒（绝对坐标）减去其内部小盒"的形状对，
 * 断言 `joinIsNotEmpty(大, 小收缩, ONLY_SECOND) == false`。该断言若成立，矛盾必然在第 2 点。
 */
TEST_F(JavaAnvilVillageStructureParityTest, VillagePiecesMatchJavaSave)
{
    const std::vector<StartView>& starts = javaStarts();
    std::vector<StartView> villages;
    for (const auto& start : starts) {
        if (start.structureId.starts_with("minecraft:village")) {
            villages.push_back(start);
        }
    }
    ASSERT_FALSE(villages.empty()) << "素材中没有村庄起点";

    for (const auto& village : villages) {
        world::chunk::ChunkPrimer* primer = generateStarts(village.cx, village.cz);
        ASSERT_NE(primer, nullptr);
        const auto* cubiumStart = primer->getStructureStart(ResourceLocation(village.structureId));
        const std::string title = fmt::format("村庄 ({},{})", village.cx, village.cz);
        ASSERT_NE(cubiumStart, nullptr) << title << " 没有生成结构起点 " << village.structureId;

        const std::vector<PieceView>& javaViews = village.pieces;
        const auto cubiumViews = toPieceViews(*cubiumStart);
        printPieceDiff(title, javaViews, cubiumViews);
        printJunctionDiff(title, javaViews, cubiumViews);

        EXPECT_EQ(cubiumViews.size(), javaViews.size()) << title << " 的构件数与原版不一致";
        for (size_t i = 0; i < std::min(javaViews.size(), cubiumViews.size()); ++i) {
            EXPECT_EQ(cubiumViews[i].sortKey(), javaViews[i].sortKey())
                << title << " 第 " << i << " 个构件（放置顺序）与原版不一致";
        }
    }
}

// ============================================================================
// 【门禁】用例：村庄街道（terrain_matching 构件）的方块落点与原版一致
// ============================================================================

/// 区块内的方块位置（区块局部 X/Z + 世界 Y）
using LocalBlockPos = std::array<i32, 3>;

/**
 * @brief 收集区块中某个方块的**全部**位置
 *
 * 逐方块遍历（而非只看高度图列顶）才能发现"方块被放到别的层"这类错误：
 * 村庄街道被整体下移一格时，列顶仍然是方块，只有逐格比对才能看出这格去哪了。
 */
[[nodiscard]] std::set<LocalBlockPos> collectBlockPositions(const ChunkData& chunk, const std::string& blockName)
{
    std::set<LocalBlockPos> positions;
    for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                const BlockState* state = BlockRegistry::instance().getBlockState(chunk.getBlockStateId(x, y, z));
                if (state != nullptr && state->getBlock().blockLocation().toString() == blockName) {
                    positions.insert({x, y, z});
                }
            }
        }
    }
    return positions;
}

/**
 * @brief 判定某格是否"被埋"：正上方是实心（不透明碰撞箱）方块
 *
 * 被埋的路面在游戏里就是"看不见的路"——这正是本用例要锁死的缺陷形态。
 * 用"不透明碰撞箱"而非"非空气"作为判据：干草堆、雪层、草丛等不遮挡路面的方块不算埋。
 */
[[nodiscard]] bool isBuriedBySolidBlock(const ChunkData& chunk, const LocalBlockPos& pos)
{
    const i32 above = pos[1] + 1;
    if (above >= world::MAX_BUILD_HEIGHT) {
        return false;
    }
    const BlockState* state = BlockRegistry::instance().getBlockState(chunk.getBlockStateId(pos[0], above, pos[2]));
    return state != nullptr && !state->isAir() && state->hasOpaqueCollisionShape();
}

/// 路面逐格复现率下限（原版路面格中，Cubium 在同一格也放下路面的比例）
constexpr f64 kMinRoadReproductionRatio = 0.85;

/// 路面总格数相对原版的偏差上限
constexpr f64 kMaxRoadCountDeltaRatio = 0.05;

/**
 * 村庄街道（terrain_matching 构件）的方块落点：既不得被埋，也要与原版逐格吻合。
 *
 * 【为什么单独测"街道"】村庄的建筑（houses/town_centers/decor）是 RIGID 投影，模板按绝对
 * 坐标放置；而**街道与 terminators 是 terrain_matching 投影**，每个方块的落点由
 * "该列地形高度 + 构件局部 Y"决定。这条链路（GravityStructureProcessor）有两处极易出错的
 * 语义，且错了**不会报错**、只看构件包围盒也完全看不出来：
 *   - 原版 `LevelReader.getHeight` 返回"首个可放置高度"（最高方块 Y + 1），而项目的
 *     `IWorld::getHeight` 返回"最高方块 Y"，少加 1 会把整片构件下移一格；
 *   - 原版还会叠加**方块在模板内的局部 Y**（整块模板沿 Y 平移），漏掉它会把构件压扁到同一层。
 * 两者叠加的实际症状：路面 dirt_path 落到地表之下、被草方块盖住——玩家侧就是"村庄里看不见路"。
 *
 * 【为什么用 dirt_path 作探针】整个原版数据包中只有村庄的模板含 dirt_path（plains/desert/
 * savanna/taiga/snowy 的 streets+terminators+houses+town_centers，共 244 个模板），世界中不存在
 * 其它生成 dirt_path 的途径，它也不会被任何自然过程（随机刻、流体、生物 AI）创建或移除。
 * 因此它是既灵敏（任何一格错位都会暴露）又抗噪（不受树/草等装饰差异干扰）的探针。
 *
 * 【两条判据】
 *   1) 严格（不依赖素材的方块内容）：Cubium 放下的路面被埋的格数 **不得多于** 原版同区块的
 *      被埋格数。原版有 6 格被埋在干草堆之下（村庄农场装饰），属合法；而"整片路面下沉一格"
 *      会让 Cubium 的被埋格数暴增到接近全部格数，立刻 FAIL。这条判据直接锁死"看不见路"。
 *   2) 量化（对照原版存档）：路面逐格复现率与总格数偏差。当前实测 0.895（272/304），
 *      门限取 0.85。
 *
 * 【残余差异的定位结论（已排查，非结构缺陷）】实测全部差异格都落在**区块边界 8 格以内**，
 * 一格不差（61/61）。该带正是原版 `ChunkGenerator.applyBiomeDecoration` 用
 * `[chunkMin-8, chunkMax+8]` 包围盒放置构件的重叠区：同一格会被本区块与邻区块各放一遍，
 * 每次读的是该时刻的高度图，因此该带内的逐格结果本身对区块处理顺序/时刻敏感
 * （与 JavaAnvilWorldGenParityTest 文件头记录的"parity 数值不是顺序无关"同源）。
 * 且差异格在素材侧表现为**从未被结构触碰过的原始地表**（草方块上有短草），
 * 而按模板这些格本应有路面——素材本身在这些格上与"纯 worldgen 产物"不自洽。
 * 故这里把逐格完全相等作为**收敛目标**而非当前门限。
 *
 * 【边界】本用例要求整条生成管线跑到 FULL（含 FEATURES 阶段的构件放置），与只跑
 * STRUCTURE_STARTS 的 VillagePiecesMatchJavaSave 互补：后者定位"装配出的构件列表错在哪"，
 * 本用例定位"构件里的方块放错在哪"。
 */
TEST_F(JavaAnvilVillageStructureParityTest, VillageStreetBlocksMatchJavaSave)
{
    const std::vector<StartView>& starts = javaStarts();
    std::set<i64> villageChunks;
    for (const auto& start : starts) {
        if (start.structureId == "minecraft:village_plains") {
            villageChunks.insert(packChunkKey(start.cx, start.cz));
        }
    }
    ASSERT_FALSE(villageChunks.empty()) << "素材中没有 village_plains 起点，无法校验方块落点";

    // 独立于 SetUpTestSuite 生成器的区块管理器：本用例要把区块推进到 FULL，
    // 会驱动 carver/feature/structure 全链路。
    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, static_cast<u64>(m_seed));
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
    auto generator =
        std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
    server::ServerChunkManager manager(std::move(generator));

    i64 javaTotal = 0;
    i64 cubiumTotal = 0;
    i64 reproduced = 0;
    i64 javaBuriedTotal = 0;
    i64 cubiumBuriedTotal = 0;

    for (const i64 key : villageChunks) {
        const ChunkCoord cx = static_cast<i32>(key >> 32);
        const ChunkCoord cz = static_cast<i32>(static_cast<u32>(key));

        auto javaResult = m_backend.loadChunk(cx, cz, 0);
        ASSERT_TRUE(javaResult.success()) << javaResult.error().message();
        ASSERT_TRUE(javaResult.value().has_value()) << "素材中不存在区块 (" << cx << "," << cz << ")";
        const ChunkData& javaChunk = *javaResult.value();

        ChunkData* cubiumChunk = manager.requestFullChunkSync(cx, cz);
        ASSERT_NE(cubiumChunk, nullptr) << "生成区块 (" << cx << "," << cz << ") 失败";
        ASSERT_TRUE(cubiumChunk->isFullyGenerated()) << "区块 (" << cx << "," << cz << ") 未推进到 FULL";

        const std::set<LocalBlockPos> javaRoad = collectBlockPositions(javaChunk, "minecraft:dirt_path");
        const std::set<LocalBlockPos> cubiumRoad = collectBlockPositions(*cubiumChunk, "minecraft:dirt_path");

        std::vector<LocalBlockPos> missing; // 原版有、Cubium 无
        std::vector<LocalBlockPos> extra;   // Cubium 有、原版无
        std::set_difference(
            javaRoad.begin(), javaRoad.end(), cubiumRoad.begin(), cubiumRoad.end(), std::back_inserter(missing));
        std::set_difference(
            cubiumRoad.begin(), cubiumRoad.end(), javaRoad.begin(), javaRoad.end(), std::back_inserter(extra));

        i64 javaBuried = 0;
        for (const auto& pos : javaRoad) {
            javaBuried += isBuriedBySolidBlock(javaChunk, pos) ? 1 : 0;
        }
        i64 cubiumBuried = 0;
        i64 cubiumBuriedSampleShown = 0;
        for (const auto& pos : cubiumRoad) {
            if (isBuriedBySolidBlock(*cubiumChunk, pos)) {
                ++cubiumBuried;
                if (cubiumBuriedSampleShown < 3) {
                    ++cubiumBuriedSampleShown;
                    std::printf("[VILLAGE-BLOCKS]   被埋 局部(%2d,%3d,%2d)（世界 %d,%d,%d）\n",
                        pos[0],
                        pos[1],
                        pos[2],
                        cx * world::CHUNK_WIDTH + pos[0],
                        pos[1],
                        cz * world::CHUNK_WIDTH + pos[2]);
                }
            }
        }

        const i64 inBoth = static_cast<i64>(javaRoad.size() + cubiumRoad.size() - missing.size() - extra.size()) / 2;

        javaTotal += static_cast<i64>(javaRoad.size());
        cubiumTotal += static_cast<i64>(cubiumRoad.size());
        reproduced += inBoth;
        javaBuriedTotal += javaBuried;
        cubiumBuriedTotal += cubiumBuried;

        std::printf("[VILLAGE-BLOCKS] 区块 (%2d,%2d) dirt_path：原版 %zu 格 / Cubium %zu 格，逐格相同 %lld 格"
                    "（缺 %zu、多 %zu）；被埋 原版 %lld / Cubium %lld\n",
            cx,
            cz,
            javaRoad.size(),
            cubiumRoad.size(),
            static_cast<long long>(inBoth),
            missing.size(),
            extra.size(),
            static_cast<long long>(javaBuried),
            static_cast<long long>(cubiumBuried));

        // 严格判据：Cubium 不得比原版埋得更多
        EXPECT_LE(cubiumBuried, javaBuried)
            << "区块 (" << cx << "," << cz << ") 的村庄路面被下方地形埋住的格数多于原版——路面被放低了一格";
    }

    // 素材侧必须真的含路面方块，否则"相等"可能只是"两边都空"的假通过
    ASSERT_GT(javaTotal, 0) << "素材中的村庄区块不含 dirt_path，探针失效";

    const f64 reproductionRatio = static_cast<f64>(reproduced) / static_cast<f64>(javaTotal);
    const f64 countDeltaRatio = std::abs(static_cast<f64>(cubiumTotal - javaTotal)) / static_cast<f64>(javaTotal);
    std::printf("[VILLAGE-BLOCKS] 合计：原版 %lld 格 / Cubium %lld 格，逐格相同 %lld 格（复现率 %.3f），"
                "被埋 原版 %lld / Cubium %lld\n",
        static_cast<long long>(javaTotal),
        static_cast<long long>(cubiumTotal),
        static_cast<long long>(reproduced),
        reproductionRatio,
        static_cast<long long>(javaBuriedTotal),
        static_cast<long long>(cubiumBuriedTotal));

    EXPECT_GE(reproductionRatio, kMinRoadReproductionRatio)
        << "村庄路面与原版的逐格复现率低于门限（收敛目标为 1.0，见用例注释）";
    EXPECT_LE(countDeltaRatio, kMaxRoadCountDeltaRatio) << "村庄路面的总格数与原版偏差过大";
}

} // namespace
