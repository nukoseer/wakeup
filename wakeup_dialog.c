// NOTE: Source: https://github.com/mmozeiko/wcap/blob/main/wcap_config.c

#define COL_WIDTH    (240)
#define ROW_HEIGHT   ((2 + 1) * ITEM_HEIGHT)
#define ROW2_HEIGHT  (ITEM_HEIGHT)
#define BUTTON_WIDTH (50)
#define ITEM_HEIGHT  (14)
#define PADDING      (4)

#define ID_START          (0)
#define ID_STOP           (1)
#define ID_CANCEL         (2)
#define ID_REFRESH        (99)

#define ID_WINDOW         (100)
#define ID_PERIOD         (200)
#define ID_SHORTCUT_MENU  (300)

#define ITEM_CHECKBOX     (1 << 0)
#define ITEM_NUMBER       (1 << 1)
#define ITEM_STRING       (1 << 2)
#define ITEM_CONST_STRING (1 << 3)
#define ITEM_LABEL        (1 << 4)
#define ITEM_COMBOBOX     (1 << 5)
#define ITEM_CENTER       (1 << 6)
#define ITEM_HOTKEY       (1 << 7)

#define CONTROL_BUTTON    (0x0080)
#define CONTROL_EDIT      (0x0081)
#define CONTROL_STATIC    (0x0082)
#define CONTROL_COMBOBOX  (0x0085)

#define WAKEUP_DIALOG_INI_SETTINGS_SECTION  (L"settings")

#define WAKEUP_DIALOG_INI_PERIOD_KEY        (L"period")
#define WAKEUP_DIALOG_INI_SHORTCUT_MENU_KEY (L"menu-shortcut")

typedef struct
{
	int left;
	int top;
	int width;
	int height;
} WakeupDialogRect;

typedef struct
{
	const char* text;
	WORD id;
	WORD item;
	DWORD width;
} WakeupDialogItem;

typedef struct
{
	const char* caption;
	WakeupDialogRect rect;
	WakeupDialogItem items[MAX_ITEMS];
} WakeupDialogGroup;

typedef struct
{
	WakeupDialogGroup* groups;
    const char* title;
	const char* font;
    WORD font_size;
} WakeupDialogLayout;

static HWND global_dialog_window;
static WCHAR* global_ini_path;
static HICON global_icon;
static int global_started;
static int global_selected_window_index;
static const char global_check_mark[] = "\x20\x00\x20\x00\x20\x00\x20\x00\x13\x27\x00\x00"; // NOTE: Four space and check mark for easy printing.
struct {
	WNDPROC window_proc;
    WakeupDialogConfig* dialog_config;
	int control;
} static global_config_shortcut;

typedef struct
{
    WCHAR name[256];
    HWND window;
} WakeupWindowInfo;

static WakeupWindowInfo window_infos[32] = { 0 };
static unsigned int window_info_count = 0;

static BOOL CALLBACK wakeup_dialog__enum_window_proc(HWND window, LPARAM lparam)
{
    (void)lparam;

    if (window_info_count >= ARRAY_COUNT(window_infos))
    {
        return FALSE;
    }
    
    if (window && IsWindowVisible(window))
    {
        WCHAR buffer[256] = { 0 };

        GetWindowTextW(window, buffer, sizeof(buffer));

        if (buffer[0])
        {
            WakeupWindowInfo* window_info = window_infos + window_info_count++;
            memcpy(window_info->name, buffer, sizeof(buffer));
            window_info->window = window;
        }
    }

    return TRUE;
}

static void wakeup_dialog__set_window_list(HWND window)
{
    for (unsigned int i = 0; i < window_info_count; ++i)
    {
        SendDlgItemMessageW(window, ID_WINDOW, CB_DELETESTRING, 0, (LPARAM)window_infos[i].name);
    }

    memset(window_infos, 0, sizeof(window_infos));
    window_info_count = 0;

    EnumWindows(wakeup_dialog__enum_window_proc, 0);
    
    for (unsigned int i = 0; i < window_info_count; ++i)
    {
        SendDlgItemMessageW(window, ID_WINDOW, CB_ADDSTRING, 0, (LPARAM)window_infos[i].name);
    }

    SendDlgItemMessageW(window, ID_WINDOW, CB_SETCURSEL, 0, 0);
}

static LRESULT CALLBACK wakeup_dialog__proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_INITDIALOG)
	{
        WakeupDialogConfig* dialog_config = (WakeupDialogConfig*)lparam;

        SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)dialog_config);
        
        dialog_config->period = GetPrivateProfileIntW(WAKEUP_DIALOG_INI_SETTINGS_SECTION, WAKEUP_DIALOG_INI_PERIOD_KEY, 5, global_ini_path);
        SetDlgItemInt(window, ID_PERIOD, dialog_config->period, FALSE);

        wakeup_dialog__set_window_list(window);
                
        SendMessage(window, WM_SETICON, ICON_BIG, (LPARAM)global_icon);

        if (global_started)
        {
            EnableWindow(GetDlgItem(window, ID_WINDOW), 0);
            EnableWindow(GetDlgItem(window, ID_PERIOD), 0);
            EnableWindow(GetDlgItem(window, ID_REFRESH), 0);
        }

        SendMessageW(window, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(window, ID_START), TRUE);
        
        SetForegroundWindow(window);
		global_dialog_window = window;
        global_config_shortcut.control = 0;

		return TRUE;
    }
    else if (message == WM_DESTROY)
    {
        global_dialog_window = 0;
    }
    else if (message == WM_COMMAND)
	{
        int control = LOWORD(wparam);

		if (control == ID_START)
        {
            if (!global_started)
            {
                WakeupDialogConfig* dialog_config = (WakeupDialogConfig*)GetWindowLongPtrW(window, GWLP_USERDATA);
                
                dialog_config->selected_window = window_infos[global_selected_window_index].window;
                
                global_started = 1;

                EnableWindow(GetDlgItem(window, ID_WINDOW), 0);
                EnableWindow(GetDlgItem(window, ID_PERIOD), 0);
                EnableWindow(GetDlgItem(window, ID_REFRESH), 0);
                
                wakeup_set_timer(dialog_config->period);

                return TRUE;
            }
        }
        else if (control == ID_STOP)
        {
            if (global_started)
            {
                global_started = 0;

                EnableWindow(GetDlgItem(window, ID_WINDOW), 1);
                EnableWindow(GetDlgItem(window, ID_PERIOD), 1);
                EnableWindow(GetDlgItem(window, ID_REFRESH), 1);
                wakeup_stop_timer();

                return TRUE;
            }
        }
        else if (control == ID_REFRESH)
        {
            if (!global_started)
            {
                wakeup_dialog__set_window_list(window);

                return TRUE;
            }
        }
        else if (control == ID_CANCEL)
        {
            EndDialog(window, 0);
            return FALSE;
        }
        else if (control == ID_WINDOW && HIWORD(wparam) == CBN_SELCHANGE)
		{
			LRESULT index = SendDlgItemMessageW(window, ID_WINDOW, CB_GETCURSEL, 0, 0);

            global_selected_window_index = (unsigned int)index;

			return TRUE;
		}
        else if (control == ID_PERIOD)
        {
            WakeupDialogConfig* dialog_config = (WakeupDialogConfig*)GetWindowLongPtrW(window, GWLP_USERDATA);
            WCHAR period_string[8] = { 0 };

            dialog_config->period = GetDlgItemInt(window, ID_PERIOD, 0, FALSE);

            if (dialog_config->period > 86400)
            {
                dialog_config->period = 86400;
                SetDlgItemInt(window, ID_PERIOD, dialog_config->period, FALSE);
            }

            _snwprintf(period_string, ARRAY_COUNT(period_string), L"%u", (unsigned int)dialog_config->period);
            WritePrivateProfileStringW(WAKEUP_DIALOG_INI_SETTINGS_SECTION, WAKEUP_DIALOG_INI_PERIOD_KEY, period_string, global_ini_path);
        }

        return TRUE;
    }

    return FALSE;
}

static void* wakeup_dialog__align(BYTE* data, SIZE_T size)
{
	SIZE_T pointer = (SIZE_T)data;
	return data + ((pointer + size - 1) & ~(size - 1)) - pointer;
}

static BYTE* wakeup__do_dialog_item(BYTE* buffer, LPCSTR text, WORD id, WORD control, DWORD style, int x, int y, int w, int h)
{
	buffer = wakeup_dialog__align(buffer, sizeof(DWORD));

	*(DLGITEMTEMPLATE*)buffer = (DLGITEMTEMPLATE)
	{
		.style = style | WS_CHILD | WS_VISIBLE,
		.x = (short)x,
		.y = (short)y + (control == CONTROL_STATIC ? 2 : 0),
		.cx = (short)w,
		.cy = (short)h - (control == CONTROL_EDIT ? 2 : 0) - (control == CONTROL_STATIC ? 2 : 0),
		.id = id,
	};
	buffer += sizeof(DLGITEMTEMPLATE);

	// window class
	buffer = wakeup_dialog__align(buffer, sizeof(WORD));
	*(WORD*)buffer = 0xffff;
	buffer += sizeof(WORD);
	*(WORD*)buffer = control;
	buffer += sizeof(WORD);

	// item text
	buffer = wakeup_dialog__align(buffer, sizeof(WCHAR));
	DWORD item_chars = MultiByteToWideChar(CP_UTF8, 0, text, -1, (WCHAR*)buffer, 128);
	buffer += item_chars * sizeof(WCHAR);

	// create extras
	buffer = wakeup_dialog__align(buffer, sizeof(WORD));
	*(WORD*)buffer = 0;
	buffer += sizeof(WORD);

	return buffer;
}

static void wakeup__do_dialog_layout(const WakeupDialogLayout* dialog_layout, BYTE* buffer, SIZE_T buffer_size)
{
	BYTE* end = buffer + buffer_size;

	// header
	DLGTEMPLATE* dialog = (void*)buffer;
	buffer += sizeof(DLGTEMPLATE);

	// menu
	buffer = wakeup_dialog__align(buffer, sizeof(WCHAR));
	*(WCHAR*)buffer = 0;
	buffer += sizeof(WCHAR);

	// window class
	buffer = wakeup_dialog__align(buffer, sizeof(WCHAR));
	*(WCHAR*)buffer = 0;
	buffer += sizeof(WCHAR);

	// title
	buffer = wakeup_dialog__align(buffer, sizeof(WCHAR));
	DWORD title_chars = MultiByteToWideChar(CP_UTF8, 0, dialog_layout->title, -1, (WCHAR*)buffer, 128);
	buffer += title_chars * sizeof(WCHAR);

	// font size
	buffer = wakeup_dialog__align(buffer, sizeof(WORD));
	*(WORD*)buffer = dialog_layout->font_size;
	buffer += sizeof(WORD);

	// font name
	buffer = wakeup_dialog__align(buffer, sizeof(WCHAR));
	DWORD font_chars = MultiByteToWideChar(CP_UTF8, 0, dialog_layout->font, -1, (WCHAR*)buffer, 128);
	buffer += font_chars * sizeof(WCHAR);

	int item_count = 4;

	int button_x = PADDING + COL_WIDTH + PADDING - 4 * (PADDING + BUTTON_WIDTH);
	int button_y = PADDING + ROW_HEIGHT + PADDING + ROW2_HEIGHT + PADDING;

	DLGITEMTEMPLATE* start_buffer = wakeup_dialog__align(buffer, sizeof(DWORD));
	buffer = wakeup__do_dialog_item(buffer, "Start", ID_START, CONTROL_BUTTON, WS_TABSTOP | BS_DEFPUSHBUTTON, button_x, button_y, BUTTON_WIDTH, ITEM_HEIGHT);
	button_x += BUTTON_WIDTH + PADDING;
    (void)start_buffer;

	DLGITEMTEMPLATE* stop_buffer = wakeup_dialog__align(buffer, sizeof(DWORD));
	buffer = wakeup__do_dialog_item(buffer, "Stop", ID_STOP, CONTROL_BUTTON, WS_TABSTOP | BS_PUSHBUTTON, button_x, button_y, BUTTON_WIDTH, ITEM_HEIGHT);
	button_x += BUTTON_WIDTH + PADDING;
    (void)stop_buffer;

    DLGITEMTEMPLATE* refresh_buffer = wakeup_dialog__align(buffer, sizeof(DWORD));
	buffer = wakeup__do_dialog_item(buffer, "Refresh", ID_REFRESH, CONTROL_BUTTON, WS_TABSTOP | BS_PUSHBUTTON, button_x, button_y, BUTTON_WIDTH, ITEM_HEIGHT);
	button_x += BUTTON_WIDTH + PADDING;
    (void)refresh_buffer;
        
	DLGITEMTEMPLATE* cancel_buffer = wakeup_dialog__align(buffer, sizeof(DWORD));
	buffer = wakeup__do_dialog_item(buffer, "Cancel", ID_CANCEL, CONTROL_BUTTON, WS_TABSTOP | BS_PUSHBUTTON, button_x, button_y, BUTTON_WIDTH, ITEM_HEIGHT);
	button_x += BUTTON_WIDTH + PADDING;
    (void)cancel_buffer;

	for (const WakeupDialogGroup* group = dialog_layout->groups; group->caption; group++)
	{
		int x = group->rect.left + PADDING;
		int y = group->rect.top + PADDING;
		int w = group->rect.width;
		int h = group->rect.height;

		buffer = wakeup__do_dialog_item(buffer, group->caption, (WORD)-1, CONTROL_BUTTON, BS_GROUPBOX, x, y, w, h);
		item_count++;

		x += PADDING;
		y += ITEM_HEIGHT - PADDING;
		w -= 2 * PADDING;

		for (unsigned int item_index = 0; item_index < ARRAY_COUNT(group->items); item_index++)
		{
            const WakeupDialogItem* item = group->items + item_index;
			int has_number = !!(item->item & ITEM_NUMBER);
            int has_string = !!(item->item & ITEM_STRING);
            int has_const_string = !!(item->item & ITEM_CONST_STRING);
            int has_label = !!(item->item & ITEM_LABEL);
            int has_combobox = !!(item->item & ITEM_COMBOBOX);
            int has_checkbox = !!(item->item & ITEM_CHECKBOX);
            int has_center = !!(item->item & ITEM_COMBOBOX);
            int has_hotkey = !!(item->item & ITEM_HOTKEY);

			int item_x = x;
			int item_w = w;
			int item_id = item->id;

            if (has_checkbox)
			{
				buffer = wakeup__do_dialog_item(buffer, item->text, (WORD)item_id, (WORD)CONTROL_BUTTON, WS_TABSTOP | BS_AUTOCHECKBOX, item_x, y, item_w, ITEM_HEIGHT);
				item_count++;
				item_id++;
			}

            if (has_label)
            {
                buffer = wakeup__do_dialog_item(buffer, item->text, (WORD)-1, (WORD)CONTROL_STATIC, 0, item_x, y, item->width, ITEM_HEIGHT);
				item_count++;
                item_x += item->width + PADDING;
				item_w -= item->width + PADDING;
            }

            if (has_string)
            {
                DWORD style = WS_TABSTOP | WS_BORDER;

                if (has_label)
                {
                    style |= ES_RIGHT;
                }

                buffer = wakeup__do_dialog_item(buffer, "", (WORD)item_id, (WORD)CONTROL_EDIT, style, item_x, y, item_w, ITEM_HEIGHT);
				item_count++;
            }

            if (has_const_string)
            {
                DWORD style = 0;

                if (has_label)
                {
                    style |= ES_RIGHT;
                }

                if (has_center)
                {
                    style |= ES_CENTER;
                }

                buffer = wakeup__do_dialog_item(buffer, "", (WORD)item_id, (WORD)CONTROL_STATIC, style, item_x, y, item_w, ITEM_HEIGHT);
				item_count++;
            }

			if (has_number)
			{
				buffer = wakeup__do_dialog_item(buffer, "", (WORD)item_id, (WORD)CONTROL_EDIT, WS_TABSTOP | WS_BORDER | ES_NUMBER, item_x, y, item_w, ITEM_HEIGHT);
				item_count++;
			}

            if (has_hotkey)
			{
				buffer = wakeup__do_dialog_item(buffer, "", (WORD)item_id, (WORD)CONTROL_BUTTON, WS_TABSTOP, item_x, y, item_w, ITEM_HEIGHT);
				item_count++;
			}

            if (has_combobox)
			{
				buffer = wakeup__do_dialog_item(buffer, "", (WORD)item_id, (WORD)CONTROL_COMBOBOX, WS_HSCROLL | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS, item_x, y, item_w, ITEM_HEIGHT);
				item_count++;
			}

			y += ITEM_HEIGHT;
		}
	}

	*dialog = (DLGTEMPLATE)
	{
		.style = DS_SETFONT | DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU,
		.cdit = (WORD)item_count,
		.cx = PADDING + COL_WIDTH + PADDING,
		.cy = PADDING + ROW_HEIGHT + PADDING + ROW2_HEIGHT + PADDING + ITEM_HEIGHT + PADDING,
	};

	ASSERT(buffer <= end);
}

LRESULT wakeup_dialog_show(WakeupDialogConfig* dialog_config)
{
	if (global_dialog_window)
	{
        EndDialog(global_dialog_window, 0);
		return FALSE;
	}

	WakeupDialogLayout dialog_layout = (WakeupDialogLayout)
	{
		.title = "wakeup",
		.font = "Segoe UI",
		.font_size = 9,
		.groups = (WakeupDialogGroup[])
		{
            {
				.caption = "Settings",
				.rect = { 0, 0, COL_WIDTH, ROW_HEIGHT },
                .items = 
                {
                    { "Windows", ID_WINDOW, ITEM_COMBOBOX | ITEM_LABEL, 64 },
                    { "Period (sec)", ID_PERIOD, ITEM_NUMBER | ITEM_LABEL, 64 },
                    { NULL },
                },
			},
			{ NULL },
		},
	};

	BYTE __declspec(align(4)) buffer[4096];
	wakeup__do_dialog_layout(&dialog_layout, buffer, sizeof(buffer));

	return DialogBoxIndirectParamW(GetModuleHandleW(NULL), (LPCDLGTEMPLATEW)buffer, NULL, wakeup_dialog__proc, (LPARAM)dialog_config);
}

void wakeup_dialog_init(WakeupDialogConfig* dialog_config, WCHAR* ini_path, HICON icon)
{
    global_ini_path = ini_path;
    global_icon = icon;

    dialog_config->menu_shortcut = (DWORD)HOT_KEY('U', MOD_ALT | MOD_CONTROL);
    wakeup_enable_hotkeys();

    wakeup_dialog_show(dialog_config);
}
