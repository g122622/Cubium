package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.ItemCooldowns;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;

/**
 * 物品冷却追踪（原版 {@code ItemCooldowns}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemCooldownsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>原版只提供 {@code addCooldown(ItemStack|Identifier, int)} 与
 * {@code getCooldownPercent(ItemStack, float)}，没有按分组查询的形状；因此这里统一用
 * 一个真实物品（末影珍珠）作为冷却键，探针侧用它的物品 ID 作为分组名，
 * 两侧的键空间因此等价。
 *
 * <p>只输出原版直接提供的两个量：进度百分比与是否仍在冷却。「剩余 tick」没有原版 getter，
 * 用旁路推算会让 harness 自己成为参照，因此不输出——进度已经足以定位所有公式错误。
 */
public final class ItemCooldownsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ItemCooldownsOracle() {}

    /**
     * 一个真实的冷却键：末影珍珠。
     *
     * 必须惰性创建：类初始化早于 {@code Bootstrap.bootStrap()}，静态字段里直接造
     * {@code ItemStack} 会在 {@code Item.<clinit>} 触发「registry Not bootstrapped」。
     */
    private static ItemStack key() {
        return new ItemStack(Items.ENDER_PEARL);
    }

    private record Step(String kind, int ticks, float partial) {}

    private record Case(String name, List<Step> steps) {}

    private static Step tick() {
        return new Step("tick", 0, 0.0f);
    }

    private static Step set(int ticks) {
        return new Step("set", ticks, 0.0f);
    }

    private static Step remove() {
        return new Step("remove", 0, 0.0f);
    }

    private static Step query(float partial) {
        return new Step("query", 0, partial);
    }

    private static List<Step> ticks(int count) {
        List<Step> steps = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            steps.add(tick());
        }
        return steps;
    }

    private static List<Step> concat(List<Step> first, List<Step> second, List<Step> third) {
        List<Step> all = new ArrayList<>(first);
        all.addAll(second);
        all.addAll(third);
        return all;
    }

    private static final List<Case> CASES = List.of(
        new Case("set_then_query", List.of(set(20), query(0.0f))),
        new Case("set_advance_half", concat(List.of(set(20)), ticks(10), List.of(query(0.0f)))),
        new Case("expire_exactly", concat(List.of(set(20)), ticks(20), List.of(query(0.0f)))),
        new Case("expire_past", concat(List.of(set(5)), ticks(6), List.of(query(0.0f)))),
        new Case("partial_ticks", List.of(set(20), tick(), query(0.0f), query(0.5f), query(0.25f), query(0.75f))),
        new Case("partial_beyond_end", List.of(set(4), query(3.5f), query(4.0f), query(10.0f))),
        new Case("partial_negative", List.of(set(10), query(-1.0f), query(-0.5f))),
        new Case("reset_cooldown", List.of(set(20), tick(), tick(), tick(), set(10), query(0.0f), tick(), query(0.0f))),
        new Case("zero_cooldown", List.of(set(0), query(0.0f))),
        new Case("negative_cooldown", List.of(set(-5), query(0.0f))),
        new Case("remove_after_set", List.of(set(20), query(0.0f), remove(), query(0.0f))),
        new Case("query_without_set", List.of(query(0.0f))),
        new Case("very_long", List.of(set(72000), query(0.0f), tick(), query(0.0f))),
        new Case("one_tick", List.of(set(1), query(0.0f), query(0.5f), tick(), query(0.0f)))
    );

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (Case caseItem : CASES) {
                text.append(caseItem.name()).append('\n');
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

        throw new IllegalArgumentException("usage: ItemCooldownsOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        Case caseItem =
            CASES.stream().filter(candidate -> candidate.name().equals(requested)).findFirst().orElse(null);
        if (caseItem == null) {
            return missing(requested);
        }

        ItemCooldowns cooldowns = new ItemCooldowns();
        Identifier group = groupOf(key());

        JsonArray states = new JsonArray();
        for (Step step : caseItem.steps()) {
            switch (step.kind()) {
                case "tick" -> cooldowns.tick();
                case "set" -> cooldowns.addCooldown(group, step.ticks());
                case "remove" -> cooldowns.removeCooldown(group);
                default -> {
                    JsonObject state = new JsonObject();
                    JsonFloats.add(state, "progress", cooldowns.getCooldownPercent(key(), step.partial()));
                    state.addProperty("active", cooldowns.isOnCooldown(key()));
                    states.add(state);
                }
            }
        }

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.add("states", states);
        return GSON.toJson(output);
    }

    /** 冷却分组：物品没带 use_cooldown 组件时就是它自己的 ID。 */
    private static Identifier groupOf(ItemStack stack) {
        return BuiltInRegistries.ITEM.getKey(stack.getItem());
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}