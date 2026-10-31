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
import net.minecraft.world.entity.EntityDimensions;

/**
 * 实体尺寸与包围盒的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntityDimensionsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@code EntityDimensions} 的构造器、{@code scale} 与
 * {@code makeBoundingBox}，不重写公式——本能力要钉住的正是这些公式本身。
 *
 * <p>场景表必须与 probes/entity_dimensions.cpp 逐项一致。
 */
public final class EntityDimensionsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 覆盖用的场景表，与 C++ 侧 SCENES 逐项一致。 */
    private static final List<String> SCENES =
            List.of(
                    "0.6|1.8|1|1|0|0|0",
                    "0.6|1.8|0.5|1|0|0|0",
                    "0.6|1.8|1|0.5|0|0|0",
                    "0.6|1.8|0.5|0.5|0|0|0",
                    "0.3|0.9|1|1|0|0|0",
                    "0.6|2.9|1|1|0|0|0",
                    "1.4|2.7|1|1|0|0|0",
                    "4|0.5|1|1|0|0|0",
                    "0.6|1.8|1|1|100|64|-200",
                    "0.6|1.8|1|1|-1234.5|63.5|987.25",
                    "0.6|1.8|0.5|0.5|0.5|0.5|0.5",
                    "0.8|1.95|1|1|0|0|0");

    private EntityDimensionsOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String scene : SCENES) {
                text.append(scene).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: EntityDimensionsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            throw new IllegalArgumentException("entity_dimensions: 单元应为 7 段: " + unit);
        }

        float width = Float.parseFloat(parts[0]);
        float height = Float.parseFloat(parts[1]);
        float scaleX = Float.parseFloat(parts[2]);
        float scaleY = Float.parseFloat(parts[3]);
        double posX = Double.parseDouble(parts[4]);
        double posY = Double.parseDouble(parts[5]);
        double posZ = Double.parseDouble(parts[6]);

        // 原版构造器会自动算 eyeHeight = height * 0.85F。
        EntityDimensions base = EntityDimensions.scalable(width, height);
        EntityDimensions scaled = base.scale(scaleX, scaleY);
        var box = scaled.makeBoundingBox(posX, posY, posZ);

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("width", formatFloat(base.width()));
        result.addProperty("height", formatFloat(base.height()));
        result.addProperty("eyeHeight", formatFloat(base.eyeHeight()));
        result.addProperty("scaleX", formatFloat(scaleX));
        result.addProperty("scaleY", formatFloat(scaleY));
        result.addProperty("scaledWidth", formatFloat(scaled.width()));
        result.addProperty("scaledHeight", formatFloat(scaled.height()));
        result.addProperty("scaledEyeHeight", formatFloat(scaled.eyeHeight()));
        result.addProperty("bboxMinX", formatFloat(box.minX));
        result.addProperty("bboxMinY", formatFloat(box.minY));
        result.addProperty("bboxMinZ", formatFloat(box.minZ));
        result.addProperty("bboxMaxX", formatFloat(box.maxX));
        result.addProperty("bboxMaxY", formatFloat(box.maxY));
        result.addProperty("bboxMaxZ", formatFloat(box.maxZ));
        return result;
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