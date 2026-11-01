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
 * {@code BlockPos.betweenClosed} 迭代顺序的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockPosIterateOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link BlockPos#betweenClosed(int, int, int, int, int, int)} 与
 * {@link BlockPos#betweenClosed(BlockPos, BlockPos)}，把迭代结果逐个输出。
 *
 * <p>本能力不需要注册表。
 *
 * <p>输出格式：`x,y,z` 用 `;` 连接；空结果输出空串。
 *
 * <p>单元表必须与 probes/blockpos_iterate.cpp 逐项一致。
 */
public final class BlockPosIterateOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "iter|0|0|0|0|0|0",
                    "iter|0|0|0|2|0|0",
                    "iter|0|0|0|0|2|0",
                    "iter|0|0|0|0|0|2",
                    "iter|0|0|0|2|1|1",
                    "iter|0|0|0|1|1|1",
                    "iter|-1|-1|-1|0|0|0",
                    "pair|0|0|0|2|1|1",
                    "pair|2|1|1|0|0|0",
                    "pair|5|5|5|5|5|5");

    private BlockPosIterateOracle() {}

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
        System.err.println("usage: BlockPosIterateOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
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

    /** 把可迭代对象渲染成 `x,y,z;x,y,z;...`。 */
    private static String render(Iterable<BlockPos> positions) {
        StringBuilder out = new StringBuilder();
        for (BlockPos pos : positions) {
            if (out.length() > 0) {
                out.append(';');
            }
            out.append(pos.getX()).append(',').append(pos.getY()).append(',').append(pos.getZ());
        }
        return out.toString();
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 7) {
            throw new IllegalArgumentException("blockpos_iterate: 单元需要 7 段: " + unit);
        }

        int[] a = new int[3];
        int[] b = new int[3];
        for (int i = 0; i < 3; i++) {
            a[i] = Integer.parseInt(parts[1 + i]);
            b[i] = Integer.parseInt(parts[4 + i]);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "iter" -> out.addProperty("value", render(
                    BlockPos.betweenClosed(a[0], a[1], a[2], b[0], b[1], b[2])));
            case "pair" -> out.addProperty("value", render(BlockPos.betweenClosed(
                    new BlockPos(a[0], a[1], a[2]), new BlockPos(b[0], b[1], b[2]))));
            default -> throw new IllegalArgumentException(
                    "blockpos_iterate: 无法识别的单元: " + unit);
        }

        return out;
    }
}