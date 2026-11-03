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
import net.minecraft.core.dispenser.DefaultDispenseItemBehavior;
import net.minecraft.core.dispenser.DispenseItemBehavior;
import net.minecraft.core.dispenser.OptionalDispenseItemBehavior;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.level.block.DispenserBlock;

/**
 * 发射器行为表的原版 oracle：每个物品对应的行为「种类标签」。
 *
 * <pre>
 *   java dev.horizonlab.diff.DispenseBehaviorOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么比对「标签」而不是类名</h2>
 *
 * 原版的 169 条注册里，109 条是 {@code DispenserBlock} 静态块中的**匿名子类**
 * （如 {@code new DefaultDispenseItemBehavior() { ... }}），类名是 {@code DispenserBlock$1}
 * 这样的序号，跨实现毫无可比性；Cubium 侧的类拆得也比原版细。
 *
 * 因此两侧都归一到少数几个**语义标签**，只比对「这个物品被当作哪一类处理」这个
 * 真正影响游戏行为的结论。标签定义见 {@link #kindOf}。
 *
 * <h2>successDefault</h2>
 *
 * 原版 {@code OptionalDispenseItemBehavior} 的 {@code isSuccess()} 初始为 false，
 * 只有子类真正做成事才置 true——决定**是否播发射音效、是否消耗物品**。
 * 普通行为恒为 true。这个布尔值是最容易被漏抄的一位语义。
 */
public final class DispenseBehaviorOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private DispenseBehaviorOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            BuiltInRegistries.ITEM.keySet().stream().map(Object::toString).sorted().forEach(units::add);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: DispenseBehaviorOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            String unit = obj.get("unit").getAsString();
            results.add(GSON.toJson(solve(unit)));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        var id = net.minecraft.resources.Identifier.tryParse(unit);
        var holder = id == null ? null : BuiltInRegistries.ITEM.get(id).orElse(null);
        if (holder == null) {
            out.addProperty("missing", true);
            return out;
        }

        DispenseItemBehavior behavior = DispenserBlock.DISPENSER_REGISTRY.get(holder.value());
        out.addProperty("registered", behavior != null);
        out.addProperty("kind", kindOf(behavior));
        out.addProperty("successDefault", !(behavior instanceof OptionalDispenseItemBehavior));
        return out;
    }

    /**
     * 把原版的行为实现归一成语义标签。
     *
     * <p>未注册返回 {@code none}；具名的独立行为类各成一类；其余（含全部匿名子类）
     * 统一为 {@code custom}。
     */
    private static String kindOf(DispenseItemBehavior behavior) {
        if (behavior == null) {
            return "none";
        }
        String name = behavior.getClass().getSimpleName();
        return switch (name) {
            case "ProjectileDispenseBehavior" -> "projectile";
            case "BoatDispenseItemBehavior" -> "boat";
            case "MinecartDispenseItemBehavior" -> "minecart";
            case "ShearsDispenseItemBehavior" -> "shears";
            case "ShulkerBoxDispenseBehavior" -> "shulker_box";
            case "EquipmentDispenseItemBehavior" -> "equipment";
            case "" -> "custom";
            default -> "custom";
        };
    }
}
