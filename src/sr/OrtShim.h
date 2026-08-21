#pragma once

// ORT C-API 头在 _WIN32 下用 MSVC 拼写 `_stdcall` 且依赖 SAL 注解,
// MinGW g++ 需要映射到 GCC 拼写并补齐缺失的 SAL 宏, 统一在此处理。
#ifndef _stdcall
#define _stdcall __stdcall
#endif
#include <sal.h>          // MinGW 提供 _In_/_Out_/_Success_ 等 SAL 注解
#ifndef _Frees_ptr_opt_
#define _Frees_ptr_opt_   // MinGW sal.h 缺此项, 置空
#endif
#include <onnxruntime_cxx_api.h>
