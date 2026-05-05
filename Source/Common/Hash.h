#pragma once
#include <cstdint>
#include <string_view>
#include <string>

// see
// https://en.wikipedia.org/wiki/MurmurHash

/**
 * compile time hash function
 * TODO resolve conflict
 *
 */

inline uint64_t computeHash(const std::string_view str)
{
    // FNV-1a hash algorithm
    constexpr uint64_t prime = 1099511628211ULL;
    constexpr uint64_t offset = 14695981039346656037ULL;

    uint64_t hash = offset;
    for (char c : str)
    {
        hash ^= static_cast<uint64_t>(c);
        hash *= prime;
    }

    return hash;
}

constexpr uint64_t compileTimeHash(std::string_view str)
{
    return computeHash(str);
}

constexpr uint64_t compileTimeHash(const std::string &str)
{
    return compileTimeHash(std::string_view(str));
}

constexpr uint64_t compileTimeHash(const char *str)
{
    return compileTimeHash(std::string_view(str));
}

uint64_t runtimeHash(const std::string &str)
{
    return computeHash(str);
}

uint64_t runtimeHash(const char *str)
{
    return computeHash(str);
}

#define MachiCompileTimeHash(x) compileTimeHash(x)
#define MachiSafeHash(x) runtimeHash(x)
