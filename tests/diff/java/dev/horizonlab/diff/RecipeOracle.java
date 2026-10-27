package dev.horizonlab.diff;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import net.minecraft.core.Holder;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.crafting.AbstractCookingRecipe;
import net.minecraft.world.item.crafting.CraftingInput;
import net.minecraft.world.item.crafting.SingleRecipeInput;
import net.minecraft.world.item.crafting.CustomRecipe;
import net.minecraft.world.item.crafting.Ingredient;
import net.minecraft.world.item.crafting.Recipe;
import net.minecraft.world.item.crafting.RecipeHolder;
import net.minecraft.world.item.crafting.RecipeManager;
import net.minecraft.world.item.crafting.ShapedRecipe;

/**
 * 配方加载结果的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RecipeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/recipe.cpp 同构：类型、分组、原料（每个原料的可选物品集合）、有序配方的
 * 宽高、结果物品与数量、烹饪时间/经验。特殊配方（`CustomRecipe` 子类）在数据包里只有
 * 类型，输出只含类型与序列化名。
 */
public final class RecipeOracle {
    private RecipeOracle() {}

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
        System.err.println("usage: RecipeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    /** 服务端加载到的全部配方 ID（字典序）。 */
    private static List<String> ids() {
        List<String> ids = new ArrayList<>();
        for (RecipeHolder<?> holder : LootServerData.get().resources.getRecipeManager().getRecipes()) {
            ids.add(holder.id().identifier().toString());
        }
        ids.sort(Comparator.naturalOrder());
        return ids;
    }

    private static void run(Path input, Path output) throws IOException {
        RecipeManager manager = LootServerData.get().resources.getRecipeManager();
        Map<Identifier, RecipeHolder<?>> byId = new HashMap<>();
        for (RecipeHolder<?> holder : manager.getRecipes()) {
            byId.put(holder.id().identifier(), holder);
        }

        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        StringBuilder out = new StringBuilder();
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String id = request.get("recipe").getAsString();
            out.append(summarise(id, byId.get(Identifier.parse(id)))).append('\n');
        }
        Files.writeString(output, out.toString(), StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String id, RecipeHolder<?> holder) {
        JsonObject result = new JsonObject();
        result.addProperty("recipe", id);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }
        Recipe<?> recipe = holder.value();
        result.addProperty("type", BuiltInRegistries.RECIPE_SERIALIZER.getKey(recipe.getSerializer()).toString());

        if (recipe instanceof CustomRecipe) {
            // 特殊配方：JSON 里只有 type，行为由子类实现。没有固定网格布局的（修复、
            // 染色、复制……）只在发放配方时存在，这里只给类型；有布局的（地图扩展）
            // 按普通的放置信息一并输出。
            List<Ingredient> placed = recipe.placementInfo().ingredients();
            if (placed.isEmpty()) {
                return result;
            }
            result.addProperty("group", recipe.group());
            result.add("ingredients", summariseIngredients(placed));
            if (recipe instanceof ShapedRecipe shaped) {
                result.addProperty("width", shaped.getWidth());
                result.addProperty("height", shaped.getHeight());
            }
            result.add("result", JsonNull.INSTANCE);
            return result;
        }

        result.addProperty("group", recipe.group());
        result.add("ingredients", summariseIngredients(recipe.placementInfo().ingredients()));
        if (recipe instanceof ShapedRecipe shaped) {
            result.addProperty("width", shaped.getWidth());
            result.addProperty("height", shaped.getHeight());
        }
        result.add("result", summariseResult(recipe));
        if (recipe instanceof AbstractCookingRecipe cooking) {
            result.addProperty("cookTime", cooking.cookingTime());
            result.addProperty("experience", (double) cooking.experience());
        }
        return result;
    }

    /**
     * 结果物品：对每个配方类型用对应的方法算出「给定原料能合成出什么」。
     *
     * 原版配方对象不存「声明的结果」，结果只在合成时算出来；两侧都跑一次真实合成，
     * 比对的就是玩家实际拿到的物品与数量。
     */
    private static JsonElement summariseResult(Recipe<?> recipe) {
        ItemStack stack = tryAssemble(recipe);
        if (stack == null || stack.isEmpty()) {
            return JsonNull.INSTANCE;
        }
        JsonObject json = new JsonObject();
        json.addProperty("id", BuiltInRegistries.ITEM.getKey(stack.getItem()).toString());
        json.addProperty("count", stack.getCount());
        return json;
    }

    @SuppressWarnings("unchecked")
    private static ItemStack tryAssemble(Recipe<?> recipe) {
        ServerLevel level = LootLevel.get().level;
        List<Ingredient> ingredients = recipe.placementInfo().ingredients();
        try {
            if (recipe instanceof net.minecraft.world.item.crafting.SingleItemRecipe) {
                // 熔炼 / 切石 / 烟熏 / 营火：单物品输入。
                ItemStack input = ingredients.isEmpty() ? ItemStack.EMPTY : firstOption(ingredients.get(0));
                return ((Recipe<net.minecraft.world.item.crafting.SingleRecipeInput>) recipe)
                    .assemble(new net.minecraft.world.item.crafting.SingleRecipeInput(input), level.registryAccess());
            }
            if (recipe instanceof net.minecraft.world.item.crafting.SmithingRecipe smithing) {
                java.util.Optional<Ingredient> template = optionalIngredient(recipe, "templateIngredient");
                Ingredient base = requiredIngredient(recipe, "baseIngredient");
                java.util.Optional<Ingredient> addition = optionalIngredient(recipe, "additionIngredient");
                var input = new net.minecraft.world.item.crafting.SmithingRecipeInput(
                    optionStack(template), firstOption(base), optionStack(addition));
                return ((Recipe<net.minecraft.world.item.crafting.SmithingRecipeInput>) recipe)
                    .assemble(input, level.registryAccess());
            }
            int width = 3;
            int height = 3;
            if (recipe instanceof ShapedRecipe shaped) {
                width = shaped.getWidth();
                height = shaped.getHeight();
            }
            List<ItemStack> grid = new ArrayList<>();
            for (int index = 0; index < width * height; index++) {
                grid.add(index < ingredients.size() ? firstOption(ingredients.get(index)) : ItemStack.EMPTY);
            }
            return ((Recipe<CraftingInput>) recipe)
                .assemble(CraftingInput.of(width, height, grid), level.registryAccess());
        } catch (final RuntimeException ignored) {
            // 代表物品凑不出合法组合（例如需要特定组件）：结果按 null 输出，两侧同样处理。
            return ItemStack.EMPTY;
        }
    }

    private static ItemStack firstOption(Ingredient ingredient) {
        return ingredient.items().findFirst().map(holder -> new ItemStack(holder.value())).orElse(ItemStack.EMPTY);
    }

    private static ItemStack optionStack(java.util.Optional<Ingredient> ingredient) {
        return ingredient.map(RecipeOracle::firstOption).orElse(ItemStack.EMPTY);
    }

    private static Ingredient requiredIngredient(Recipe<?> recipe, String accessor) {
        try {
            var method = recipe.getClass().getMethod(accessor);
            return (Ingredient) method.invoke(recipe);
        } catch (final ReflectiveOperationException error) {
            throw new IllegalStateException("配方类型缺少 " + accessor, error);
        }
    }

    @SuppressWarnings("unchecked")
    private static java.util.Optional<Ingredient> optionalIngredient(Recipe<?> recipe, String accessor) {
        try {
            var method = recipe.getClass().getMethod(accessor);
            return (java.util.Optional<Ingredient>) method.invoke(recipe);
        } catch (final ReflectiveOperationException error) {
            throw new IllegalStateException("配方类型缺少 " + accessor, error);
        }
    }

    private static JsonArray summariseIngredients(List<Ingredient> ingredients) {
        JsonArray array = new JsonArray();
        for (Ingredient ingredient : ingredients) {
            array.add(summariseIngredient(ingredient));
        }
        return array;
    }

    private static JsonObject summariseIngredient(Ingredient ingredient) {
        JsonObject json = new JsonObject();
        JsonArray items = new JsonArray();
        ingredient.items()
            .map(Holder::value)
            .map(item -> BuiltInRegistries.ITEM.getKey(item).toString())
            .distinct()
            .sorted()
            .forEach(items::add);
        json.add("items", items);
        return json;
    }
}