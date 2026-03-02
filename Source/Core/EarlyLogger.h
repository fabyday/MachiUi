// #pragma once
// #include <vector>
// #include <string>
// #include <iostream>
// #include "ILogger.h"

// class EarlyLogger
// {
// private:
//     // 아직 로거가 없을 때 로그를 임시 저장할 저장소
//     static inline std::vector<std::pair<Level, std::string>> m_buffer;
//     static inline bool m_is_transitioned = false;

// public:
//     static void Log(Level &level, const std::string &msg)
//     {
//         if (m_is_transitioned)
//         {
//             // 2단계: 전환 완료 후에는 바로 LogManager 사용
//             // (LogManager::Get().Log(...) 호출)
//             DispatchToLogManager(level, msg);
//         }
//         else
//         {
//             // 1단계: 전환 전에는 버퍼에 쌓고 콘솔에도 즉시 출력(안전빵)
//             m_buffer.push_back({level, msg});
//             std::cout << "EarlyLogger : [" << level << "-EARLY] " << msg << std::endl;
//         }
//     }

//     // ★ 핵심: LogManager 초기화 직후 이 함수를 한 번 호출해줍니다.
//     static void TransitionToLogManager();

// private:
//     static void DispatchToLogManager(Level &level, const std::string &msg);
// };