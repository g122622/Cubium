package dev.horizonlab.diff;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonPrimitive;

/**
 * 进度（advancement）定义的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AdvancementOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/advancement.cpp 同构：比对的是**定义本身**（父进度、显示信息、条件、
 * 需求、奖励），不是玩家的完成状态。输出 {@code {"advancement": ..., ...}}。
 */
public final class AdvancementOracle {
    private AdvancementOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String id : ids()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: AdvancementOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    /** 服务端加载到的全部进度 ID（字典序）。 */
    private static List<String> ids() {
        var advancements = LootServerData.get().resources.getAdvancements().getAllAdvancements();
        List<String> ids = new ArrayList<>();
        for (var holder : advancements) {
            ids.add(holder.id().toString());
        }
        ids.sort(Comparator.naturalOrder());
        return ids;
    }

    private static void run(Path input, Path output) throws IOException {
        var resources = LootServerData.get().resources;
        var byId = new java.util.HashMap<String, net.minecraft.advancements.AdvancementHolder>();
        for (var holder : resources.getAdvancements().getAllAdvancements()) {
            byId.put(holder.id().toString(), holder);
        }

        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        StringBuilder out = new StringBuilder();
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String id = request.get("advancement").getAsString();
            out.append(summarise(id, byId.get(id))).append('\n');
        }
        Files.writeString(output, out.toString(), StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String id, net.minecraft.advancements.AdvancementHolder holder) {
        JsonObject result = new JsonObject();
        result.addProperty("advancement", id);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }
        var advancement = holder.value();
        result.add("parent",
            advancement.parent().map(p -> (JsonElement) new JsonPrimitive(p.toString())).orElse(JsonNull.INSTANCE));

        JsonObject display = new JsonObject();
        Optional<net.minecraft.advancements.DisplayInfo> info = advancement.display();
        if (info.isPresent()) {
            var d = info.get();
            JsonObject icon = new JsonObject();
            var stack = d.getIcon();
            if (!stack.isEmpty()) {
                icon.addProperty("id", net.minecraft.core.registries.BuiltInRegistries.ITEM.getKey(stack.getItem()).toString());
                icon.addProperty("count", stack.getCount());
            }
            display.add("icon", icon);
            display.add("title", text(d.getTitle()));
            display.add("description", text(d.getDescription()));
            display.addProperty("frame", d.getType().getSerializedName());
            display.add("background",
                d.getBackground()
                    .map(bg -> (JsonElement) new JsonPrimitive(bg.texturePath().toString()))
                    .orElse(JsonNull.INSTANCE));
            display.addProperty("toast", d.shouldShowToast());
            display.addProperty("announce", d.shouldAnnounceChat());
            display.addProperty("hidden", d.isHidden());
            result.add("display", display);
        } else {
            result.add("display", JsonNull.INSTANCE);
        }

        JsonObject criteria = new JsonObject();
        advancement.criteria().forEach((name, criterion) -> {
            // 触发器类型 = 注册表键。
            String trigger = net.minecraft.core.registries.BuiltInRegistries.TRIGGER_TYPES
                .getKey(criterion.trigger())
                .toString();
            criteria.addProperty(name, trigger);
        });
        result.add("criteria", criteria);

        JsonArray requirements = new JsonArray();
        for (List<String> group : advancement.requirements().requirements()) {
            JsonArray inner = new JsonArray();
            group.forEach(inner::add);
            requirements.add(inner);
        }
        result.add("requirements", requirements);

        var rewards = advancement.rewards();
        JsonObject rewardsJson = new JsonObject();
        rewardsJson.addProperty("experience", rewards.experience());
        // ResourceKey#location 已在 1.21 移除：表/配方的资源键本身就是注册名。
        rewardsJson.add("loot", sortedStrings(rewards.loot().stream().map(key -> key.identifier().toString()).toList()));
        rewardsJson.add("recipes",
            sortedStrings(rewards.recipes().stream().map(key -> key.identifier().toString()).toList()));
        rewardsJson.add("function",
            rewards.function().map(f -> (JsonElement) new JsonPrimitive(f.getId().toString())).orElse(JsonNull.INSTANCE));
        result.add("rewards", rewardsJson);
        return result;
    }

    private static JsonArray sortedStrings(List<String> values) {
        List<String> sorted = new ArrayList<>(values);
        sorted.sort(Comparator.naturalOrder());
        JsonArray array = new JsonArray();
        sorted.forEach(array::add);
        return array;
    }

    /**
     * 文本组件：翻译文本给 `{"translate": key}`，其余给 `{"text": 纯文本}`。
     */
    private static JsonObject text(net.minecraft.network.chat.Component component) {
        JsonObject result = new JsonObject();
        if (component.getContents() instanceof net.minecraft.network.chat.contents.TranslatableContents translatable) {
            result.addProperty("translate", translatable.getKey());
        } else {
            result.addProperty("text", component.getString());
        }
        return result;
    }
}