// WifiAudioSender.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "WifiAudioSender.h"
#pragma comment(lib, "comctl32.lib")
#include <commctrl.h>
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

HWND hNowActive = 0;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WIFIAUDIOSENDER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow)) return FALSE;

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIFIAUDIOSENDER));

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			if (NULL == hNowActive || !IsDialogMessage(hNowActive, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIFIAUDIOSENDER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_WIFIAUDIOSENDER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	HWND hWnd;
	hInst = hInstance; // Store instance handle in our global variable
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, CW_USEDEFAULT, 0, 320, 220, NULL, NULL, hInstance, NULL);
	if (!hWnd) return FALSE;
	UpdateWindow(hWnd);
	return TRUE;
}
AudioProcesser ap = AudioProcesser();
NOTIFYICONDATA notify;
#define WM_NOTIFY_ICON (WM_USER+1)
void ShowContextMenu(HWND hWnd) {
	POINT curPoint;
	GetCursorPos(&curPoint);
	HMENU pMenu = CreatePopupMenu();
	if (pMenu) {
		if (ap.isRuninng()) {
			AppendMenu(pMenu, MF_STRING | MF_DISABLED, ID_FEATURE_START, L"Start");
			AppendMenu(pMenu, MF_STRING, ID_FEATURE_STOP, L"Stop");
		} else {
			AppendMenu(pMenu, MF_STRING, ID_FEATURE_START, L"Start");
			AppendMenu(pMenu, MF_STRING | MF_DISABLED, ID_FEATURE_STOP, L"Stop");
		}
		AppendMenu(pMenu, MF_SEPARATOR, 0, L"");
		AppendMenu(pMenu, MF_STRING, IDM_EXIT, L"Exit");
		SetForegroundWindow(hWnd);
		TrackPopupMenu(pMenu, TPM_BOTTOMALIGN, curPoint.x, curPoint.y, 0, hWnd, NULL);
		DestroyMenu(pMenu);
	}
}
HWND btnStart, btnStop, txtIP, comboFrom, comboTo;
HFONT hFont;
void MulDivRect(RECT &rct, int nNumerator, int nDenominator) {
	rct.top = MulDiv(rct.top, nNumerator, nDenominator);
	rct.left = MulDiv(rct.left, nNumerator, nDenominator);
	rct.bottom = MulDiv(rct.bottom, nNumerator, nDenominator);
	rct.right = MulDiv(rct.right, nNumerator, nDenominator);
}
RECT rctIP, rctStart, rctStop;
WNDPROC prevEditProc; HWND hWndWindow;
LRESULT CALLBACK editProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_KEYDOWN && wParam == VK_RETURN) {
		SendMessage(hWndWindow, WM_COMMAND, ID_FEATURE_START, NULL);
		return 0;
	}
	return prevEditProc(hWnd, message, wParam, lParam);
}
void loadControls(HWND hWnd) {
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
	hFont = CreateFontIndirect(&(ncm.lfMessageFont));

	RECT rct;
	int nMargin = 15, nTextWidth = 32;
	SendMessage(hWnd, WM_SETFONT, (WPARAM) hFont, true);

	GetClientRect(hWnd, &rct);

	rctStart.right = 60;
	rctStart.bottom = 24;
	rctStart.left = (rct.right - rct.left - rctStart.right * 2) / 3;
	rctStart.top = (rct.bottom - rct.top) - rctStart.bottom - nMargin;

	rctStop.right = 60;
	rctStop.bottom = 24;
	rctStop.left = (rct.right - rct.left - rctStart.right * 2) / 3 * 2 + rctStart.right;
	rctStop.top = (rct.bottom - rct.top) - rctStop.bottom - nMargin;

	rctIP.left = 80;
	rctIP.top = nMargin - 3;
	rctIP.right = 320 - rctIP.left * 2;
	rctIP.bottom = 22;

	btnStart = CreateWindow(WC_BUTTON, _T("Start"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_TEXT, rctStart.left, rctStart.top, rctStart.right, rctStart.bottom, hWnd, (HMENU) ID_FEATURE_START, hInst, NULL);
	btnStop = CreateWindow(WC_BUTTON, _T("Stop"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_TEXT | WS_DISABLED, rctStop.left, rctStop.top, rctStop.right, rctStop.bottom, hWnd, (HMENU) ID_FEATURE_STOP, hInst, NULL);
	txtIP = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, _T(""), WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL, rctIP.left, rctIP.top, rctIP.right, rctIP.bottom, hWnd, (HMENU) ID_CONTROL_IP, hInst, NULL);

	rctIP.top += rctIP.bottom;
	comboFrom = CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, _T(""), WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS, rctIP.left, rctIP.top, rctIP.right, rctIP.bottom, hWnd, (HMENU) ID_CONTROL_COMBO_FROM, hInst, NULL);
	rctIP.top += rctIP.bottom;
	comboTo = CreateWindowEx(WS_EX_CLIENTEDGE, WC_COMBOBOX, _T(""), WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS, rctIP.left, rctIP.top, rctIP.right, rctIP.bottom, hWnd, (HMENU) ID_CONTROL_COMBO_TO, hInst, NULL);

	prevEditProc = (WNDPROC) SetWindowLongPtr(txtIP, GWLP_WNDPROC, (LONG_PTR) editProc);

	SendMessage(btnStart, WM_SETFONT, (WPARAM) hFont, true);
	SendMessage(btnStop, WM_SETFONT, (WPARAM) hFont, true);
	SendMessage(txtIP, WM_SETFONT, (WPARAM) hFont, true);
	SendMessage(txtIP, EM_SETLIMITTEXT, 250, 0);
	SendMessage(comboFrom, WM_SETFONT, (WPARAM) hFont, true);
	SendMessage(comboTo, WM_SETFONT, (WPARAM) hFont, true);
	ShowWindow(hWnd, SW_SHOW);
}
WCHAR dev[2][256][256] = {};
UINT deviceCount = 0;
void fillCombo() {
	IMMDeviceCollection* pDevices = getAudioDevices();
	IMMDevice *pDevice;
	WCHAR *pName;
	IPropertyStore *pProps = NULL;
	HRESULT hr = NULL;
	SendMessage(comboFrom, CB_ADDSTRING, 0, (LPARAM) _T("Blank audio"));
	SendMessage(comboFrom, CB_ADDSTRING, 0, (LPARAM) _T("UDP In"));
	SendMessage(comboTo, CB_ADDSTRING, 0, (LPARAM) _T("Don't copy"));
	if (pDevices == NULL)
		return;
	pDevices->GetCount(&deviceCount);
	for (UINT i = 0; i < deviceCount; i++) {
		hr = pDevices->Item(i, &pDevice);
		EXIT_ON_ERROR(hr);
		hr = pDevice->GetId(&pName);
		EXIT_ON_ERROR(hr);
		lstrcpy(dev[0][i], pName);
		CoTaskMemFree(pName);
		hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
		EXIT_ON_ERROR(hr);
		PROPVARIANT sDevName;
		hr = pProps->GetValue(PKEY_DeviceInterface_FriendlyName, &sDevName);
		EXIT_ON_ERROR(hr);
		lstrcpy(dev[1][i], sDevName.pwszVal);
		PropVariantClear(&sDevName);
		SAFE_RELEASE(pDevice);
		SendMessage(comboFrom, CB_ADDSTRING, 0, (LPARAM) dev[1][i]);
		SendMessage(comboTo, CB_ADDSTRING, 0, (LPARAM) dev[1][i]);
	}
	SendMessage(comboFrom, (UINT) CB_SETCURSEL, deviceCount, 0);
	SendMessage(comboTo, (UINT) CB_SETCURSEL, deviceCount, 0);
Exit:
	SAFE_RELEASE(pDevice);
	SAFE_RELEASE(pDevices);
}
TCHAR *selFrom, *selTo;
HWND hLastFocus;
bool visible;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	RECT rctThis;
	switch (message) {
		case WM_ACTIVATE:
		{
			if (WA_INACTIVE == wParam) {
				hLastFocus = GetFocus();
				hNowActive = 0;
			} else {
				hNowActive = 0;
			}
			break;
		}
		case WM_SETFOCUS:
		{
			if (hLastFocus) {
				SetFocus(hLastFocus);
			}
			break;
		}
		case WM_CREATE:
		{
			hWndWindow = hWnd;
			loadControls(hWnd);
			fillCombo();
			memset(&notify, 0, sizeof(notify));
			notify.cbSize = sizeof(notify);
			notify.hWnd = hWnd;
			notify.uID = 1;
			notify.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
			notify.uVersion = NOTIFYICON_VERSION_4;
			notify.uCallbackMessage = WM_NOTIFY_ICON;
			notify.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_WIFIAUDIOSENDER));
			GetWindowText(hWnd, notify.szTip, sizeof(notify.szTip));
			GetWindowText(hWnd, notify.szInfo, sizeof(notify.szInfo));
			Shell_NotifyIcon(NIM_ADD, &notify);
			SetFocus(txtIP);

			TCHAR app[512];
			TCHAR str[255];
			GetModuleFileName(NULL, app, sizeof(app));
			wsprintf(wcsrchr(app, L'.') + 1, L"ini");
			GetPrivateProfileString(L"WAS", L"IP", NULL, str, sizeof(str), app);
			SetWindowText(txtIP, str);
			GetPrivateProfileString(L"WAS", L"from", NULL, str, sizeof(str), app);
			if (wcscmp(str, L"0")) {
				if (wcscmp(str, L"1") == 0)
					SendMessage(comboFrom, CB_SETCURSEL, 1, 0);
				else
					for (UINT i = 0; i < deviceCount; i++)
						if (wcscmp(dev[0][i], str) == 0) {
							SendMessage(comboFrom, CB_SETCURSEL, i + 2, 0);
							break;
						}
			}
			GetPrivateProfileString(L"WAS", L"to", NULL, str, sizeof(str), app);
			if (wcscmp(str, L"0"))
				for (UINT i = 0; i < deviceCount; i++)
					if (wcscmp(dev[0][i], str) == 0) {
						SendMessage(comboTo, CB_SETCURSEL, i + 1, 0);
						break;
					}
			if (GetPrivateProfileInt(L"WAS", L"started", 0, app)) {
				SendMessage(hWnd, WM_COMMAND, ID_FEATURE_START, 0);
				if (!(visible = GetPrivateProfileInt(L"WAS", L"visible", 1, app)))
					ShowWindow(hWnd, SW_HIDE);
			}
			break;
		}
		case WM_NOTIFY_ICON:
		{
			// LOWORD(lParam) contains notification events, such as NIN_BALLOONSHOW, NIN_POPUPOPEN, or WM_CONTEXTMENU.
			switch (LOWORD(lParam)) {
				case 0x202:
				{ // Up L
					visible = !visible;
					if (!visible) {
						ShowWindow(hWnd, SW_HIDE);
					} else {
						ShowWindow(hWnd, SW_SHOW);
						BringWindowToTop(hWnd);
						SetForegroundWindow(hWnd);
					}
					break;
				}
				case 0x205:
				{ // Up R
					ShowContextMenu(hWnd);
					break;
				}
			}
			break;
		}
		case WM_COMMAND:
		{
			int wmId, wmEvent;
			wmId = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			switch (wmId) {
				case ID_CONTROL_COMBO_FROM:
				{
					break;
				}
				case ID_CONTROL_COMBO_TO:
				{
					int pos = SendMessage((HWND) lParam, (UINT) CB_GETCURSEL, 0, 0);
					selTo = dev[0][pos];
					break;
				}
				case ID_FEATURE_START:
				{
					TCHAR str[256];
					char sIp[256];
					int len;
					GetWindowText(txtIP, str, 255);
					WideCharToMultiByte(CP_ACP, 0, str, len = lstrlen(str), sIp, sizeof(sIp), NULL, NULL);
					sIp[len] = 0;
					int posFrom = SendMessage(comboFrom, (UINT) CB_GETCURSEL, 0, 0);
					int posTo = SendMessage(comboTo, (UINT) CB_GETCURSEL, 0, 0);
					int res = ap.StartSending(sIp,
						posFrom < 2 ? (WCHAR*) posFrom : dev[0][posFrom - 2],
						posTo < 1 ? NULL : dev[0][posTo - 1]
					);
					if (res == ERROR_SUCCESS) {
						EnableMenuItem(GetMenu(hWnd), ID_FEATURE_START, MF_DISABLED);
						EnableMenuItem(GetMenu(hWnd), ID_FEATURE_STOP, 0);
						EnableWindow(btnStart, false);
						EnableWindow(btnStop, true);
						EnableWindow(txtIP, false);
						SetFocus(btnStop);
					}
					break;
				}
				case ID_FEATURE_STOP:
				{
					if (ap.StopSending() == ERROR_SUCCESS) {
						EnableMenuItem(GetMenu(hWnd), ID_FEATURE_START, 0);
						EnableMenuItem(GetMenu(hWnd), ID_FEATURE_STOP, MF_DISABLED);
						EnableWindow(btnStart, true);
						EnableWindow(btnStop, false);
						EnableWindow(txtIP, true);
						SetFocus(btnStart);
					}
					break;
				}
				case IDM_ABOUT:
				{
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;
				}
				case IDM_EXIT:
				{
					DestroyWindow(hWnd);
					break;
				}
				default:
				{
					return DefWindowProc(hWnd, message, wParam, lParam);
				}
			}
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			GetClientRect(hWnd, &rctThis);
			hdc = BeginPaint(hWnd, &ps);
			SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			HGDIOBJ hOldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
			Rectangle(hdc, 0, 0, rctThis.right - rctThis.left, rctStart.top - (rctThis.bottom - rctThis.top - rctStart.top - rctStart.bottom));
			TextOut(hdc, rctIP.left - 64, rctIP.top - rctIP.bottom * 2, _T("IP:"), 3);
			TextOut(hdc, rctIP.left - 64, rctIP.top - rctIP.bottom * 1, _T("From:"), 5);
			TextOut(hdc, rctIP.left - 64, rctIP.top - rctIP.bottom * 0, _T("To:"), 3);
			DeleteObject(SelectObject(hdc, hOldPen));
			EndPaint(hWnd, &ps);
			break;
		}
		case WM_CLOSE:
		{
			if (ap.isRuninng()) {
				visible = false;
				ShowWindow(hWnd, SW_HIDE);
			} else
				DestroyWindow(hWnd);
			break;
		}
		case WM_DESTROY:
		{
			TCHAR app[512];
			TCHAR str[255];
			GetModuleFileName(NULL, app, sizeof(app));
			wsprintf(wcsrchr(app, L'.') + 1, L"ini");
			GetWindowText(txtIP, str, 255);
			int from = SendMessage(comboFrom, (UINT) CB_GETCURSEL, 0, 0);
			int to = SendMessage(comboTo, (UINT) CB_GETCURSEL, 0, 0);
			WritePrivateProfileString(L"WAS", L"from", from == 0 ? L"0" : from == 1 ? L"1" : dev[0][from - 2], app);
			WritePrivateProfileString(L"WAS", L"to", to == 0 ? L"0" : dev[0][to - 1], app);
			WritePrivateProfileString(L"WAS", L"IP", str, app);
			WritePrivateProfileString(L"WAS", L"started", visible ? L"0" : L"1", app);
			WritePrivateProfileString(L"WAS", L"visible", IsWindowVisible(hWnd) ? L"1" : L"0", app);
			Shell_NotifyIcon(NIM_DELETE, &notify);
			ap.StopSending();
			PostQuitMessage(0);
			break;
		}
		default:
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
		case WM_INITDIALOG:
			return (INT_PTR) TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR) TRUE;
			}
			break;
	}
	return (INT_PTR) FALSE;
}
