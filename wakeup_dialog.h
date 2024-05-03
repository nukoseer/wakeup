#ifndef H_WAKEUP_DIALOG_H

#define MAX_ITEMS        16
#define MAX_ITEMS_LENGTH 64

#define HOT_KEY(key, mod) ((key) | ((mod) << 24))
#define HOT_GET_KEY(key_mod) ((key_mod) & 0xffffff)
#define HOT_GET_MOD(key_mod) (((key_mod) >> 24) & 0xff)

typedef struct
{
    DWORD value_type;
    DWORD menu_shortcut;
} WakeupDialogConfig;

LRESULT wakeup_dialog_show(WakeupDialogConfig* dialog_config);

#define H_WAKEUP_DIALOG_H
#endif
