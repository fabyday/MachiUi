#include <gtest/gtest.h>
#include "Scripting/ScriptManager.h"
#include "Core/ServiceProvider.h"
#include <unordered_map>
#include <stdexcept>

namespace {
class MemoryFileLoader : public IFIleLoader
{
public:
    std::unordered_map<std::string, std::string> files;
    void onInit(ServiceProvider *) override {}
    std::optional<std::string> readFile(const std::string &path) override
    {
        auto it = files.find(path);
        return it == files.end() ? std::nullopt : std::optional<std::string>(it->second);
    }
    std::optional<std::string> resolvePath(const std::string &, const std::string &) override
    {
        return std::nullopt;
    }
};
class ManifestTest : public ::testing::Test
{
protected:
    ServiceProvider provider;
    ScriptManager scripts;
    MemoryFileLoader *files;
    void SetUp() override
    {
        auto loader = std::make_unique<MemoryFileLoader>();
        files = loader.get();
        provider.registerService(std::type_index(typeid(IFIleLoader)), std::move(loader));
        scripts.onInit(&provider);
    }
};
}

TEST_F(ManifestTest, MissingManifestOrEntryUsesNativeStartup)
{
    EXPECT_FALSE(scripts.readManifestEntry(""));
    EXPECT_FALSE(scripts.readManifestEntry("missing.json"));
    for (const auto &json : {"{}", "{\"entry\":\"\"}", "{\"name\":\"entry\"}"})
    {
        files->files["example/manifest.json"] = json;
        EXPECT_FALSE(scripts.readManifestEntry("example/manifest.json"));
    }
}

TEST_F(ManifestTest, ResolvesEntryRelativeToManifest)
{
    files->files["examples/app/manifest.json"] = "{\"entry\":\"./dist/main.js\"}";
    EXPECT_EQ(scripts.readManifestEntry("examples/app/manifest.json"), "examples/app/dist/main.js");
}

TEST_F(ManifestTest, RejectsMalformedManifestAndNonStringEntry)
{
    for (const auto &json : {"{", "[]", "null", "{\"entry\":12}", "{\"entry\":null}"})
    {
        files->files["manifest.json"] = json;
        EXPECT_THROW(scripts.readManifestEntry("manifest.json"), std::runtime_error);
    }
    files->files["manifest.json"] = "{}";
    EXPECT_FALSE(scripts.readManifestEntry("manifest.json"));
}
