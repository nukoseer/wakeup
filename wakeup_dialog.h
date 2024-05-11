#ifndef H_WAKEUP_DIALOG_H

#define MAX_ITEMS 16

#define HOT_KEY(key, mod) ((key) | ((mod) << 24))
#define HOT_GET_KEY(key_mod) ((key_mod) & 0xffffff)
#define HOT_GET_MOD(key_mod) (((key_mod) >> 24) & 0xff)

typedef struct
{
    HWND selected_window;
    DWORD period;
    DWORD menu_shortcut;
} WakeupDialogConfig;

LRESULT wakeup_dialog_show(WakeupDialogConfig* dialog_config);
void wakeup_dialog_init(WakeupDialogConfig* dialog_config, WCHAR* ini_path, HICON icon);

#define H_WAKEUP_DIALOG_H
#endif
