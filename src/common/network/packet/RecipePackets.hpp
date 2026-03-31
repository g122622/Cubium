#pragma once

#include "../../core/Types.hpp"
#include "../../core/Result.hpp"
#include "PacketSerializer.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../item/core/ItemStack.hpp"
#include <vector>
#include <memory>

namespace mc {

/**
 * @brief 配方同步包 (服务端 -> 客户端)
 *
 * 同步单个配方到客户端。
 * 参考: MC 1.16.5 SPacketRecipeBook
 */
class RecipeSyncPacket {
public:
    RecipeSyncPacket() = default;

    /**
     * @brief 构造配方同步包
     * @param recipeId 配方ID
     * @param recipeType 配方类型
     * @param recipeData 序列化的配方数据
     */
    RecipeSyncPacket(const ResourceLocation& recipeId,
                     const String& recipeType,
                     const String& recipeData)
        : m_recipeId(recipeId)
        , m_recipeType(recipeType)
        , m_recipeData(recipeData)
    {}

    // Getters
    [[nodiscard]] const ResourceLocation& recipeId() const { return m_recipeId; }
    [[nodiscard]] const String& recipeType() const { return m_recipeType; }
    [[nodiscard]] const String& recipeData() const { return m_recipeData; }

    // Setters
    void setRecipeId(const ResourceLocation& id) { m_recipeId = id; }
    void setRecipeType(const String& type) { m_recipeType = type; }
    void setRecipeData(const String& data) { m_recipeData = data; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeString(m_recipeId.toString());
        ser.writeString(m_recipeType);
        ser.writeString(m_recipeData);
    }

    // 反序列化
    [[nodiscard]] static Result<RecipeSyncPacket> deserialize(network::PacketDeserializer& deser) {
        RecipeSyncPacket packet;

        auto idResult = deser.readString();
        if (idResult.failed()) return idResult.error();
        packet.m_recipeId = ResourceLocation(idResult.value());

        auto typeResult = deser.readString();
        if (typeResult.failed()) return typeResult.error();
        packet.m_recipeType = typeResult.value();

        auto dataResult = deser.readString();
        if (dataResult.failed()) return dataResult.error();
        packet.m_recipeData = dataResult.value();

        return packet;
    }

private:
    ResourceLocation m_recipeId;
    String m_recipeType;
    String m_recipeData;
};

/**
 * @brief 配方列表同步包 (服务端 -> 客户端)
 *
 * 批量同步配方到客户端。
 * 用于客户端登录时同步所有配方。
 */
class RecipeListSyncPacket {
public:
    RecipeListSyncPacket() = default;

    /**
     * @brief 构造配方列表同步包
     * @param recipes 配方列表
     */
    explicit RecipeListSyncPacket(std::vector<RecipeSyncPacket> recipes)
        : m_recipes(std::move(recipes))
    {}

    // Getters
    [[nodiscard]] const std::vector<RecipeSyncPacket>& recipes() const { return m_recipes; }
    [[nodiscard]] size_t size() const { return m_recipes.size(); }
    [[nodiscard]] bool empty() const { return m_recipes.empty(); }

    // Setters
    void setRecipes(std::vector<RecipeSyncPacket> recipes) { m_recipes = std::move(recipes); }
    void addRecipe(const RecipeSyncPacket& recipe) { m_recipes.push_back(recipe); }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeVarUInt(static_cast<u32>(m_recipes.size()));
        for (const auto& recipe : m_recipes) {
            recipe.serialize(ser);
        }
    }

    // 反序列化
    [[nodiscard]] static Result<RecipeListSyncPacket> deserialize(network::PacketDeserializer& deser) {
        RecipeListSyncPacket packet;

        auto countResult = deser.readVarUInt();
        if (countResult.failed()) return countResult.error();
        u32 count = countResult.value();

        packet.m_recipes.reserve(count);
        for (u32 i = 0; i < count; ++i) {
            auto recipeResult = RecipeSyncPacket::deserialize(deser);
            if (recipeResult.failed()) return recipeResult.error();
            packet.m_recipes.push_back(recipeResult.value());
        }

        return packet;
    }

private:
    std::vector<RecipeSyncPacket> m_recipes;
};

/**
 * @brief 配方解锁包 (服务端 -> 客户端)
 *
 * 通知客户端解锁新配方。
 * 参考: MC 1.16.5 SPacketRecipeBookAdd
 */
class RecipeUnlockPacket {
public:
    RecipeUnlockPacket() = default;

    /**
     * @brief 构造配方解锁包
     * @param recipeId 配方ID
     * @param display 是否在配方书中显示
     */
    explicit RecipeUnlockPacket(const ResourceLocation& recipeId, bool display = true)
        : m_recipeId(recipeId)
        , m_display(display)
    {}

    // Getters
    [[nodiscard]] const ResourceLocation& recipeId() const { return m_recipeId; }
    [[nodiscard]] bool display() const { return m_display; }

    // Setters
    void setRecipeId(const ResourceLocation& id) { m_recipeId = id; }
    void setDisplay(bool display) { m_display = display; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeString(m_recipeId.toString());
        ser.writeU8(m_display ? 1 : 0);
    }

    // 反序列化
    [[nodiscard]] static Result<RecipeUnlockPacket> deserialize(network::PacketDeserializer& deser) {
        RecipeUnlockPacket packet;

        auto idResult = deser.readString();
        if (idResult.failed()) return idResult.error();
        packet.m_recipeId = ResourceLocation(idResult.value());

        auto displayResult = deser.readU8();
        if (displayResult.failed()) return displayResult.error();
        packet.m_display = displayResult.value() != 0;

        return packet;
    }

private:
    ResourceLocation m_recipeId;
    bool m_display = true;
};

/**
 * @brief 合成结果预览包 (服务端 -> 客户端)
 *
 * 同步当前合成网格的匹配结果。
 * 当玩家在工作台中放置物品时发送。
 */
class CraftResultPreviewPacket {
public:
    CraftResultPreviewPacket() = default;

    /**
     * @brief 构造合成结果预览包
     * @param containerId 容器ID
     * @param resultItem 结果物品（可以为空）
     * @param recipeId 匹配的配方ID（可选）
     */
    CraftResultPreviewPacket(i32 containerId, const ItemStack& resultItem,
                              const ResourceLocation& recipeId = ResourceLocation())
        : m_containerId(containerId)
        , m_resultItem(resultItem)
        , m_recipeId(recipeId)
    {}

    // Getters
    [[nodiscard]] i32 containerId() const { return m_containerId; }
    [[nodiscard]] const ItemStack& resultItem() const { return m_resultItem; }
    [[nodiscard]] const ResourceLocation& recipeId() const { return m_recipeId; }
    [[nodiscard]] bool hasRecipe() const { return !m_recipeId.path().empty(); }

    // Setters
    void setContainerId(i32 id) { m_containerId = id; }
    void setResultItem(const ItemStack& item) { m_resultItem = item; }
    void setRecipeId(const ResourceLocation& id) { m_recipeId = id; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeVarInt(m_containerId);
        m_resultItem.serialize(ser);
        ser.writeString(m_recipeId.toString());
    }

    // 反序列化
    [[nodiscard]] static Result<CraftResultPreviewPacket> deserialize(network::PacketDeserializer& deser) {
        CraftResultPreviewPacket packet;

        auto idResult = deser.readVarInt();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto itemResult = ItemStack::deserialize(deser);
        if (itemResult.failed()) return itemResult.error();
        packet.m_resultItem = itemResult.value();

        auto recipeIdResult = deser.readString();
        if (recipeIdResult.failed()) return recipeIdResult.error();
        packet.m_recipeId = ResourceLocation(recipeIdResult.value());

        return packet;
    }

private:
    i32 m_containerId = 0;
    ItemStack m_resultItem;
    ResourceLocation m_recipeId;
};

} // namespace mc
