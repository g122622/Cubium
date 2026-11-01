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
import net.minecraft.world.level.material.Fluid;
import net.minecraft.world.level.material.FluidState;
import net.minecraft.world.level.material.Fluids;

/**
 * 流体扩散的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.FluidSpreadOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link FluidState#getOwnHeight()} 与 {@link FluidState#getAmount()}。
 * 扩散参数（`getDropOff` / `getSlopeFindDistance`）在 `WaterFluid` / `LavaFluid` 上是
 * 实例方法，通过 {@code Fluids} 取得实例后调用。
 *
 * <p>单元表必须与 probes/fluid_spread.cpp 逐项一致。
 */
public final class FluidSpreadOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "water|1|false|false",
                    "water|2|false|false",
                    "water|3|false|false",
                    "water|8|false|false",
                    "water|9|false|false",
                    "water|1|true|false",
                    "water|8|true|false",
                    "lava|1|false|false",
                    "lava|4|false|false",
                    "water|8|false|true",
                    "water|7|false|true",
                    "water|1|false|true",
                    "lava|8|false|false",
                    "lava|3|false|false",
                    "lava|2|false|false",
                    "lava|1|false|false",
                    "lava|3|false|true",
                    "lava|1|false|true",
                    "slope|water|1|0",
                    "slope|water|1|1",
                    "slope|water|1|2",
                    "slope|water|1|3",
                    "slope|water|1|4",
                    "slope|water|1|5",
                    "slope|water|1|-1",
                    "slope|lava|1|0",
                    "slope|lava|1|1",
                    "slope|lava|1|2",
                    "slope|lava|1|3",
                    "slope|lava_fast|1|3",
                    "slope|lava_fast|1|4");

    private FluidSpreadOracle() {}

    public static void main(String[] args) throws IOException {
        DataPackRegistries.load();
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
                "usage: FluidSpreadOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 按名字取流体实例。`lava_fast` 与 `lava` 使用同一个流体对象。 */
    private static Fluid fluidByName(String name) {
        switch (name) {
            case "water":
                return Fluids.WATER;
            case "lava":
            case "lava_fast":
                return Fluids.LAVA;
            default:
                throw new IllegalArgumentException("fluid_spread: 未知流体: " + name);
        }
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("fluid_spread: 单元至少两段: " + unit);
        }

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (parts[0].equals("slope") && parts.length == 4) {
            String fluidName = parts[1];
            int depth = Integer.parseInt(parts[2]);
            int holeDistance = Integer.parseInt(parts[3]);
            int limit = slopeFindDistanceFor(fluidName);

            // 复刻 getSlopeDistance 的一维形态。
            int value;
            if (holeDistance < 0) {
                value = 1000;
            } else {
                int currentDepth = depth;
                int remaining = holeDistance;
                int answer = 1000;
                while (true) {
                    if (remaining < 0) {
                        answer = 1000;
                        break;
                    }
                    if (remaining == 0) {
                        answer = currentDepth;
                        break;
                    }
                    if (currentDepth >= limit) {
                        answer = 1000;
                        break;
                    }
                    currentDepth++;
                    remaining--;
                }
                value = answer;
            }

            result.addProperty("slopeFindDistance", limit);
            result.addProperty("result", value);
            return result;
        }

        if (parts.length == 4) {
            String fluidName = parts[0];
            int amount = Integer.parseInt(parts[1]);
            boolean hasSameAbove = Boolean.parseBoolean(parts[2]);
            boolean falling = Boolean.parseBoolean(parts[3]);

            Fluid fluid = fluidByName(fluidName);
            FluidState state = withAmount(fluid, amount);

            float ownHeight = state.getOwnHeight();
            float height = hasSameAbove ? 1.0f : ownHeight;

            int dropOff = dropOffFor(fluidName);
            int spreadLevel;
            if (falling) {
                spreadLevel = 7;
            } else {
                int level = state.getAmount() - dropOff;
                spreadLevel = Math.max(level, 0);
            }

            result.addProperty("height", formatNumber(height));
            result.addProperty("spreadLevel", spreadLevel);
            result.addProperty("dropOff", dropOff);
            return result;
        }

        throw new IllegalArgumentException("fluid_spread: 无法识别的单元: " + unit);
    }

    /**
     * 把流体状态设到指定等级。
     *
     * <p>`Fluids.WATER` / `Fluids.LAVA` 的默认状态来自 **Source** 内部类（`getAmount()` 恒为
     * 8，状态上只有 `falling` 属性，没有 `level`）。要构造任意等级，必须改用 **Flowing**
     * 内部类（`getAmount()` 读 `LEVEL` 属性）。
     */
    private static FluidState withAmount(Fluid fluid, int amount) {
        int clamped = Math.max(1, Math.min(8, amount));
        FluidState base = flowingInstance(fluid).defaultFluidState();
        for (var property : base.getProperties()) {
            if (property.getName().equals("level")) {
                @SuppressWarnings("unchecked")
                net.minecraft.world.level.block.state.properties.IntegerProperty levelProp =
                        (net.minecraft.world.level.block.state.properties.IntegerProperty) property;
                return base.setValue(levelProp, clamped);
            }
        }
        return base;
    }

    /**
     * 取得对应流体的 `Flowing` 实例。
     *
     * <p>不能 `new WaterFluid.Flowing()`——`Fluid` 的构造器会调用 `createIntrusiveHolder`，
     * 而注册表在启动后已冻结，会抛 `IllegalStateException`。改为从注册表中按标识符取出已
     * 注册的实例：`minecraft:flowing_water` / `minecraft:flowing_lava`。
     */
    private static Fluid flowingInstance(Fluid fluid) {
        if (fluid == Fluids.WATER) {
            return Fluids.FLOWING_WATER;
        }
        if (fluid == Fluids.LAVA) {
            return Fluids.FLOWING_LAVA;
        }
        throw new IllegalArgumentException("fluid_spread: 不支持的流体");
    }

    /** 水的 dropOff 恒为 1；岩浆在主世界为 2、下界为 1。 */
    private static int dropOffFor(String fluidName) {
        switch (fluidName) {
            case "water":
                return 1;
            case "lava":
                return 2;
            case "lava_fast":
                return 1;
            default:
                throw new IllegalArgumentException("fluid_spread: 未知流体: " + fluidName);
        }
    }

    /** 坡度搜索上限：水 4、主世界岩浆 2、下界岩浆 4。 */
    private static int slopeFindDistanceFor(String fluidName) {
        switch (fluidName) {
            case "water":
                return 4;
            case "lava":
                return 2;
            case "lava_fast":
                return 4;
            default:
                throw new IllegalArgumentException("fluid_spread: 未知流体: " + fluidName);
        }
    }

    private static String formatNumber(double value) {
        String text = String.format("%.9g", value);
        int expPos = -1;
        for (int i = 0; i < text.length(); i++) {
            char ch = text.charAt(i);
            if (ch == 'e' || ch == 'E') {
                expPos = i;
                break;
            }
        }
        String mantissa = expPos < 0 ? text : text.substring(0, expPos);
        String suffix = expPos < 0 ? "" : text.substring(expPos);
        if (mantissa.indexOf('.') >= 0) {
            while (mantissa.endsWith("0")) {
                mantissa = mantissa.substring(0, mantissa.length() - 1);
            }
            if (mantissa.endsWith(".")) {
                mantissa = mantissa.substring(0, mantissa.length() - 1);
            }
        }
        return mantissa + suffix;
    }
}
