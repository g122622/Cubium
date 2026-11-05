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
 * 密度函数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DensityFunctionOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.DensityFunctionOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>只覆盖<b>不依赖噪声、不依赖其他密度函数</b>的纯函数——它们完全由单元名字里的参数
 * 重建，两侧各造一份即可比对。噪声类（{@code NoiseDensity} / {@code ShiftedNoise}）
 * 与缓存类（{@code Cache2D} / {@code FlatCache}）留待后续能力。
 *
 * <p>密度函数直接决定地形：`YClampedGradient` 是主世界「从 Y=-64 的实心到 Y=320 的空气」
 * 那类梯度的载体，`Mapped` 的七种变换则参与几乎所有噪声整形。它们都是纯算术，
 * 一旦有偏差会表现为地形高度或洞穴形状不对，靠人工几乎无法定位。
 *
 * <p>单元格式与探针侧逐项一致：{@code <类型>|<参数...>@<采样点下标>}。
 */
public final class DensityFunctionOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private DensityFunctionOracle() {}

    private static String doubleBits(double value) {
        return String.format("0x%016x", Double.doubleToRawLongBits(value));
    }

    /** 与探针侧逐项一致。 */
    private static final String[] UNITS = {
        "const|0", "const|1", "const|-1", "const|0.1", "const|-0.3333333333", "const|1e-7",
        "ygrad|-64|320|1.5625|-0.078125",
        "ygrad|-64|320|0|1",
        "ygrad|0|256|-1|1",
        "ygrad|100|0|1|-1",
        "ygrad|0|0|0|1",
        "ygrad|-64|-64|0.5|-0.5",
        "ygrad|20|80|0.1|-0.7",
        "mapabs|-0.3333333333", "mapabs|0.3333333333", "mapabs|0",
        "mapsq|-1.5", "mapsq|1.5", "mapsq|0.1",
        "mapcube|-2.5", "mapcube|2.5", "mapcube|0.1",
        "maphalf|-1.5", "maphalf|1.5", "maphalf|0.1",
        "mapquarter|-1.5", "mapquarter|1.5", "mapquarter|0.1",
        "mapsqueeze|-1.5", "mapsqueeze|1.5", "mapsqueeze|0.1",
        "mapinvert|-1.5", "mapinvert|1.5", "mapinvert|0.1",
    };

    /** 采样坐标，与探针侧逐项一致。 */
    private static final int[][] SAMPLE_POINTS = {
        {0, -64, 0},
        {0, -32, 0},
        {0, 0, 0},
        {0, 20, 0},
        {0, 64, 0},
        {0, 100, 0},
        {0, 256, 0},
        {0, 320, 0},
        {17, 0, -23},
        {-1000, 50, 2000},
    };

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                for (int i = 0; i < SAMPLE_POINTS.length; i++) {
                    text.append(unit).append('@').append(i).append('\n');
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

        throw new IllegalArgumentException("usage: DensityFunctionOracle list <units.txt> | run <in> <out>");
    }

    /**
     * 构造 Mapped 密度函数。
     *
     * `DensityFunctions.Mapped.Type` 是 protected 的嵌套枚举，公开 API 里没有为各变换
     * 提供独立的工厂方法（只有 `map(input, Type)`），编译期无法引用该类型，因此全部走反射。
     * 取的是原版真实枚举常量与真实 `map` 方法，不是复制一份枚举值当参照。
     */
    private static DensityFunction mappedFunction(DensityFunction input, String kind) {
        String name = switch (kind) {
            case "mapabs" -> "ABS";
            case "mapsq" -> "SQUARE";
            case "mapcube" -> "CUBE";
            case "maphalf" -> "HALF_NEGATIVE";
            case "mapquarter" -> "QUARTER_NEGATIVE";
            case "mapsqueeze" -> "SQUEEZE";
            case "mapinvert" -> "INVERT";
            default -> null;
        };
        if (name == null) {
            return null;
        }
        try {
            Class<?> typeClass = Class.forName("net.minecraft.world.level.levelgen.DensityFunctions$Mapped$Type");
            @SuppressWarnings({"unchecked", "rawtypes"})
            Object type = Enum.valueOf((Class<? extends Enum>) typeClass.asSubclass(Enum.class), name);
            for (java.lang.reflect.Method method : DensityFunctions.class.getDeclaredMethods()) {
                if (!method.getName().equals("map") || method.getParameterCount() != 2) {
                    continue;
                }
                method.setAccessible(true);
                return (DensityFunction) method.invoke(null, input, type);
            }
            throw new IllegalStateException("DensityFunctions.map(DensityFunction, Mapped.Type) not found");
        } catch (ReflectiveOperationException error) {
            throw new IllegalStateException("failed to build Mapped density function", error);
        }
    }

    private static DensityFunction buildFunction(String[] parts) {
        if (parts.length == 0) {
            return null;
        }
        String kind = parts[0];
        if (kind.equals("const") && parts.length == 2) {
            return DensityFunctions.constant(Double.parseDouble(parts[1]));
        }
        if (kind.equals("ygrad") && parts.length == 5) {
            return DensityFunctions.yClampedGradient(
                    Integer.parseInt(parts[1]),
                    Integer.parseInt(parts[2]),
                    Double.parseDouble(parts[3]),
                    Double.parseDouble(parts[4]));
        }
        if (parts.length == 2) {
            DensityFunction input = DensityFunctions.constant(Double.parseDouble(parts[1]));
            // Mapped.Type 是 protected 的嵌套枚举，编译期无法引用类型，
            // 因此取枚举常量与调用 map 都走反射。取的是原版真实定义，不是复制一份枚举值。
            return mappedFunction(input, kind);
        }
        return null;
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int at = requested.lastIndexOf('@');
        if (at < 0) {
            return missing(requested);
        }
        String spec = requested.substring(0, at);
        int pointIndex;
        try {
            pointIndex = Integer.parseInt(requested.substring(at + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        if (pointIndex < 0 || pointIndex >= SAMPLE_POINTS.length) {
            return missing(requested);
        }

        DensityFunction function = buildFunction(spec.split("\\|", -1));
        if (function == null) {
            return missing(requested);
        }
        int[] point = SAMPLE_POINTS[pointIndex];
        DensityFunction.FunctionContext context = new DensityFunction.FunctionContext() {
            @Override
            public int blockX() {
                return point[0];
            }

            @Override
            public int blockY() {
                return point[1];
            }

            @Override
            public int blockZ() {
                return point[2];
            }
        };
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("value", doubleBits(function.compute(context)));
        output.addProperty("min", doubleBits(function.minValue()));
        output.addProperty("max", doubleBits(function.maxValue()));
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}