#pragma once

class UiEngine;

class ServiceInitializer {
public:
  explicit ServiceInitializer(UiEngine* engine) : m_engine(engine) {}
  ~ServiceInitializer() = default;

  template <typename T>
  T* GetService() const;

  UiEngine* GetEngine() const { return m_engine; }

private:
  UiEngine* m_engine;
};

#include "UiEngine.h"

template <typename T>
T* ServiceInitializer::GetService() const {
    return m_engine->GetService<T>();
}