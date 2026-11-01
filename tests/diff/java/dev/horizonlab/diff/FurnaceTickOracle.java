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
import net.minecraft.util.Mth;

/**
 * 熔炉每 tick 状态机的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.FurnaceTickOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>复刻 {@code AbstractFurnaceBlockEntity.serverTick} 的完整方法体。该方法需要真实的
 * {@code ServerLevel} 与 {@code BlockEntity}（读配方、改方块状态、发同步包），且
 * {@code litTimeRemaining} 等字段没有公开写访问器，因此这里按源码逐行复写，把外部可变
 * 因素（配方是否存在、原料/燃料槽是否非空、燃料燃烧时长）作为输入显式传入。
 *
 * <p>单元表必须与 probes/furnace_tick.cpp 逐项一致。
 */
public final class FurnaceTickOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表：{@code lit|litTotal|cook|cookTotal|canBurn|hasInput|hasFuel|burnDuration}。 */
    private static final List<String> UNITS =
            List.of(
                    "100|200|5|200|true|true|true|200",
                    "100|200|5|6|true|true|true|200",
                    "100|200|5|200|false|true|true|200",
                    "0|0|0|200|true|true|true|200",
                    "0|0|5|200|true|false|true|200",
                    "0|0|5|200|true|true|false|200",
                    "0|0|10|200|false|false|false|200",
                    "0|0|1|200|false|false|false|200",
                    "0|0|0|200|false|false|false|200",
                    "1|200|5|200|false|true|true|200",
                    "1|200|5|200|true|true|true|200",
                    "0|0|100|200|false|true|false|200");

    private FurnaceTickOracle() {}

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
                "usage: FurnaceTickOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 8) {
            throw new IllegalArgumentException("furnace_tick: 单元应为 8 段: " + unit);
        }

        int litTimeRemaining = Integer.parseInt(parts[0]);
        int litTotalTime = Integer.parseInt(parts[1]);
        int cookingTimer = Integer.parseInt(parts[2]);
        int cookingTotalTime = Integer.parseInt(parts[3]);
        boolean canBurn = Boolean.parseBoolean(parts[4]);
        boolean hasInput = Boolean.parseBoolean(parts[5]);
        boolean hasFuel = Boolean.parseBoolean(parts[6]);
        int burnDuration = Integer.parseInt(parts[7]);

        boolean wasLit = litTimeRemaining > 0;
        boolean changed = false;
        boolean smelted = false;

        // 1) 先递减燃烧计时。
        if (litTimeRemaining > 0) {
            litTimeRemaining--;
        }

        // 2) 主分支。
        if (litTimeRemaining > 0 || (hasFuel && hasInput)) {
            // 2a) 未燃烧且可以烧 -> 点燃。
            if (litTimeRemaining <= 0 && canBurn) {
                litTimeRemaining = burnDuration;
                litTotalTime = litTimeRemaining;
                if (litTimeRemaining > 0) {
                    changed = true;
                }
            }

            // 2b) 燃烧中且可以烧 -> 推进烹饪。
            if (litTimeRemaining > 0 && canBurn) {
                cookingTimer++;
                if (cookingTimer == cookingTotalTime) {
                    cookingTimer = 0;
                    smelted = true;
                    changed = true;
                }
            } else {
                cookingTimer = 0;
            }
        } else if (litTimeRemaining <= 0 && cookingTimer > 0) {
            // 3) 完全熄火：减 2。
            cookingTimer = Mth.clamp(cookingTimer - 2, 0, cookingTotalTime);
        }

        // 4) 燃烧状态变化。
        boolean nowLit = litTimeRemaining > 0;
        if (wasLit != nowLit) {
            changed = true;
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("litTimeRemaining", litTimeRemaining);
        result.addProperty("litTotalTime", litTotalTime);
        result.addProperty("cookingTimer", cookingTimer);
        result.addProperty("cookingTotalTime", cookingTotalTime);
        result.addProperty("smelted", smelted);
        result.addProperty("stateChanged", changed);
        return result;
    }
}
