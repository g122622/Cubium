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
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Optional;
import net.minecraft.core.Holder;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.ResourceKey;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.biome.Biome;
import net.minecraft.world.level.chunk.PalettedContainerFactory;
import net.minecraft.world.level.chunk.ProtoChunk;
import net.minecraft.world.level.chunk.UpgradeData;
import net.minecraft.world.level.levelgen.RandomState;
import net.minecraft.world.level.levelgen.WorldgenRandom;
import net.minecraft.world.level.levelgen.structure.BoundingBox;
import net.minecraft.world.level.levelgen.structure.Structure;
import net.minecraft.world.level.levelgen.structure.StructurePiece;
import net.minecraft.world.level.levelgen.structure.StructureStart;
import net.minecraft.world.level.levelgen.structure.templatesystem.StructureTemplateManager;

/**
 * 单个结构在指定区块上的原版起点。
 *
 * <pre>
 *   java dev.horizonlab.diff.StructureStartSingleOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 输入一行一个样本：维度名、世界种子、区块坐标、结构 id。输出该区块上**这一个结构**单独
 * 生成的起点：有效性、各构件的包围盒与旋转（旋转按原版枚举序号输出，与探针侧的
 * `static_cast&lt;i32&gt;` 同口径）。
 *
 * <p>随机源与生产一致：{@code new WorldgenRandom(new LegacyRandomSource(0L))} 之后
 * {@code setLargeFeatureSeed(seed, chunkX, chunkZ)}，即每格从同一个初始状态起算。
 * 这条能力绕开结构集，专门用于把「起点位置错」与「结构集选择错」分开。
 */
public final class StructureStartSingleOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private StructureStartSingleOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: StructureStartSingleOracle run <input> <output>");
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
        final ResourceKey<Structure> key =
                ResourceKey.create(Registries.STRUCTURE, net.minecraft.resources.Identifier.parse(input.get("structure").getAsString()));
        final Holder<Structure> holder = registries.worldGenAccess.lookupOrThrow(Registries.STRUCTURE).getOrThrow(key);
        final Structure structure = holder.value();

        StructureStartsOracle.Dimension dimension = StructureStartsOracle.makeDimension(dimensionName, seed, registries.lookup);
        ProtoChunk chunk = new ProtoChunk(new ChunkPos(chunkX, chunkZ), UpgradeData.EMPTY, dimension.height(),
                PalettedContainerFactory.create(registries.access), null);
        StructureTemplateManager templates = StructureStartsOracle.templateManager(registries);

        // 与生产同源：每格新建实例并从同一个初始状态起算。
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
                // 生产路径（ChunkGenerator.tryGenerateStructure）传的是 `structure.biomes()::contains`，
                // 即结构定义 JSON 的 biomes 标签。传 `biome -> true` 会把群系校验整个摘掉，
                // 让 oracle 在「放置规则选中但群系不符」的候选区块上产出起点，而探针侧按标签拒绝，
                // 差分就退化成两侧谓词不同造成的假差异。
                structure.biomes()::contains);

        if (System.getenv("MC_DIFF_DIAG_BIOME") != null) {
            try {
                java.lang.reflect.Method m =
                        Structure.class.getDeclaredMethod("findGenerationPoint", Structure.GenerationContext.class);
                m.setAccessible(true);
                @SuppressWarnings("unchecked")
                Optional<Structure.GenerationStub> raw = (Optional<Structure.GenerationStub>) m.invoke(structure, context);
                if (raw.isPresent()) {
                    net.minecraft.core.BlockPos p = raw.get().position();
                    Holder<Biome> b = dimension.generator().getBiomeSource().getNoiseBiome(
                            net.minecraft.core.QuartPos.fromBlock(p.getX()),
                            net.minecraft.core.QuartPos.fromBlock(p.getY()),
                            net.minecraft.core.QuartPos.fromBlock(p.getZ()),
                            randomState.sampler());
                    System.err.println("[diag] " + unit + " stub=" + p
                            + " biome=" + b.unwrapKey().map(Object::toString).orElse("?")
                            + " contains=" + structure.biomes().contains(b)
                            + " tag=" + structure.biomes().unwrap().toString());
                }
            } catch (ReflectiveOperationException error) {
                throw new IllegalStateException(error);
            }
        }
        Optional<Structure.GenerationStub> stub = structure.findValidGenerationPoint(context);
        JsonArray pieces = new JsonArray();
        boolean valid = false;
        if (stub.isPresent()) {
            StructureStart start = new StructureStart(structure, new ChunkPos(chunkX, chunkZ), 0, stub.get().getPiecesBuilder().build());
            valid = start.isValid();
            if (valid) {
                for (StructurePiece piece : start.getPieces()) {
                    BoundingBox box = piece.getBoundingBox();
                    JsonObject entry = new JsonObject();
                    JsonArray array = new JsonArray();
                    array.add(box.minX());
                    array.add(box.minY());
                    array.add(box.minZ());
                    array.add(box.maxX());
                    array.add(box.maxY());
                    array.add(box.maxZ());
                    entry.add("box", array);
                    entry.addProperty("rotation", piece.getRotation() == null ? 0 : piece.getRotation().ordinal());
                    entry.addProperty("template", templateNameOf(piece));
                    pieces.add(entry);
                }
            }
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("valid", valid);
        result.add("pieces", pieces);
        return result;
    }

    /**
     * 取构件对应的模板标识（仅拼图构件有），用于把"同一尺寸但选错模板/旋转"的差异暴露出来。
     *
     * 原版 `SinglePoolElement.template` 是 `Either<ResourceLocation, StructureTemplate>`，
     * 数据包加载的拼图元素一律是左侧的 ResourceLocation；地物池元素没有模板，返回占位名。
     */
    private static String templateNameOf(StructurePiece piece) {
        if (!(piece instanceof net.minecraft.world.level.levelgen.structure.PoolElementStructurePiece poolPiece)) {
            return "";
        }
        net.minecraft.world.level.levelgen.structure.pools.StructurePoolElement element = poolPiece.getElement();
        if (element == null) {
            return "";
        }
        for (Class<?> type = element.getClass(); type != null; type = type.getSuperclass()) {
            try {
                java.lang.reflect.Field field = type.getDeclaredField("template");
                field.setAccessible(true);
                Object value = field.get(element);
                if (value instanceof com.mojang.datafixers.util.Either<?, ?> either) {
                    return either.left().map(Object::toString).orElse("");
                }
                return "";
            } catch (NoSuchFieldException ignored) {
                // 继续向父类查找
            } catch (IllegalAccessException error) {
                throw new IllegalStateException("StructureStartSingleOracle: cannot read template", error);
            }
        }
        // 列表/地物/空池元素没有模板文件：返回池元素类型的注册路径（与探针侧的
        // `JigsawPiece::getTypeName()` 同口径），否则这些构件在差分里只能是空串。
        var key = BuiltInRegistries.STRUCTURE_POOL_ELEMENT.getKey(element.getType());
        return key == null ? element.getClass().getSimpleName() : key.getPath();
    }
}