// UnifiedGameSaves.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "UnifiedGameSaves.h"
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")

#define MAX_LOADSTRING 100

// Data structure for a game entry
struct GameEntry
{
    std::wstring name;
    std::wstring savePath;
    std::wstring newPath;
    bool hidden;
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
        std::wstring name, savePath, newPath, hiddenStr;
        
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
             << (entry.hidden ? L"1" : L"0") << L"\n";
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

   // Enable grid lines
   ListView_SetExtendedListViewStyle(hListView, LVS_EX_GRIDLINES);

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

   // Load games from file
   LoadGamesFromFile();
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
