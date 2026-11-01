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

/**
 * 信标效果范围与时长的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BeaconEffectsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>{@code BeaconBlockEntity.applyEffects} 是 private 且需要 {@code Level}（查询范围内玩家、
 * 施加效果），无法直接调用。这里按源码逐行复算范围与时长这两个纯数值公式；原版常量
 * （每级半径 10、基础半径 10、时长基数 9、每级时长 2 秒）在代码里是内联字面量，没有可
 * 引用的常量字段，因此同样以字面量形式复现。
 *
 * <p>单元表必须与 probes/beacon_effects.cpp 逐项一致。
 */
public final class BeaconEffectsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版信标的最大层数。 */
    private static final int MAX_LEVELS = 4;

    /** 每级增加的效果范围（格）。 */
    private static final int RADIUS_PER_LEVEL = 10;

    /** 效果范围的基础值（格）。 */
    private static final int RADIUS_BASE = 10;

    /** 效果时长公式里的基础秒数偏移（注意是 9，不是 10）。 */
    private static final int DURATION_BASE_SECONDS = 9;

    /** 每级增加的时长（秒）。 */
    private static final int DURATION_PER_LEVEL_SECONDS = 2;

    /** 单元表：{@code levels|sameEffect|hasSecondary}。 */
    private static final List<String> UNITS =
            List.of(
                    "1|false|false",
                    "2|false|false",
                    "3|false|false",
                    "4|true|false",
                    "4|true|true",
                    "4|false|true",
                    "4|false|false",
                    "3|true|false",
                    "3|false|true",
                    "0|false|false");

    private BeaconEffectsOracle() {}

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
                "usage: BeaconEffectsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 3) {
            throw new IllegalArgumentException("beacon_effects: 单元应为 3 段: " + unit);
        }

        int levels = Integer.parseInt(parts[0]);
        boolean sameEffect = Boolean.parseBoolean(parts[1]);
        boolean hasSecondary = Boolean.parseBoolean(parts[2]);

        // 原版 applyEffects 里的两段公式，逐行对应。
        int radius = levels * RADIUS_PER_LEVEL + RADIUS_BASE;
        int amplifier = (levels >= MAX_LEVELS && sameEffect) ? 1 : 0;
        int durationTicks = (DURATION_BASE_SECONDS + levels * DURATION_PER_LEVEL_SECONDS) * 20;
        boolean appliesSecondary = levels >= MAX_LEVELS && !sameEffect && hasSecondary;

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("levels", levels);
        result.addProperty("radius", radius);
        result.addProperty("durationTicks", durationTicks);
        result.addProperty("amplifier", amplifier);
        result.addProperty("appliesSecondary", appliesSecondary);
        return result;
    }
}