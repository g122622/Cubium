package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.mojang.serialization.JsonOps;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.world.level.levelgen.DensityFunction;
import net.minecraft.world.level.levelgen.DensityFunctions;

/**
 * 嵌套密度函数树的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DensityNestedOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>树用原版 {@code DensityFunction.DIRECT_CODEC} 从 JSON 反序列化得到——这正是原版加载
 * 数据包里的密度函数所走的路径。不用构造函数直拼，是因为 {@code DensityFunctions.Clamp} 与
 * {@code RangeChoice} 是包私有 record，外部无法实例化；走 codec 既绕开可见性，又保证测的是
 * 原版真正构造出来的那种对象。
 *
 * <p>树的形状与 {@code probes/density_nested.cpp} 的 {@code buildTree} 逐项对应。
 */
public final class DensityNestedOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private DensityNestedOracle() {}

    /** 采样点，与探针侧逐项一致。 */
    private static final int[][] SAMPLE_POINTS = {
        {0, -64, 0},
        {0, 0, 0},
        {0, 64, 0},
        {0, 319, 0},
        {15, 128, 15},
        {-16, 0, -16},
        {-1875000, 100, 1875000},
        {1000000, -192, -1000000},
        {7, 63, -3},
        {-1, 62, 1},
    };

    /** 树名，与探针侧逐项一致。 */
    private static final String[] TREES = {
        "constant",
        "gradient",
        "mapped_gradient_abs",
        "mapped_gradient_square",
        "mapped_gradient_squeeze",
        "add_two_gradients",
        "mul_two_gradients",
        "min_two_gradients",
        "max_two_gradients",
        "clamp_after_add",
        "clamp_after_mul",
        "mapped_after_clamp",
        "clamp_after_mapped",
        "lerp_three_gradients",
        "range_choice_gradient",
        "mul_by_zero",
        "deep_mix",
    };

    /** 主世界常见梯度：Y=-64 处 1.0，Y=320 处 -1.0。 */
    private static String overworldGradient() {
        return "{\"type\":\"minecraft:y_clamped_gradient\",\"from_y\":-64,\"to_y\":320,\"from_value\":1.0,\"to_value\":-1.0}";
    }

    /** 反向梯度：从 0 到 1。 */
    private static String risingGradient() {
        return "{\"type\":\"minecraft:y_clamped_gradient\",\"from_y\":-64,\"to_y\":320,\"from_value\":0.0,\"to_value\":1.0}";
    }

    /** 把 Mapped.Type 包一层，对应原版 `type: "minecraft:abs"` 这类单参数节点。 */
    private static String mapped(String type, String input) {
        return "{\"type\":\"minecraft:" + type + "\",\"argument\":" + input + "}";
    }

    private static String twoArg(String type, String left, String right) {
        return "{\"type\":\"minecraft:" + type + "\",\"argument1\":" + left + ",\"argument2\":" + right + "}";
    }

    /** Clamp 是包私有 record，用注册表里的 clamp 类型表达。 */
    private static String clampJson(String input, double min, double max) {
        return "{\"type\":\"minecraft:clamp\",\"input\":" + input + ",\"min\":" + min + ",\"max\":" + max + "}";
    }

    /** RangeChoice 同样是包私有 record，用注册表里的 range_choice 类型表达。 */
    private static String rangeChoiceJson(String input, String rising) {
        return "{\"type\":\"minecraft:range_choice\",\"input\":" + input
            + ",\"min_inclusive\":0.0,\"max_exclusive\":0.5"
            + ",\"when_in_range\":{\"type\":\"minecraft:constant\",\"argument\":10.0}"
            + ",\"when_out_of_range\":" + mapped("cube", rising) + "}";
    }

    /**
     * 构造一棵树。
     *
     * <p>两类节点来源不同，必须混用：
     * <ul>
     *   <li>{@code Clamp} 与 {@code RangeChoice} 是包私有 record，外部拿不到构造函数，只能走
     *       {@code DensityFunction.DIRECT_CODEC} 从 JSON 反序列化——这也正是原版加载数据包的路径；</li>
     *   <li>{@code add/mul/min/max/lerp} 与 Mapped 系列（{@code abs/square/cube/...}）**不在**
     *       codec 的注册表里（注册表只有 18 个类型，见 {@code DensityFunctions.register} 调用），
     *       只能用公开静态方法构造。</li>
     * </ul>
     *
     * <p>包私有的 {@code Mapped.Type} 也不需要直接引用：{@code DensityFunctions.map(df, type)} 的
     * 参数类型虽不可见，但传 {@code null} 之外无法调用，因此改用 codec 能表达的等价写法——
     * 这几类在注册表里有独立名字（{@code abs}、{@code square} 等），直接用 JSON 解析。
     */
    private static DensityFunction buildTree(String name) {
        String gradient = overworldGradient();
        String rising = risingGradient();

        DensityFunction gradientFn = parseJson(gradient);
        DensityFunction risingFn = parseJson(rising);

        return switch (name) {
            case "constant" -> parseJson("{\"type\":\"minecraft:constant\",\"argument\":0.25}");
            case "gradient" -> gradientFn;
            case "mapped_gradient_abs" -> parseJson(mapped("abs", gradient));
            case "mapped_gradient_square" -> parseJson(mapped("square", gradient));
            case "mapped_gradient_squeeze" -> parseJson(mapped("squeeze", gradient));
            case "add_two_gradients" -> DensityFunctions.add(gradientFn, risingFn);
            case "mul_two_gradients" -> DensityFunctions.mul(gradientFn, risingFn);
            case "min_two_gradients" -> DensityFunctions.min(gradientFn, risingFn);
            case "max_two_gradients" -> DensityFunctions.max(gradientFn, risingFn);
            case "clamp_after_add" -> parseJson(clampJson(twoArg("add", gradient, rising), -0.5, 0.5));
            case "clamp_after_mul" -> parseJson(clampJson(twoArg("mul", gradient, rising), -0.25, 0.75));
            case "mapped_after_clamp" -> parseJson(mapped("half_negative", clampJson(gradient, -0.5, 0.5)));
            case "clamp_after_mapped" -> parseJson(clampJson(mapped("half_negative", gradient), -0.5, 0.5));
            case "lerp_three_gradients" ->
                DensityFunctions.lerp(gradientFn, risingFn, DensityFunctions.constant(-3.0));
            case "range_choice_gradient" -> parseJson(rangeChoiceJson(gradient, rising));
            case "mul_by_zero" -> DensityFunctions.mul(DensityFunctions.constant(0.0), gradientFn);
            case "deep_mix" -> buildDeepMix();
            default -> null;
        };
    }

    /** deep_mix 需要先算出内层 lerp，再包两层 Mapped，单独处理。 */
    private static DensityFunction buildDeepMix() {
        DensityFunction lerped = DensityFunctions.lerp(
            parseJson(risingGradient()),
            parseJson(clampJson(overworldGradient(), -0.5, 0.5)),
            DensityFunctions.mul(parseJson(overworldGradient()), parseJson(risingGradient())));
        // Mapped 的 Type 是包私有枚举，只能借 codec 表达：先把内层编码成 JSON 再包一层。
        String innerJson = DensityFunction.DIRECT_CODEC
            .encodeStart(JsonOps.INSTANCE, lerped)
            .getOrThrow(error -> new IllegalStateException("deep_mix inner encode failed: " + error))
            .toString();
        return parseJson(mapped("invert", mapped("squeeze", innerJson)));
    }

    private static DensityFunction parseJson(String text) {
        return DensityFunction.DIRECT_CODEC
            .parse(JsonOps.INSTANCE, JsonParser.parseString(text))
            .getOrThrow(error -> new IllegalStateException("density tree failed to parse: " + error));
    }

    /** 单点上下文：与探针侧传入的三元组同构。 */
    private static DensityFunction.FunctionContext context(int[] point) {
        return new DensityFunction.SinglePointContext(point[0], point[1], point[2]);
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        // DIRECT_CODEC 是静态字段，初始化时要用到已注册的序列化器，必须先引导。
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String tree : TREES) {
                for (int i = 0; i < SAMPLE_POINTS.length; i++) {
                    text.append(tree).append('@').append(i).append('\n');
                }
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

        throw new IllegalArgumentException("usage: DensityNestedOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int at = requested.lastIndexOf('@');
        if (at < 0) {
            return missing(requested);
        }
        String treeName = requested.substring(0, at);
        int pointIndex;
        try {
            pointIndex = Integer.parseInt(requested.substring(at + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        if (pointIndex < 0 || pointIndex >= SAMPLE_POINTS.length) {
            return missing(requested);
        }

        DensityFunction tree = buildTree(treeName);
        if (tree == null) {
            return missing(requested);
        }

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        JsonFloats.add(output, "value", tree.compute(context(SAMPLE_POINTS[pointIndex])));
        JsonFloats.add(output, "min", tree.minValue());
        JsonFloats.add(output, "max", tree.maxValue());
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}