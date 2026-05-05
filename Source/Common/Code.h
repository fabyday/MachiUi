#pragma once

typedef enum
{
    // Success
    MACHI_SUCCESS = 0,

    // Generic Errors (Negative values)
    MACHI_FAIL = -1,
    MACHI_ERR_NULL_PTR = -2,
    MACHI_ERR_INVALID_PARAM = -3,
    MACHI_ERR_OUT_OF_MEMORY = -4,
    MACHI_ERR_NOT_IMPLEMENTED = -5,

    // IO / Resource Errors
    MACHI_ERR_FILE_IO = -50,
    MACHI_ERR_ALREADY_EXISTS = -51,

    // UI & Render Errors
    MACHI_ERR_UI_RENDER = -101,
    MACHI_ERR_DEVICE_LOST = -102, // DX12/Metal device lost error

    // Window & Message Errors
    MACHI_ERR_WINDOW_CREATE_FAIL = -201,
    MACHI_ERR_MESSAGE_BUS_FULL = -202,

} MachiCodeEnum;

typedef struct
{
    MachiCodeEnum code;
    const char *msg;
#ifdef _DEBUG
    const char *file;
    int line;
#endif
} MachiCode;

#define MAKE_MACHI_ERR(c, m) \
    MachiCode { c, m, __FILE__, __LINE__ }

#define MAKE_MACHI_SUCCESS MachiCode{MACHI_SUCCESS, "Success"}

// Helper functions to check MachiCode results
inline bool isMachiSuccess(const MachiCode &code)
{
    return code.code >= 0;
}
inline bool isMachiFailed(const MachiCode &code)
{
    return code.code < 0;
}
