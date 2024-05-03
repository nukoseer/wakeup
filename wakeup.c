#pragma warning(push, 1)
#include <Windows.h>
#include <tlhelp32.h>
#pragma warning(pop)

#include <stdio.h>

#pragma comment (lib, "kernel32")
#pragma comment (lib, "user32")

#define SECONDS_TO_MILLISECONDS   (1000)
#define MINUTES_TO_SECONDS        (60)

#ifdef _DEBUG
#define ASSERT(x) do { if (!(x)) { *(volatile int*)0; } } while (0)
#else
#define ASSERT(x) (void)(x);
#endif

static unsigned int get_process_id(const char* process_name)
{
    PROCESSENTRY32 entry = { .dwSize = sizeof(PROCESSENTRY32W) };
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    unsigned int process_id = 0;

    if (Process32First(snapshot, &entry))
    {
        do
        {
            if (!lstrcmp(entry.szExeFile, process_name))
            {
                // process_id = (unsigned int)entry.th32ParentProcessID;
                process_id = (unsigned int)entry.th32ProcessID;
            }
        } while (!process_id && Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);

    return process_id;
}

static HWND found_window;

static BOOL CALLBACK enum_windows_proc(HWND window, LPARAM lparam)
{
    const char* process_name = (const char*)lparam;
    char buffer[256] = { 0 };
    GetWindowText(window, buffer, sizeof(buffer));

    if (buffer[0] && strstr(buffer, process_name))
    {
        found_window = window;
        // LRESULT result = SendMessage(window, WM_CLOSE, 0, 0);
        fprintf(stderr, "buffer: %s\n", buffer);
        return FALSE;
        // return FALSE;

    }

    return TRUE;
}

static void timer_proc(LPVOID arg_to_completion_routine, DWORD timer_low, DWORD timer_high)
{
    (void)timer_low, (void)timer_high;
    HWND window = (HWND)arg_to_completion_routine;
    LRESULT result = SendMessage(window, WM_CLOSE, 0, 0);
    fprintf(stderr, "result: %u\n", (unsigned int)result);
}

static LRESULT CALLBACK window_proc(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    (void)window_handle, (void)message, (void)wparam, (void)lparam;

    switch (message)
    {
        case WM_TIMER:
        {
            fprintf(stderr, "WM_TIMER\n");
        }
        break;
    }
    
    return 0;
}
#ifdef _DEBUG
int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int n_show_cmd)
{
    (void)instance, (void)prev_instance, (void)cmd_line, (void)n_show_cmd;
#else
void WinMainCRTStartup(void)
{
#endif

    // WNDCLASSEX window_class =
	// {
	// 	.cbSize = sizeof(window_class),
	// 	.lpfnWndProc = &window_proc,
	// 	.hInstance = GetModuleHandle(0),
	// 	.lpszClassName = "wakeup",
    // };
    
    // ATOM atom = RegisterClassEx(&window_class);
	// ASSERT(atom);

	// RegisterWindowMessage("wakeup");

    // HWND wakeup_window = CreateWindowExA(0, window_class.lpszClassName, window_class.lpszClassName, WS_POPUP,
    //                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    //                                      NULL, NULL, window_class.hInstance, NULL);
    // (void)wakeup_window;


    // unsigned int process_id = get_process_id("Teams.exe");
    // fprintf(stderr, "process_id: %u\n", process_id);

    // HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, 0, process_id);
    // fprintf(stderr, "process_handle: %p\n", process_handle);

    // HWND process_window = FindWindowA(0, "");
    // fprintf(stderr, "process_window: %p\n", process_window);

    // char buffer[256] = { 0 };
    // GetWindowText(process_window, buffer, sizeof(buffer));
    // fprintf(stderr, "buffer: %s\n", buffer);
     
    // LRESULT result = SendMessage(process_window, WM_CLOSE, 0, 0);
    // fprintf(stderr, "result: %u\n", (unsigned int)result);

    EnumWindows(enum_windows_proc, (LPARAM)"Microsoft Teams classic");

    if (found_window)
    {
        HANDLE timer_handle = CreateWaitableTimer(0, 0, 0);
        LARGE_INTEGER due_time = { 0 };
        BOOL is_timer_set = 0;
        FILETIME file_time = { 0 };

        GetSystemTimeAsFileTime(&file_time);

        due_time.LowPart = file_time.dwLowDateTime;
        due_time.HighPart = file_time.dwHighDateTime;

        is_timer_set = SetWaitableTimer(timer_handle, &due_time, 10 * 1000, 0, 0, 0);
        ASSERT(is_timer_set);
        fprintf(stderr, "is_timer_set: %u\n", is_timer_set);

        // BOOL pv_param = TRUE;
        // BOOL system_result = SystemParametersInfoA(SPI_SETACTIVEWINDOWTRACKING, 0, &pv_param, SPIF_SENDCHANGE);
        // fprintf(stderr, "system_result: %u, pv_param: %u\n", system_result, pv_param);


        WPARAM wparam = WA_ACTIVE;
        // WPARAM wparam = (WPARAM)(DWORD)((1 << 16) | WA_ACTIVE);       
        for (;;)
        {
            DWORD wait = WaitForSingleObject(timer_handle, INFINITE);

            if (wait == WAIT_OBJECT_0)
            {
                LRESULT result = SendMessage(found_window, WM_ACTIVATE, (WPARAM)wparam, (LPARAM)0);
                // wparam = wparam == WA_ACTIVE ? WA_INACTIVE : WA_ACTIVE;

                // TRACKMOUSEEVENT track_mouse_event = {
                //     .cbSize = sizeof(track_mouse_event),
                //     .dwFlags = TME_HOVER,
                //     .hwndTrack = found_window,
                //     .dwHoverTime = 2000,
                // };

                // BOOL track_result = TrackMouseEvent(&track_mouse_event);
                // fprintf(stderr, "track_result: %u\n", track_result);
                
                // LRESULT result = SendMessage(found_window, WM_MOUSEHOVER, (WPARAM)0, (LPARAM)0);

                fprintf(stderr, "result: %u, wparam: %u\n", (unsigned int)result, (unsigned int)wparam);
            }
        }

        // SetTimer(wakeup_window, 1, 1000, 0);
        // for (;;)
        // {
        //     MSG message;
        //     BOOL result = PeekMessageA(&message, NULL, 0, 0, PM_NOREMOVE);
        //     (void)result;
        //     // if (result == 0)
        //     // {
        //     //     ExitProcess(0);
        //     // }
        //     // ASSERT(result > 0);

        //     TranslateMessage(&message);
        //     DispatchMessage(&message);
        // }
    }
}
