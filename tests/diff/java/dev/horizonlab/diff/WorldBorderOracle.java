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
import net.minecraft.SharedConstants;
import net.minecraft.core.BlockPos;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.level.border.WorldBorder;
import net.minecraft.world.phys.AABB;

/**
 * {@code WorldBorder} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.WorldBorderOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link WorldBorder#isWithinBounds(BlockPos)}、{@link
 * WorldBorder#isWithinBounds(AABB)} 与 {@code getMinX()/getMaxX()/getMinZ()/getMaxZ()}，
 * 不重写公式。
 *
 * <p>需要 bootstrap：{@code WorldBorder} 的静态初始化会触碰 {@code Mth} 与
 * {@code SharedConstants}。
 *
 * <p>单元表必须与 probes/world_border.cpp 逐项一致。
 */
public final class WorldBorderOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。与 probes/world_border.cpp 的 writeList 逐行一致。 */
    private static final List<String> UNITS = buildUnits();

    /** 与外层保持一致：17 位有效数字，避免低位截断掩盖浮点差异。 */
    private static String formatDouble(double value) {
        if (value == 0.0) {
            return "0";
        }
        // 与 C++ 侧一致：14 位有效数字（见 probes/world_border.cpp 的说明）。
        String text = String.format("%.14g", value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }

    private static double[] parseDoubles(String text) {
        String[] pieces = text.split(",", -1);
        double[] values = new double[pieces.length];
        for (int i = 0; i < pieces.length; i++) {
            values[i] = Double.parseDouble(pieces[i]);
        }
        return values;
    }

    private static List<String> buildUnits() {
        String[] configs = {"0,0|100", "0,0|1", "100,-200|512", "-1.5,3.25|7.5", "0.1,0.2|0.3"};
        String[] probes = {
            "0,0",
            "50,50",
            "50,50.0001",
            "49.9999,50",
            "-50,-50",
            "-50.0001,-50",
            "0.15000000000000002,0.15",
            "1,1",
            "-1,-1",
            "25,25"
        };
        String[] boxes = {
            "0,0,50,50",
            "0,0,49.99999,49.99999",
            "0,0,50.00001,50.00001",
            "-49.99999,-49.99999,0,0",
            "-50.00001,-50.00001,0,0",
            "0,0,1,1"
        };
        java.util.ArrayList<String> units = new java.util.ArrayList<>();
        for (String config : configs) {
            units.add("extent|" + config);
            for (String probe : probes) {
                units.add("block|" + config + "|" + probe);
            }
            for (String box : boxes) {
                units.add("aabb|" + config + "|" + box);
            }
        }
        return List.copyOf(units);
    }

    private WorldBorderOracle() {}

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
        System.err.println("usage: WorldBorderOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            JsonObject row = summarise(unit);
            row.addProperty("unit", unit);
            text.append(GSON.toJson(row)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 3) {
            throw new IllegalArgumentException("world_border: 单元至少三段: " + unit);
        }
        String mode = parts[0];
        double[] center = parseDoubles(parts[1]);
        if (center.length != 2) {
            throw new IllegalArgumentException("world_border: 中心应为两个分量: " + unit);
        }
        double size = Double.parseDouble(parts[2]);

        WorldBorder border = new WorldBorder();
        border.setCenter(center[0], center[1]);
        border.setSize(size);

        String result;
        switch (mode) {
            case "extent" -> result = "minX=" + formatDouble(border.getMinX()) + " minZ=" + formatDouble(border.getMinZ())
                    + " maxX=" + formatDouble(border.getMaxX()) + " maxZ=" + formatDouble(border.getMaxZ());
            case "block" -> {
                if (parts.length < 4) {
                    throw new IllegalArgumentException("world_border: 缺少坐标段: " + unit);
                }
                double[] coords = parseDoubles(parts[3]);
                if (coords.length != 2) {
                    throw new IllegalArgumentException("world_border: 坐标应为两个分量: " + unit);
                }
                BlockPos pos = new BlockPos((int) Math.floor(coords[0]), 0, (int) Math.floor(coords[1]));
                result = border.isWithinBounds(pos) ? "true" : "false";
            }
            case "aabb" -> {
                if (parts.length < 4) {
                    throw new IllegalArgumentException("world_border: 缺少盒段: " + unit);
                }
                double[] box = parseDoubles(parts[3]);
                if (box.length != 4) {
                    throw new IllegalArgumentException("world_border: AABB 应为四个分量: " + unit);
                }
                AABB bounds = new AABB(box[0], 0.0, box[1], box[2], 0.0, box[3]);
                result = border.isWithinBounds(bounds) ? "true" : "false";
            }
            default -> throw new IllegalArgumentException("world_border: 未知模式: " + mode);
        }

        JsonObject out = new JsonObject();
        out.addProperty("result", result);
        return out;
    }
}
