// reschanger.cpp : �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include "resolutionchanger.h"
#include <Commdlg.h>

#define IDB_CHANGE  1001
#define IDB_RESTORE 1002

#define MAX_LOADSTRING 100

static WCHAR *gProgPath;                   // ���̃v���O�����t�@�C���̃t���p�X
static HINSTANCE ghInst;                   // ���݂̃C���^�[�t�F�C�X
static TCHAR gTitle[MAX_LOADSTRING];       // �^�C�g�� �o�[�̃e�L�X�g
static TCHAR gWindowClass[MAX_LOADSTRING]; // ���C�� �E�B���h�E �N���X��

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

	// �������C���X�^���X�͈����
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
//  �֐�: MyRegisterClass()
//
//  �ړI: �E�B���h�E �N���X��o�^���܂��B
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
//  �֐�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  �ړI:  ���C�� �E�B���h�E�̃��b�Z�[�W���������܂��B
//
//  WM_COMMAND	- �A�v���P�[�V���� ���j���[�̏���
//  WM_PAINT	- ���C�� �E�B���h�E�̕`��
//  WM_DESTROY	- ���~���b�Z�[�W��\�����Ė߂�
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
		// �𑜓x�ύX �{�^��
		hWndBtnChange = CreateWindowW(L"BUTTON", L"�𑜓x�ύX", WS_CHILD | WS_VISIBLE | BS_TEXT,
			30, 90, 75, 25, hWnd, (HMENU)IDB_CHANGE, ghInst, NULL);
		SendMessage(hWndBtnChange, WM_SETFONT, (WPARAM)hFont, 0);

		// �𑜓x���A �{�^��
		hWndBtnRestore = CreateWindowW(L"BUTTON", L"���ɖ߂�", WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_TEXT,
			120, 90, 75, 25, hWnd, (HMENU)IDB_RESTORE, ghInst, NULL);
		SendMessage(hWndBtnRestore, WM_SETFONT, (WPARAM)hFont, 0);

		hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH, L"FixedSys");
		// �𑜓x�ꗗ�\�� �R���{�{�b�N�X
		hWndCb = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_SORT | CBS_SIMPLE | CBS_DROPDOWN,
			15, 10, 200, 110, hWnd, NULL, ghInst, NULL);
		SendMessage(hWndCb, WM_SETFONT, (WPARAM)hFont, 0);

		// �R���{�{�b�N�X�̃��X�g�ɉ𑜓x�ꗗ���Z�b�g����
		if (! listResolution(hWndCb)) {
			MessageBox(NULL, L"�𑜓x�ꗗ���擾�ł��܂���", L"�G���[", MB_OK);
			exit(-1);
		}

		// ���Ƃ��Ƃ̃A�C�R���ʒu��ۑ�����
		if (!setIconPosition()) {
			MessageBox(NULL, L"�A�C�R���ʒu���擾�ł��܂���", L"�G���[", MB_OK);
			exit(-1);
		}
		break;
	}
	case WM_COMMAND:
	{
		// �A�C�R���ʒu���t�@�C���Ɋւ���ϐ�
		static OPENFILENAME ofn;
		static TCHAR filename_full[MAX_PATH];
		static TCHAR filename[MAX_PATH];

		int wmId = LOWORD(wParam);
		int wmEvent = HIWORD(wParam);
		HMENU hMenu = GetMenu(hWnd);

		switch (wmId) {
		// �𑜓x�ύX���j���[ or �{�^��
		case IDM_CHANGE:
		case IDB_CHANGE:
			if (changeResolution(hWndCb)) {  // �𑜓x��ύX����
				EnableWindow(hWndBtnChange, FALSE);
				EnableWindow(hWndBtnRestore, TRUE);
				EnableMenuItem(hMenu, IDM_CHANGE, MF_GRAYED);
				EnableMenuItem(hMenu, IDM_RESTORE, MF_ENABLED);
				EnableMenuItem(hMenu, IDM_SAVE, MF_GRAYED);
				EnableMenuItem(hMenu, IDM_READ, MF_GRAYED);
			}
			break;

		// ���ɖ߂����j���[ or �{�^��
		case IDM_RESTORE:
		case IDB_RESTORE:
			EnableWindow(hWndBtnRestore, FALSE);
			if (restoreResolution()) {       // �𑜓x�ƃA�C�R���̔z�u�����ɖ߂�
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
			ZeroMemory(&ofn, sizeof ofn);           // �ŏ��Ƀ[���N���A���Ă���
			ofn.lStructSize = sizeof ofn;           // �\���̂̃T�C�Y
			ofn.hwndOwner = hWnd;                   // �R�����_�C�A���O�̐e�E�B���h�E�n���h��
			ofn.lpstrFilter = L"ICON�ʒu���t�@�C��(*.icp)\0*.icp\0"; // �t�@�C���̎��
			ofn.lpstrFile = filename_full;          // �I�����ꂽ�t�@�C����(�t���p�X)���󂯎��ϐ��̃A�h���X
			ofn.lpstrFileTitle = filename;          // �I�����ꂽ�t�@�C�������󂯎��ϐ��̃A�h���X
			ofn.nMaxFile = _countof(filename_full); // lpstrFile�Ɏw�肵���ϐ��̃T�C�Y
			ofn.nMaxFileTitle = _countof(filename); // lpstrFileTitle�Ɏw�肵���ϐ��̃T�C�Y
			ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;  // �t���O�w��
			ofn.lpstrTitle = L"ICON�ʒu���t�@�C��������"; // �R�����_�C�A���O�̃L���v�V����
			ofn.lpstrDefExt = L"icp";               // �f�t�H���g�̃t�@�C���̎��
			// �t�@�C�����J���R�����_�C�A���O���쐬
			if (GetSaveFileName(&ofn)) {
				if (wcscmp(filename_full + ofn.nFileExtension, ofn.lpstrDefExt) != 0) {
					if (_countof(filename_full) - ofn.nFileExtension > wcslen(ofn.lpstrDefExt)) {
						(void)wcscpy_s(filename_full + ofn.nFileExtension,
							_countof(filename_full) - ofn.nFileExtension,
							ofn.lpstrDefExt);
					}
				}
				if (!saveIconPosition(filename_full)) {
					MessageBox(NULL, L"�������߂܂���", L"�G���[", MB_OK);
				}
			}
			break;

		case IDM_READ:
			ZeroMemory(&ofn, sizeof ofn);           // �ŏ��Ƀ[���N���A���Ă���
			ofn.lStructSize = sizeof ofn;           // �\���̂̃T�C�Y
			ofn.hwndOwner = hWnd;                   // �R�����_�C�A���O�̐e�E�B���h�E�n���h��
			ofn.lpstrFilter = L"ICON�ʒu���t�@�C��(*.icp)\0*.icp\0"; // �t�@�C���̎��
			ofn.lpstrFile = filename_full;          // �I�����ꂽ�t�@�C����(�t���p�X)���󂯎��ϐ��̃A�h���X
			ofn.lpstrFileTitle = filename;          // �I�����ꂽ�t�@�C�������󂯎��ϐ��̃A�h���X
			ofn.nMaxFile = _countof(filename_full); // lpstrFile�Ɏw�肵���ϐ��̃T�C�Y
			ofn.nMaxFileTitle = _countof(filename); // lpstrFileTitle�Ɏw�肵���ϐ��̃T�C�Y
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_READONLY;   // �t���O�w��
			ofn.lpstrTitle = L"ICON�ʒu���t�@�C����ǂ�"; // �R�����_�C�A���O�̃L���v�V����
			ofn.lpstrDefExt = L"icp";               // �f�t�H���g�̃t�@�C���̎��
			// �t�@�C�����J���R�����_�C�A���O���쐬
			if (GetOpenFileName(&ofn)) {
				if (wcscmp(filename_full + ofn.nFileExtension, ofn.lpstrDefExt) != 0) {
					MessageBox(NULL, L"ICON�ʒu���t�@�C���ł͂���܂���", L"�G���[", MB_OK);
				} else {
					(void)readIconPosition(filename_full);
				}
			}
			break;

		// �I�����j���[
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		// About���j���[
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
		// TODO: �`��R�[�h�������ɒǉ����Ă�������...
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		// ���̃A�v���P�[�V�����̃E�B���h�E������Ƃ�
		if (IsWindowEnabled(hWndBtnRestore)) {
			if (MessageBox(NULL, L"�𑜓x�����ɖ߂��܂���?", L"�I���m�F", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
				(void)restoreResolution(); // �𑜓x�ƃA�C�R���̔z�u�����ɖ߂�
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
 * AboutDialog �̃��b�Z�[�W�n���h��
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
