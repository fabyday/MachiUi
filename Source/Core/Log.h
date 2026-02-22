#pragma once
#include <memory>
#include <string>

namespace MyCore {
    // 1. 로그 동작만 정의하는 인터페이스
    class ILogger {
    public:
        virtual ~ILogger() = default;
        virtual void LogInfo(const std::string& msg) = 0;
        virtual void LogWarn(const std::string& msg) = 0;
        virtual void LogError(const std::string& msg) = 0;
    };

    // 2. 구현체를 관리하는 매니저
    class LogManager {
    public:
        // 엔진이나 테스트 앱에서 구현체를 주입할 때 사용
        static void SetLogger(std::shared_ptr<ILogger> logger) {
            Get().m_activeLogger = logger;
        }

        static ILogger* GetLogger() {
            return Get().m_activeLogger.get();
        }

    private:
        static LogManager& Get() {
            static LogManager instance;
            return instance;
        }
        std::shared_ptr<ILogger> m_activeLogger = nullptr;
    };
}

// UI 라이브러리 내부에서 사용할 매크로 (편의성)
#define UI_LOG_INFO(msg)  if(auto* l = MyCore::LogManager::GetLogger()) l->LogInfo(msg)
#define UI_LOG_ERROR(msg) if(auto* l = MyCore::LogManager::GetLogger()) l->LogError(msg)