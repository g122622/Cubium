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
import net.minecraft.world.level.levelgen.DensityFunction;
import net.minecraft.world.level.levelgen.DensityFunctions;

/**
 * 组合型密度函数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DensityCompositeOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.DensityCompositeOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>覆盖 {@code TwoArgument}（add/mul/min/max）、{@code Lerp}、{@code RangeChoice}。
 * 子函数全部用常量，整棵树可以由单元名字的参数量化重建，两侧各造一份即可比对。
 *
 * <p>三类都不依赖噪声，但都是噪声整形的实际主力：{@code add}/{@code mul} 拼出密度组合，
 * {@code Lerp} 做双线性混合，{@code RangeChoice} 在洞穴与地形之间切换。
 */
public final class DensityCompositeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private DensityCompositeOracle() {}


    /** 与探针侧逐项一致。 */
    private static final String[] UNITS = {
        "add|0.1|0.2",
        "add|-1.5|0.33",
        "add|1e10|-1e10",
        "add|3|4",
        "mul|0|5",
        "mul|0|-1.5e300",
        "mul|2.5|0.4",
        "mul|-1.5|-0.6666666666",
        "mul|1e200|1e200",
        "min|1.5|-2.5",
        "min|-2.5|1.5",
        "min|0.1|0.1",
        "max|1.5|-2.5",
        "max|-2.5|1.5",
        "max|0.1|0.1",
        "lerp|0|10|-4",
        "lerp|1|10|-4",
        "lerp|0.5|10|-4",
        "lerp|-0.5|10|-4",
        "lerp|1.5|10|-4",
        "lerp|0.3333333333|1.5625|-0.078125",
        "lerp|0.98927504|10|0",
        "range|0.5|0|1|7|-7",
        "range|0|0|1|7|-7",
        "range|1|0|1|7|-7",
        "range|-1|0|1|7|-7",
        "range|0.9999999999|0|1|7|-7",
        "range|-64|-64|320|1.5|-1.5",
        "range|319|-64|320|1.5|-1.5",
        "range|320|-64|320|1.5|-1.5",
    };

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text.toString(), StandardCharsets.UTF_8);
            return;
        }

        if (args.length == 3 && args[0].equals("run")) {
            List<String> inputs = Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8);
            List<String> outputs = new ArrayList<>(inputs.size());
            for (String line : inputs) {
                if (line.isBlank()) {
                    continue;
                }
                outputs.add(runLine(line));
            }
            Files.write(Path.of(args[2]), outputs, StandardCharsets.UTF_8);
            return;
        }

        throw new IllegalArgumentException("usage: DensityCompositeOracle list <units.txt> | run <in> <out>");
    }

    private static DensityFunction buildFunction(String[] parts) {
        if (parts.length == 0) {
            return null;
        }
        String kind = parts[0];

        if (parts.length == 3) {
            DensityFunction a = DensityFunctions.constant(Double.parseDouble(parts[1]));
            DensityFunction b = DensityFunctions.constant(Double.parseDouble(parts[2]));
            return switch (kind) {
                case "add" -> DensityFunctions.add(a, b);
                case "mul" -> DensityFunctions.mul(a, b);
                case "min" -> DensityFunctions.min(a, b);
                case "max" -> DensityFunctions.max(a, b);
                default -> null;
            };
        }

        if (kind.equals("lerp") && parts.length == 4) {
            return DensityFunctions.lerp(
                    DensityFunctions.constant(Double.parseDouble(parts[1])),
                    DensityFunctions.constant(Double.parseDouble(parts[2])),
                    DensityFunctions.constant(Double.parseDouble(parts[3])));
        }

        if (kind.equals("range") && parts.length == 6) {
            return DensityFunctions.rangeChoice(
                    DensityFunctions.constant(Double.parseDouble(parts[1])),
                    Double.parseDouble(parts[2]),
                    Double.parseDouble(parts[3]),
                    DensityFunctions.constant(Double.parseDouble(parts[4])),
                    DensityFunctions.constant(Double.parseDouble(parts[5])));
        }
        return null;
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        DensityFunction function = buildFunction(requested.split("\\|", -1));
        if (function == null) {
            return missing(requested);
        }
        // 三个参数都是常量，采样坐标不影响结果；仍取一组非零坐标与探针侧一致。
        DensityFunction.FunctionContext context = new DensityFunction.FunctionContext() {
            @Override
            public int blockX() {
                return 17;
            }

            @Override
            public int blockY() {
                return 42;
            }

            @Override
            public int blockZ() {
                return -23;
            }
        };
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        JsonFloats.add(output, "value", function.compute(context));
        JsonFloats.add(output, "min", function.minValue());
        JsonFloats.add(output, "max", function.maxValue());
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}