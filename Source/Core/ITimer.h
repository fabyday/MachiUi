#pragma once
#include "IService.h"
#include <cstdint>

// Convert _** to NanoSeconds
constexpr uint64_t operator"" _ns(unsigned long long ns) { return ns; }
constexpr uint64_t operator"" _ms(unsigned long long ms) { return ms * 1000000ULL; }
constexpr uint64_t operator"" _s(unsigned long long s) { return s * 1000000000ULL; }

constexpr uint64_t operator"" _ns(long double ns) { return static_cast<uint64_t>(ns); }
constexpr uint64_t operator"" _ms(long double ms) { return static_cast<uint64_t>(ms * 1000000.0L); }
constexpr uint64_t operator"" _s(long double s) { return static_cast<uint64_t>(s * 1000000000.0L); }

/**
 * @brief
 * Convert NonoSecond to MileSeconds
 * @param ns
 * @return constexpr double
 */
constexpr double NsToMs(uint64_t ns) { return static_cast<double>(ns) / 1e6; }
/**
 * @brief
 * Convert NonoSecond to Second
 * @param ns
 * @return constexpr double
 */
constexpr double NsToSec(uint64_t ns) { return static_cast<double>(ns) / 1e9; }

class ITimer : public IService
{

public:
    using Nanoseconds = uint64_t;
    virtual ~ITimer() = default;

    virtual void onInit(UiEngine *engine) = 0;
    // 일시정지 제어
    virtual void setPaused(bool paused) = 0;
    virtual bool isPaused() const = 0;

    /**
     * @brief
     *
     */
    // this method is called by engine
    virtual void tick() = 0;

protected:
    /**
     *
     * @brief
     *
     * @return * current time Nanoseconds
     *
     */
    virtual Nanoseconds now() const;

public:
    /**
     * @brief Get the Delta Time object
     *
     * @return double
     */
    virtual double getDeltaTime() = 0;

    /**
     * @brief Get the Total Active Time object
     *
     * @return double
     */
    virtual double getTotalActiveTime() const = 0;
    /**
     * @brief Get the Absolute Time object
     *
     * @return double
     */
    virtual double getAbsoluteTime() const = 0;

    /**
     * @brief Get the Total Time object
     *
     * @return uint64_t
     */
    virtual uint64_t getTotalTime() = 0;
    // virtual double getCurrentTime();
};