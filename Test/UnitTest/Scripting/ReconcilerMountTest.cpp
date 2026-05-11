#include <gtest/gtest.h>

#include <Core/DefaultFileLoader.h>
#include <Core/Element.h>
#include <Core/ElementFactory.h>
#include <Core/LogManager.h>
#include <Core/SceneGraph.h>
#include <Core/SceneManager.h>
#include <Core/ServiceProvider.h>
#include <Scripting/ScriptManager.h>

TEST(ReconcilerMountTest, LoadsExampleViewIntoSceneGraph)
{
    ServiceProvider provider;
    provider.registerService<LogManager>(std::make_unique<LogManager>());
    provider.registerService<ElementFactory>(std::make_unique<ElementFactory>());
    provider.registerService<SceneManager>(std::make_unique<SceneManager>());
    provider.registerService(std::type_index(typeid(IFIleLoader)), std::make_unique<DefaultFileLoader>());
    provider.registerService<ScriptManager>(std::make_unique<ScriptManager>());

    provider.getService<LogManager>()->initialize(&provider);
    provider.getService<ElementFactory>()->initialize(&provider);
    provider.getService<SceneManager>()->initialize(&provider);
    provider.getService<IFIleLoader>()->initialize(&provider);
    provider.getService<ScriptManager>()->initialize(&provider);

    auto *sceneManager = provider.getService<SceneManager>();
    auto *scriptManager = provider.getService<ScriptManager>();
    ASSERT_NE(sceneManager, nullptr);
    ASSERT_NE(scriptManager, nullptr);

    const uint64_t sceneGraphId = sceneManager->createSceneGraph("ReconcilerMountTest");
    ASSERT_TRUE(sceneManager->createRoot(sceneGraphId));

    scriptManager->ExecuteModule("Assets/TestUI/dist/TestUI.js", sceneGraphId);

    SceneGraph *graph = sceneManager->getSceneGraph(sceneGraphId);
    ASSERT_NE(graph, nullptr);
    ASSERT_NE(graph->getRoot(), nullptr);
    ASSERT_FALSE(graph->getRoot()->getChildren().empty());

    Element *div = graph->getRoot()->getChildren().front();
    ASSERT_NE(div, nullptr);

    const auto *backgroundColor = div->getAttribute("backgroundColor");
    ASSERT_NE(backgroundColor, nullptr);
    ASSERT_NE(std::get_if<std::string>(backgroundColor), nullptr);
    EXPECT_EQ(*std::get_if<std::string>(backgroundColor), "#111827");

    ASSERT_FALSE(div->getChildren().empty());
    Element *sidebar = div->getChildren().front();
    ASSERT_NE(sidebar, nullptr);

    const auto *sidebarColor = sidebar->getAttribute("backgroundColor");
    ASSERT_NE(sidebarColor, nullptr);
    ASSERT_NE(std::get_if<std::string>(sidebarColor), nullptr);
    EXPECT_EQ(*std::get_if<std::string>(sidebarColor), "#1f2a44");
}
