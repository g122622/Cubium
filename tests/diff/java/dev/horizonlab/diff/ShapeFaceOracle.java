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
import java.util.ArrayList;
import java.util.List;
import net.minecraft.SharedConstants;
import net.minecraft.core.Direction;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.phys.shapes.Shapes;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * {@code VoxelShape.getFaceShape} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ShapeFaceOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是形状描述（归一化坐标，多个盒子用 {@code ;} 分隔），输出 6 个方向的切片特征串。
 * 单元表必须与 probes/shape_face.cpp 的 UNITS 逐项一致。
 */
public final class ShapeFaceOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] UNITS = {
        "0,0,0,1,1,1",
        "0.0625,0,0.0625,0.9375,0.09375,0.9375",
        "0.375,0.4375,0.375,0.625,0.5625,0.625",
        "0,0.3125,0.375,1,1.5,0.625",
        "0.25,0,0.25,0.75,0.5,0.75",
        "0.375,0.25,0.375,0.625,1,0.625",
        "0.0625,0,0.0625,0.9375,0.5,0.9375",
        "0,0.875,0.375,1,1,0.625;0.0625,0,0.4375,0.9375,0.625,0.5625",
        "0,0,0,1,0.5,1;0,0.5,0,0.5,1,1",
        "0,0,0,1,0.0625,1",
        "0.4375,0,0,0.5625,1,1",
        "0,0,0,1,1,0.25;0,0,0.75,1,1,1",
        "0.15625,0,0.15625,0.34375,1,0.34375",
        "0,1,0.375,1,1.5,0.625",
        "0,0.25,0.25,0.0625,0.75,0.75",
    };

    private static final Direction[] DIRECTIONS = {
        Direction.DOWN, Direction.UP, Direction.NORTH, Direction.SOUTH, Direction.WEST, Direction.EAST,
    };

    private static final String[] NAMES = {"down", "up", "north", "south", "west", "east"};

    private ShapeFaceOracle() {}

    /** 解析形状描述。 */
    private static VoxelShape parseShape(String spec) {
        List<VoxelShape> boxes = new ArrayList<>();
        for (String part : spec.split(";")) {
            if (part.isEmpty()) {
                continue;
            }
            String[] raw = part.split(",");
            double[] c = new double[6];
            for (int i = 0; i < 6; i++) {
                c[i] = Double.parseDouble(raw[i].trim());
            }
            boxes.add(Shapes.box(c[0], c[1], c[2], c[3], c[4], c[5]));
        }
        if (boxes.isEmpty()) {
            return Shapes.empty();
        }
        VoxelShape result = boxes.get(0);
        for (int i = 1; i < boxes.size(); i++) {
            result = Shapes.or(result, boxes.get(i));
        }
        return result;
    }

    /** 与探针侧 {@code describe} 保持完全相同的格式。 */
    private static String describe(VoxelShape shape) {
        if (shape.isEmpty()) {
            return "EMPTY";
        }
        StringBuilder out = new StringBuilder();
        char[] axisNames = {'X', 'Y', 'Z'};
        Direction.Axis[] axes = {Direction.Axis.X, Direction.Axis.Y, Direction.Axis.Z};
        for (int a = 0; a < 3; a++) {
            out.append(axisNames[a]).append('[');
            double[] coords = shape.getCoords(axes[a]).toDoubleArray();
            for (int i = 0; i < coords.length; i++) {
                if (i != 0) {
                    out.append(',');
                }
                out.append(String.format(java.util.Locale.ROOT, "%.6f", coords[i]));
            }
            out.append(']');
        }
        return out.toString();
    }

    public static void main(String[] args) throws IOException {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
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
        System.err.println("usage: ShapeFaceOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            VoxelShape shape = parseShape(unit);

            // 与探针侧同样的扁平格式，避免嵌套 JSON 的键序差异。
            StringBuilder result = new StringBuilder();
            for (int i = 0; i < DIRECTIONS.length; i++) {
                if (i != 0) {
                    result.append('|');
                }
                result.append(NAMES[i]).append('=').append(describe(shape.getFaceShape(DIRECTIONS[i])));
            }

            JsonObject row = new JsonObject();
            row.addProperty("result", result.toString());
            row.addProperty("unit", unit);
            text.append(GSON.toJson(row)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }
}