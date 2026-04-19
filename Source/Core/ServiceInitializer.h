#pragma once

class ServiceProvider;
class ServiceRegistry;

class ServiceInitializer
{
public:
  // this is Object Createion Phase, only create objects, do not call onInit
  static bool createAllServices(ServiceRegistry &registry, ServiceProvider &provider);

  /**
   * Initialization phase( I mean... (onInit) phase),
   * setup and ready to use Provider with a fully Functional Services
   */
  static bool initializeAllServices(ServiceRegistry &registry, ServiceProvider &provider);
  ~ServiceInitializer() = default;
};
