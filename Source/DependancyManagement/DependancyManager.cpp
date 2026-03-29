#include "DependancyManager.h"

#include <boost-di/di.hpp>
#include <typeindex>

#include <unordered_map>
#include <Core/ComponentRegistry.h>

namespace di = boost::di;

struct DependancyManager::Impl
{
    auto create_injector()
    {

     
    }

    decltype(di::make_injector()) injector;

    Impl() : injector(create_injector()) {}
};

DependancyManager::DependancyManager() : pImpl(std::make_unique<Impl>()) {}
DependancyManager::~DependancyManager() = default;

std::shared_ptr<void> DependancyManager::resolve_impl(const std::type_info &type)
{

    return nullptr;
}