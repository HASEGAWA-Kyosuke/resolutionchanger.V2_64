// reschanger.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "resolutionchanger.h"
#include <Commdlg.h>

#define IDB_CHANGE  1001
#define IDB_RESTORE 1002

#define MAX_LOADSTRING 100

static WCHAR *gProgPath;                   // このプログラムファイルのフルパス
static HINSTANCE ghInst;                   // 現在のインターフェイス
static TCHAR gTitle[MAX_LOADSTRING];       // タイトル バーのテキスト
static TCHAR gWindowClass[MAX_LOADSTRING]; // メイン ウィンドウ クラス名

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK AboutDialog(HWND, UINT, WPARAM, LPARAM);
static ATOM MyRegisterClass(HINSTANCE hInstance);

extern BOOL listResolution(HWND hwnd);
extern BOOL setIconPosition(void);
extern BOOL changeResolution(HWND hwnd);
extern BOOL restoreResolution(void);
extern BOOL saveIconPosition(WCHAR *fname);
extern BOOL readIconPosition(WCHAR *fname);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	gProgPath = __wargv[0];

	MSG msg;
	HACCEL hAccelTable;

	LoadString(hInstance, IDS_APP_TITLE, gTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_RESOLUTIONCHANGER, gWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 動かすインスタンスは一つだけ
	HWND hWnd = FindWindow(gWindowClass, NULL);
	if (hWnd) {
		SetForegroundWindow(hWnd);
		return 0;
	}

	ghInst = hInstance;

	hWnd = CreateWindow(gWindowClass, gTitle,
		WS_CAPTION | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 240, 180, NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		return FALSE;
	}
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_RESOLUTIONCHANGER));

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (! TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
static ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style         = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc   = WndProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = hInstance;
	wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LARGE));
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName  = MAKEINTRESOURCE(IDR_RESOLUTIONCHANGER);
	wcex.lpszClassName = gWindowClass;
	wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:  メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 中止メッセージを表示して戻る
//
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hWndCb, hWndBtnChange, hWndBtnRestore;

	switch (message) {
	case WM_CREATE:
	{
		HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH, L"MS UI Gothic");
		// 解像度変更 ボタン
		hWndBtnChange = CreateWindowW(L"BUTTON", L"解像度変更", WS_CHILD | WS_VISIBLE | BS_TEXT,
			30, 90, 75, 25, hWnd, (HMENU)IDB_CHANGE, ghInst, NULL);
		SendMessage(hWndBtnChange, WM_SETFONT, (WPARAM)hFont, 0);

		// 解像度復帰 ボタン
		hWndBtnRestore = CreateWindowW(L"BUTTON", L"元に戻す", WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_TEXT,
			120, 90, 75, 25, hWnd, (HMENU)IDB_RESTORE, ghInst, NULL);
		SendMessage(hWndBtnRestore, WM_SETFONT, (WPARAM)hFont, 0);

		hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH, L"FixedSys");
		// 解像度一覧表示 コンボボックス
		hWndCb = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_SORT | CBS_SIMPLE | CBS_DROPDOWN,
			15, 10, 200, 110, hWnd, NULL, ghInst, NULL);
		SendMessage(hWndCb, WM_SETFONT, (WPARAM)hFont, 0);

		// コンボボックスのリストに解像度一覧をセットする
		if (! listResolution(hWndCb)) {
			MessageBox(NULL, L"解像度一覧が取得できません", L"エラー", MB_OK);
			exit(-1);
		}

		// もともとのアイコン位置を保存する
		if (!setIconPosition()) {
			MessageBox(NULL, L"アイコン位置が取得できません", L"エラー", MB_OK);
			exit(-1);
		}
		break;
	}
	case WM_COMMAND:
	{
		// アイコン位置情報ファイルに関する変数
		static OPENFILENAME ofn;
		static TCHAR filename_full[MAX_PATH];
		static TCHAR filename[MAX_PATH];

		int wmId = LOWORD(wParam);
		int wmEvent = HIWORD(wParam);
		HMENU hMenu = GetMenu(hWnd);

		switch (wmId) {
		// 解像度変更メニュー or ボタン
		case IDM_CHANGE:
		case IDB_CHANGE:
			if (changeResolution(hWndCb)) {  // 解像度を変更する
				EnableWindow(hWndBtnChange, FALSE);
				EnableWindow(hWndBtnRestore, TRUE);
				EnableMenuItem(hMenu, IDM_CHANGE, MF_GRAYED);
				EnableMenuItem(hMenu, IDM_RESTORE, MF_ENABLED);
				EnableMenuItem(hMenu, IDM_SAVE, MF_GRAYED);
				EnableMenuItem(hMenu, IDM_READ, MF_GRAYED);
			}
			break;

		// 元に戻すメニュー or ボタン
		case IDM_RESTORE:
		case IDB_RESTORE:
			EnableWindow(hWndBtnRestore, FALSE);
			if (restoreResolution()) {       // 解像度とアイコンの配置を元に戻す
				EnableWindow(hWndBtnChange, TRUE);
				EnableMenuItem(hMenu, IDM_CHANGE, MF_ENABLED);
				EnableMenuItem(hMenu, IDM_RESTORE, MF_GRAYED);
				EnableMenuItem(hMenu, IDM_SAVE, MF_ENABLED);
				EnableMenuItem(hMenu, IDM_READ, MF_ENABLED);
			} else {
				EnableWindow(hWndBtnRestore, TRUE);
			}
			break;

		case IDM_SAVE:
			ZeroMemory(&ofn, sizeof ofn);           // 最初にゼロクリアしておく
			ofn.lStructSize = sizeof ofn;           // 構造体のサイズ
			ofn.hwndOwner = hWnd;                   // コモンダイアログの親ウィンドウハンドル
			ofn.lpstrFilter = L"ICON位置情報ファイル(*.icp)\0*.icp\0"; // ファイルの種類
			ofn.lpstrFile = filename_full;          // 選択されたファイル名(フルパス)を受け取る変数のアドレス
			ofn.lpstrFileTitle = filename;          // 選択されたファイル名を受け取る変数のアドレス
			ofn.nMaxFile = _countof(filename_full); // lpstrFileに指定した変数のサイズ
			ofn.nMaxFileTitle = _countof(filename); // lpstrFileTitleに指定した変数のサイズ
			ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;  // フラグ指定
			ofn.lpstrTitle = L"ICON位置情報ファイルを書く"; // コモンダイアログのキャプション
			ofn.lpstrDefExt = L"icp";               // デフォルトのファイルの種類
			// ファイルを開くコモンダイアログを作成
			if (GetSaveFileName(&ofn)) {
				if (wcscmp(filename_full + ofn.nFileExtension, ofn.lpstrDefExt) != 0) {
					if (_countof(filename_full) - ofn.nFileExtension > wcslen(ofn.lpstrDefExt)) {
						(void)wcscpy_s(filename_full + ofn.nFileExtension,
							_countof(filename_full) - ofn.nFileExtension,
							ofn.lpstrDefExt);
					}
				}
				if (!saveIconPosition(filename_full)) {
					MessageBox(NULL, L"書き込めません", L"エラー", MB_OK);
				}
			}
			break;

		case IDM_READ:
			ZeroMemory(&ofn, sizeof ofn);           // 最初にゼロクリアしておく
			ofn.lStructSize = sizeof ofn;           // 構造体のサイズ
			ofn.hwndOwner = hWnd;                   // コモンダイアログの親ウィンドウハンドル
			ofn.lpstrFilter = L"ICON位置情報ファイル(*.icp)\0*.icp\0"; // ファイルの種類
			ofn.lpstrFile = filename_full;          // 選択されたファイル名(フルパス)を受け取る変数のアドレス
			ofn.lpstrFileTitle = filename;          // 選択されたファイル名を受け取る変数のアドレス
			ofn.nMaxFile = _countof(filename_full); // lpstrFileに指定した変数のサイズ
			ofn.nMaxFileTitle = _countof(filename); // lpstrFileTitleに指定した変数のサイズ
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_READONLY;   // フラグ指定
			ofn.lpstrTitle = L"ICON位置情報ファイルを読む"; // コモンダイアログのキャプション
			ofn.lpstrDefExt = L"icp";               // デフォルトのファイルの種類
			// ファイルを開くコモンダイアログを作成
			if (GetOpenFileName(&ofn)) {
				if (wcscmp(filename_full + ofn.nFileExtension, ofn.lpstrDefExt) != 0) {
					MessageBox(NULL, L"ICON位置情報ファイルではありません", L"エラー", MB_OK);
				} else {
					(void)readIconPosition(filename_full);
				}
			}
			break;

		// 終了メニュー
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		// Aboutメニュー
		case IDM_ABOUT:
			DialogBox(ghInst, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDialog);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		// このアプリケーションのウィンドウが閉じるとき
		if (IsWindowEnabled(hWndBtnRestore)) {
			if (MessageBox(NULL, L"解像度を元に戻しますか?", L"終了確認", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
				(void)restoreResolution(); // 解像度とアイコンの配置を元に戻す
			}
		}
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
	return 0;
}

/*
 * AboutDialog のメッセージハンドラ
 */
static INT_PTR CALLBACK AboutDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message) {
	case WM_INITDIALOG:
	{
		int bufSize = GetFileVersionInfoSize(gProgPath, 0);
		if (bufSize > 0) {
			void *buf = (void *)malloc(sizeof (BYTE) * bufSize);
			if (buf != NULL) {
				GetFileVersionInfo(gProgPath, 0, bufSize, buf);

				void *str;
				unsigned int strSize;
				VerQueryValue(buf, L"\\StringFileInfo\\0411fde9\\ProductName", (LPVOID *)&str, &strSize);
				SetWindowText(hDlg, (WCHAR *)str);
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT2), (WCHAR *)str);

				VerQueryValue(buf, L"\\StringFileInfo\\0411fde9\\FileVersion", (LPVOID *)&str, &strSize);
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), (WCHAR *)str);

				VerQueryValue(buf, L"\\StringFileInfo\\0411fde9\\LegalCopyright", (LPVOID *)&str, &strSize);
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT3), (WCHAR *)str);

				free(buf);
			}
		}
		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}

	return (INT_PTR)FALSE;
}
