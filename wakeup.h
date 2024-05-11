#ifndef H_WAKEUP_H

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define _UNICODE

#pragma warning(push, 0)
#include <windows.h>
#include <tlhelp32.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <roapi.h>
#pragma warning(pop)

#include <stdio.h>

#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "ole32")
#pragma comment (lib, "runtimeobject")

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*(x)))

#ifdef _DEBUG
#define ASSERT(x) do { if (!(x)) { *(volatile int*)0; } } while (0)
#else
#define ASSERT(x) (void)(x);
#endif

#define SECONDS_TO_MILLISECONDS   (1000)

void wakeup_set_timer(int seconds);
void wakeup_stop_timer(void);
BOOL wakeup_enable_hotkeys(void);

#define H_WAKEUP_H
#endif
