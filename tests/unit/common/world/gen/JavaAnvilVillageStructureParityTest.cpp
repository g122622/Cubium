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
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/storage/JavaRealWorldFixture.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/feature/template/TemplateManagerHostBinding.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/storage/backend/JavaAnvilBackend.hpp"
#include "server/world/storage/core/SaveFormat.hpp"
#include "server/world/storage/reader/java/RegionFile.hpp"

#include <algorithm>
#include <cstdio>
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
                    piece.templateLocation = findString(*element, "location");
                    if (piece.templateLocation.empty()) {
                        // list_pool_element / feature_pool_element 没有 location，退化为类型名
                        piece.templateLocation = findString(*element, "element_type");
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
            collectStartsFromRoot(
                *root, regionX * 32 + localX, regionZ * 32 + localZ, starts);
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
        m_generator = std::make_unique<NoiseChunkGenerator>(
            std::move(settings), std::move(biomeSource), std::move(randomState));
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
            views.push_back(std::move(view));
        }
        std::sort(views.begin(), views.end(), [](const PieceView& a, const PieceView& b) {
            return a.sortKey() < b.sortKey();
        });
        return views;
    }

    /// 打印两侧构件列表的对照（只打印前 limit 条，避免刷屏）
    static void printPieceDiff(const std::string& title,
        const std::vector<PieceView>& javaViews,
        const std::vector<PieceView>& cubiumViews,
        size_t limit)
    {
        std::printf("[STRUCT-PARITY] %s 原版 %zu 个构件 / Cubium %zu 个构件\n",
            title.c_str(),
            javaViews.size(),
            cubiumViews.size());
        const size_t count = std::min({ javaViews.size(), cubiumViews.size(), limit });
        for (size_t i = 0; i < count; ++i) {
            const bool same = javaViews[i].sortKey() == cubiumViews[i].sortKey();
            std::printf("[STRUCT-PARITY]   [%2zu]%s 原版   %s\n", i, same ? "  " : " !", javaViews[i].text().c_str());
            if (!same) {
                std::printf("[STRUCT-PARITY]   [%2zu]   Cubium %s\n", i, cubiumViews[i].text().c_str());
            }
        }
        // 数量不等时把多出来的部分也打出来，否则"多/少了哪些构件"无从判断
        const size_t longer = std::max(javaViews.size(), cubiumViews.size());
        for (size_t i = count; i < longer && i < count + limit; ++i) {
            if (i < javaViews.size()) {
                std::printf("[STRUCT-PARITY]   [%2zu] - 原版   %s\n", i, javaViews[i].text().c_str());
            } else {
                std::printf("[STRUCT-PARITY]   [%2zu] + Cubium %s\n", i, cubiumViews[i].text().c_str());
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

    const auto* pool = TemplatePoolRegistry::instance().getPool(
        ResourceLocation::parse("minecraft:village/plains/town_centers"));
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
 * 【当前状态】失败，但已从"每个村庄只有 1 个退化构件"推进到"构件数与起始构件均正确"：
 *
 *   区块        原版构件数   Cubium 构件数   起始构件是否一致
 *   (-9,-9)        89            15            一致
 *   (-9,0)         78            11            一致
 *   (0,-9)        179            15            一致
 *   (0,0)         212            13            一致
 *
 * 起始构件（第 0 个）的模板路径、旋转、包围盒现已与原版逐项相同——说明
 * **起始旋转/起始块选择的随机数顺序、起始点 X/Z（区块最小角）、Y 投影与地面线对齐
 * 都已对齐**。剩余差距全部在 jigsaw 展开（Placer）环节。
 *
 * 【剩余差距的定位】差距不在"随机数总量"，而在**候选枚举方式**：原版对每个父连接点是
 * 四层嵌套遍历——候选元素（打乱）→ 旋转（打乱）→ 该候选的连接点（打乱）→ 命中即
 * 放置并跳到父块的下一个连接点；本实现是把某候选的全部 (连接点, 旋转) 匹配收集起来
 * **随机挑一个**。二者有双重后果：
 *   1. 随机数消耗模式不同（原版每个候选消耗 3 次旋转洗牌 + (m-1) 次连接点洗牌）；
 *   2. 随机挑中的那一个若因碰撞被拒，本实现会放弃该连接点去试下一个候选元素，
 *      而原版会继续尝试该候选的其余旋转/连接点——直接导致大量连接点被浪费、
 *      构件数远低于原版。
 * 这与"村庄里 name 与 target 同名"无关，是 Placer 循环结构的差异。
 *
 * 【收敛路径】把 tryPlacePiece 改写成原版 Placer.tryPlacingChildren 的形态：待处理队列
 * 由"连接点"改为"构件 + 该构件的 freeShape 持有者 + 深度"（PieceState），一次调用处理
 * 一个父构件的全部连接点，内部按上述四层顺序遍历、命中即放置并 break；同时对齐
 * JigsawJunction 的双向记录与 `i3` 的三分支公式。
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
        const auto* cubiumStart =
            primer->getStructureStart(ResourceLocation(village.structureId));
        const std::string title =
            fmt::format("村庄 ({},{})", village.cx, village.cz);
        ASSERT_NE(cubiumStart, nullptr) << title << " 没有生成结构起点 " << village.structureId;

        auto javaViews = village.pieces;
        std::sort(javaViews.begin(), javaViews.end(), [](const PieceView& a, const PieceView& b) {
            return a.sortKey() < b.sortKey();
        });
        const auto cubiumViews = toPieceViews(*cubiumStart);
        printPieceDiff(title, javaViews, cubiumViews, 8);

        EXPECT_EQ(cubiumViews.size(), javaViews.size()) << title << " 的构件数与原版不一致";
        for (size_t i = 0; i < std::min(javaViews.size(), cubiumViews.size()); ++i) {
            EXPECT_EQ(cubiumViews[i].sortKey(), javaViews[i].sortKey())
                << title << " 第 " << i << " 个构件与原版不一致";
        }
    }
}

} // namespace
