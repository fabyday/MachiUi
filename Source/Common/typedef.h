#include <cstdint>

/**
 * ViewId
 * 0 := NULL, Invalid ID
 * 1 ~ 0xFFFFFFFF := Valid ID Range
 */
typedef uint32_t ViewId;

/**
 * will generate types and namespace definitions for typescript bindings
 */
#define MACHIUI_FUNCTION
#define MACHIUI_METHOD
#define MACHIUI_PROPERTY
#define MACHIUI_EVENT
#define MACHIUI_CLASS



