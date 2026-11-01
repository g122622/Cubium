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
import net.minecraft.core.Direction;
import net.minecraft.core.FrontAndTop;

/**
 * {@code FrontAndTop} 查找表与序列化名的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.FrontAndTopOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link FrontAndTop#fromFrontAndTop(Direction, Direction)} /
 * {@link FrontAndTop#getSerializedName()} / {@link FrontAndTop#front()} /
 * {@link FrontAndTop#top()}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>非法组合（`fromFrontAndTop` 返回 `null`）统一输出字符串 `"&lt;null&gt;"`。
 *
 * <p>单元表必须与 probes/front_and_top.cpp 逐项一致。
 */
public final class FrontAndTopOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 非法组合的输出标记。 */
    private static final String NULL_MARK = "<null>";

    /** 单元表：全部 36 种组合的 serial + 若干显式用例。 */
    private static final List<String> UNITS = buildUnits();

    private FrontAndTopOracle() {}

    /** 构造单元表：与探针逐项一致。 */
    private static List<String> buildUnits() {
        List<String> units = new ArrayList<>();
        for (Direction front : Direction.values()) {
            for (Direction top : Direction.values()) {
                units.add("serial|" + front.getName() + "|" + top.getName());
            }
        }
        units.add("front|down|north");
        units.add("front|up|east");
        units.add("front|west|up");
        units.add("top|down|north");
        units.add("top|up|east");
        units.add("top|west|up");
        units.add("front|north|south");
        units.add("top|north|south");
        return units;
    }

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
                "usage: FrontAndTopOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 按名字查方向。 */
    private static Direction directionByName(String name) {
        for (Direction direction : Direction.values()) {
            if (direction.getName().equals(name)) {
                return direction;
            }
        }
        throw new IllegalArgumentException("front_and_top: 未知方向: " + name);
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length != 3) {
            throw new IllegalArgumentException("front_and_top: 单元需要 3 段: " + unit);
        }

        Direction front = directionByName(parts[1]);
        Direction top = directionByName(parts[2]);
        FrontAndTop result = FrontAndTop.fromFrontAndTop(front, top);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "serial" -> out.addProperty("value",
                    result == null ? NULL_MARK : result.getSerializedName());
            case "front" -> out.addProperty("value",
                    result == null ? NULL_MARK : result.front().getName());
            case "top" -> out.addProperty("value",
                    result == null ? NULL_MARK : result.top().getName());
            default -> throw new IllegalArgumentException(
                    "front_and_top: 无法识别的单元: " + unit);
        }

        return out;
    }
}