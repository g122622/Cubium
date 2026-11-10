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

/**
 * Direction 的轴向正负与朝向角度判定的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DirectionFacingOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 本能力不需要注册表。
 *
 * 单元表必须与 probes/direction_facing.cpp 逐项一致。
 */
public final class DirectionFacingOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final List<String> UNITS =
            List.of(
                    "axisdir|down",
                    "axisdir|up",
                    "axisdir|north",
                    "axisdir|south",
                    "axisdir|west",
                    "axisdir|east",
                    "facing|north|0",
                    "facing|south|0",
                    "facing|west|0",
                    "facing|east|0",
                    "facing|north|90",
                    "facing|south|90",
                    "facing|west|90",
                    "facing|east|90",
                    "facing|north|180",
                    "facing|south|180",
                    "facing|west|180",
                    "facing|east|180",
                    "facing|north|270",
                    "facing|south|270",
                    "facing|west|270",
                    "facing|east|270",
                    "facing|north|45",
                    "facing|north|44.9",
                    "facing|north|45.1",
                    "facing|east|45",
                    "facing|east|44.9",
                    "facing|east|45.1",
                    "facing|south|45",
                    "facing|west|45",
                    "facing|north|135",
                    "facing|east|135",
                    "facing|north|225",
                    "facing|east|225",
                    "facing|north|315",
                    "facing|east|315",
                    "facing|north|-45",
                    "facing|east|-45",
                    "facing|north|360",
                    "facing|east|-90",
                    "facing|north|720",
                    "facing|up|0",
                    "facing|down|0",
                    "facing|up|90");

    private DirectionFacingOracle() {}

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
                "usage: DirectionFacingOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static Direction parseDirection(String name) {
        Direction parsed = Direction.byName(name);
        if (parsed == null) {
            throw new IllegalArgumentException("direction_facing: unknown direction: " + name);
        }
        return parsed;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("direction_facing: unit needs at least two fields: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String mode = parts[0];

        if (mode.equals("axisdir")) {
            Direction dir = parseDirection(parts[1]);
            out.addProperty("value", dir.getAxisDirection().getName());
            return out;
        }

        if (mode.equals("facing")) {
            if (parts.length != 3) {
                throw new IllegalArgumentException("direction_facing: facing needs an angle: " + unit);
            }
            Direction dir = parseDirection(parts[1]);
            float yaw = (float) Double.parseDouble(parts[2]);
            out.addProperty("value", dir.isFacingAngle(yaw));
            return out;
        }

        throw new IllegalArgumentException("direction_facing: unrecognized unit: " + unit);
    }
}