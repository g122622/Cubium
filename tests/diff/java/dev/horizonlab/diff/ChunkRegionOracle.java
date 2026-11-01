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
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.world.level.ChunkPos;

/**
 * {@code ChunkPos} 区域换算的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ChunkRegionOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ChunkPos#getRegionX()} / {@link ChunkPos#getRegionZ()} /
 * {@link ChunkPos#getRegionLocalX()} / {@link ChunkPos#getRegionLocalZ()} /
 * {@link ChunkPos#minFromRegion(int, int)} / {@link ChunkPos#maxFromRegion(int, int)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>{@code isValid} **不在本能力范围内**——其阈值依赖 {@code ChunkPyramid} 的依赖链长度。
 *
 * <p>单元表必须与 probes/chunk_region.cpp 逐项一致。
 */
public final class ChunkRegionOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "regionX|0",
                    "regionX|31",
                    "regionX|32",
                    "regionX|63",
                    "regionX|64",
                    "regionX|-1",
                    "regionX|-32",
                    "regionX|-33",
                    "regionX|-64",
                    "regionZ|32",
                    "regionZ|-33",
                    "localX|0",
                    "localX|31",
                    "localX|32",
                    "localX|-1",
                    "localX|-32",
                    "localX|-33",
                    "localZ|-1",
                    "localZ|33",
                    "minFromRegion|0|0",
                    "minFromRegion|1|-1",
                    "minFromRegion|-1|-2",
                    "maxFromRegion|0|0",
                    "maxFromRegion|1|-1",
                    "maxFromRegion|-1|-2");

    private ChunkRegionOracle() {}

    public static void main(String[] args) throws IOException {
        // ChunkPos.<clinit> 会经 ChunkPyramid → ChunkStatus → BuiltInRegistries，
        // 而注册表在 Bootstrap.bootStrap() 之前一碰就抛 "Not bootstrapped"；
        // bootStrap 本身又要求先设置游戏版本。
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: ChunkRegionOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("chunk_region: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "regionX" -> out.addProperty("value",
                    new ChunkPos(Integer.parseInt(parts[1]), 0).getRegionX());
            case "regionZ" -> out.addProperty("value",
                    new ChunkPos(0, Integer.parseInt(parts[1])).getRegionZ());
            case "localX" -> out.addProperty("value",
                    new ChunkPos(Integer.parseInt(parts[1]), 0).getRegionLocalX());
            case "localZ" -> out.addProperty("value",
                    new ChunkPos(0, Integer.parseInt(parts[1])).getRegionLocalZ());
            case "minFromRegion" -> {
                ChunkPos pos = ChunkPos.minFromRegion(
                        Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
                out.addProperty("value", pos.x + "," + pos.z);
            }
            case "maxFromRegion" -> {
                ChunkPos pos = ChunkPos.maxFromRegion(
                        Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
                out.addProperty("value", pos.x + "," + pos.z);
            }
            default -> throw new IllegalArgumentException(
                    "chunk_region: 无法识别的单元: " + unit);
        }

        return out;
    }
}