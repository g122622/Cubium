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
import net.minecraft.world.level.block.entity.HopperBlockEntity;

/**
 * 漏斗传输冷却时序的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.HopperCooldownOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>复刻 {@code HopperBlockEntity.pushItemsTick} 与 {@code tryMoveItems} 的冷却时序。
 * 这两个方法需要真实的 {@code Level} 与 {@code BlockEntity}（判断启用状态、遍历槽位、
 * 实际移动物品），且 {@code cooldownTime} 没有公开写访问器，因此这里按源码逐行复写，
 * 把外部可变因素（是否启用、是否有物品可动）作为输入显式传入。
 *
 * <p>{@code MOVE_ITEM_SPEED} 直接引用原版常量。
 *
 * <p>单元表必须与 probes/hopper_cooldown.cpp 逐项一致。
 */
public final class HopperCooldownOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表：{@code cooldown|enabled|hasMovable}。 */
    private static final List<String> UNITS =
            List.of(
                    "-1|true|true",
                    "0|true|true",
                    "1|true|true",
                    "2|true|true",
                    "8|true|true",
                    "9|true|true",
                    "1|false|true",
                    "1|true|false",
                    "0|false|true",
                    "5|true|false");

    private HopperCooldownOracle() {}

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
                "usage: HopperCooldownOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            throw new IllegalArgumentException("hopper_cooldown: 单元应为 3 段: " + unit);
        }

        int cooldown = Integer.parseInt(parts[0]);
        boolean enabled = Boolean.parseBoolean(parts[1]);
        boolean hasMovable = Boolean.parseBoolean(parts[2]);

        // 1) pushItemsTick：无条件递减。
        int cooldownAfter = cooldown - 1;

        boolean attempted = false;
        boolean moved = false;
        boolean cooldownSet = false;

        // 2) 递减后再判定 isOnCooldown()。
        if (cooldownAfter <= 0) {
            // 3) setCooldown(0) 后进入 tryMoveItems。
            cooldownAfter = 0;
            attempted = true;

            if (enabled && hasMovable) {
                // 4) 传输成功才设置冷却。
                moved = true;
                cooldownAfter = HopperBlockEntity.MOVE_ITEM_SPEED;
                cooldownSet = true;
            }
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("cooldownAfter", cooldownAfter);
        result.addProperty("attempted", attempted);
        result.addProperty("moved", moved);
        result.addProperty("cooldownSet", cooldownSet);
        return result;
    }
}