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

package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonPrimitive;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.function.Predicate;
import java.lang.reflect.Proxy;
import java.util.concurrent.CompletableFuture;
import it.unimi.dsi.fastutil.longs.LongSet;
import net.minecraft.core.Holder;
import net.minecraft.core.HolderGetter;
import net.minecraft.core.HolderLookup;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.ResourceKey;
import net.minecraft.util.datafix.DataFixers;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.LevelHeightAccessor;
import net.minecraft.world.level.biome.Biome;
import net.minecraft.world.level.biome.BiomeSource;
import net.minecraft.world.level.biome.MultiNoiseBiomeSource;
import net.minecraft.world.level.biome.MultiNoiseBiomeSourceParameterLists;
import net.minecraft.world.level.biome.TheEndBiomeSource;
import net.minecraft.world.level.chunk.ChunkGenerator;
import net.minecraft.world.level.chunk.ChunkGeneratorStructureState;
import net.minecraft.world.level.chunk.PalettedContainerFactory;
import net.minecraft.world.level.chunk.ProtoChunk;
import net.minecraft.world.level.chunk.UpgradeData;
import net.minecraft.world.level.levelgen.Heightmap;
import net.minecraft.world.level.levelgen.NoiseBasedChunkGenerator;
import net.minecraft.world.level.levelgen.NoiseGeneratorSettings;
import net.minecraft.world.level.levelgen.RandomState;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.Structure;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.StructureManager;
import net.minecraft.world.level.levelgen.WorldOptions;
import net.minecraft.world.level.levelgen.structure.StructureCheck;
import net.minecraft.world.level.levelgen.structure.StructureSet;
import net.minecraft.world.level.levelgen.structure.PoolElementStructurePiece;
import net.minecraft.world.level.levelgen.structure.StructureStart;
import net.minecraft.world.level.levelgen.structure.pools.FeaturePoolElement;
import net.minecraft.world.level.levelgen.structure.pools.JigsawJunction;
import net.minecraft.world.level.levelgen.structure.pools.SinglePoolElement;
import net.minecraft.world.level.levelgen.structure.pools.StructurePoolElement;
import net.minecraft.world.level.levelgen.structure.placement.ConcentricRingsStructurePlacement;
import net.minecraft.world.level.levelgen.structure.placement.RandomSpreadStructurePlacement;
import net.minecraft.world.level.levelgen.structure.pools.StructureTemplatePool;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplateManager;
import net.minecraft.core.RegistryAccess;
import net.minecraft.world.flag.FeatureFlags;
import net.minecraft.world.level.LevelAccessor;
import net.minecraft.world.level.dimension.BuiltinDimensionTypes;
import net.minecraft.world.level.chunk.ChunkAccess;
import net.minecraft.world.level.chunk.ChunkSource;
import net.minecraft.world.level.chunk.status.ChunkStatus;
import net.minecraft.world.level.storage.LevelData;
import net.minecraft.util.RandomSource;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.level.entity.EntityTypeTest;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;
import net.minecraft.world.level.storage.LevelStorageSource;

/**
 * 原版结构起点（STRUCTURE_STARTS 阶段产物）的参照实现。
 *
 * <p>输入一行一个样本：维度名、世界种子、区块坐标。输出该区块里落下的全部结构起点，
 * 每个起点给出被选中的结构、整体包围盒、各个片段（顺序即装配顺序）的包围盒 / 旋转 /
 * 地面高度偏移 / 模板名 / 投影模式 / 连接点。
 *
 * <p>两侧走的是同一段生产逻辑：原版调 {@code ChunkGenerator#createStructures}，本项目的
 * 探针调 {@code NoiseChunkGenerator::generateStructureStarts}。语料由编排层统一生成
 * （{@code harnesses/structure_starts.ts}），因此不提供 {@code list}。
 *
 * <p>这里不造世界：结构生成要回读的只是「本区块里已有哪些起点」，因此用一个只持有
 * 空区块与坐标的 {@link StructureAccess} 适配层顶替 {@code StructureManager}。
 * 区块是 {@link ProtoChunk}，只给出坐标与高度范围；高度图与噪声群系全部由生成器自己
 * 的 {@link RandomState} 现算，与真实区块生成一致。
 */
public final class StructureStartsOracle {
    private static final Gson GSON = new GsonBuilder().serializeSpecialFloatingPointValues().create();

    private StructureStartsOracle() {}

    /** 一个维度的材质：生成器 + 结构放置状态 + 高度范围。 */
    private record Dimension(
            ChunkGenerator generator,
            ChunkGeneratorStructureState placementState,
            LevelHeightAccessor height,
            ResourceKey<Level> levelKey,
            RandomState randomState) {}

    private static Dimension makeDimension(String name, long seed, HolderLookup.Provider lookup) {
        HolderLookup.RegistryLookup<StructureSet> sets = lookup.lookupOrThrow(Registries.STRUCTURE_SET);
        BiomeSource biomeSource;
        Holder<NoiseGeneratorSettings> settingsHolder;
        NoiseGeneratorSettings settings;
        ResourceKey<Level> levelKey;
        LevelHeightAccessor height;
        if (name.equals("overworld")) {
            biomeSource = MultiNoiseBiomeSource.createFromPreset(
                    lookup.lookupOrThrow(Registries.MULTI_NOISE_BIOME_SOURCE_PARAMETER_LIST)
                            .getOrThrow(MultiNoiseBiomeSourceParameterLists.OVERWORLD));
            settingsHolder = lookup.lookupOrThrow(Registries.NOISE_SETTINGS).getOrThrow(NoiseGeneratorSettings.OVERWORLD);
            levelKey = Level.OVERWORLD;
            height = LevelHeightAccessor.create(-64, 384);
        } else if (name.equals("the_end")) {
            biomeSource = TheEndBiomeSource.create(lookup.lookupOrThrow(Registries.BIOME));
            settingsHolder = lookup.lookupOrThrow(Registries.NOISE_SETTINGS).getOrThrow(NoiseGeneratorSettings.END);
            levelKey = Level.END;
            height = LevelHeightAccessor.create(0, 256);
        } else {
            throw new IllegalArgumentException("StructureStartsOracle: unknown dimension " + name);
        }
        settings = settingsHolder.value();
        ChunkGenerator generator = new NoiseBasedChunkGenerator(biomeSource, settingsHolder);
        RandomState randomState = RandomState.create(settings, lookup.lookupOrThrow(Registries.NOISE), seed);
        ChunkGeneratorStructureState placementState =
                ChunkGeneratorStructureState.createForNormal(randomState, seed, biomeSource, sets);
        return new Dimension(generator, placementState, height, levelKey, randomState);
    }

    /**
     * 模板管理器：模板只从原版数据包读，不落盘任何世界目录。
     *
     * 构造函数要求的 {@code LevelStorageAccess} 只用来定位「生成目录」（保存自动生成的模板），
     * 结构生成从不写生成模板，因此给一个临时空目录即可；它会被持有到进程结束。
     */
    private static StructureTemplateManager templateManager(DataPackRegistries registries) {
        try {
            Path emptyLevel = Files.createTempDirectory("diff-structure-starts");
            var source = LevelStorageSource.createDefault(emptyLevel);
            var access = source.createAccess("diff");
            return new StructureTemplateManager(registries.resourceManager, access,
                    DataFixers.getDataFixer(), registries.lookup.lookupOrThrow(Registries.BLOCK));
        } catch (Exception error) {
            throw new IllegalStateException("StructureStartsOracle: 无法创建模板管理器", error);
        }
    }

    /**
     * 结构管理器：只服务本区块的结构起点读写。
     *
     * 结构检查器的构造需要「区块扫描器」，它只在「本区块没有已加载起点时去读存档」这条
     * 路径上被调用；探测的是新区块，调用方从不会走到那里。因此给一个永远失败的扫描器，
     * 一旦真的被调用就会立刻暴露（而不是静默返回错误数据）。
     */
    private static StructureManager structureManager(DataPackRegistries registries, Dimension dimension,
            ProtoChunk chunk, StructureTemplateManager templates) {
        StructureCheck check = new StructureCheck(
                (pos, visitor) -> CompletableFuture.failedFuture(new UnsupportedOperationException("no storage in diff probe")),
                registries.access,
                templates,
                dimension.levelKey(),
                dimension.generator(),
                dimension.placementState().randomState(),
                dimension.height(),
                dimension.generator().getBiomeSource(),
                dimension.placementState().getLevelSeed(),
                DataFixers.getDataFixer());
        return new StructureManager(singleChunkLevel(chunk, registries.access),
                new WorldOptions(dimension.placementState().getLevelSeed(), true, false), check);
    }

    /**
     * 只含一个区块的最小世界视图。
     *
     * 结构管理器只在这一层上读「本区块的起点引用」。{@code LevelAccessor} 有几十个抽象方法，
     * 逐个实现既冗长又容易写错；这里用动态代理只接管真正会被读到的几个，其余一律抛错——
     * 任何意外的世界访问都会立刻暴露，而不是拿到编造的数据后悄悄产出不同的结构。
     */
    private static LevelAccessor singleChunkLevel(ProtoChunk chunk, RegistryAccess registries) {
        return (LevelAccessor) Proxy.newProxyInstance(
                StructureStartsOracle.class.getClassLoader(),
                new Class<?>[] {LevelAccessor.class},
                (proxy, method, args) -> {
                    switch (method.getName()) {
                        case "registryAccess":
                            return registries;
                        case "getChunk":
                            return chunkLevelChunk(chunk, (int) args[0], (int) args[1]);
                        case "hasChunk":
                            return (int) args[0] == chunk.getPos().x && (int) args[1] == chunk.getPos().z;
                        case "getHeight":
                            return chunk.getHeight();
                        case "getMinY":
                            return chunk.getMinY();
                        case "getMaxY":
                            return chunk.getMaxY();
                        case "nextSubTickCount":
                            return 0L;
                        case "isClientSide":
                            return false;
                        case "enabledFeatures":
                            return FeatureFlags.DEFAULT_FLAGS;
                        case "dimensionType":
                            return registries.lookupOrThrow(Registries.DIMENSION_TYPE)
                                    .getOrThrow(BuiltinDimensionTypes.OVERWORLD)
                                    .value();
                        case "getEntities":
                        case "players":
                            return List.of();
                        case "toString":
                            return "SingleChunkLevel";
                        case "hashCode":
                            return System.identityHashCode(proxy);
                        case "equals":
                            return proxy == args[0];
                        default:
                            throw new UnsupportedOperationException(
                                    "StructureStartsOracle: 结构生成不应调用 LevelAccessor#" + method.getName());
                    }
                });
    }

    private static ChunkAccess chunkLevelChunk(ProtoChunk chunk, int chunkX, int chunkZ) {
        if (chunkX == chunk.getPos().x && chunkZ == chunk.getPos().z) {
            return chunk;
        }
        throw new IllegalStateException("StructureStartsOracle: 意外读取了区块外数据 (" + chunkX + "," + chunkZ + ")");
    }

    private static JsonArray box(BoundingBox box) {
        JsonArray array = new JsonArray();
        array.add(box.minX());
        array.add(box.minY());
        array.add(box.minZ());
        array.add(box.maxX());
        array.add(box.maxY());
        array.add(box.maxZ());
        return array;
    }

    /** 结构对象 → 注册表里的资源位置。 */
    private static String lookupKey(DataPackRegistries registries, Structure structure) {
        return registries.worldGenAccess.lookupOrThrow(Registries.STRUCTURE).getResourceKey(structure).orElseThrow().identifier().toString();
    }

    /**
     * 从 {@code Feature[ResourceKey[minecraft:placed_feature / minecraft:oak]]} 里取出地物 id。
     *
     * 必须取**第一个**闭合方括号：toString 后面还跟着配置对象的描述，其中也可能含 {@code ]}，
     * 用最后一个方括号会把整段配置说明一并截出来。
     */
    private static String unpackFeatureId(String text) {
        int slash = text.lastIndexOf('/');
        int close = text.indexOf(']', slash + 1);
        if (slash < 0 || close <= slash) {
            return "";
        }
        return text.substring(slash + 1, close).trim();
    }

    private static JsonObject summary(StructurePiece piece) {
        JsonObject object = new JsonObject();
        object.add("box", box(piece.getBoundingBox()));
        // 非拼图片段（要塞、矿井……）没有旋转概念，原版返回 null，两侧都按 NONE 输出。
        object.addProperty("rotation", piece.getRotation() == null ? "NONE" : piece.getRotation().name());
        // 地面高度偏移与连接点只有拼图片段有；其它片段（要塞房间、矿井走廊……）在两侧
        // 都按 0 与空列表输出。
        boolean jigsaw = piece instanceof PoolElementStructurePiece;
        object.addProperty("groundLevelDelta", jigsaw ? ((PoolElementStructurePiece) piece).getGroundLevelDelta() : 0);
        // 模板名取池元素的资源 ID（与数据包 template_pool 里写的一致）；
        // 无模板的片段（地物片段、空占位）在两侧都输出空串。
        String template = "";
        if (jigsaw) {
            StructurePoolElement element = ((PoolElementStructurePiece) piece).getElement();
            if (element instanceof SinglePoolElement single) {
                template = single.getTemplateLocation().toString();
            } else if (element instanceof FeaturePoolElement) {
                // 地物片段没有模板文件，构件标识取被放置的配置化地物（与原版"地物构件按 id 区分"
                // 一致）。原版没有公开访问器，这里按 toString 解析——它形如
                // `Feature[ResourceKey[minecraft:placed_feature / minecraft:oak]]`。
                template = unpackFeatureId(element.toString());
            }
        }
        object.addProperty("template", template);
        object.addProperty("projection",
                jigsaw && ((PoolElementStructurePiece) piece).getElement().getProjection() == StructureTemplatePool.Projection.TERRAIN_MATCHING
                        ? "terrain_matching"
                        : "rigid");
        JsonArray junctions = new JsonArray();
        for (JigsawJunction junction : jigsaw
                ? ((PoolElementStructurePiece) piece).getJunctions()
                : java.util.List.<JigsawJunction>of()) {
            JsonObject entry = new JsonObject();
            entry.addProperty("x", junction.getSourceX());
            entry.addProperty("groundY", junction.getSourceGroundY());
            entry.addProperty("z", junction.getSourceZ());
            entry.addProperty("deltaY", junction.getDeltaY());
            entry.addProperty("dest",
                    junction.getDestProjection() == StructureTemplatePool.Projection.RIGID ? "rigid" : "terrain_matching");
            junctions.add(entry);
        }
        object.add("junctions", junctions);
        return object;
    }

    private static JsonObject runSample(JsonObject input, DataPackRegistries registries) {
        String dimensionName = input.get("dimension").getAsString();
        long seed = input.get("seed").getAsLong();
        int chunkX = input.get("chunkX").getAsInt();
        int chunkZ = input.get("chunkZ").getAsInt();

        Dimension dimension = makeDimension(dimensionName, seed, registries.lookup);
        ProtoChunk chunk = new ProtoChunk(new ChunkPos(chunkX, chunkZ), UpgradeData.EMPTY, dimension.height(),
                PalettedContainerFactory.create(registries.access), null);
        StructureTemplateManager templates = templateManager(registries);
        StructureManager structureManager = structureManager(registries, dimension, chunk, templates);
        dimension.generator().createStructures(registries.worldGenAccess, dimension.placementState(), structureManager, chunk, templates,
                dimension.levelKey());

        JsonArray starts = new JsonArray();
        for (var entry : chunk.getAllStarts().entrySet()) {
            StructureStart start = entry.getValue();
            if (!start.isValid()) {
                continue;
            }
            JsonObject object = new JsonObject();
            object.addProperty("structure",
                    lookupKey(registries, entry.getKey()));
            object.add("box", box(start.getBoundingBox()));
            JsonArray pieces = new JsonArray();
            for (StructurePiece piece : start.getPieces()) {
                pieces.add(summary(piece));
            }
            object.add("pieces", pieces);
            starts.add(object);
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", input.has("unit") ? input.get("unit").getAsString() : "");
        result.add("starts", starts);
        return result;
    }

    /**
     * 导出候选区块表（每个维度、每个种子各一行 `dimension seed chunkX chunkZ`）。
     *
     * 随机散布与同心环两种放置规则分开枚举：前者按网格逐块判定，扫描范围足够大才能
     * 保证覆盖整张网格；后者直接给出预生成的环形位置，不必扫描。
     */
    private static void exportCandidates(String[] seeds, Path output) throws IOException {
        DataPackRegistries registries = DataPackRegistries.load();
        StringBuilder lines = new StringBuilder();
        for (String dimensionName : List.of("overworld", "the_end")) {
            for (String seedText : seeds) {
                long seed = Long.parseLong(seedText);
                Dimension dimension = makeDimension(dimensionName, seed, registries.lookup);
                for (Holder<StructureSet> setHolder : dimension.placementState().possibleStructureSets()) {
                    StructureSet set = setHolder.value();
                    if (set.placement() instanceof ConcentricRingsStructurePlacement concentric) {
                        List<ChunkPos> ring = dimension.placementState().getRingPositionsFor(concentric);
                        if (ring != null) {
                            for (ChunkPos pos : ring) {
                                lines.append(dimensionName).append(' ').append(seed).append(' ')
                                        .append(pos.x).append(' ').append(pos.z).append('\n');
                            }
                        }
                        continue;
                    }
                    // 随机散布：扫描到足以覆盖若干个网格周期，保证每个结构集都有代表。
                    int spacing = ((RandomSpreadStructurePlacement) set.placement()).spacing();
                    int radius = Math.max(64, spacing * 3);
                    for (int x = -radius; x <= radius; x += spacing) {
                        for (int z = -radius; z <= radius; z += spacing) {
                            // 网格内落点是每格唯一一个，扫整格会得到 spacing² 次判定；这里每格只扫一个
                            // 足够小的邻域，按放置规则自己算出的候选点先跳到该格，再验证一次。
                            ChunkPos potential = ((RandomSpreadStructurePlacement) set.placement())
                                    .getPotentialStructureChunk(seed, x, z);
                            if (set.placement().isStructureChunk(dimension.placementState(), potential.x, potential.z)) {
                                lines.append(dimensionName).append(' ').append(seed).append(' ')
                                        .append(potential.x).append(' ').append(potential.z).append('\n');
                            }
                        }
                    }
                }
            }
        }
        Files.writeString(output, lines.toString(), StandardCharsets.UTF_8);
    }

    public static void main(String[] args) throws Exception {
        if (args.length == 2 && args[0].equals("list")) {
            throw new IllegalArgumentException("StructureStartsOracle: 单元由共同语料决定，不支持 list");
        }
        if (args.length >= 4 && args[0].equals("candidates")) {
            exportCandidates(java.util.Arrays.copyOfRange(args, 1, args.length - 1), Path.of(args[args.length - 1]));
            return;
        }
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException(
                    "usage: StructureStartsOracle run <input> <output>\n"
                            + "       StructureStartsOracle candidates <seed>... <output>");
        }

        DataPackRegistries registries = DataPackRegistries.load();
        StringBuilder output = new StringBuilder();
        for (String line : Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject input = JsonParser.parseString(line).getAsJsonObject();
            output.append(GSON.toJson(runSample(input, registries))).append('\n');
        }
        Files.writeString(Path.of(args[2]), output.toString(), StandardCharsets.UTF_8);
    }
}
