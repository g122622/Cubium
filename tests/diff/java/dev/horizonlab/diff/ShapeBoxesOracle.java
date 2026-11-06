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
import java.util.Locale;
import net.minecraft.world.phys.shapes.Shapes;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * VoxelShape.forAllBoxes 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ShapeBoxesOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 本能力不需要注册表。
 *
 * 回调结果按顺序拼接后比对，因此既是数值比对也是遍历顺序比对。
 *
 * 单元表必须与 probes/shape_boxes.cpp 逐项一致。
 */
public final class ShapeBoxesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final List<String> UNITS =
            List.of(
                    "boxes|0,0,0,1,1,1",
                    "boxes|0.0625,0,0.0625,0.9375,0.09375,0.9375",
                    "boxes|0.25,0,0.25,0.75,0.5,0.75",
                    "boxes|0.375,0.25,0.375,0.625,1,0.625",
                    "boxes|-1,-2,-3,4,5,6",
                    "boxes|0,0,0,0,0,0",
                    "edges|0,0,0,1,1,1",
                    "edges|0.0625,0,0.0625,0.9375,0.09375,0.9375",
                    "edges|0.25,0,0.25,0.75,0.5,0.75",
                    "edges|0.375,0.25,0.375,0.625,1,0.625",
                    "edges|-1,-2,-3,4,5,6",
                    "edges|0,0,0,0,0,0");

    private ShapeBoxesOracle() {}

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
                "usage: ShapeBoxesOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        String[] sections = unit.split("\\|", -1);
        if (sections.length != 2) {
            throw new IllegalArgumentException("shape_boxes: unit needs two fields: " + unit);
        }
        String mode = sections[0];
        String[] parts = sections[1].split(",", -1);
        if (parts.length != 6) {
            throw new IllegalArgumentException("shape_boxes: expected 6 numbers: " + unit);
        }
        double[] box = new double[6];
        for (int i = 0; i < 6; i++) {
            box[i] = Double.parseDouble(parts[i]);
        }

        VoxelShape shape = Shapes.box(box[0], box[1], box[2], box[3], box[4], box[5]);

        StringBuilder joined = new StringBuilder();
        Shapes.DoubleLineConsumer collect = (x1, y1, z1, x2, y2, z2) -> {
            if (joined.length() > 0) {
                joined.append(';');
            }
            joined.append(String.format(Locale.ROOT, "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f", x1, y1, z1, x2, y2, z2));
        };
        if (mode.equals("boxes")) {
            shape.forAllBoxes(collect);
        } else if (mode.equals("edges")) {
            shape.forAllEdges(collect);
        } else {
            throw new IllegalArgumentException("shape_boxes: unknown mode: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        out.addProperty("value", joined.toString());
        return out;
    }
}