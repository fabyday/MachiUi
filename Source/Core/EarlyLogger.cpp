// #include "EarlyLogger.h"

// static std::ostream &operator<<(std::ostream &os, Level level)
// {
//     switch (level)
//     {
//     case Level::INFO:
//         return os << "INFO";
//     case Level::ERROR:
//         return os << "ERROR";
//     default:
//         return os << "UNKNOWN";
//     }
// };

// void EarlyLogger::Log(Level &level, const std::string &msg)
// {
// }

// void EarlyLogger::TransitionToLogManager()
// {
//     if (m_is_transitioned)
//         return;

//     for (const auto &[level, msg] : m_buffer)
//     {
//         DispatchToLogManager(level, "[Buffered] " + msg);
//     }

//     m_buffer.clear();
//     m_buffer.shrink_to_fit(); // 메모리 해제
//     m_is_transitioned = true;
// }

// void EarlyLogger::DispatchToLogManager(Level &level, const std::string &msg)
// {
// }