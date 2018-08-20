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
 * �R���{�{�b�N�X�̃��X�g�ɉ𑜓x�ꗗ���Z�b�g����
 */
BOOL listResolution(HWND hwnd) {
	// ���݂̉𑜓x�𓾂�
	if (! EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devModeOrg)) {
		return FALSE;
	}

	// �𑜓x�ꗗ�𓾂�
	int listIndex;
	DEVMODE devMode;
	for (int resIndex = 0; EnumDisplaySettings(NULL, resIndex, &devMode); resIndex++) {
		// �𑜓x���X�g�ɍڂ�����̂��i��
		if (devMode.dmDisplayFrequency == devModeOrg.dmDisplayFrequency         // ���g��������Ɠ���
		 && devMode.dmBitsPerPel == devModeOrg.dmBitsPerPel                     // 1�s�N�Z���̃r�b�g��������Ɠ���
		 && devMode.dmDisplayOrientation == 0                                   // �����̂�
		 && devMode.dmDefaultSource == 0
		 && devMode.dmPelsWidth >= 640
		 && devMode.dmPelsHeight >= 480
		 && ChangeDisplaySettings(&devMode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL // �e�X�g����
		) {
			WCHAR buf[30];
			_snwprintf_s(buf, _countof(buf),
					L"%4dx%4d %dbit %dHz",
					devMode.dmPelsWidth, devMode.dmPelsHeight, devMode.dmBitsPerPel, devMode.dmDisplayFrequency,
					_TRUNCATE);
			if ((listIndex = SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)buf)) >= MAXLISTCOUNT - 1) {
				// MAXLISTCOUNT �𒴂�����ł��؂�
				break;
			}

			// �𑜓x���X�g��ID�ƁA�S�𑜓x�ꗗID�Ƃ̃}�b�v��ݒ�
			resModeMap[listIndex] = resIndex;

			// ���݂̉𑜓x��I����Ԃɂ���
			if (devMode.dmPelsWidth == devModeOrg.dmPelsWidth
			 && devMode.dmPelsHeight == devModeOrg.dmPelsHeight) {
				SendMessage(hwnd, CB_SETCURSEL, listIndex, 0);
			}
		}
	}
	return TRUE;
}

/*
 * ���Ƃ��Ƃ̃A�C�R���ʒu���O���[�o���ϐ��ɃZ�b�g����
 */
BOOL setIconPosition(void)
{
	if (! iconInfoOrg) {
		// ���Ƃ��Ƃ̉𑜓x�ƃA�C�R���ʒu���O���[�o���ϐ��ɃZ�b�g����
		iconInfoOrg = getIconPos(&numIconOrg);
	}
	return (iconInfoOrg != NULL);
}

/*
 * �𑜓x��ύX����
 */
BOOL changeResolution(HWND hListWnd)
{
	// ���X�g����I������Ă���𑜓x�𓾂�
	DEVMODE devMode;
	int listIndex = SendMessage(hListWnd, CB_GETCURSEL, 0, 0);
	int resIndex = resModeMap[listIndex];
	if (! EnumDisplaySettings(NULL, resIndex, &devMode)) {
		return FALSE;
	}

	// �𑜓x��ύX����
	if (ChangeDisplaySettings(&devMode, CDS_UPDATEREGISTRY | CDS_GLOBAL) != DISP_CHANGE_SUCCESSFUL) {
		return FALSE;
	}

	return TRUE;
}

/*
 * �𑜓x�ƃA�C�R���̈ʒu�����ɖ߂�
 */
BOOL restoreResolution(void)
{
	//�𑜓x�����ɖ߂�
	if (ChangeDisplaySettings(&devModeOrg, CDS_UPDATEREGISTRY | CDS_GLOBAL) != DISP_CHANGE_SUCCESSFUL) {
		return FALSE;
	}

	//�����҂� (��ʂ��\������ăA�C�R���������Ĕz�u�����̂Ɏ��Ԃ�������)
	Sleep(1000);

	//���݂̃A�C�R���̈ʒu���擾
	int numIcon;
	ICON_INFO *iconInfo = getIconPos(&numIcon);
	//���Ƃ��Ƃ̃A�C�R���̈ʒu���A�C�R�����ŕR�t�����Ȃ���
	for (int i = 0; i < numIcon; i++) {
		ICON_INFO *a = search(iconInfoOrg, numIconOrg, iconInfo[i].name);
		if (a != NULL) {
			iconInfo[i].point = a->point;
		} else {
			//�Ȃ��ꍇ(�ォ����ꂽ�ꍇ)�̓f�X�N�g�b�v�����ɔz�u����
			iconInfo[i].point.x = devModeOrg.dmPelsWidth / 2;
			iconInfo[i].point.y = devModeOrg.dmPelsHeight / 2;
		}
	}

	//���ォ�牺�։E�֍Ĕz�u���邱�Ƃɂ��A1��Ő��񂳂���
	qsort(iconInfo, numIcon, sizeof (ICON_INFO), comp);

	//�f�X�N�g�b�v�̃E�B���h�E�n���h�����擾����
	HWND hWnd = getDesktopHandle();
	if (! hWnd) {
		free(iconInfo);
		return FALSE;
	}
	//�A�C�R���̔z�u�����ɖ߂�
	for (int i = 0; i < numIcon; i++) {
		ListView_SetItemPosition(hWnd, iconInfo[i].index, iconInfo[i].point.x, iconInfo[i].point.y);
	}
	free(iconInfo);

	return TRUE;
}

/*
 * �A�C�R���z�u���t�@�C������ǂݍ���ŏ����l�ƒu��������
 * ����ɂ��̔z�u�ɏ]���čĔz�u����
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
 * ���݂̃A�C�R���z�u���t�@�C���ɏ���
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
 * �f�X�N�g�b�v�̃A�C�R���ʒu���擾����
 */
static ICON_INFO *getIconPos(int *pNumIcon)
{
	*pNumIcon = 0;

	//�f�X�N�g�b�v�̃E�B���h�E�n���h�����擾����
	HWND hWnd = getDesktopHandle();
	if (! hWnd) {
		return NULL;
	}

	//�f�X�N�g�b�v�E�B���h�E�����v���Z�X�̃n���h�����J��
	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, dwProcessId);
	if (! hProcess) {
		return NULL;
	}

	//�f�X�N�g�b�v�A�C�R���̐����擾
	*pNumIcon = ListView_GetItemCount(hWnd);
	if (*pNumIcon < 0) {
		return NULL;
	}

	//�A�C�R���̈ʒu���i�[����z����m��
	ICON_INFO *iconInfo = (ICON_INFO *)malloc(sizeof (ICON_INFO) * *pNumIcon);
	if (iconInfo == NULL) {
		return NULL;
	}

	//�v���Z�X�փ�������Ԃ��R�~�b�g
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
	lvi.pszText = (WCHAR *)(adr + sizeof (LVITEM));	//�������󂯎��o�b�t�@ �I�t�Z�b�g��LVITEM�\���̂̃T�C�Y
	lvi.cchTextMax = MAX_PATH;

	for (int iconIndex = 0; iconIndex < *pNumIcon; iconIndex++) {
		//�A�C�R��ID���Z�b�g
		iconInfo[iconIndex].index = iconIndex;

		//�A�C�R���ʒu���擾
		ListView_GetItemPosition(hWnd, iconIndex, pnt);
		//���������R�s�[ (�A�C�R���ʒu���Z�b�g)
		ReadProcessMemory(hProcess, pnt, &(iconInfo[iconIndex].point), sizeof (POINT), NULL);

		//�A�C�R���̖��O���擾
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
 * �f�X�N�g�b�v�̃n���h�����擾
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
		if (! hWnd) {              //�𑜓x�ύX����͎��s���邱�Ƃ�����
			Sleep(1);              //���̏ꍇ�A1ms�҂��Ďn�߂���Ď��s
			if (nTry++ > 100) {    //100��(100ms)���s�����ꍇ�͖{���ɃG���[
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
 * �A�C�R�����̃\�[�g�p��r���W�b�N
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
 * �A�C�R����񌟍����W�b�N
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
 * �A�C�R���̔z�u���t�@�C���ɏ���
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
 * �A�C�R���̔z�u���t�@�C������ǂ�
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

	//�A�C�R���̈ʒu���i�[����z����m��
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
		while ((c = getwc(fp)) != EOF) {  // 3���ږڂ̃A�C�R�����܂ł̋󔒂�ǂݔ�΂�
			if (c != ' ') {
				ungetwc(c, fp);
				break;
			}
		}
		if (fgetws(iconInfo[i].name, MAX_PATH, fp) == NULL) {  // �A�C�R������ǂ�
			break;
		}
		int len = wcslen(iconInfo[i].name);
		if (iconInfo[i].name[len - 1] == L'\n') {  // �Ō�̉��s����������
			iconInfo[i].name[len - 1] = L'\0';
		}
	}
	fclose(fp);

	return iconInfo;
}
