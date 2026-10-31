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
import java.util.ArrayList;
import java.util.List;
import net.minecraft.util.Mth;

/**
 * 爆炸可见度采样网格的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ExplosionSeenGridOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>复刻 {@code ServerExplosion.getSeenPercent} 里网格生成那一半：三个步长、两个偏移量、
 * 三重循环给出的采样点序列与总数。射线检测那一半需要世界，本不动。
 *
 * <p>盒子表必须与 probes/explosion_seen_grid.cpp 逐项一致。
 */
public final class ExplosionSeenGridOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 覆盖用的碰撞箱尺寸表，与 C++ 侧 BOXES 逐项一致。 */
    private static final double[][] BOXES = {
        {0.0, 0.0, 0.0, 0.6, 1.8, 0.6},
        {0.0, 0.0, 0.0, 0.3, 0.9, 0.3},
        {0.0, 0.0, 0.0, 1.0, 1.0, 1.0},
        {0.0, 0.0, 0.0, 0.6, 1.7, 0.6},
        {0.0, 0.0, 0.0, 0.6, 2.9, 0.6},
        {0.0, 0.0, 0.0, 1.4, 2.7, 1.4},
        {0.0, 0.0, 0.0, 0.15, 0.15, 0.15},
        {0.0, 0.0, 0.0, 2.5, 2.5, 2.5},
        {0.0, 0.0, 0.0, 0.7, 1.9, 1.1},
        {-0.25, 1.5, -0.75, 0.35, 3.3, -0.15}
    };

    private ExplosionSeenGridOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (double[] box : BOXES) {
                text.append(formatFloat(box[0])).append('|')
                    .append(formatFloat(box[1])).append('|')
                    .append(formatFloat(box[2])).append('|')
                    .append(formatFloat(box[3])).append('|')
                    .append(formatFloat(box[4])).append('|')
                    .append(formatFloat(box[5])).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: ExplosionSeenGridOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            var obj = JsonParser.parseString(line).getAsJsonObject();
            text.append(GSON.toJson(summarise(obj))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(JsonObject input) {
        double minX = input.get("minX").getAsDouble();
        double minY = input.get("minY").getAsDouble();
        double minZ = input.get("minZ").getAsDouble();
        double maxX = input.get("maxX").getAsDouble();
        double maxY = input.get("maxY").getAsDouble();
        double maxZ = input.get("maxZ").getAsDouble();

        JsonObject result = new JsonObject();
        result.addProperty("unit", input.get("maxY").getAsString());
        result.addProperty("minX", input.get("minX").getAsString());
        result.addProperty("minY", input.get("minY").getAsString());
        result.addProperty("minZ", input.get("minZ").getAsString());
        result.addProperty("maxX", input.get("maxX").getAsString());
        result.addProperty("maxY", input.get("maxY").getAsString());
        result.addProperty("maxZ", input.get("maxZ").getAsString());

        // 原版全部用 double。
        double stepX = 1.0 / ((maxX - minX) * 2.0 + 1.0);
        double stepY = 1.0 / ((maxY - minY) * 2.0 + 1.0);
        double stepZ = 1.0 / ((maxZ - minZ) * 2.0 + 1.0);

        double offsetX = (1.0 - Math.floor(1.0 / stepX) * stepX) / 2.0;
        double offsetZ = (1.0 - Math.floor(1.0 / stepZ) * stepZ) / 2.0;

        if (stepX < 0.0 || stepY < 0.0 || stepZ < 0.0) {
            result.addProperty("degenerate", true);
            result.addProperty("sampleCount", 0);
            return result;
        }

        List<double[]> samples = new ArrayList<>();
        for (double fx = 0.0; fx <= 1.0; fx += stepX) {
            for (double fy = 0.0; fy <= 1.0; fy += stepY) {
                for (double fz = 0.0; fz <= 1.0; fz += stepZ) {
                    double sx = Mth.lerp(fx, minX, maxX) + offsetX;
                    double sy = Mth.lerp(fy, minY, maxY);
                    double sz = Mth.lerp(fz, minZ, maxZ) + offsetZ;
                    samples.add(new double[] {sx, sy, sz});
                }
            }
        }

        result.addProperty("degenerate", false);
        result.addProperty("stepX", formatFloat(stepX));
        result.addProperty("stepY", formatFloat(stepY));
        result.addProperty("stepZ", formatFloat(stepZ));
        result.addProperty("offsetX", formatFloat(offsetX));
        result.addProperty("offsetZ", formatFloat(offsetZ));
        result.addProperty("sampleCount", samples.size());
        result.addProperty("visibleDenominator", samples.size());

        JsonArray first = new JsonArray();
        for (int i = 0; i < samples.size() && i < 3; i++) {
            first.add(toArray(samples.get(i)));
        }
        result.add("firstSample", first);

        JsonArray last = new JsonArray();
        for (int i = Math.max(0, samples.size() - 3); i < samples.size(); i++) {
            last.add(toArray(samples.get(i)));
        }
        result.add("lastSample", last);
        return result;
    }

    private static JsonArray toArray(double[] sample) {
        JsonArray array = new JsonArray();
        array.add(formatFloat(sample[0]));
        array.add(formatFloat(sample[1]));
        array.add(formatFloat(sample[2]));
        return array;
    }

    private static String formatFloat(double value) {
        String text = String.format("%.6g", value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }
}
