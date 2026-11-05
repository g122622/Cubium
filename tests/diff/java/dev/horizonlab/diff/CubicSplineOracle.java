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
import net.minecraft.util.CubicSpline;
import net.minecraft.util.KeyDispatchDataCodec;
import net.minecraft.world.level.levelgen.DensityFunction;
import net.minecraft.world.level.levelgen.DensityFunctions;

/**
 * 三次样条的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CubicSplineOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.CubicSplineOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接用原版的 {@link CubicSpline.Builder} 配 {@link DensityFunctions#spline}，
 * 不重写 Hermite 公式。
 *
 * <p><b>关键点：原版整条样条链是 {@code float}。</b>
 * {@code CubicSpline.Multipoint} 的 {@code locations}、{@code derivatives}、
 * {@code minValue}/{@code maxValue} 全是 {@code float}，{@code apply} 内部也全程
 * {@code float} 运算。Cubium 侧用的是 {@code f64}。
 *
 * <p>为把任意坐标喂进样条，这里用一个「返回固定坐标值」的密度函数包裹成
 * {@code Spline.Coordinate}——{@code Coordinate.apply} 会调用它的 {@code compute}。
 */
public final class CubicSplineOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private CubicSplineOracle() {}

    private static String doubleBits(double value) {
        return String.format("0x%016x", Double.doubleToRawLongBits(value));
    }

    /** 与探针侧逐项一致：{location, value, derivative}。 */
    private record SplineCase(String id, double[][] points) {}

    private static List<SplineCase> splineCases() {
        List<SplineCase> list = new ArrayList<>();
        list.add(new SplineCase("linear2", new double[][] {{0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}}));
        list.add(new SplineCase("flat3", new double[][] {{-1.0, 0.5, 0.0}, {0.0, -0.25, 0.0}, {1.0, 0.75, 0.0}}));
        list.add(new SplineCase("hermite3", new double[][] {{-1.0, 0.5, 0.25}, {0.0, -0.25, 0.5}, {1.0, 0.75, -0.125}}));
        list.add(new SplineCase("uneven", new double[][] {{0.0, 0.0, 1.0}, {0.1, 0.6, -0.5}, {2.0, -0.3, 0.2}}));
        list.add(new SplineCase("fractional", new double[][] {{0.1, 0.5, 0.125}, {0.3, -0.5, 0.25}, {0.7, 1.5, -0.375}}));
        list.add(new SplineCase("offset_like", new double[][] {
            {-1.0, 2.5, 0.0}, {-0.35, 1.2, 0.0}, {-0.18, -0.4, 0.0}, {0.25, 0.9, 0.0}
        }));
        list.add(new SplineCase("descending", new double[][] {{-2.0, 1.0, -0.5}, {0.0, 0.0, -0.25}, {2.0, -1.0, 0.0}}));
        list.add(new SplineCase("single", new double[][] {{0.5, 0.25, 0.75}}));
        return list;
    }

    /** 采样输入值，与探针侧逐项一致。 */
    private static final double[] SAMPLE_INPUTS = {
        -3.0, -2.0, -1.0, -0.5, -0.35, -0.18, -0.1, 0.0, 0.05, 0.1, 0.2, 0.3,
        0.5, 0.7, 1.0, 1.5, 2.0, 3.0,
    };

    /** 提供一个固定坐标值的密度函数，用来把任意坐标喂进样条。 */
    private static final class FixedCoordinateFunction implements DensityFunction.SimpleFunction {
        private final double value;

        FixedCoordinateFunction(double value) {
            this.value = value;
        }

        @Override
        public double compute(DensityFunction.FunctionContext context) {
            return this.value;
        }

        @Override
        public double minValue() {
            return this.value;
        }

        @Override
        public double maxValue() {
            return this.value;
        }

        @Override
        public KeyDispatchDataCodec<? extends DensityFunction> codec() {
            throw new UnsupportedOperationException("oracle-only density function");
        }
    }

    /** 用原版的 CubicSpline.builder 构造样条（保持 float 语义）。 */
    private static DensityFunction buildSpline(SplineCase splineCase, float coordinateValue) {
        DensityFunction coordinateFunction = new FixedCoordinateFunction(coordinateValue);
        DensityFunctions.Spline.Coordinate coordinate =
                new DensityFunctions.Spline.Coordinate(net.minecraft.core.Holder.direct(coordinateFunction));
        CubicSpline.Builder<DensityFunctions.Spline.Point, DensityFunctions.Spline.Coordinate> builder =
                CubicSpline.builder(coordinate);
        double[][] points = splineCase.points();
        for (double[] point : points) {
            // addPoint(float, float, float) 是公开重载：原版会把 value 包成 Constant 并全程 float。
            builder.addPoint((float) point[0], (float) point[1], (float) point[2]);
        }
        return DensityFunctions.spline(builder.build());
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (SplineCase splineCase : splineCases()) {
                for (int i = 0; i < SAMPLE_INPUTS.length; i++) {
                    text.append(splineCase.id()).append('@').append(i).append('\n');
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

        throw new IllegalArgumentException("usage: CubicSplineOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int at = requested.lastIndexOf('@');
        if (at < 0) {
            return missing(requested);
        }
        String caseId = requested.substring(0, at);
        int index;
        try {
            index = Integer.parseInt(requested.substring(at + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        if (index < 0 || index >= SAMPLE_INPUTS.length) {
            return missing(requested);
        }

        for (SplineCase splineCase : splineCases()) {
            if (!caseId.equals(splineCase.id())) {
                continue;
            }
            double coordinate = SAMPLE_INPUTS[index];
            DensityFunction function = buildSpline(splineCase, (float) coordinate);
            DensityFunction.FunctionContext context = new DensityFunction.FunctionContext() {
                @Override
                public int blockX() {
                    return 0;
                }

                @Override
                public int blockY() {
                    return 0;
                }

                @Override
                public int blockZ() {
                    return 0;
                }
            };
            JsonObject output = new JsonObject();
            output.addProperty("unit", requested);
            output.addProperty("value", doubleBits(function.compute(context)));
            output.addProperty("min", doubleBits(function.minValue()));
            output.addProperty("max", doubleBits(function.maxValue()));
            return GSON.toJson(output);
        }
        return missing(requested);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}