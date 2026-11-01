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
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.shapes.BooleanOp;
import net.minecraft.world.phys.shapes.Shapes;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * {@code VoxelShape} 布尔组合的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ShapesBooleanOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Shapes#join(VoxelShape, VoxelShape, BooleanOp)}，结果用
 * {@link VoxelShape#toAabbs()} 取出，不重写布尔函数。
 *
 * <p>形状由 8×8×8 的单位格子拼成：每格调用 {@link Shapes#box} 创建，再用
 * {@link Shapes#or} 合并。
 *
 * <p>单元表必须与 probes/shapes_boolean.cpp 逐项一致。
 */
public final class ShapesBooleanOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 网格边长。 */
    private static final int GRID = 8;

    /** 单元格边长。 */
    private static final int CELL = 1;

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "OR|lower|upper",
                    "OR|xhalf|stair",
                    "AND|xhalf|stair",
                    "AND|lower|upper",
                    "OR|empty|center",
                    "AND|empty|center",
                    "OR|empty|empty",
                    "AND|empty|full",
                    "NOT_SAME|lower|upper",
                    "NOT_SAME|center|center",
                    "FIRST|lower|upper",
                    "SECOND|lower|upper",
                    "ONLY_FIRST|lower|upper",
                    "ONLY_SECOND|lower|upper",
                    "OR|full|center",
                    "AND|full|center",
                    "OR|stair|center",
                    "NOT_SAME|stair|center");

    private ShapesBooleanOracle() {}

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
                "usage: ShapesBooleanOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 形状名 → 单格谓词。 */
    private static boolean shapeContains(String shape, int x, int y, int z) {
        switch (shape) {
            case "empty":
                return false;
            case "full":
                return true;
            case "lower":
                return y < 4;
            case "upper":
                return y >= 4;
            case "xhalf":
                return x < 4;
            case "center":
                return x >= 3 && x <= 4 && y >= 3 && y <= 4 && z >= 3 && z <= 4;
            case "stair":
                return y + z < 5;
            default:
                throw new IllegalArgumentException("shapes_boolean: 未知形状名: " + shape);
        }
    }

    /** 由形状名构造原版 `VoxelShape`：逐格 `Shapes.box` 再用 `Shapes.or` 合并。 */
    private static VoxelShape buildShape(String shape) {
        VoxelShape result = Shapes.empty();
        boolean any = false;
        for (int z = 0; z < GRID; z++) {
            for (int y = 0; y < GRID; y++) {
                for (int x = 0; x < GRID; x++) {
                    if (!shapeContains(shape, x, y, z)) {
                        continue;
                    }
                    VoxelShape cell = Shapes.box(
                            x * CELL, y * CELL, z * CELL,
                            x * CELL + CELL, y * CELL + CELL, z * CELL + CELL);
                    result = any ? Shapes.or(result, cell) : cell;
                    any = true;
                }
            }
        }
        return any ? result : Shapes.empty();
    }

    /** 运算名 → 原版 `BooleanOp`。 */
    private static BooleanOp resolveOp(String name) {
        switch (name) {
            case "FALSE":
                return BooleanOp.FALSE;
            case "NOT_OR":
                return BooleanOp.NOT_OR;
            case "ONLY_SECOND":
                return BooleanOp.ONLY_SECOND;
            case "NOT_FIRST":
                return BooleanOp.NOT_FIRST;
            case "ONLY_FIRST":
                return BooleanOp.ONLY_FIRST;
            case "NOT_SECOND":
                return BooleanOp.NOT_SECOND;
            case "NOT_SAME":
                return BooleanOp.NOT_SAME;
            case "NOT_AND":
                return BooleanOp.NOT_AND;
            case "AND":
                return BooleanOp.AND;
            case "SAME":
                return BooleanOp.SAME;
            case "SECOND":
                return BooleanOp.SECOND;
            case "CAUSES":
                return BooleanOp.CAUSES;
            case "FIRST":
                return BooleanOp.FIRST;
            case "CAUSED_BY":
                return BooleanOp.CAUSED_BY;
            case "OR":
                return BooleanOp.OR;
            case "TRUE":
                return BooleanOp.TRUE;
            default:
                throw new IllegalArgumentException("shapes_boolean: 未知布尔运算: " + name);
        }
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 3) {
            throw new IllegalArgumentException("shapes_boolean: 单元应为 '运算|形状A|形状B': " + unit);
        }

        BooleanOp op = resolveOp(parts[0]);

        // 原版 `Shapes.joinUnoptimized` 的第一条断言：空 ∩ 空必须为空。
        if (op.apply(false, false)) {
            throw new IllegalArgumentException(
                    "shapes_boolean: 运算 '" + parts[0] + "' 在 (false, false) 上为真");
        }

        VoxelShape shapeA = buildShape(parts[1]);
        VoxelShape shapeB = buildShape(parts[2]);
        VoxelShape joined = Shapes.join(shapeA, shapeB, op);

        // 输出规范化位图：逐格判断形状是否覆盖该格中心。`toAabbs()` 的盒互不重叠，
        // 逐点判断即可还原原始格子集合（含 `optimize()` 合并过的相邻盒）。
        java.util.List<AABB> boxes = new java.util.ArrayList<>(joined.toAabbs());
        StringBuilder bitmap = new StringBuilder(GRID * GRID * GRID);
        int count = 0;
        for (int z = 0; z < GRID; z++) {
            for (int y = 0; y < GRID; y++) {
                for (int x = 0; x < GRID; x++) {
                    double cx = (x + 0.5) * CELL;
                    double cy = (y + 0.5) * CELL;
                    double cz = (z + 0.5) * CELL;
                    boolean solid = containsPoint(boxes, cx, cy, cz);
                    bitmap.append(solid ? '1' : '0');
                    if (solid) {
                        count++;
                    }
                }
            }
        }
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("count", count);
        result.addProperty("bitmap", bitmap.toString());
        return result;
    }
/**
     * 判断点是否落在任一个盒内（盒互不重叠，命中一个即可）。
     */
    private static boolean containsPoint(java.util.List<AABB> boxes, double x, double y, double z) {
        for (AABB box : boxes) {
            if (x >= box.minX && x <= box.maxX
                    && y >= box.minY && y <= box.maxY
                    && z >= box.minZ && z <= box.maxZ) {
                return true;
            }
        }
        return false;
    }
}
