#include "wakeup.h"
#include "wakeup_dialog.h"
#include "wakeup_dialog.c"

#define WAKEUP_NAME                  (L"wakeup")
#define WAKEUP_URL                   (L"https://github.com/nukoseer/wakeup")
#define WAKEUP_INI                   (L"wakeup.ini")

#define WM_WAKEUP_COMMAND            (WM_USER + 0)
#define WM_WAKEUP_ALREADY_RUNNING    (WM_USER + 1)
#define CMD_WAKEUP                   (1)
#define CMD_QUIT                     (2)
#define HOT_MENU                     (13)

typedef struct
{
    HWND window_handle;
    HANDLE timer_handle;
    WCHAR ini_path[MAX_PATH];
    HICON icon;
    WakeupDialogConfig dialog_config;
} Wakeup;

static Wakeup global_wakeup;

// NOTE: https://learn.microsoft.com/en-us/windows/win32/shell/how-to-add-shortcuts-to-the-start-menu
static void wakeup__create_shortcut_link(void)
{
    HRESULT handle_result = 0;
    WCHAR link_path[MAX_PATH];

    RoInitialize(RO_INIT_SINGLETHREADED);

    GetEnvironmentVariableW(L"APPDATA", link_path, ARRAY_COUNT(link_path));
    PathAppendW(link_path, L"Microsoft\\Windows\\Start Menu\\Programs");
    PathAppendW(link_path, WAKEUP_NAME);
    StrCatW(link_path, L".lnk");

    if (GetFileAttributesW(link_path) == INVALID_FILE_ATTRIBUTES)
    {
        IShellLinkW* shell_link;

        handle_result = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (LPVOID*)&shell_link);

        if (SUCCEEDED(handle_result))
        {
            IPersistFile* persist_file;
            WCHAR exe_path[MAX_PATH];

            GetModuleFileNameW(NULL, exe_path, ARRAY_COUNT(exe_path));

            IShellLinkW_SetPath(shell_link, exe_path);
            PathRemoveFileSpecW(exe_path);
            IShellLinkW_SetWorkingDirectory(shell_link, exe_path);

            handle_result = IShellLinkW_QueryInterface(shell_link, &IID_IPersistFile, (LPVOID*)&persist_file);

            if (SUCCEEDED(handle_result))
            {
                handle_result = IPersistFile_Save(persist_file, link_path, TRUE);
                IPersistFile_Release(persist_file);
            }

            IShellLinkW_Release(shell_link);
        }
    }
}

static HANDLE wakeup__create_timer(void)
{
    HANDLE timer_handle = CreateWaitableTimerW(0, 0, 0);
    ASSERT(timer_handle);

    return timer_handle;
}

static void wakeup__show_notification(HWND window_handle, LPCWSTR message, LPCWSTR title, DWORD flags)
{
	NOTIFYICONDATAW data =
	{
		.cbSize = sizeof(data),
		.hWnd = window_handle,
		.uFlags = NIF_INFO | NIF_TIP,
		.dwInfoFlags = flags, // NIIF_INFO, NIIF_WARNING, NIIF_ERROR
	};
	StrCpyNW(data.szTip, WAKEUP_NAME, ARRAY_COUNT(data.szTip));
	StrCpyNW(data.szInfo, message, ARRAY_COUNT(data.szInfo));
	StrCpyNW(data.szInfoTitle, title ? title : WAKEUP_NAME, ARRAY_COUNT(data.szInfoTitle));
	Shell_NotifyIconW(NIM_MODIFY, &data);
}

static void wakeup__add_tray_icon(HWND window_handle)
{
    NOTIFYICONDATAW data =
    {
        .cbSize = sizeof(data),
        .hWnd = window_handle,
        .uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP,
        .uCallbackMessage = WM_WAKEUP_COMMAND,
        .hIcon = global_wakeup.icon,
    };
    StrCpyNW(data.szInfoTitle, WAKEUP_NAME, ARRAY_COUNT(data.szInfoTitle));
    Shell_NotifyIconW(NIM_ADD, &data);
}

static void wakeup__remove_tray_icon(HWND window_handle)
{
	NOTIFYICONDATAW data =
	{
		.cbSize = sizeof(data),
		.hWnd = window_handle,
	};
	Shell_NotifyIconW(NIM_DELETE, &data);
}

static LRESULT CALLBACK wakeup__window_proc(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_CREATE)
    {
        wakeup__add_tray_icon(window_handle);
        return 0;
    }
    else if (message == WM_DESTROY)
	{
		wakeup__remove_tray_icon(window_handle);
		PostQuitMessage(0);

		return 0;
	}
    else if (message == WM_WAKEUP_ALREADY_RUNNING)
	{
		wakeup__show_notification(window_handle, L"wakeup is already running!", 0, NIIF_INFO);

		return 0;
	}
    else if (message == WM_WAKEUP_COMMAND)
	{
		if (LOWORD(lparam) == WM_LBUTTONUP)
		{
            wakeup_dialog_show(&global_wakeup.dialog_config);
		}
        else if (LOWORD(lparam) == WM_RBUTTONUP)
		{
			HMENU menu = CreatePopupMenu();
			ASSERT(menu);

            AppendMenuW(menu, MF_STRING, CMD_WAKEUP, WAKEUP_NAME);
			AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
			AppendMenuW(menu, MF_STRING, CMD_QUIT, L"Exit");

			POINT mouse;
			GetCursorPos(&mouse);

			SetForegroundWindow(window_handle);
			int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, mouse.x, mouse.y, 0, window_handle, NULL);

            if (command == CMD_WAKEUP)
			{
				ShellExecuteW(NULL, L"open", WAKEUP_URL, NULL, NULL, SW_SHOWNORMAL);
			}
			else if (command == CMD_QUIT)
			{
				DestroyWindow(window_handle);
			}

            DestroyMenu(menu);
		}

        return 0;
    }
    else if (message == WM_HOTKEY)
	{
        if (wparam == HOT_MENU)
        {
            wakeup_dialog_show(&global_wakeup.dialog_config);
        }
    }

    return DefWindowProcW(window_handle, message, wparam, lparam);
}

static LRESULT wakeup__send_activate_message(HWND window_handle)
{
    static WPARAM wparam = WA_ACTIVE;

    LRESULT result = SendMessage(window_handle, WM_ACTIVATE, (WPARAM)wparam, (LPARAM)0);
    // wparam = wparam == WA_ACTIVE ? WA_INACTIVE : WA_ACTIVE;

    return result;
}

static DWORD WINAPI wakeup__timer_thread_proc(LPVOID parameter)
{
    Wakeup* wakeup = (Wakeup*)parameter;
    HANDLE timer_handle = wakeup->timer_handle;

    for (;;)
    {
        DWORD wait = WaitForSingleObject(timer_handle, INFINITE);

        if (wait == WAIT_OBJECT_0)
        {
            wakeup__send_activate_message(wakeup->dialog_config.selected_window);
        }
    }

    return 0;
}

void wakeup_set_timer(int seconds)
{
    LARGE_INTEGER due_time = { 0 };
    BOOL is_timer_set = 0;
    FILETIME file_time = { 0 };

    GetSystemTimeAsFileTime(&file_time);

    due_time.LowPart = file_time.dwLowDateTime;
    due_time.HighPart = file_time.dwHighDateTime;

    is_timer_set = SetWaitableTimer(global_wakeup.timer_handle, &due_time, seconds * SECONDS_TO_MILLISECONDS, 0, 0, 0);
    ASSERT(is_timer_set);
}

void wakeup_stop_timer(void)
{
    CancelWaitableTimer(global_wakeup.timer_handle);
}

BOOL wakeup_enable_hotkeys(void)
{
	BOOL success = TRUE;
    
	if (global_wakeup.dialog_config.menu_shortcut)
	{
		success = success && RegisterHotKey(global_wakeup.window_handle, HOT_MENU, HOT_GET_MOD(global_wakeup.dialog_config.menu_shortcut), HOT_GET_KEY(global_wakeup.dialog_config.menu_shortcut));
	}

	return success;
}

#ifdef _DEBUG
int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int n_show_cmd)
{
    (void)instance, (void)prev_instance, (void)cmd_line, (void)n_show_cmd;
#else
void WinMainCRTStartup(void)
{
#endif
    WNDCLASSEXW window_class =
	{
		.cbSize = sizeof(window_class),
		.lpfnWndProc = &wakeup__window_proc,
		.hInstance = GetModuleHandle(0),
		.lpszClassName = WAKEUP_NAME,
	};

    HWND existing = FindWindowW(window_class.lpszClassName, NULL);
	if (existing)
	{
		PostMessageW(existing, WM_WAKEUP_ALREADY_RUNNING, 0, 0);
		ExitProcess(0);
	}

    ATOM atom = RegisterClassExW(&window_class);
	ASSERT(atom);

	RegisterWindowMessageW(L"TaskbarCreated");

    global_wakeup.icon = LoadIconW(GetModuleHandleW(0), MAKEINTRESOURCEW(1));
    global_wakeup.window_handle = CreateWindowExW(0, window_class.lpszClassName, window_class.lpszClassName, WS_POPUP,
                                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                                NULL, NULL, window_class.hInstance, NULL);
    global_wakeup.timer_handle = wakeup__create_timer();

    CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&wakeup__timer_thread_proc, (LPVOID)&global_wakeup, 0, 0));

    WCHAR exe_path[MAX_PATH];
    GetModuleFileNameW(NULL, exe_path, ARRAY_COUNT(exe_path));
    PathRemoveFileSpecW(exe_path);
    PathCombineW(global_wakeup.ini_path, exe_path, WAKEUP_INI);

    wakeup__create_shortcut_link();
    wakeup_dialog_init(&global_wakeup.dialog_config, global_wakeup.ini_path, global_wakeup.icon);

    for (;;)
	{
        MSG message;
        BOOL result = GetMessageW(&message, NULL, 0, 0);

        if (result == 0)
        {
            ExitProcess(0);
        }
        ASSERT(result > 0);

        TranslateMessage(&message);
        DispatchMessageW(&message);
	}
}
