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
import net.minecraft.world.level.material.FluidState;
import net.minecraft.world.level.material.FlowingFluid;

/**
 * 流体量值与高度的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.FluidHeightOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么本 oracle 不调用 {@code FluidState} 实例方法</h2>
 *
 * <p>原版 {@link FluidState#getAmount()} 是实例方法，但它返回的值取决于该状态下 {@code LEVEL}
 * 属性的实际值。而 {@code LEVEL}（即 {@link FlowingFluid#LEVEL} =
 * {@code BlockStateProperties.LEVEL_FLOWING}）**挂在流体状态上，不在方块状态上**——
 * {@code Blocks.WATER.defaultBlockState()} 上没有 {@code level} 属性，尝试
 * {@code setValue(FlowingFluid.LEVEL, n)} 会抛
 * {@code IllegalArgumentException: ... does not exist in Block{minecraft:water}}。
 *
 * <p>流体状态只能由世界在实际流动中产生（{@code FluidState} 没有公开的、可指定 level 的
 * 工厂）。因此**无法在独立 oracle 里构造任意 level 的流体状态**。
 *
 * <h2>本 oracle 的做法：以原版常量为基准</h2>
 *
 * <p>改用两个**公开常量**作为基准，它们定义了高度公式的取值边界：
 *
 * <ul>
 *   <li>{@link FluidState#AMOUNT_MAX} = {@code 9} —— 分母</li>
 *   <li>{@link FluidState#AMOUNT_FULL} = {@code 8} —— 满值（源头）</li>
 *   <li>{@link FlowingFluid#LEVEL} 的取值范围 {@code [1, 8]}</li>
 * </ul>
 *
 * <p>`getAmount` 直接回显输入（与 `state.getValue(LEVEL)` 等价）；`ownHeight` 用
 * `amount / (float) AMOUNT_MAX` 计算——**分母取自原版常量而非字面量 9**，因此若原版把
 * `AMOUNT_MAX` 改成别的值，本 oracle 会立即跟随。
 *
 * <p>本能力**不需要注册表**（不构造任何 `FluidState`）。
 *
 * <p>单元表必须与 probes/fluid_height.cpp 逐项一致。
 */
public final class FluidHeightOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "amount|1",
                    "amount|2",
                    "amount|3",
                    "amount|4",
                    "amount|5",
                    "amount|6",
                    "amount|7",
                    "amount|8",
                    "ownHeight|1",
                    "ownHeight|2",
                    "ownHeight|3",
                    "ownHeight|4",
                    "ownHeight|5",
                    "ownHeight|6",
                    "ownHeight|7",
                    "ownHeight|8");

    private FluidHeightOracle() {}

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
                "usage: FluidHeightOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 按 `%.9g` 格式化后去掉尾随零。 */
    private static String formatNumber(float value) {
        String text = String.format(Locale.ROOT, "%.9g", (double) value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            int end = text.length();
            while (end > 0 && text.charAt(end - 1) == '0') {
                end--;
            }
            if (end > 0 && text.charAt(end - 1) == '.') {
                end--;
            }
            text = text.substring(0, end);
        }
        return text;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("fluid_height: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        int amount = Integer.parseInt(parts[1]);
        if (amount < 1 || amount > FluidState.AMOUNT_FULL) {
            throw new IllegalArgumentException(
                    "fluid_height: amount 必须在 [1," + FluidState.AMOUNT_FULL + "]: " + amount);
        }

        switch (parts[0]) {
            case "amount" -> out.addProperty("value", amount);
            case "ownHeight" -> out.addProperty("value",
                    formatNumber(amount / (float) FluidState.AMOUNT_MAX));
            default -> throw new IllegalArgumentException("fluid_height: 无法识别的单元: " + unit);
        }

        return out;
    }
}