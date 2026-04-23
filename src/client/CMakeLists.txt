# src/client/CMakeLists.txt
# 客户端可执行文件

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(OpenAL CONFIG REQUIRED)

add_executable(minecraft-client
    main.cpp
    application/ClientApplication.cpp
    application/features/ClientApplicationHelpers.cpp
    application/features/ClientApplicationBootstrap.cpp
    application/features/ClientApplicationAudio.cpp
    application/features/ClientApplicationTimeWeather.cpp
    application/features/ClientApplicationTargetInfo.cpp
    application/features/ClientApplicationTargetInfoUi.cpp
    application/features/ClientApplicationUiFrame.cpp
    application/features/ClientApplicationInput.cpp
    application/features/ClientApplicationNetwork.cpp
    application/features/ClientApplicationResource.cpp
    application/features/ClientApplicationUi.cpp
    application/features/ClientApplicationSettings.cpp
    window/Window.cpp
    input/InputManager.cpp
    settings/ClientSettings.cpp
    # Trident 渲染引擎 API 层
    renderer/api/Types.cpp
    # Trident 渲染引擎核心 (Vulkan 实现)
    renderer/trident/core/TridentContext.cpp
    renderer/trident/core/TridentSwapchain.cpp
    renderer/trident/core/TridentEngine.cpp
    renderer/trident/core/buffer/TridentBuffer.cpp
    renderer/trident/core/texture/TridentTexture.cpp
    renderer/trident/core/pipeline/TridentPipeline.cpp
    renderer/trident/core/render/RenderPassManager.cpp
    renderer/trident/core/render/FrameManager.cpp
    renderer/trident/core/render/DescriptorManager.cpp
    renderer/trident/core/render/UniformManager.cpp
    # 雾效果模块
    renderer/trident/fog/FogManager.cpp
    # 云渲染模块
    renderer/trident/cloud/CloudRenderer.cpp
    # 天气渲染模块
    renderer/trident/weather/WeatherRenderer.cpp
    # 粒子系统
    renderer/trident/particle/Particle.cpp
    renderer/trident/particle/ParticleManager.cpp
    renderer/trident/particle/ParticleTextureAtlas.cpp
    renderer/trident/particle/particles/RainParticle.cpp
    renderer/trident/particle/particles/SnowParticle.cpp
    # 方块破坏效果
    renderer/trident/block/BreakProgressManager.cpp
    renderer/trident/block/BreakProgressRenderer.cpp
    # 子渲染器
    renderer/trident/chunk/ChunkRenderer.cpp
    renderer/trident/chunk/ChunkMesher.cpp
    renderer/trident/chunk/AmbientOcclusionCalculator.cpp
    renderer/trident/sky/SkyRenderer.cpp
    renderer/trident/sky/CelestialCalculations.cpp
    renderer/trident/gui/GuiRenderer.cpp
    renderer/trident/gui/GuiAtlasRegistry.cpp
    renderer/trident/gui/GuiTextureAtlas.cpp
    renderer/trident/gui/GuiTextureManager.cpp
    renderer/trident/gui/GuiSpriteAtlas.cpp
    renderer/trident/gui/GuiSpriteManager.cpp
    renderer/trident/gui/GuiSpriteParser.cpp
    renderer/trident/gui/GuiSpriteRegistry.cpp
    renderer/trident/gui/GuiTextureLoader.cpp
    renderer/trident/item/ItemRenderer.cpp
    renderer/trident/entity/EntityRenderer.cpp
    renderer/trident/entity/EntityRendererManager.cpp
    renderer/trident/entity/ItemEntityRenderer.cpp
    renderer/trident/entity/ExperienceOrbRenderer.cpp
    renderer/trident/entity/model/ModelRenderer.cpp
    renderer/trident/entity/model/EntityModel.cpp
    renderer/trident/entity/model/AnimalModels.cpp
    renderer/trident/entity/EntityPipeline.cpp
    renderer/trident/entity/EntityTextureAtlas.cpp
    # 第一人称手部渲染
    renderer/trident/firstperson/MatrixStack.cpp
    renderer/trident/firstperson/ItemCameraTransforms.cpp
    renderer/trident/firstperson/PlayerModel.cpp
    renderer/trident/firstperson/FirstPersonRenderer.cpp
    renderer/trident/firstperson/ItemInHandRenderer.cpp
    # 共享渲染组件
    renderer/mesh/MeshWorkerPool.cpp
    renderer/mesh/MeshBuildScheduler.cpp
    renderer/MeshTypes.cpp
    renderer/Camera.cpp
    # 资源加载
    resource/BlockModelLoader.cpp
    resource/BlockStateLoader.cpp
    resource/TextureAtlasBuilder.cpp
    resource/ResourceManager.cpp
    resource/BlockModelCache.cpp
    resource/ItemTextureAtlas.cpp
    resource/EntityTextureLoader.cpp
    resource/DestroyStageTextures.cpp
    # 声音系统
    sound/resource/SoundDefinition.cpp
    sound/resource/SoundRegistry.cpp
    sound/SoundHandler.cpp
    sound/SoundLoader.cpp
    sound/StbVorbisImpl.cpp
    sound/SoundPool.cpp
    sound/SoundEngine.cpp
    sound/MusicPlayer.cpp
    sound/AudioService.cpp
    sound/backend/AudioBuffer.cpp
    sound/backend/OpenALBackend.cpp
    sound/instance/SoundInstance.cpp
    sound/handler/BiomeAmbientHandler.cpp
    sound/handler/UnderwaterAmbientHandler.cpp
    # 世界
    world/ClientWorld.cpp
    world/player/LocalPlayerIdentity.cpp
    world/color/BiomeColors.cpp
    world/color/blend/BiomeColorCache.cpp
    world/color/blend/BiomeColorBlender.cpp
    world/color/blend/ChunkBiomeAccessor.cpp
    # 维度
    dimension/ClientDimensionManager.cpp
    # 命令补全
    command/ClientCommandManager.cpp
    network/NetworkClient.cpp
    # 聊天系统
    chat/ChatHistory.cpp
    # UI (平台无关)
    ui/FontTextureAtlas.cpp
    ui/Font.cpp
    ui/FontRenderer.cpp
    ui/DefaultAsciiFont.cpp
    ui/GuiScale.cpp
    ui/TridentCanvas.cpp
    ui/screen/ScreenManager.cpp
    ui/screen/CraftingScreen.cpp
    ui/screen/CreativeScreen.cpp
    ui/screen/ChestScreen.cpp
    ui/screen/FurnaceScreen.cpp
    # UI Kagero模板系统
    ui/kagero/template/parser/Ast.cpp
    ui/kagero/template/parser/AstVisitor.cpp
    ui/kagero/template/parser/Lexer.cpp
    ui/kagero/template/parser/Parser.cpp
    ui/kagero/template/binder/BindingContext.cpp
    ui/kagero/template/compiler/TemplateCompiler.cpp
    ui/kagero/template/runtime/TemplateInstance.cpp
    ui/kagero/template/runtime/UpdateScheduler.cpp
    ui/kagero/template/bindings/BuiltinWidgets.cpp
    ui/kagero/template/bindings/BuiltinEvents.cpp
    ui/kagero/template/TemplateSystem.cpp
    # UI Kagero布局系统
    ui/kagero/layout/core/LayoutEngine.cpp
    ui/kagero/layout/algorithms/FlexLayout.cpp
    ui/kagero/layout/algorithms/GridLayout.cpp
    ui/kagero/layout/algorithms/AnchorLayout.cpp
    ui/kagero/layout/integration/WidgetLayoutAdaptor.cpp
    # UI Kagero Paint抽象
    ui/kagero/paint/Geometry.cpp
    ui/kagero/paint/PaintContext.cpp
    ui/kagero/paint/TextureImage.cpp
    ui/kagero/widget/ContainerWidget.cpp
    # UI Kagero 引擎
    ui/kagero/KageroEngine.cpp
    # UI Kagero widgets
    # Minecraft UI业务层
    ui/minecraft/widgets/CrosshairWidget.cpp
    ui/minecraft/MinecraftUIContext.cpp
    ui/minecraft/resources/ResourceProvider.cpp
    ui/minecraft/widgets/SlotWidget.cpp
    ui/minecraft/widgets/InventorySlot.cpp
    ui/minecraft/widgets/HotbarWidget.cpp
    ui/minecraft/widgets/HealthBarWidget.cpp
    ui/minecraft/widgets/HungerBarWidget.cpp
    ui/minecraft/widgets/ExperienceBar.cpp
    ui/minecraft/widgets/Viewport3DWidget.cpp
    ui/minecraft/widgets/HudWidget.cpp
    ui/minecraft/targetinfo/TargetInfo.cpp
    ui/minecraft/targetinfo/TargetInfoResolver.cpp
    ui/minecraft/targetinfo/TargetInfoWidget.cpp
    ui/minecraft/widgets/ChatWidget.cpp
    ui/minecraft/widgets/ScreenStackWidget.cpp
    ui/minecraft/screens/Screen.cpp
    ui/minecraft/screens/ScreenManager.cpp
    ui/minecraft/screens/MainMenuScreen.cpp
    ui/minecraft/screens/InventoryScreen.cpp
    ui/minecraft/screens/ContainerScreen.cpp
    ui/minecraft/screens/PauseScreen.cpp
    ui/minecraft/screens/OptionsScreen.cpp
    ui/minecraft/screens/DebugScreenWidget.cpp
    # 客户端实体管理
    world/entity/ClientEntity.cpp
    world/entity/ClientEntityManager.cpp
    # 本地玩家身份
    world/player/LocalPlayerIdentity.cpp
    # 客户端玩家预测器
    world/player/ClientPlayerPredictor.cpp
    # 内置服务端支持
    ${CMAKE_SOURCE_DIR}/src/server/application/IntegratedServer.cpp
    ${CMAKE_SOURCE_DIR}/src/server/application/MinecraftServer.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/PlayerManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/ConnectionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/TimeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/TeleportManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/KeepAliveManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/PositionTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/PacketHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/GameModeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ServerWorld.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ChunkWorkerPool.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/spawn/NaturalSpawner.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/entity/ItemPickupManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/player/ServerPlayerEntityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/player/ServerPlayer.cpp
    ${CMAKE_SOURCE_DIR}/src/server/menu/CraftingMenu.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/support/PlayerResolver.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameModeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/DifficultyCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/DefaultGameModeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TimeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/KillCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ListCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/HelpCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SeedCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeleportCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/GiveCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ClearCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/WeatherCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExperienceCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/KickCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SayCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/StopCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetIdleTimeoutCommand.cpp
    # 交互管理器
    ${CMAKE_SOURCE_DIR}/src/server/interaction/BlockInteractionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/interaction/MiningManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/interaction/ContainerManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/interaction/InventoryManager.cpp
    # 维度模块
    ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimension.cpp
    ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimensionManager.cpp
    # 同步管理器
    ${CMAKE_SOURCE_DIR}/src/server/sync/EntitySyncManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/sync/BlockUpdateSyncManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/sync/ChunkSendManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/sync/LightSyncManager.cpp
)

target_link_libraries(minecraft-client
    PRIVATE
        mc_common
        nlohmann_json::nlohmann_json
        glfw
        Vulkan::Vulkan
        OpenAL::OpenAL
)

if(WIN32)
    target_link_libraries(minecraft-client PRIVATE user32 gdi32 ws2_32)
endif()

mc_set_compiler_warnings(minecraft-client)

target_compile_features(minecraft-client PRIVATE cxx_std_17)

target_compile_definitions(minecraft-client PRIVATE
    GLFW_INCLUDE_VULKAN
)

set_target_properties(minecraft-client PROPERTIES
    OUTPUT_NAME "minecraft-client"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 依赖着色器编译
if(TARGET compile_shaders)
    add_dependencies(minecraft-client compile_shaders)
endif()
