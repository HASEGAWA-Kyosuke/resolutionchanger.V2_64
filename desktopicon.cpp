// desktopIcon.cpp
//

#include "stdafx.h"
#include <stdio.h>
#include <commctrl.h>

#define MAXLISTCOUNT 1000

typedef struct {
	int   index;
	POINT point;
	WCHAR name[MAX_PATH];
} ICON_INFO;

static int resModeMap[MAXLISTCOUNT];
static DEVMODE devModeOrg;
static ICON_INFO *iconInfoOrg = NULL;
static int numIconOrg = 0;

static HWND getDesktopHandle(void);
static ICON_INFO *getIconPos(int *pNumIcon);
static ICON_INFO *readIconPos(WCHAR *fname, int *pNumIcon);
static BOOL writeIconPos(WCHAR *fname, ICON_INFO *iconInfo, int numIcon);
static int comp(const void* a, const void* b);
static ICON_INFO *search(ICON_INFO *list, int num, WCHAR *name);

/*
 * コンボボックスのリストに解像度一覧をセットする
 */
BOOL listResolution(HWND hwnd) {
	// 現在の解像度を得る
	if (! EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devModeOrg)) {
		return FALSE;
	}

	// 解像度一覧を得る
	int listIndex;
	DEVMODE devMode;
	for (int resIndex = 0; EnumDisplaySettings(NULL, resIndex, &devMode); resIndex++) {
		// 解像度リストに載せるものを絞る
		if (devMode.dmDisplayFrequency == devModeOrg.dmDisplayFrequency         // 周波数が現状と同じ
		 && devMode.dmBitsPerPel == devModeOrg.dmBitsPerPel                     // 1ピクセルのビット数が現状と同じ
		 && devMode.dmDisplayOrientation == 0                                   // 正立のみ
		 && devMode.dmDefaultSource == 0
		 && devMode.dmPelsWidth >= 640
		 && devMode.dmPelsHeight >= 480
		 && ChangeDisplaySettings(&devMode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL // テスト成功
		) {
			WCHAR buf[30];
			_snwprintf_s(buf, _countof(buf),
					L"%4dx%4d %dbit %dHz",
					devMode.dmPelsWidth, devMode.dmPelsHeight, devMode.dmBitsPerPel, devMode.dmDisplayFrequency,
					_TRUNCATE);
			if ((listIndex = SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)buf)) >= MAXLISTCOUNT - 1) {
				// MAXLISTCOUNT を超えたら打ち切り
				break;
			}

			// 解像度リストのIDと、全解像度一覧IDとのマップを設定
			resModeMap[listIndex] = resIndex;

			// 現在の解像度を選択状態にする
			if (devMode.dmPelsWidth == devModeOrg.dmPelsWidth
			 && devMode.dmPelsHeight == devModeOrg.dmPelsHeight) {
				SendMessage(hwnd, CB_SETCURSEL, listIndex, 0);
			}
		}
	}
	return TRUE;
}

/*
 * もともとのアイコン位置をグローバル変数にセットする
 */
BOOL setIconPosition(void)
{
	if (! iconInfoOrg) {
		// もともとの解像度とアイコン位置をグローバル変数にセットする
		iconInfoOrg = getIconPos(&numIconOrg);
	}
	return (iconInfoOrg != NULL);
}

/*
 * 解像度を変更する
 */
BOOL changeResolution(HWND hListWnd)
{
	// リストから選択されている解像度を得る
	DEVMODE devMode;
	int listIndex = SendMessage(hListWnd, CB_GETCURSEL, 0, 0);
	int resIndex = resModeMap[listIndex];
	if (! EnumDisplaySettings(NULL, resIndex, &devMode)) {
		return FALSE;
	}

	// 解像度を変更する
	if (ChangeDisplaySettings(&devMode, CDS_UPDATEREGISTRY | CDS_GLOBAL) != DISP_CHANGE_SUCCESSFUL) {
		return FALSE;
	}

	return TRUE;
}

/*
 * 解像度とアイコンの位置を元に戻す
 */
BOOL restoreResolution(void)
{
	//解像度を元に戻す
	if (ChangeDisplaySettings(&devModeOrg, CDS_UPDATEREGISTRY | CDS_GLOBAL) != DISP_CHANGE_SUCCESSFUL) {
		return FALSE;
	}

	//少し待つ (画面が表示されてアイコンが自動再配置されるのに時間がかかる)
	Sleep(1000);

	//現在のアイコンの位置を取得
	int numIcon;
	ICON_INFO *iconInfo = getIconPos(&numIcon);
	//もともとのアイコンの位置をアイコン名で紐付けしなおす
	for (int i = 0; i < numIcon; i++) {
		ICON_INFO *a = search(iconInfoOrg, numIconOrg, iconInfo[i].name);
		if (a != NULL) {
			iconInfo[i].point = a->point;
		} else {
			//ない場合(後から作られた場合)はデスクトップ中央に配置する
			iconInfo[i].point.x = devModeOrg.dmPelsWidth / 2;
			iconInfo[i].point.y = devModeOrg.dmPelsHeight / 2;
		}
	}

	//左上から下へ右へ再配置することにより、1回で整列させる
	qsort(iconInfo, numIcon, sizeof (ICON_INFO), comp);

	//デスクトップのウィンドウハンドルを取得する
	HWND hWnd = getDesktopHandle();
	if (! hWnd) {
		free(iconInfo);
		return FALSE;
	}
	//アイコンの配置を元に戻す
	for (int i = 0; i < numIcon; i++) {
		ListView_SetItemPosition(hWnd, iconInfo[i].index, iconInfo[i].point.x, iconInfo[i].point.y);
	}
	free(iconInfo);

	return TRUE;
}

/*
 * アイコン配置をファイルから読み込んで初期値と置き換える
 * さらにその配置に従って再配置する
 */
BOOL readIconPosition(WCHAR *fname)
{
	if (! iconInfoOrg) {
		free(iconInfoOrg);
	}
	iconInfoOrg = readIconPos(fname, &numIconOrg);

	return restoreResolution(); 
}

/*
 * 現在のアイコン配置をファイルに書く
 */
BOOL saveIconPosition(WCHAR *fname)
{
	int numIcon;
	ICON_INFO *iconInfo= getIconPos(&numIcon);
	BOOL ret = writeIconPos(fname, iconInfo, numIcon);
	free(iconInfo);

	return ret;
}


////////////////////////////////////////////////////////////////////////////////////

/*
 * デスクトップのアイコン位置を取得する
 */
static ICON_INFO *getIconPos(int *pNumIcon)
{
	*pNumIcon = 0;

	//デスクトップのウィンドウハンドルを取得する
	HWND hWnd = getDesktopHandle();
	if (! hWnd) {
		return NULL;
	}

	//デスクトップウィンドウを持つプロセスのハンドルを開く
	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, dwProcessId);
	if (! hProcess) {
		return NULL;
	}

	//デスクトップアイコンの数を取得
	*pNumIcon = ListView_GetItemCount(hWnd);
	if (*pNumIcon < 0) {
		return NULL;
	}

	//アイコンの位置を格納する配列を確保
	ICON_INFO *iconInfo = (ICON_INFO *)malloc(sizeof (ICON_INFO) * *pNumIcon);
	if (iconInfo == NULL) {
		return NULL;
	}

	//プロセスへメモリ空間をコミット
	POINT *pnt = (POINT *)VirtualAllocEx(hProcess, NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (pnt == NULL) {
		return NULL;
	}
	BYTE *adr = (BYTE *)VirtualAllocEx(hProcess, NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (adr == NULL) {
		VirtualFreeEx(hProcess, pnt, 0, MEM_RELEASE);
		return NULL;
	}

	LVITEM lvi;
	lvi.iSubItem = 0;
	lvi.pszText = (WCHAR *)(adr + sizeof (LVITEM));	//文字を受け取るバッファ オフセットはLVITEM構造体のサイズ
	lvi.cchTextMax = MAX_PATH;

	for (int iconIndex = 0; iconIndex < *pNumIcon; iconIndex++) {
		//アイコンIDをセット
		iconInfo[iconIndex].index = iconIndex;

		//アイコン位置を取得
		ListView_GetItemPosition(hWnd, iconIndex, pnt);
		//メモリをコピー (アイコン位置をセット)
		ReadProcessMemory(hProcess, pnt, &(iconInfo[iconIndex].point), sizeof (POINT), NULL);

		//アイコンの名前を取得
		WriteProcessMemory(hProcess, adr, &lvi, sizeof (LVITEM), NULL);
		SendMessage(hWnd, LVM_GETITEMTEXT, iconIndex, (LPARAM)adr);
		ReadProcessMemory(hProcess, adr + sizeof (LVITEM), iconInfo[iconIndex].name, MAX_PATH, NULL);
	}
	VirtualFreeEx(hProcess, pnt, 0, MEM_RELEASE);
	VirtualFreeEx(hProcess, adr, 0, MEM_RELEASE);

	qsort(iconInfo, *pNumIcon, sizeof (ICON_INFO), comp);

	return iconInfo;
}

/*
 * デスクトップのハンドルを取得
 */
static HWND getDesktopHandle(void)
{
	HWND hWnd;

	int nTry = 0;
	while (true) {
		hWnd = FindWindow(L"ProgMan", NULL);
		if (! hWnd) {
			return 0;
		}
		hWnd = GetWindow(hWnd, GW_CHILD);
		if (! hWnd) {              //解像度変更直後は失敗することがある
			Sleep(1);              //その場合、1ms待って始めから再試行
			if (nTry++ > 100) {    //100回(100ms)失敗した場合は本当にエラー
				return 0;
			}
		} else {
			break;
		}
	}
	hWnd = GetWindow(hWnd, GW_CHILD);
	if (! hWnd) {
		return 0;
	}

	return hWnd;
}

/*
 * アイコン情報のソート用比較ロジック
 */
static int comp(const void* a, const void* b)
{
	ICON_INFO *m, *n;
	m = (ICON_INFO *)a;
	n = (ICON_INFO *)b;

	if (m->point.x < n->point.x) {
		return -1;
	} else if (m->point.x > n->point.x) {
		return 1;
	} else if (m->point.y < n->point.y) {
		return -1;
	} else if (m->point.y > n->point.y) {
		return 1;
	}
	return 0;
}

/*
 * アイコン情報検索ロジック
 */
static ICON_INFO *search(ICON_INFO *list, int num, WCHAR *name)
{
	for (int i = 0; i < num; i++) {
		if (wcscmp(list[i].name, name) == 0) {
			return list + i;
		}
	}
	return NULL;
}

/*
 * アイコンの配置をファイルに書く
 */
static BOOL writeIconPos(WCHAR *fname, ICON_INFO *iconInfo, int numIcon)
{
	FILE *fp;
	if (_wfopen_s(&fp, fname, L"w, ccs=UNICODE") != 0) {
		return FALSE;
	}
	if (fwprintf_s(fp, L"%d\n", numIcon) < 0) {
		fclose(fp);
		return FALSE;
	}
	for (int i = 0; i < numIcon; i++) {
		if (fwprintf_s(fp, L"%d %d %d %s\n",
				iconInfo[i].index, iconInfo[i].point.x, iconInfo[i].point.y, iconInfo[i].name) < 0) {
			fclose(fp);
			return FALSE;
		}
	}
	fclose(fp);

	return TRUE;
}

/*
 * アイコンの配置をファイルから読む
 */
static ICON_INFO *readIconPos(WCHAR *fname, int *pNumIcon)
{
	FILE *fp;
	if (_wfopen_s(&fp, fname, L"r, ccs=UNICODE") != 0) {
		return NULL;
	}

	int scanRet = fwscanf_s(fp, L"%d", pNumIcon);
	if (scanRet == EOF || scanRet == 0) {
		fclose(fp);
		return NULL;
	}

	//アイコンの位置を格納する配列を確保
	ICON_INFO *iconInfo = (ICON_INFO *)malloc(sizeof (ICON_INFO) * *pNumIcon);
	if (iconInfo == NULL) {
		fclose(fp);
		return NULL;
	}

	for (int i = 0; i < *pNumIcon; i++) {
		scanRet = fwscanf_s(fp, L"%d", &(iconInfo[i].index));
		if (scanRet == EOF || scanRet == 0 || iconInfo[i].index >= *pNumIcon) {
			break;
		}
		scanRet = fwscanf_s(fp, L"%d %d", &(iconInfo[i].point.x), &(iconInfo[i].point.y));
		if (scanRet == EOF || scanRet == 0) {
			break;
		}

		WCHAR c;
		while ((c = getwc(fp)) != EOF) {  // 3項目目のアイコン名までの空白を読み飛ばす
			if (c != ' ') {
				ungetwc(c, fp);
				break;
			}
		}
		if (fgetws(iconInfo[i].name, MAX_PATH, fp) == NULL) {  // アイコン名を読む
			break;
		}
		int len = wcslen(iconInfo[i].name);
		if (iconInfo[i].name[len - 1] == L'\n') {  // 最後の改行文字を消す
			iconInfo[i].name[len - 1] = L'\0';
		}
	}
	fclose(fp);

	return iconInfo;
}
