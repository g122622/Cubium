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
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.core.BlockPos;

/**
 * {@code Vec3i} / {@code BlockPos} 距离与包围盒迭代的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockPosDistOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link BlockPos#distManhattan(net.minecraft.core.Vec3i)} /
 * {@link BlockPos#distSqr(net.minecraft.core.Vec3i)} /
 * {@link BlockPos#betweenClosed(int, int, int, int, int, int)}，不重写公式。
 *
 * <p>单元表必须与 probes/blockpos_dist.cpp 逐项一致。
 */
public final class BlockPosDistOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "manhattan|0|0|0|1|2|3",
                    "manhattan|0|0|0|0|0|0",
                    "manhattan|10|20|30|10|20|30",
                    "manhattan|-5|-5|-5|5|5|5",
                    "manhattan|0|0|0|100|0|0",
                    "manhattan|0|0|0|0|100|0",
                    "manhattan|0|0|0|0|0|100",
                    "manhattan|0|0|0|10000000|0|0",
                    "manhattan|0|0|0|16777216|0|0",
                    "manhattan|0|0|0|20000000|0|0",
                    "manhattan|-10000000|0|0|10000000|0|0",
                    "sqr|0|0|0|3|4|0",
                    "sqr|0|0|0|0|0|0",
                    "sqr|1|2|3|4|6|3",
                    "sqr|-5|-5|-5|5|5|5",
                    "sqr|0|0|0|10000000|0|0",
                    "box|0|0|0|2|1|0",
                    "box|0|0|0|0|0|2",
                    "box|0|0|0|2|0|0",
                    "box|0|0|0|1|1|1",
                    "box|2|1|0|0|0|0",
                    "box|-1|-1|-1|0|0|0",
                    "boxsize|0|0|0|2|1|0",
                    "boxsize|0|0|0|0|0|0",
                    "boxsize|0|0|0|1|1|1",
                    "boxsize|-5|-5|-5|5|5|5");

    private BlockPosDistOracle() {}

    public static void main(String[] args) throws IOException {
        // `BlockPos` / `ChunkPos` 的构造可能触发注册表访问，先加载数据包注册表。
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
                "usage: BlockPosDistOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 7) {
            throw new IllegalArgumentException("blockpos_dist: 单元应为 7 段: " + unit);
        }

        String mode = parts[0];
        BlockPos a = new BlockPos(
                Integer.parseInt(parts[1]),
                Integer.parseInt(parts[2]),
                Integer.parseInt(parts[3]));
        BlockPos b = new BlockPos(
                Integer.parseInt(parts[4]),
                Integer.parseInt(parts[5]),
                Integer.parseInt(parts[6]));

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        switch (mode) {
            case "manhattan" -> result.addProperty("value", a.distManhattan(b));
            case "sqr" -> result.addProperty("value", formatNumber(a.distSqr(b)));
            case "boxsize" -> {
                long volume = (long) (Math.abs(b.getX() - a.getX()) + 1)
                        * (Math.abs(b.getY() - a.getY()) + 1)
                        * (Math.abs(b.getZ() - a.getZ()) + 1);
                result.addProperty("volume", Long.toString(volume));
            }
            case "box" -> {
                JsonArray order = new JsonArray();
                int emitted = 0;
                for (BlockPos pos : BlockPos.betweenClosed(a, b)) {
                    if (emitted >= 8) {
                        break;
                    }
                    order.add(new JsonArray());
                    JsonArray entry = order.get(order.size() - 1).getAsJsonArray();
                    entry.add(pos.getX());
                    entry.add(pos.getY());
                    entry.add(pos.getZ());
                    emitted++;
                }
                long volume = (long) (Math.abs(b.getX() - a.getX()) + 1)
                        * (Math.abs(b.getY() - a.getY()) + 1)
                        * (Math.abs(b.getZ() - a.getZ()) + 1);
                result.add("order", order);
                result.addProperty("volume", Long.toString(volume));
            }
            default -> throw new IllegalArgumentException("blockpos_dist: 无法识别的单元: " + unit);
        }

        return result;
    }

    private static String formatNumber(double value) {
        String text = String.format("%.9g", value);

        // 指数形式也要裁掉尾零，否则 `1e+14` 与 `1.00000000e+14` 会被当成不同。
        int expPos = -1;
        for (int i = 0; i < text.length(); i++) {
            char ch = text.charAt(i);
            if (ch == 'e' || ch == 'E') {
                expPos = i;
                break;
            }
        }
        String mantissa = expPos < 0 ? text : text.substring(0, expPos);
        String suffix = expPos < 0 ? "" : text.substring(expPos);
        if (mantissa.indexOf('.') >= 0) {
            while (mantissa.endsWith("0")) {
                mantissa = mantissa.substring(0, mantissa.length() - 1);
            }
            if (mantissa.endsWith(".")) {
                mantissa = mantissa.substring(0, mantissa.length() - 1);
            }
        }
        return mantissa + suffix;
    }
}