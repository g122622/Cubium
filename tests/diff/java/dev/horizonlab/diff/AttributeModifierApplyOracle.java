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
import net.minecraft.world.entity.ai.attributes.AttributeInstance;
import net.minecraft.world.entity.ai.attributes.AttributeModifier;
import net.minecraft.world.entity.ai.attributes.RangedAttribute;

/**
 * 属性修饰符三阶段计算的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AttributeModifierApplyOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 直接构造 {@link RangedAttribute} 与 {@link AttributeInstance}，走原版真实的
 * {@code calculateValue} 实现，输出最终值的位模式。
 */
public final class AttributeModifierApplyOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与 C++ 探针 writeList 完全同构的场景表。 */
    private static final String[] SCENES = {
        "10|-|0|100",
        "10|add:5|0|100",
        "10|base:0.5|0|100",
        "10|total:0.5|0|100",
        "10|add:5;base:0.5|0|100",
        "10|add:100;base:0.5|0|1000",
        "0|add:10;base:1.0|0|100",
        "10|total:0.5;total:0.5|0|100",
        "10|total:1;total:1;total:1|0|100",
        "10|total:0.25;total:0;total:0.25|0|100",
        "10|add:5;base:0.5;total:0.5|0|100",
        "20|add:-5;base:-0.5;total:-0.5|0|100",
        "10|add:1000|0|100",
        "10|add:-1000|0|100",
        "3|add:1000|-5|5",
        "100|add:0|0|100",
        "0.1|add:0.2|0|100",
        "1|total:0.1;total:0.1|0|100",
        "3|add:0.1;add:0.2;base:0.3|0|100",
        "10|add:nan|0|100",
        "10|add:inf|0|100",
        "10|total:inf|0|100",
    };

    private AttributeModifierApplyOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>(List.of(SCENES));
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: AttributeModifierApplyOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            results.add(GSON.toJson(solve(obj.get("unit").getAsString())));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String[] parts = unit.split("\\|", -1);
        double base = parseNumber(parts[0]);
        double min = parseNumber(parts[2]);
        double max = parseNumber(parts[3]);

        RangedAttribute attribute = new RangedAttribute("minecraft:test", base, min, max);
        // 原版构造器要 Holder<Attribute>；用 Holder.direct 包一层即可，不注册进任何注册表。
        AttributeInstance instance =
            new AttributeInstance(net.minecraft.core.Holder.direct(attribute), inst -> {});
        instance.setBaseValue(base);

        if (!parts[1].equals("-")) {
            int index = 0;
            for (String token : parts[1].split(";", -1)) {
                if (token.isEmpty()) {
                    continue;
                }
                String[] pieces = token.split(":", -1);
                AttributeModifier.Operation operation = switch (pieces[0]) {
                    case "add" -> AttributeModifier.Operation.ADD_VALUE;
                    case "base" -> AttributeModifier.Operation.ADD_MULTIPLIED_BASE;
                    case "total" -> AttributeModifier.Operation.ADD_MULTIPLIED_TOTAL;
                    default -> throw new IllegalArgumentException("unknown op: " + pieces[0]);
                };
                instance.addTransientModifier(
                    new AttributeModifier(
                        net.minecraft.resources.Identifier.parse("test:" + index),
                        parseNumber(pieces[1]),
                        operation));
                index++;
            }
        }

        out.addProperty("value", bits(instance.getValue()));
        return out;
    }

    private static double parseNumber(String text) {
        return switch (text) {
            case "nan" -> Double.NaN;
            case "inf" -> Double.POSITIVE_INFINITY;
            case "-inf" -> Double.NEGATIVE_INFINITY;
            default -> Double.parseDouble(text);
        };
    }

    /** 浮点位模式，格式与 C++ 侧 "0x%016llx" 完全一致。 */
    private static String bits(double value) {
        return String.format("0x%016x", Double.doubleToRawLongBits(value));
    }
}
