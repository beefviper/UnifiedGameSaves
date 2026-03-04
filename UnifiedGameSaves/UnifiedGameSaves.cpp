// UnifiedGameSaves.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "UnifiedGameSaves.h"
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>
#include <winioctl.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")

#ifndef REPARSE_DATA_BUFFER_HEADER_SIZE
typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#define REPARSE_DATA_BUFFER_HEADER_SIZE FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#endif

#define MAX_LOADSTRING 100

// Data structure for a game entry
struct GameEntry
{
    std::wstring name;
    std::wstring savePath;
    std::wstring newPath;
    bool hidden;
    bool junctionActive;
};

// Global vector to store game entries
std::vector<GameEntry> games;
const wchar_t* SAVES_FILE = L"UnifiedGameSaves.txt";

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hListView;                                 // ListView control handle

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    AddGameDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    RemoveGameDlg(HWND, UINT, WPARAM, LPARAM);

 // Helper functions
void LoadGamesFromFile();
void SaveGamesToFile();
void RefreshListView();
void AddGameToListView(const GameEntry& entry);
std::wstring BrowseForFolder(HWND hwndOwner, const wchar_t* lpszTitle);
bool PathExists(const std::wstring& path);
bool IsJunction(const std::wstring& path);
std::wstring GetJunctionTarget(const std::wstring& path);
void VerifyJunctionStates();
bool ActivateJunction(HWND hwndOwner, int gameIndex);
bool DeactivateJunction(HWND hwndOwner, int gameIndex);
void ToggleHidden(HWND hwndOwner, int gameIndex);
bool MoveDirectoryContents(const std::wstring& src, const std::wstring& dst);
bool CreateDirectoryRecursive(const std::wstring& path);

// Implementation of helper functions
void LoadGamesFromFile()
{
    games.clear();
    std::wifstream file(SAVES_FILE);
    if (!file.is_open()) return;

    std::wstring line;
    while (std::getline(file, line))
    {
        if (line.empty()) continue;
        
        std::wistringstream iss(line);
        std::wstring name, savePath, newPath, hiddenStr, junctionStr;
        
        if (std::getline(iss, name, L'|') && 
            std::getline(iss, savePath, L'|') &&
            std::getline(iss, newPath, L'|') &&
            std::getline(iss, hiddenStr))
        {
            GameEntry entry;
            entry.name = name;
            entry.savePath = savePath;
            entry.newPath = newPath;
            entry.hidden = (hiddenStr == L"1");
            entry.junctionActive = false;
            if (std::getline(iss, junctionStr, L'|'))
                entry.junctionActive = (junctionStr == L"1");
            games.push_back(entry);
        }
    }
    file.close();
}

void SaveGamesToFile()
{
    std::wofstream file(SAVES_FILE);
    for (const auto& entry : games)
    {
        file << entry.name << L"|" 
             << entry.savePath << L"|" 
             << entry.newPath << L"|" 
             << (entry.hidden ? L"1" : L"0") << L"|"
             << (entry.junctionActive ? L"1" : L"0") << L"\n";
    }
    file.close();
}

void RefreshListView()
{
    ListView_DeleteAllItems(hListView);
    for (const auto& entry : games)
    {
        AddGameToListView(entry);
    }
}

void AddGameToListView(const GameEntry& entry)
{
    int index = ListView_GetItemCount(hListView);
    LVITEMW item = { 0 };
    item.mask = LVIF_TEXT;
    item.iItem = index;
    item.iSubItem = 0;
    item.pszText = const_cast<LPWSTR>(entry.name.c_str());
    ListView_InsertItem(hListView, &item);

    item.iSubItem = 1;
    item.pszText = const_cast<LPWSTR>(entry.savePath.c_str());
    ListView_SetItem(hListView, &item);

    item.iSubItem = 2;
    item.pszText = const_cast<LPWSTR>(entry.newPath.c_str());
    ListView_SetItem(hListView, &item);

    item.iSubItem = 3;
    item.pszText = const_cast<LPWSTR>(entry.hidden ? L"Yes" : L"No");
    ListView_SetItem(hListView, &item);

    item.iSubItem = 4;
    if (entry.junctionActive)
        item.pszText = const_cast<LPWSTR>(L"Deactivate");
    else if (PathExists(entry.savePath) && !entry.newPath.empty())
        item.pszText = const_cast<LPWSTR>(L"Activate");
    else
        item.pszText = const_cast<LPWSTR>(L"");
    ListView_SetItem(hListView, &item);
}

std::wstring BrowseForFolder(HWND hwndOwner, const wchar_t* lpszTitle)
{
    std::wstring result;
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hwndOwner;
    bi.lpszTitle = lpszTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl != nullptr)
    {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path))
        {
            result = path;
        }
        CoTaskMemFree(pidl);
    }

    return result;
}

bool PathExists(const std::wstring& path)
{
    if (path.empty()) return false;
    DWORD attribs = GetFileAttributesW(path.c_str());
    return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

bool IsJunction(const std::wstring& path)
{
    DWORD attribs = GetFileAttributesW(path.c_str());
    if (attribs == INVALID_FILE_ATTRIBUTES) return false;
    return (attribs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

std::wstring GetJunctionTarget(const std::wstring& path)
{
    std::wstring target;
    HANDLE hDir = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (hDir == INVALID_HANDLE_VALUE) return target;

    BYTE buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    DWORD bytesReturned = 0;
    if (DeviceIoControl(hDir, FSCTL_GET_REPARSE_POINT, nullptr, 0,
        buffer, sizeof(buffer), &bytesReturned, nullptr))
    {
        REPARSE_DATA_BUFFER* rdb = reinterpret_cast<REPARSE_DATA_BUFFER*>(buffer);
        if (rdb->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
        {
            WCHAR* targetName = rdb->MountPointReparseBuffer.PathBuffer +
                (rdb->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
            USHORT targetLen = rdb->MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
            target.assign(targetName, targetLen);
            // Strip the \??\ prefix if present
            if (target.substr(0, 4) == L"\\??\\")
                target = target.substr(4);
        }
    }
    CloseHandle(hDir);
    return target;
}

void VerifyJunctionStates()
{
    bool changed = false;
    for (auto& entry : games)
    {
        bool shouldBeActive = false;
        if (IsJunction(entry.savePath) && !entry.newPath.empty())
        {
            std::wstring target = GetJunctionTarget(entry.savePath);
            // Normalize: remove trailing backslash for comparison
            while (!target.empty() && target.back() == L'\\')
                target.pop_back();
            std::wstring expected = entry.newPath;
            while (!expected.empty() && expected.back() == L'\\')
                expected.pop_back();
            if (_wcsicmp(target.c_str(), expected.c_str()) == 0)
                shouldBeActive = true;
        }
        if (entry.junctionActive != shouldBeActive)
        {
            entry.junctionActive = shouldBeActive;
            changed = true;
        }
    }
    if (changed)
        SaveGamesToFile();
}

static bool MoveDirectoryContents(const std::wstring& src, const std::wstring& dst)
{
    std::wstring searchPath = src + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return true;

    bool ok = true;
    do
    {
        std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") continue;

        std::wstring srcItem = src + L"\\" + name;
        std::wstring dstItem = dst + L"\\" + name;

        if (!MoveFileW(srcItem.c_str(), dstItem.c_str()))
        {
            ok = false;
            break;
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return ok;
}

static bool CreateDirectoryRecursive(const std::wstring& path)
{
    if (PathExists(path)) return true;

    // Find parent
    size_t pos = path.find_last_of(L"\\/"); 
    if (pos != std::wstring::npos && pos > 0)
    {
        std::wstring parent = path.substr(0, pos);
        if (!CreateDirectoryRecursive(parent)) return false;
    }
    return CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool ActivateJunction(HWND hwndOwner, int gameIndex)
{
    if (gameIndex < 0 || gameIndex >= (int)games.size()) return false;

    GameEntry& entry = games[gameIndex];

    if (entry.savePath.empty() || entry.newPath.empty())
    {
        MessageBoxW(hwndOwner, L"Both Save Path and New Path must be set.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!PathExists(entry.savePath))
    {
        MessageBoxW(hwndOwner, L"Save Path does not exist.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (IsJunction(entry.savePath))
    {
        MessageBoxW(hwndOwner, L"Save Path is already a junction.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Create New Path if it doesn't exist
    if (!CreateDirectoryRecursive(entry.newPath))
    {
        MessageBoxW(hwndOwner, L"Failed to create New Path directory.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Move contents from Save Path to New Path
    if (!MoveDirectoryContents(entry.savePath, entry.newPath))
    {
        MessageBoxW(hwndOwner, L"Failed to move directory contents.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Remove the now-empty Save Path directory
    if (!RemoveDirectoryW(entry.savePath.c_str()))
    {
        MessageBoxW(hwndOwner, L"Failed to remove Save Path directory.\nSome files may still be in use.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Create junction: mklink /J "savePath" "newPath"
    wchar_t cmdLine[1024];
    swprintf_s(cmdLine, L"cmd.exe /C mklink /J \"%s\" \"%s\"", entry.savePath.c_str(), entry.newPath.c_str());

    STARTUPINFOW si2 = { 0 };
    PROCESS_INFORMATION pi2 = { 0 };
    si2.cb = sizeof(si2);
    si2.dwFlags = STARTF_USESHOWWINDOW;
    si2.wShowWindow = SW_HIDE;

    if (!CreateProcessW(nullptr, cmdLine, nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si2, &pi2))
    {
        MessageBoxW(hwndOwner, L"Failed to create junction.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    WaitForSingleObject(pi2.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi2.hProcess, &exitCode);
    CloseHandle(pi2.hProcess);
    CloseHandle(pi2.hThread);

    if (exitCode != 0)
    {
        MessageBoxW(hwndOwner, L"Junction creation command failed.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    entry.junctionActive = true;

    // Restore hidden attribute if user previously set it
    if (entry.hidden)
    {
        DWORD attribs = GetFileAttributesW(entry.savePath.c_str());
        if (attribs != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesW(entry.savePath.c_str(), attribs | FILE_ATTRIBUTE_HIDDEN);
        }
    }

    SaveGamesToFile();
    RefreshListView();

    return true;
}

bool DeactivateJunction(HWND hwndOwner, int gameIndex)
{
    if (gameIndex < 0 || gameIndex >= (int)games.size()) return false;

    GameEntry& entry = games[gameIndex];

    if (!IsJunction(entry.savePath))
    {
        MessageBoxW(hwndOwner, L"Save Path is not a junction.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!PathExists(entry.newPath))
    {
        MessageBoxW(hwndOwner, L"New Path does not exist. Cannot restore.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Remove the junction (it's a directory reparse point, rmdir removes it)
    wchar_t cmdLine[1024];
    swprintf_s(cmdLine, L"cmd.exe /C rmdir \"%s\"", entry.savePath.c_str());

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcessW(nullptr, cmdLine, nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        MessageBoxW(hwndOwner, L"Failed to remove junction.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Create the Save Path directory again
    if (!CreateDirectoryW(entry.savePath.c_str(), nullptr))
    {
        MessageBoxW(hwndOwner, L"Failed to recreate Save Path directory.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Move contents back from New Path to Save Path
    if (!MoveDirectoryContents(entry.newPath, entry.savePath))
    {
        MessageBoxW(hwndOwner, L"Failed to move directory contents back.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Remove the now-empty New Path directory
    RemoveDirectoryW(entry.newPath.c_str());

    // If hidden was set, remove the hidden attribute from the restored directory
    if (entry.hidden)
    {
        DWORD attribs = GetFileAttributesW(entry.savePath.c_str());
        if (attribs != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesW(entry.savePath.c_str(), attribs & ~FILE_ATTRIBUTE_HIDDEN);
        }
    }

    entry.junctionActive = false;
    SaveGamesToFile();
    RefreshListView();

    return true;
}

void ToggleHidden(HWND hwndOwner, int gameIndex)
{
    if (gameIndex < 0 || gameIndex >= (int)games.size()) return;

    GameEntry& entry = games[gameIndex];

    if (!entry.hidden)
    {
        // Trying to set hidden — only allow if junction is active
        if (!entry.junctionActive)
        {
            MessageBoxW(hwndOwner, L"Junction must be active before hiding the Save Path.", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        // Set hidden attribute on Save Path
        DWORD attribs = GetFileAttributesW(entry.savePath.c_str());
        if (attribs != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesW(entry.savePath.c_str(), attribs | FILE_ATTRIBUTE_HIDDEN);
        }

        entry.hidden = true;
    }
    else
    {
        // Remove hidden attribute from Save Path
        DWORD attribs = GetFileAttributesW(entry.savePath.c_str());
        if (attribs != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesW(entry.savePath.c_str(), attribs & ~FILE_ATTRIBUTE_HIDDEN);
        }

        entry.hidden = false;
    }

    SaveGamesToFile();
    RefreshListView();
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UNIFIEDGAMESAVES));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_UNIFIEDGAMESAVES);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // Create ListView control
   hListView = CreateWindowExW(0, WC_LISTVIEWW, L"",
      WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_AUTOARRANGE,
      0, 0, 0, 0,
      hWnd, (HMENU)IDC_LISTVIEW, hInstance, nullptr);

   if (!hListView)
   {
      return FALSE;
   }

   // Enable grid lines and full row select for subitem click detection
   ListView_SetExtendedListViewStyle(hListView, LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);

   // Initialize the ListView with columns
   LVCOLUMNW lvc = { 0 };
   lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
   lvc.fmt = LVCFMT_LEFT;

   // Add "Game" column
   lvc.cx = 150;
   lvc.pszText = const_cast<LPWSTR>(L"Game");
   ListView_InsertColumn(hListView, 0, &lvc);

   // Add "Save Path" column
   lvc.cx = 200;
   lvc.pszText = const_cast<LPWSTR>(L"Save Path");
   ListView_InsertColumn(hListView, 1, &lvc);

   // Add "New Path" column
   lvc.cx = 200;
   lvc.pszText = const_cast<LPWSTR>(L"New Path");
   ListView_InsertColumn(hListView, 2, &lvc);

   // Add "Hidden" column
   lvc.cx = 80;
   lvc.pszText = const_cast<LPWSTR>(L"Hidden");
   ListView_InsertColumn(hListView, 3, &lvc);

   // Add "Junction" column
   lvc.cx = 80;
   lvc.pszText = const_cast<LPWSTR>(L"Junction");
   ListView_InsertColumn(hListView, 4, &lvc);

   // Load games from file
   LoadGamesFromFile();
   VerifyJunctionStates();
   RefreshListView();

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_GAME_ADD:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDGAME), hWnd, AddGameDlg);
                break;
            case IDM_GAME_REMOVE:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_REMOVEGAME), hWnd, RemoveGameDlg);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            if (pnmhdr->idFrom == IDC_LISTVIEW)
            {
                if (pnmhdr->code == NM_CUSTOMDRAW)
                {
                    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
                    switch (lplvcd->nmcd.dwDrawStage)
                    {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;
                    case CDDS_ITEMPREPAINT:
                        return CDRF_NOTIFYSUBITEMDRAW;
                    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                        {
                            int row = (int)lplvcd->nmcd.dwItemSpec;
                            int col = lplvcd->iSubItem;

                            if (row < 0 || row >= (int)games.size())
                                return CDRF_DODEFAULT;

                            // Game column: green if junction is active
                            if (col == 0)
                            {
                                if (games[row].junctionActive)
                                {
                                    lplvcd->clrTextBk = RGB(200, 220, 200);
                                    lplvcd->clrText = RGB(0, 0, 0);
                                    return CDRF_NEWFONT;
                                }
                                return CDRF_DODEFAULT;
                            }
                            // Save Path and New Path columns
                            if (col == 1 || col == 2)
                            {
                                std::wstring path = (col == 1) ? games[row].savePath : games[row].newPath;
                                if (PathExists(path))
                                {
                                    lplvcd->clrTextBk = RGB(200, 220, 200);
                                    lplvcd->clrText = RGB(0, 0, 0);
                                    return CDRF_NEWFONT;
                                }
                                else if (!path.empty())
                                {
                                    lplvcd->clrTextBk = RGB(220, 200, 200);
                                    lplvcd->clrText = RGB(0, 0, 0);
                                    return CDRF_NEWFONT;
                                }
                                return CDRF_DODEFAULT;
                            }
                            // Hidden column: clickable toggle
                            if (col == 3)
                            {
                                if (games[row].hidden)
                                {
                                    lplvcd->clrTextBk = RGB(200, 220, 200);
                                    lplvcd->clrText = RGB(0, 0, 0);
                                }
                                else if (games[row].junctionActive)
                                {
                                    lplvcd->clrTextBk = RGB(255, 255, 255);
                                    lplvcd->clrText = RGB(0, 0, 200);
                                }
                                else
                                {
                                    lplvcd->clrTextBk = RGB(255, 255, 255);
                                    lplvcd->clrText = RGB(180, 180, 180);
                                }
                                return CDRF_NEWFONT;
                            }
                            // Junction column
                            if (col == 4)
                            {
                                if (games[row].junctionActive)
                                {
                                    lplvcd->clrTextBk = RGB(220, 200, 200);
                                    lplvcd->clrText = RGB(150, 0, 0);
                                }
                                else if (PathExists(games[row].savePath) && !games[row].newPath.empty())
                                {
                                    lplvcd->clrTextBk = RGB(255, 255, 255);
                                    lplvcd->clrText = RGB(0, 0, 200);
                                }
                                else
                                {
                                    lplvcd->clrTextBk = RGB(255, 255, 255);
                                    lplvcd->clrText = RGB(180, 180, 180);
                                }
                                return CDRF_NEWFONT;
                            }
                            return CDRF_DODEFAULT;
                        }
                    }
                }
                else if (pnmhdr->code == NM_CLICK)
                {
                    LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
                    if (pnmia->iItem >= 0 && pnmia->iItem < (int)games.size())
                    {
                        if (pnmia->iSubItem == 3)
                        {
                            ToggleHidden(hWnd, pnmia->iItem);
                        }
                        else if (pnmia->iSubItem == 4)
                        {
                            if (games[pnmia->iItem].junctionActive)
                            {
                                DeactivateJunction(hWnd, pnmia->iItem);
                            }
                            else if (PathExists(games[pnmia->iItem].savePath) &&
                                     !games[pnmia->iItem].newPath.empty())
                            {
                                ActivateJunction(hWnd, pnmia->iItem);
                            }
                        }
                    }
                }
            }
        }
        break;
    case WM_SIZE:
        {
            int width = GET_X_LPARAM(lParam);
            int height = GET_Y_LPARAM(lParam);
            if (hListView)
            {
                MoveWindow(hListView, 0, 0, width, height, TRUE);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for add game dialog.
INT_PTR CALLBACK AddGameDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BROWSE_SAVEPATH)
        {
            std::wstring folder = BrowseForFolder(hDlg, L"Select Save Path");
            if (!folder.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SAVEPATH_INPUT, folder.c_str());
            }
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_BROWSE_NEWPATH)
        {
            std::wstring folder = BrowseForFolder(hDlg, L"Select New Path");
            if (!folder.empty())
            {
                SetDlgItemTextW(hDlg, IDC_NEWPATH_INPUT, folder.c_str());
            }
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDOK)
        {
            wchar_t nameBuffer[256] = { 0 };
            wchar_t savePathBuffer[256] = { 0 };
            wchar_t newPathBuffer[256] = { 0 };

            GetDlgItemTextW(hDlg, IDC_NAME_INPUT, nameBuffer, 256);
            GetDlgItemTextW(hDlg, IDC_SAVEPATH_INPUT, savePathBuffer, 256);
            GetDlgItemTextW(hDlg, IDC_NEWPATH_INPUT, newPathBuffer, 256);
            BOOL isHidden = IsDlgButtonChecked(hDlg, IDC_HIDDEN_CHECK);

            if (wcslen(nameBuffer) == 0)
            {
                MessageBoxW(hDlg, L"Please enter a game name.", L"Error", MB_OK | MB_ICONERROR);
                return (INT_PTR)FALSE;
            }

            GameEntry entry;
            entry.name = nameBuffer;
            entry.savePath = savePathBuffer;
            entry.newPath = newPathBuffer;
            entry.hidden = (isHidden == BST_CHECKED);
            entry.junctionActive = false; // Default to false for new entries

            games.push_back(entry);
            SaveGamesToFile();
            RefreshListView();

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for remove game dialog.
INT_PTR CALLBACK RemoveGameDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            HWND hList = GetDlgItem(hDlg, IDC_GAME_LIST);
            ListView_SetExtendedListViewStyle(hList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            // Add a single column that fills the width
            RECT rc;
            GetClientRect(hList, &rc);
            LVCOLUMNW lvc = { 0 };
            lvc.mask = LVCF_FMT | LVCF_WIDTH;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = rc.right - rc.left;
            ListView_InsertColumn(hList, 0, &lvc);

            // Populate with game names
            for (size_t i = 0; i < games.size(); ++i)
            {
                LVITEMW item = { 0 };
                item.mask = LVIF_TEXT;
                item.iItem = (int)i;
                item.pszText = const_cast<LPWSTR>(games[i].name.c_str());
                ListView_InsertItem(hList, &item);
            }
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            HWND hList = GetDlgItem(hDlg, IDC_GAME_LIST);
            std::vector<int> selectedIndices;
            int itemCount = ListView_GetItemCount(hList);

            for (int i = 0; i < itemCount; ++i)
            {
                if (ListView_GetCheckState(hList, i))
                {
                    selectedIndices.push_back(i);
                }
            }

            // Remove in reverse order to avoid index shifting
            for (int i = (int)selectedIndices.size() - 1; i >= 0; --i)
            {
                games.erase(games.begin() + selectedIndices[i]);
            }

            SaveGamesToFile();
            RefreshListView();

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_UNIFIEDGAMESAVES, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_UNIFIEDGAMESAVES));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}
