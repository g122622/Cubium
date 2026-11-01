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
import net.minecraft.core.BlockPos;

/**
 * {@code BlockPos} 位打包与坐标运算的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockPosCodecOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link BlockPos#asLong(int, int, int)} / {@link BlockPos#getX(long)} /
 * {@link BlockPos#getY(long)} / {@link BlockPos#getZ(long)} / {@link BlockPos#offset(int, int, int)}，
 * 不重写公式。
 *
 * <p>打包结果以**字符串**输出：Java 的 long 超出 JavaScript 的安全整数范围（2^53），
 * 用 JSON number 传递会在 harness 侧退化为 double 而丢失低位。
 *
 * <p>单元表必须与 probes/blockpos_codec.cpp 逐项一致。
 */
public final class BlockPosCodecOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "pack|0|0|0",
                    "pack|1|2|3",
                    "pack|100|64|200",
                    "pack|-1|-1|-1",
                    "pack|-100|-64|-200",
                    "pack|-1|0|0",
                    "pack|0|-1|0",
                    "pack|0|0|-1",
                    "pack|0|2047|0",
                    "pack|0|-2048|0",
                    "pack|0|2048|0",
                    "pack|0|-2049|0",
                    "pack|33554431|0|0",
                    "pack|-33554432|0|0",
                    "pack|33554432|0|0",
                    "pack|-33554433|0|0",
                    "pack|-12345|-100|-54321",
                    "offset|1|2|3|1|1|1",
                    "offset|1|2|3|-1|-2|-3",
                    "offset|0|0|0|0|0|0",
                    "offset|-5|-5|-5|5|5|5");

    private BlockPosCodecOracle() {}

    public static void main(String[] args) throws IOException {
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
                "usage: BlockPosCodecOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        String[] parts = unit.split("\\|");
        if (parts.length == 0) {
            throw new IllegalArgumentException("blockpos_codec: 空单元");
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (parts[0].equals("pack") && parts.length == 4) {
            int x = Integer.parseInt(parts[1]);
            int y = Integer.parseInt(parts[2]);
            int z = Integer.parseInt(parts[3]);
            long packed = BlockPos.asLong(x, y, z);
            result.addProperty("packed", Long.toString(packed));
            result.addProperty("x", BlockPos.getX(packed));
            result.addProperty("y", BlockPos.getY(packed));
            result.addProperty("z", BlockPos.getZ(packed));
            return result;
        }

        if (parts[0].equals("offset") && parts.length == 7) {
            BlockPos origin = new BlockPos(
                    Integer.parseInt(parts[1]),
                    Integer.parseInt(parts[2]),
                    Integer.parseInt(parts[3]));
            BlockPos moved = origin.offset(
                    Integer.parseInt(parts[4]),
                    Integer.parseInt(parts[5]),
                    Integer.parseInt(parts[6]));
            result.addProperty("x", moved.getX());
            result.addProperty("y", moved.getY());
            result.addProperty("z", moved.getZ());
            return result;
        }

        throw new IllegalArgumentException("blockpos_codec: 无法识别的单元: " + unit);
    }
}