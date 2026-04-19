#include <fstream>
#include <sstream>
#include <filesystem>
#include "DefaultFileLoader.h"
#include "ServiceRegistry.h"
#include "ServiceProvider.h"
#include "UiEngine.h"

DefaultFileLoader::DefaultFileLoader() = default;

std::optional<std::string> DefaultFileLoader::readFile(const std::string &path)
{

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
};

void DefaultFileLoader::onInit(ServiceProvider *engine) {

};

std::optional<std::string> DefaultFileLoader::resolvePath(const std::string &referrer, const std::string &name)
{
    namespace fs = std::filesystem;
    try
    {
        fs::path base = fs::path(referrer).parent_path();
        fs::path full = (base / name).lexically_normal();

        // 선택사항: 실제로 파일이 존재하는지 체크까지 할 수도 있습니다.
        // if (!fs::exists(full)) return std::nullopt;

        return full.string();
    }
    catch (...)
    {
        // 경로 계산 중 발생하는 모든 오류를 안전하게 캡처
        return std::nullopt;
    }
}

REGISTER_UI_COMPONENT_AS(DefaultFileLoader, IFIleLoader, ServicePhase::System);
