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
import net.minecraft.core.Holder;
import net.minecraft.core.QuartPos;
import net.minecraft.world.level.biome.Biome;

/**
 * 生物群系源在三维网格上的采样结果。
 *
 * <pre>
 *   java dev.horizonlab.diff.BiomeSourceSampleOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 输入一行一个样本：维度名、世界种子、区块坐标。网格与探针侧同式：水平方向覆盖区块内的
 * 四个 quart 单元，竖直方向取固定的 quart 层。输出群系资源名，两侧逐点比对。
 */
public final class BiomeSourceSampleOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与探针侧同名常量一致。 */
    private static final int QUARTS_PER_AXIS = 4;

    private static final int BLOCKS_PER_QUART = 4;
    private static final int[] SAMPLE_QUART_LEVELS = {-16, 0, 16, 32, 64};

    private BiomeSourceSampleOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: BiomeSourceSampleOracle run <input> <output>");
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

        StructureStartsOracle.Dimension dimension = StructureStartsOracle.makeDimension(dimensionName, seed, registries.lookup);

        JsonArray biomes = new JsonArray();
        for (int quartY : SAMPLE_QUART_LEVELS) {
            for (int dz = 0; dz < QUARTS_PER_AXIS; dz++) {
                for (int dx = 0; dx < QUARTS_PER_AXIS; dx++) {
                    int blockX = chunkX * 16 + dx * BLOCKS_PER_QUART;
                    int blockZ = chunkZ * 16 + dz * BLOCKS_PER_QUART;
                    Holder<Biome> biome = dimension.generator()
                            .getBiomeSource()
                            .getNoiseBiome(
                                    QuartPos.fromBlock(blockX), quartY, QuartPos.fromBlock(blockZ), dimension.randomState().sampler());
                    biomes.add(biome.unwrapKey()
                            .map(key -> key.identifier().toString())
                            .orElse(""));
                }
            }
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.add("biomes", biomes);
        return result;
    }
}