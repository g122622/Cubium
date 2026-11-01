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
import net.minecraft.core.Direction;
import net.minecraft.world.level.levelgen.structure.BoundingBox;

/**
 * {@code BoundingBox.orientBox} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.StructureOrientOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link BoundingBox#orientBox(int, int, int, int, int, int, int, int, int, Direction)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>输出为 `minX,minY,minZ,maxX,maxY,maxZ`。
 *
 * <p>单元表必须与 probes/structure_orient.cpp 逐项一致。
 */
public final class StructureOrientOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "0|0|0|0|0|0|3|2|4|south",
                    "0|0|0|0|0|0|3|2|4|north",
                    "0|0|0|0|0|0|3|2|4|west",
                    "0|0|0|0|0|0|3|2|4|east",
                    "0|0|0|0|0|0|3|2|4|up",
                    "0|0|0|0|0|0|3|2|4|down",
                    "10|64|-20|1|2|3|3|2|4|south",
                    "10|64|-20|1|2|3|3|2|4|north",
                    "10|64|-20|1|2|3|3|2|4|west",
                    "10|64|-20|1|2|3|3|2|4|east",
                    "0|0|0|1|1|1|2|2|2|west",
                    "0|0|0|1|1|1|2|2|2|east",
                    "5|5|5|0|0|0|1|1|1|north",
                    "5|5|5|0|0|0|1|1|1|west");

    private StructureOrientOracle() {}

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
                "usage: StructureOrientOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static Direction directionByName(String name) {
        for (Direction direction : Direction.values()) {
            if (direction.getName().equals(name)) {
                return direction;
            }
        }
        throw new IllegalArgumentException("structure_orient: 未知方向: " + name);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 10) {
            throw new IllegalArgumentException("structure_orient: 单元需要 10 段: " + unit);
        }

        BoundingBox box = BoundingBox.orientBox(
                Integer.parseInt(parts[0]), Integer.parseInt(parts[1]),
                Integer.parseInt(parts[2]), Integer.parseInt(parts[3]),
                Integer.parseInt(parts[4]), Integer.parseInt(parts[5]),
                Integer.parseInt(parts[6]), Integer.parseInt(parts[7]),
                Integer.parseInt(parts[8]), directionByName(parts[9]));

        String value = box.minX() + "," + box.minY() + "," + box.minZ() + ","
                + box.maxX() + "," + box.maxY() + "," + box.maxZ();

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);
        out.addProperty("value", value);
        return out;
    }
}