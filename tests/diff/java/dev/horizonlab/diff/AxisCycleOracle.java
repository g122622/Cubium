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
import net.minecraft.core.AxisCycle;
import net.minecraft.core.Direction;

/**
 * {@code AxisCycle} 三轴循环映射的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AxisCycleOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 纯计算能力：不加载任何数据包，只调 {@code Bootstrap.bootStrap()} 以便统一环境。
 */
public final class AxisCycleOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 三个循环名，下标与 {@link AxisCycle#ordinal()} 一致。 */
    private static final String[] CYCLES = {"none", "forward", "backward"};
    /** 三个轴名，下标与 {@link Direction.Axis#ordinal()} 一致。 */
    private static final String[] AXES = {"x", "y", "z"};

    private AxisCycleOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            Path out = Path.of(args[1]);
            Files.write(out, buildUnits(), StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: AxisCycleOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    /** 与 C++ 探针 {@code writeList} 完全同构地枚举单元，顺序一致。 */
    private static List<String> buildUnits() {
        List<String> units = new ArrayList<>();

        // 1) cycle(Axis)：3 循环 × 3 轴。
        for (String cycle : CYCLES) {
            for (String axis : AXES) {
                units.add("axis:" + cycle + "|" + axis);
            }
        }
        // 2) inverse()：3 循环。
        for (String cycle : CYCLES) {
            units.add("inverse:" + cycle);
        }
        // 3) cycle(int,int,int,Axis)：3 循环 × 6 组分量 × 3 轴。
        String[] coords = {"1|2|3", "3|2|1", "0|0|0", "-1|-2|-3", "7|0|-5", "100|200|300"};
        for (String cycle : CYCLES) {
            for (String coord : coords) {
                for (String axis : AXES) {
                    units.add("coord:" + cycle + "|" + coord + "|" + axis);
                }
            }
        }
        // 4) between(from,to)：3 × 3。原版没有这个方法，由 cycle(Axis) 关系推出：
        //    between(from,to) 就是「把 from 映射成 to」所需的循环。
        for (String from : AXES) {
            for (String to : AXES) {
                units.add("between:" + from + "|" + to);
            }
        }
        return units;
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            String unit = obj.get("unit").getAsString();
            results.add(GSON.toJson(solve(unit)));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        // 单元形态是 `<kind>:<参数>|<参数>|...`，前缀与第一个参数用 `:` 相连，
        // 因此先按 `:` 拆出 kind，再按 `|` 拆参数（与 C++ 探针完全同构）。
        String[] parts = unit.split("\\|", -1);
        int colon = parts[0].indexOf(':');
        String kind = parts[0].substring(0, colon);
        List<String> args = new ArrayList<>();
        args.add(parts[0].substring(colon + 1));
        for (int i = 1; i < parts.length; i++) {
            args.add(parts[i]);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (kind) {
            case "axis" -> {
                AxisCycle cycle = parseCycle(args.get(0));
                Direction.Axis axis = parseAxis(args.get(1));
                out.addProperty("value", AXES[cycle.cycle(axis).ordinal()]);
            }
            case "inverse" -> out.addProperty("value", CYCLES[parseCycle(args.get(0)).inverse().ordinal()]);
            case "coord" -> {
                AxisCycle cycle = parseCycle(args.get(0));
                int x = Integer.parseInt(args.get(1));
                int y = Integer.parseInt(args.get(2));
                int z = Integer.parseInt(args.get(3));
                Direction.Axis axis = parseAxis(args.get(4));
                out.addProperty("value", cycle.cycle(x, y, z, axis));
            }
            case "between" -> {
                Direction.Axis from = parseAxis(args.get(0));
                Direction.Axis to = parseAxis(args.get(1));
                out.addProperty("value", CYCLES[between(from, to).ordinal()]);
            }
            default -> throw new IllegalArgumentException("unknown unit: " + unit);
        }
        return out;
    }

    /**
     * 原版没有 {@code between}，按定义实现：把 {@code from} 经循环映成 {@code to} 的那个循环。
     *
     * <p>遍历三个循环找出唯一满足 {@code cycle.cycle(from) == to} 的一个。
     */
    private static AxisCycle between(Direction.Axis from, Direction.Axis to) {
        for (AxisCycle cycle : AxisCycle.values()) {
            if (cycle.cycle(from) == to) {
                return cycle;
            }
        }
        throw new IllegalStateException("no axis cycle maps " + from + " to " + to);
    }

    private static AxisCycle parseCycle(String name) {
        return switch (name) {
            case "none" -> AxisCycle.NONE;
            case "forward" -> AxisCycle.FORWARD;
            case "backward" -> AxisCycle.BACKWARD;
            default -> throw new IllegalArgumentException("unknown cycle: " + name);
        };
    }

    private static Direction.Axis parseAxis(String name) {
        return switch (name) {
            case "x" -> Direction.Axis.X;
            case "y" -> Direction.Axis.Y;
            case "z" -> Direction.Axis.Z;
            default -> throw new IllegalArgumentException("unknown axis: " + name);
        };
    }
}
