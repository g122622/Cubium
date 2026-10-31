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
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.item.crafting.AbstractCookingRecipe;
import net.minecraft.world.item.crafting.Recipe;
import net.minecraft.world.item.crafting.RecipeHolder;
import net.minecraft.world.item.crafting.RecipeType;
import net.minecraft.world.item.crafting.SingleRecipeInput;
import net.minecraft.world.item.crafting.StonecutterRecipe;

/**
 * **单物品配方查询语义**的原版 oracle（熔炼 / 高炉 / 烟熏 / 营火 / 切石）。
 *
 * <pre>
 *   java dev.horizonlab.diff.SingleItemRecipeOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.SingleItemRecipeOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/single_item_recipe.cpp 同构：单元是**输入物品**，两侧各自回答「这个物品
 * 在每种炉子里会烧出什么、能切出哪些东西」。
 *
 * <h2>为什么不复用按 ID 比对的 recipe 能力</h2>
 *
 * <p>{@code RecipeOracle} 逐个配方导出类型、原料、结果。它回答不了玩家的问题：输入
 * 可能匹配**多条**配方，查询只返回第一条，返回哪一条取决于注册顺序；配方又按类型
 * 分桶，桶装错会让玩家在高炉里烧出生肉。这些只在「查询」这一层才暴露出来。
 *
 * <h2>与探针严格对齐的口径</h2>
 *
 * <ul>
 *   <li>输入物品堆的数量固定为 1——原版 {@link SingleRecipeInput} 只取一个物品，
 *       匹配不看数量；</li>
 *   <li>切石返回**整张列表**（原版切石机界面列出全部可切产物），两侧都按配方 ID
 *       排序后输出，比对的只是「有哪些条目」；</li>
 *   <li>烧制返回**单条**（与原版 {@code RecipeManager.getRecipeFor} 的 findFirst 一致），
 *       因此注册顺序真的会影响结果——这正是要比的地方。</li>
 * </ul>
 */
public final class SingleItemRecipeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 四种烧制类型：输出字段名 + 原版配方类型。 */
    private record CookingKind(String field, RecipeType<?> type) {}

    /**
     * 四种烧制类型。
     *
     * <p>不能写成静态字段的字面量初始化：{@code RecipeType.SMELTING} 等常量在读取时
     * 会触碰原版注册表，而那时 {@code Bootstrap.bootStrap()} 还没跑（它在
     * {@code LootServerData.get()} 里）。静态初始化阶段读它们会直接抛
     * "Not bootstrapped"。改在首次使用时构造。
     */
    private static List<CookingKind> cookingKinds() {
        return List.of(
            new CookingKind("furnace", RecipeType.SMELTING),
            new CookingKind("blast", RecipeType.BLASTING),
            new CookingKind("smoke", RecipeType.SMOKING),
            new CookingKind("campfire", RecipeType.CAMPFIRE_COOKING));
    }

    private SingleItemRecipeOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String id : inputItemIds()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            LootServerData data = LootServerData.get();
            run(data, Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: SingleItemRecipeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    /**
     * 全部单物品配方的原料并集（排序去重）。
     *
     * <p>按输入物品而不是配方枚举：一个物品可能同时是好几条配方的原料，按物品枚举
     * 才不会把同一件事比对两遍。标签原料展开成成员——玩家手里的是具体物品。
     */
    private static List<String> inputItemIds() {
        Set<String> ids = new TreeSet<>();
        for (RecipeHolder<?> holder : LootServerData.get().resources.getRecipeManager().getRecipes()) {
            Recipe<?> recipe = holder.value();
            if (recipe instanceof AbstractCookingRecipe || recipe instanceof StonecutterRecipe) {
                recipe.placementInfo().ingredients().forEach(ingredient ->
                    ingredient.items().forEach(item ->
                        ids.add(item.unwrapKey().orElseThrow().identifier().toString())));
            }
        }
        return new ArrayList<>(ids);
    }

    private static void run(LootServerData data, Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            text.append(GSON.toJson(evaluate(data, request.get("unit").getAsString()))).append('\n');
        }
        Files.writeString(output, text.toString(), StandardCharsets.UTF_8);
    }

    private static JsonObject evaluate(LootServerData data, String unitId) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unitId);

        Item item = data.registries.compositeAccess().lookupOrThrow(Registries.ITEM)
            .get(ResourceKey.create(Registries.ITEM, Identifier.parse(unitId)))
            .map(entry -> entry.value())
            .orElse(null);
        if (item == null || item == Items.AIR) {
            result.addProperty("ok", false);
            result.addProperty("error", "unknown item");
            return result;
        }

        ItemStack stack = new ItemStack(item, 1);
        SingleRecipeInput recipeInput = new SingleRecipeInput(stack);
        var level = LootLevel.get().level;
        var manager = data.resources.getRecipeManager();
        var lookup = data.resources.fullRegistries().lookup();

        result.addProperty("ok", true);

        // 烧制：与原版玩家实际调用一致——`getRecipeFor` 走到 findFirst。
        for (CookingKind kind : cookingKinds()) {
            result.add(kind.field(), cookingFor(kind, recipeInput, level, lookup));
        }

        // 切石：整张列表。原版 `getRecipesFor` 的顺序由注册表决定，这里与探针一样
        // 按配方 ID 排序，比对的是集合本身。
        JsonArray stonecutting = new JsonArray();
        for (RecipeHolder<StonecutterRecipe> holder : stonecuttingFor(recipeInput, level)) {
            stonecutting.add(summariseStonecutting(holder, recipeInput, lookup));
        }
        List<JsonElement> entries = new ArrayList<>();
        stonecutting.forEach(entries::add);
        entries.sort(Comparator.comparing(entry -> entry.getAsJsonObject().get("recipe").getAsString()));
        JsonArray sorted = new JsonArray();
        entries.forEach(sorted::add);
        result.add("stonecutting", sorted);

        return result;
    }

    /**
     * 一个输入在某种炉子里的查询结果。
     *
     * <p>用四个具名分支而不是对 `RecipeType<?>` 做强制转换：原版 {@code getRecipeFor}
     * 的返回类型由配方类型参数决定，通配符擦除后拿不到具体的 {@code RecipeHolder<T>}。
     * 这里按类型分派，每种都是确定的泛型实例。
     */
    private static JsonElement cookingFor(
        CookingKind kind,
        SingleRecipeInput input,
        net.minecraft.server.level.ServerLevel level,
        net.minecraft.core.HolderLookup.Provider lookup) {
        var manager = LootServerData.get().resources.getRecipeManager();
        if (kind.type() == RecipeType.SMELTING) {
            var found = manager.getRecipeFor(RecipeType.SMELTING, input, level);
            return found.map(holder -> summariseCooking(holder, input, lookup)).orElse(JsonNull.INSTANCE);
        }
        if (kind.type() == RecipeType.BLASTING) {
            var found = manager.getRecipeFor(RecipeType.BLASTING, input, level);
            return found.map(holder -> summariseCooking(holder, input, lookup)).orElse(JsonNull.INSTANCE);
        }
        if (kind.type() == RecipeType.SMOKING) {
            var found = manager.getRecipeFor(RecipeType.SMOKING, input, level);
            return found.map(holder -> summariseCooking(holder, input, lookup)).orElse(JsonNull.INSTANCE);
        }
        var found = manager.getRecipeFor(RecipeType.CAMPFIRE_COOKING, input, level);
        return found.map(holder -> summariseCooking(holder, input, lookup)).orElse(JsonNull.INSTANCE);
    }

    /**
     * 该输入的全部切石配方（原版切石机界面列出的就是这一批）。
     *
     * <p>原版 `RecipeManager#getRecipesFor` 不对外公开（在内部的 `RecipeMap` 上），
     * 它的实现就是「在该类型的桶里过滤 `matches`」。这里遍历 `getRecipes()` 做同样的
     * 过滤——判定条件与原版一致，只是不依赖那个私有入口。
     */
    @SuppressWarnings("unchecked")
    private static List<RecipeHolder<StonecutterRecipe>> stonecuttingFor(
        SingleRecipeInput input, net.minecraft.server.level.ServerLevel level) {
        List<RecipeHolder<StonecutterRecipe>> matches = new ArrayList<>();
        for (RecipeHolder<?> holder : LootServerData.get().resources.getRecipeManager().getRecipes()) {
            if (!(holder.value() instanceof StonecutterRecipe stonecutter)) {
                continue;
            }
            // 具体类型上的 matches 收具体输入，绕过 `Recipe<?>` 的捕获变量约束。
            if (stonecutter.matches(input, level)) {
                matches.add((RecipeHolder<StonecutterRecipe>) holder);
            }
        }
        return matches;
    }

    private static JsonElement summariseCooking(
        RecipeHolder<? extends AbstractCookingRecipe> holder, SingleRecipeInput input, net.minecraft.core.HolderLookup.Provider lookup) {
        JsonObject entry = new JsonObject();
        entry.addProperty("recipe", holder.id().identifier().toString());
        entry.add("result", summariseStack(holder.value().assemble(input, lookup)));
        entry.addProperty("group", holder.value().group());
        AbstractCookingRecipe cooking = (AbstractCookingRecipe) holder.value();
        entry.addProperty("cookTime", cooking.cookingTime());
        entry.addProperty("experience", (double) cooking.experience());
        return entry;
    }

    private static JsonElement summariseStonecutting(
        RecipeHolder<StonecutterRecipe> holder, SingleRecipeInput input, net.minecraft.core.HolderLookup.Provider lookup) {
        JsonObject entry = new JsonObject();
        entry.addProperty("recipe", holder.id().identifier().toString());
        entry.add("result", summariseStack(holder.value().assemble(input, lookup)));
        entry.addProperty("group", holder.value().group());
        return entry;
    }

    /** 与探针 {@code summariseStack} 同口径：只给 id 与 count，空堆为 null。 */
    private static JsonElement summariseStack(ItemStack stack) {
        if (stack.isEmpty()) {
            return JsonNull.INSTANCE;
        }
        JsonObject entry = new JsonObject();
        entry.addProperty("id", stack.getItemHolder().unwrapKey().orElseThrow().identifier().toString());
        entry.addProperty("count", stack.getCount());
        return entry;
    }
}