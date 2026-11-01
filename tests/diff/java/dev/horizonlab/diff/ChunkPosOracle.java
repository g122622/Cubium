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
 * {@code ChunkPos} 坐标编码与区域换算的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ChunkPosOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ChunkPos#asLong(int, int)} / {@link ChunkPos#getX(long)} /
 * {@link ChunkPos#getZ(long)} / {@link ChunkPos#minFromRegion(int, int)} /
 * {@link ChunkPos#maxFromRegion(int, int)} / {@link ChunkPos#hash(int, int)}，以及实例方法
 * {@code getMinBlockX} / {@code getMiddleBlockX} / {@code getMaxBlockX} 与其 Z 对应版本。
 *
 * <p>{@code ChunkPos.INVALID_CHUNK_POS} 是公开常量，直接读取。
 *
 * <p>单元表必须与 probes/chunk_pos.cpp 逐项一致。
 */
public final class ChunkPosOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "asLong|0|0",
                    "asLong|1|0",
                    "asLong|0|1",
                    "asLong|1|2",
                    "asLong|-1|-1",
                    "asLong|-1|0",
                    "asLong|0|-1",
                    "asLong|-5|-20",
                    "asLong|1875066|1875066",
                    "asLong|2147483647|-2147483648",
                    "decode|0",
                    "decode|1",
                    "decode|4294967296",
                    "decode|-1",
                    "roundtrip|1|2",
                    "roundtrip|-1|-1",
                    "roundtrip|-5|-20",
                    "roundtrip|2147483647|-2147483648",
                    "minRegion|0|0",
                    "maxRegion|0|0",
                    "minRegion|1|2",
                    "maxRegion|1|2",
                    "minRegion|-1|-1",
                    "maxRegion|-1|-1",
                    "hash|0|0",
                    "hash|0|1",
                    "hash|1|0",
                    "hash|1|2",
                    "hash|-1|-1",
                    "blocks|0|0",
                    "blocks|1|2",
                    "blocks|-1|-1",
                    "invalid");

    private ChunkPosOracle() {}

    public static void main(String[] args) throws IOException {
        // ChunkPos 的静态方法不触达注册表；但 `maxFromRegion` 会构造 `ChunkPos`，
        // 而 `ChunkPos` 的静态初始化块会访问 `BlockPos.MAX_HORIZONTAL_COORDINATE`
        // 与 `ChunkPyramid`，因此仍需先加载数据包注册表。
        DataPackRegistries.load();

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
                "usage: ChunkPosOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length < 1) {
            throw new IllegalArgumentException("chunk_pos: 空单元");
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "asLong" -> {
                long packed = ChunkPos.asLong(
                        Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
                out.addProperty("value", Long.toString(packed));
            }
            case "decode" -> {
                long packed = Long.parseLong(parts[1]);
                out.addProperty("value", ChunkPos.getX(packed) + "," + ChunkPos.getZ(packed));
            }
            case "roundtrip" -> {
                long packed = ChunkPos.asLong(
                        Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
                out.addProperty("value", ChunkPos.getX(packed) + "," + ChunkPos.getZ(packed));
            }
            case "minRegion" -> {
                ChunkPos pos = ChunkPos.minFromRegion(
                        Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
                out.addProperty("value", pos.x + "," + pos.z);
            }
            case "maxRegion" -> {
                ChunkPos pos = ChunkPos.maxFromRegion(
                        Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
                out.addProperty("value", pos.x + "," + pos.z);
            }
            case "hash" -> out.addProperty("value",
                    ChunkPos.hash(Integer.parseInt(parts[1]), Integer.parseInt(parts[2])));
            case "blocks" -> {
                ChunkPos pos = new ChunkPos(
                        Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
                out.addProperty("value",
                        pos.getMinBlockX() + "," + pos.getMiddleBlockX() + ","
                                + pos.getMaxBlockX() + "," + pos.getMinBlockZ() + ","
                                + pos.getMiddleBlockZ() + "," + pos.getMaxBlockZ());
            }
            case "invalid" -> out.addProperty("value", Long.toString(ChunkPos.INVALID_CHUNK_POS));
            default -> throw new IllegalArgumentException("chunk_pos: 无法识别的单元: " + unit);
        }

        return out;
    }
}