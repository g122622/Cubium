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
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Holder;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.ResourceKey;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.chunk.PalettedContainerFactory;
import net.minecraft.world.level.chunk.ProtoChunk;
import net.minecraft.world.level.chunk.UpgradeData;
import net.minecraft.world.level.levelgen.Beardifier;
import net.minecraft.world.level.levelgen.DensityFunction;
import net.minecraft.world.level.levelgen.RandomState;
import net.minecraft.world.level.levelgen.WorldgenRandom;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.PoolElementStructurePiece;
import net.minecraft.world.level.levelgen.structure.Structure;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.levelgen.structure.StructureStart;
import net.minecraft.world.level.levelgen.structure.TerrainAdjustment;
import net.minecraft.world.level.levelgen.structure.pools.JigsawJunction;
import net.minecraft.world.level.levelgen.structure.pools.StructureTemplatePool;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplateManager;

/**
 * 结构地形平滑（Beardifier）在结构周围的密度贡献。
 *
 * <pre>
 *   java dev.horizonlab.diff.BeardifierDensityOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 输入一行一个样本：维度名、世界种子、区块坐标、结构 id。输出该区块上这一个结构的
 * 起点、参与计算的刚性片段与连接点数量、受影响包围盒，以及派生采样网格上的密度值。
 *
 * <p>片段与连接点的筛选逐字复刻 {@link Beardifier#forStructuresInChunk}：只有与区块
 * 距离不超过核半径（12）的片段参与；地形匹配投影的池元素不贡献刚性体积（但贡献连接点）；
 * 非池元素的片段地面高度偏移恒为 0。唯一区别是这里只处理点名的那一个结构起点，
 * 与探针侧只生成一个结构保持一致。
 */
public final class BeardifierDensityOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 采样网格尺寸，与探针侧同名常量一致。 */
    private static final int GRID_X = 9;

    private static final int GRID_Y = 7;
    private static final int GRID_Z = 9;

    /** 采样范围在起点包围盒外扩的格数。 */
    private static final int SAMPLE_MARGIN = 24;

    private static final int KERNEL_RADIUS = 12;

    private BeardifierDensityOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: BeardifierDensityOracle run <input> <output>");
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
                Registries.STRUCTURE,
                net.minecraft.resources.Identifier.parse(input.get("structure").getAsString()));
        final Holder<Structure> holder = registries.worldGenAccess.lookupOrThrow(Registries.STRUCTURE).getOrThrow(key);
        final Structure structure = holder.value();

        StructureStartsOracle.Dimension dimension = StructureStartsOracle.makeDimension(dimensionName, seed, registries.lookup);
        ProtoChunk chunk = new ProtoChunk(
                new ChunkPos(chunkX, chunkZ),
                UpgradeData.EMPTY,
                dimension.height(),
                PalettedContainerFactory.create(registries.access),
                null);
        StructureTemplateManager templates = StructureStartsOracle.templateManager(registries);

        RandomSource random = new WorldgenRandom(new net.minecraft.world.level.levelgen.LegacyRandomSource(0L));
        ((WorldgenRandom) random).setLargeFeatureSeed(seed, chunkX, chunkZ);
        RandomState randomState = dimension.randomState();
        Structure.GenerationContext context = new Structure.GenerationContext(
                registries.worldGenAccess,
                dimension.generator(),
                dimension.generator().getBiomeSource(),
                randomState,
                templates,
                seed,
                new ChunkPos(chunkX, chunkZ),
                dimension.height(),
                structure.biomes()::contains);

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        Optional<Structure.GenerationStub> stub = structure.findValidGenerationPoint(context);
        if (stub.isEmpty()) {
            result.addProperty("valid", false);
            result.add("affected", JsonNull.INSTANCE);
            result.add("values", new JsonArray());
            return result;
        }
        StructureStart start = new StructureStart(
                structure, new ChunkPos(chunkX, chunkZ), 0, stub.get().getPiecesBuilder().build());
        if (!start.isValid()) {
            result.addProperty("valid", false);
            result.add("affected", JsonNull.INSTANCE);
            result.add("values", new JsonArray());
            return result;
        }
        result.addProperty("valid", true);

        // ===== 参与计算的片段与连接点（复刻 Beardifier.forStructuresInChunk 的筛选） =====
        List<Beardifier.Rigid> rigids = new ArrayList<>();
        List<JigsawJunction> junctions = new ArrayList<>();
        BoundingBox union = null;
        ChunkPos chunkPos = new ChunkPos(chunkX, chunkZ);
        int chunkMinX = chunkPos.getMinBlockX();
        int chunkMinZ = chunkPos.getMinBlockZ();
        TerrainAdjustment adaptation = structure.terrainAdaptation();
        if (adaptation != TerrainAdjustment.NONE) {
            for (StructurePiece piece : start.getPieces()) {
                if (!piece.isCloseToChunk(chunkPos, KERNEL_RADIUS)) {
                    continue;
                }
                if (piece instanceof PoolElementStructurePiece poolPiece) {
                    if (poolPiece.getElement().getProjection() == StructureTemplatePool.Projection.RIGID) {
                        rigids.add(new Beardifier.Rigid(
                                poolPiece.getBoundingBox(), adaptation, poolPiece.getGroundLevelDelta()));
                        union = include(union, piece.getBoundingBox());
                    }
                    for (JigsawJunction junction : poolPiece.getJunctions()) {
                        int junctionX = junction.getSourceX();
                        int junctionZ = junction.getSourceZ();
                        if (junctionX > chunkMinX - KERNEL_RADIUS
                                && junctionZ > chunkMinZ - KERNEL_RADIUS
                                && junctionX < chunkMinX + 15 + KERNEL_RADIUS
                                && junctionZ < chunkMinZ + 15 + KERNEL_RADIUS) {
                            junctions.add(junction);
                            union = include(
                                    union,
                                    new BoundingBox(new BlockPos(
                                            junctionX, junction.getSourceGroundY(), junctionZ)));
                        }
                    }
                } else {
                    rigids.add(new Beardifier.Rigid(piece.getBoundingBox(), adaptation, 0));
                    union = include(union, piece.getBoundingBox());
                }
            }
        }

        if (union == null) {
            result.add("affected", JsonNull.INSTANCE);
        } else {
            BoundingBox inflated = union.inflatedBy(24);
            JsonArray affected = new JsonArray();
            affected.add(inflated.minX());
            affected.add(inflated.minY());
            affected.add(inflated.minZ());
            affected.add(inflated.maxX());
            affected.add(inflated.maxY());
            affected.add(inflated.maxZ());
            result.add("affected", affected);
        }
        result.addProperty("rigidCount", rigids.size());
        result.addProperty("junctionCount", junctions.size());

        // 参与计算的输入原样输出：密度值对不上时，先分清是"输入不同"还是"算法不同"。
        JsonArray rigidJson = new JsonArray();
        for (Beardifier.Rigid rigid : rigids) {
            JsonObject entry = new JsonObject();
            JsonArray rigidBox = new JsonArray();
            rigidBox.add(rigid.box().minX());
            rigidBox.add(rigid.box().minY());
            rigidBox.add(rigid.box().minZ());
            rigidBox.add(rigid.box().maxX());
            rigidBox.add(rigid.box().maxY());
            rigidBox.add(rigid.box().maxZ());
            entry.add("box", rigidBox);
            entry.addProperty("adaptation", rigid.terrainAdjustment().ordinal());
            entry.addProperty("delta", rigid.groundLevelDelta());
            rigidJson.add(entry);
        }
        result.add("rigid", rigidJson);
        JsonArray junctionJson = new JsonArray();
        for (JigsawJunction junction : junctions) {
            JsonObject entry = new JsonObject();
            entry.addProperty("x", junction.getSourceX());
            entry.addProperty("y", junction.getSourceGroundY());
            entry.addProperty("z", junction.getSourceZ());
            entry.addProperty("delta", junction.getDeltaY());
            junctionJson.add(entry);
        }
        result.add("junctions", junctionJson);

        Beardifier beardifier = new Beardifier(
                List.copyOf(rigids), List.copyOf(junctions), union == null ? null : union.inflatedBy(24));

        BoundingBox startBox = start.getBoundingBox();
        JsonArray startBoxJson = new JsonArray();
        startBoxJson.add(startBox.minX());
        startBoxJson.add(startBox.minY());
        startBoxJson.add(startBox.minZ());
        startBoxJson.add(startBox.maxX());
        startBoxJson.add(startBox.maxY());
        startBoxJson.add(startBox.maxZ());
        result.add("startBox", startBoxJson);
        JsonArray values = new JsonArray();
        for (int iy = 0; iy < GRID_Y; iy++) {
            final int y = sampleCoordinate(iy, GRID_Y, startBox.minY() - SAMPLE_MARGIN, startBox.maxY() + SAMPLE_MARGIN);
            for (int iz = 0; iz < GRID_Z; iz++) {
                final int z =
                        sampleCoordinate(iz, GRID_Z, startBox.minZ() - SAMPLE_MARGIN, startBox.maxZ() + SAMPLE_MARGIN);
                for (int ix = 0; ix < GRID_X; ix++) {
                    final int x =
                            sampleCoordinate(ix, GRID_X, startBox.minX() - SAMPLE_MARGIN, startBox.maxX() + SAMPLE_MARGIN);
                    values.add(beardifier.compute(new DensityFunction.FunctionContext() {
                        @Override
                        public int blockX() {
                            return x;
                        }

                        @Override
                        public int blockY() {
                            return y;
                        }

                        @Override
                        public int blockZ() {
                            return z;
                        }
                    }));
                }
            }
        }
        result.add("values", values);
        return result;
    }

    private static BoundingBox include(BoundingBox current, BoundingBox addition) {
        return current == null ? addition : BoundingBox.encapsulating(current, addition);
    }

    /** 把网格的线性序号映射到 [min, max] 上的整数坐标（含端点），与探针侧同式。 */
    private static int sampleCoordinate(int index, int count, int min, int max) {
        long span = (long) max - (long) min;
        return (int) ((long) min + ((long) index * span) / (count - 1));
    }
}