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
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.BufferedWriter;
import java.io.IOException;
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.TreeMap;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Holder;
import net.minecraft.core.RegistryAccess;
import net.minecraft.core.SectionPos;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.util.RandomSource;
import net.minecraft.world.flag.FeatureFlagSet;
import net.minecraft.world.level.biome.Biome;
import net.minecraft.world.flag.FeatureFlags;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.Property;
import net.minecraft.world.level.chunk.ChunkAccess;
import net.minecraft.world.level.chunk.status.ChunkStatus;
import net.minecraft.world.level.chunk.PalettedContainerFactory;
import net.minecraft.world.level.chunk.ProtoChunk;
import net.minecraft.world.level.chunk.UpgradeData;
import net.minecraft.world.level.levelgen.Heightmap;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.WorldgenRandom;
import net.minecraft.world.level.levelgen.XoroshiroRandomSource;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.Structure;
import net.minecraft.world.level.levelgen.structure.StructureStart;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplateManager;
import net.minecraft.world.level.material.Fluid;

/**
 * 单个结构在指定区块上的方块产物（原版参照实现）。
 *
 * <pre>
 *   java dev.horizonlab.diff.StructureBlocksSingleOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 输入与 {@link StructureStartSingleOracle} 相同；输出该起点覆盖的每个区块跑完
 * {@code StructureStart#placeInChunk} 之后的方块表（按坐标去重、按 x/y/z 排序）。
 *
 * <p>世界是一个只记录写入的空世界：读取一律是空气，写入进内存表。两侧必须表现一致，
 * 任何「先读地形再决定写什么」的差异都会立刻显现。
 *
 * <p>每个区块的随机源与生产一致：
 * {@code new WorldgenRandom(new XoroshiroRandomSource(0))} →
 * {@code setDecorationSeed(seed, chunkMinX, chunkMinZ)} →
 * {@code setFeatureSeed(decorSeed, 0, structure.step().ordinal())}。
 */
public final class StructureBlocksSingleOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private StructureBlocksSingleOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: StructureBlocksSingleOracle run <input> <output>");
        }
        DataPackRegistries registries = DataPackRegistries.load();
        try (BufferedWriter writer = Files.newBufferedWriter(Path.of(args[2]), StandardCharsets.UTF_8)) {
            for (String line : Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8)) {
                if (line.isBlank()) {
                    continue;
                }
                writer.write(GSON.toJson(runSample(JsonParser.parseString(line).getAsJsonObject(), registries)));
                writer.newLine();
            }
        }
    }

    private static JsonObject runSample(JsonObject input, DataPackRegistries registries) {
        final String unit = input.has("unit") ? input.get("unit").getAsString() : "";
        final String dimensionName = input.get("dimension").getAsString();
        final long seed = input.get("seed").getAsLong();
        final int chunkX = input.get("chunkX").getAsInt();
        final int chunkZ = input.get("chunkZ").getAsInt();
        final ResourceKey<Structure> key = ResourceKey.create(
                Registries.STRUCTURE, Identifier.parse(input.get("structure").getAsString()));
        final Holder<Structure> holder = registries.worldGenAccess.lookupOrThrow(Registries.STRUCTURE).getOrThrow(key);
        final Structure structure = holder.value();

        StructureStartsOracle.Dimension dimension =
                StructureStartsOracle.makeDimension(dimensionName, seed, registries.lookup);
        ProtoChunk chunk = new ProtoChunk(new ChunkPos(chunkX, chunkZ), UpgradeData.EMPTY, dimension.height(),
                PalettedContainerFactory.create(registries.access), null);
        StructureTemplateManager templates = StructureStartsOracle.templateManager(registries);

        RandomSource random = new WorldgenRandom(new LegacyRandomSource(0L));
        ((WorldgenRandom) random).setLargeFeatureSeed(seed, chunkX, chunkZ);
        Structure.GenerationContext context = new Structure.GenerationContext(
                registries.worldGenAccess,
                dimension.generator(),
                dimension.generator().getBiomeSource(),
                dimension.randomState(),
                templates,
                seed,
                new ChunkPos(chunkX, chunkZ),
                dimension.height(),
                structure.biomes()::contains);

        Optional<Structure.GenerationStub> stub = structure.findValidGenerationPoint(context);
        StructureStart start = stub.map(s -> new StructureStart(
                        structure, new ChunkPos(chunkX, chunkZ), 0, s.getPiecesBuilder().build()))
                .orElse(null);

        JsonArray blocks = new JsonArray();
        boolean valid = start != null && start.isValid();
        if (valid) {
            int minY = dimension.height().getMinY();
            int maxY = dimension.height().getMaxY();
            FakeLevel level = FakeLevel.create(registries, minY, maxY, seed);
            BoundingBox startBox = start.getBoundingBox();
            int minChunkX = startBox.minX() >> 4;
            int maxChunkX = startBox.maxX() >> 4;
            int minChunkZ = startBox.minZ() >> 4;
            int maxChunkZ = startBox.maxZ() >> 4;
            int stageOrdinal = structure.step().ordinal();

            for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
                for (int cx = minChunkX; cx <= maxChunkX; cx++) {
                    WorldgenRandom worldgenrandom = new WorldgenRandom(new XoroshiroRandomSource(0L));
                    long decorSeed = worldgenrandom.setDecorationSeed(seed, cx * 16, cz * 16);
                    worldgenrandom.setFeatureSeed(decorSeed, 0, stageOrdinal);
                    BoundingBox writable = new BoundingBox(cx * 16, minY + 1, cz * 16, cx * 16 + 15, maxY, cz * 16 + 15);
                    start.placeInChunk(level, StructureStartsOracle.structureManager(
                                    registries, dimension, chunk, templates),
                            dimension.generator(), worldgenrandom, writable, new ChunkPos(cx, cz));
                }
            }

            List<Map.Entry<Long, BlockState>> entries = new ArrayList<>(level.blocks.entrySet());
            entries.sort(Comparator
                    .comparingInt((Map.Entry<Long, BlockState> e) -> BlockPos.getX(e.getKey()))
                    .thenComparingInt(e -> BlockPos.getY(e.getKey()))
                    .thenComparingInt(e -> BlockPos.getZ(e.getKey())));
            for (Map.Entry<Long, BlockState> entry : entries) {
                long packed = entry.getKey();
                JsonArray row = new JsonArray();
                row.add(BlockPos.getX(packed));
                row.add(BlockPos.getY(packed));
                row.add(BlockPos.getZ(packed));
                row.add(stateName(entry.getValue()));
                blocks.add(row);
            }
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("valid", valid);
        result.add("blocks", blocks);
        return result;
    }

    /** 状态的规范字符串：`minecraft:stone` 或 `minecraft:oak_log[axis=x]`（属性按名字排序）。 */
    private static String stateName(BlockState state) {
        StringBuilder sb = new StringBuilder(
                net.minecraft.core.registries.BuiltInRegistries.BLOCK.getKey(state.getBlock()).toString());
        var values = state.getValues();
        if (values.isEmpty()) {
            return sb.toString();
        }
        TreeMap<String, String> sorted = new TreeMap<>();
        for (var entry : values.entrySet()) {
            Property<?> property = entry.getKey();
            sorted.put(property.getName(), nameOf(property, entry.getValue()));
        }
        sb.append('[');
        boolean first = true;
        for (var entry : sorted.entrySet()) {
            if (!first) {
                sb.append(',');
            }
            first = false;
            sb.append(entry.getKey()).append('=').append(entry.getValue());
        }
        sb.append(']');
        return sb.toString();
    }

    private static <T extends Comparable<T>> String nameOf(Property<T> property, Comparable<?> value) {
        @SuppressWarnings("unchecked")
        T typed = (T) value;
        return property.getName(typed);
    }

    /**
     * 只记录写入的空世界。
     *
     * <p>{@link ServerLevel} 的构造函数会拉起真实区块系统，因此用 Unsafe 只分配对象，
     * 再覆盖结构放置会调用到的全部访问器。读取永远返回空气，写入进内存表。
     */
    private static final class FakeLevel extends ServerLevel {
        FakeLevel() {
            super(null, null, null, null, null, null, false, 0L, List.of(), false, null);
        }

        RegistryAccess access;
        PalettedContainerFactory factory;
        int minY;
        int maxY;
        long seed;
        Holder<Biome> biome;
        net.minecraft.world.attribute.EnvironmentAttributeSystem attributes;
        final net.minecraft.world.level.border.WorldBorder border =
                new net.minecraft.world.level.border.WorldBorder();
        // Unsafe 分配不会执行字段初始化器，这两个表必须在 create() 里显式建出来。
        Map<Long, BlockState> blocks;
        Map<Long, ProtoChunk> chunks;

        static FakeLevel create(DataPackRegistries registries, int minY, int maxY, long seed) {
            try {
                Field field = Class.forName("sun.misc.Unsafe").getDeclaredField("theUnsafe");
                field.setAccessible(true);
                Object unsafe = field.get(null);
                FakeLevel level = (FakeLevel)
                        unsafe.getClass().getMethod("allocateInstance", Class.class).invoke(unsafe, FakeLevel.class);
                var randomField = Level.class.getDeclaredField("random");
                long offset = (long) unsafe.getClass()
                        .getMethod("objectFieldOffset", Field.class)
                        .invoke(unsafe, randomField);
                unsafe.getClass()
                        .getMethod("putObject", Object.class, long.class, Object.class)
                        .invoke(unsafe, level, offset, RandomSource.create(0));
                level.access = registries.access;
                level.factory = PalettedContainerFactory.create(registries.access);
                level.minY = minY;
                level.maxY = maxY;
                level.seed = seed;
                level.biome = registries.access.lookupOrThrow(Registries.BIOME)
                        .getOrThrow(net.minecraft.world.level.biome.Biomes.PLAINS);
                level.attributes = net.minecraft.world.attribute.EnvironmentAttributeSystem.builder()
                        .addConstantLayer(net.minecraft.world.attribute.EnvironmentAttributeMap.builder().build())
                        .build();
                level.blocks = new HashMap<>();
                level.chunks = new HashMap<>();
                return level;
            } catch (ReflectiveOperationException error) {
                throw new IllegalStateException("StructureBlocksSingleOracle: cannot allocate fake level", error);
            }
        }

        @Override
        public RegistryAccess registryAccess() {
            return access;
        }

        @Override
        public BlockState getBlockState(BlockPos pos) {
            return blocks.getOrDefault(pos.asLong(), Blocks.AIR.defaultBlockState());
        }

        @Override
        public boolean setBlock(BlockPos pos, BlockState state, int flags) {
            blocks.put(pos.asLong(), state);
            return true;
        }

        @Override
        public net.minecraft.world.level.material.FluidState getFluidState(BlockPos pos) {
            return getBlockState(pos).getFluidState();
        }

        @Override
        public int getMinY() {
            return minY;
        }

        @Override
        public int getMaxY() {
            return maxY;
        }

        @Override
        public int getHeight() {
            return maxY - minY + 1;
        }

        @Override
        public int getHeight(Heightmap.Types type, int x, int z) {
            return minY;
        }

        @Override
        public Holder<Biome> getBiome(BlockPos pos) {
            // 空世界只提供一个固定的非阻挡群系：矿井的 `isInInvalidLocation` 依赖它，
            // 选 plains（不在 mineshaft_blocking 标签里）等价于「没有阻挡信息」。
            return biome;
        }

        @Override
        public long getSeed() {
            return seed;
        }

        @Override
        public net.minecraft.world.attribute.EnvironmentAttributeSystem environmentAttributes() {
            return attributes;
        }

        @Override
        public net.minecraft.world.level.border.WorldBorder getWorldBorder() {
            return border;
        }

        @Override
        public FeatureFlagSet enabledFeatures() {
            return FeatureFlags.REGISTRY.allFlags();
        }

        @Override
        public ChunkAccess getChunk(BlockPos pos) {
            // `StructurePiece.placeBlock` 对栅栏/火把等形状相关方块会调用
            // `level.getChunk(pos).markPosForPostprocessing(pos)`：必须返回 ChunkAccess，
            // 不能走 Level 的两参重载（它会把结果强转成 LevelChunk）。
            return getChunk(SectionPos.blockToSectionCoord(pos.getX()), SectionPos.blockToSectionCoord(pos.getZ()),
                    ChunkStatus.FULL, true);
        }

        @Override
        public ChunkAccess getChunk(int x, int z, ChunkStatus status, boolean load) {
            return chunks.computeIfAbsent(BlockPos.asLong(x, 0, z),
                    key -> new ProtoChunk(new ChunkPos(x, z), UpgradeData.EMPTY, this, factory, null));
        }

        @Override
        public void updateNeighborsAt(BlockPos pos, Block block) {
            // 空世界没有邻居更新器；模板放置会触发它，直接吞掉。
        }

        @Override
        public void updateNeighborsAt(BlockPos pos, Block block, net.minecraft.world.level.redstone.Orientation orientation) {
            // 同上。
        }

        @Override
        public void scheduleTick(BlockPos pos, Fluid fluid, int delay) {
            // 空世界没有刻调度器；结构放置阶段对液体只登记刻，不产生方块差异。
        }
    }
}