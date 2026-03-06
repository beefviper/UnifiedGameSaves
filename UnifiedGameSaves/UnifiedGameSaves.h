#pragma once

#include "resource.h"

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
} REPARSE_DATA_BUFFER, * PREPARSE_DATA_BUFFER;
#define REPARSE_DATA_BUFFER_HEADER_SIZE FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#endif

constexpr int MAX_LOADSTRING = 100;

// Data structure for a game entry
struct GameEntry
{
    std::wstring name;
    std::wstring savePath;
    std::wstring newPath;
    bool hidden;
    bool junctionActive;

    GameEntry() : hidden(false), junctionActive(false) {}
};

std::vector<GameEntry> games;
std::wstring savesFilePath;

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND hListView;                                 // ListView control handle

// Column width management
constexpr int MIN_COLUMN_WIDTH = 80;
constexpr int NUM_COLUMNS = 5;
int columnWidths[NUM_COLUMNS] = { 120, 280, 280, 100, 100 };

// In-place editing
HWND hEditControl = nullptr;
int editingRow = -1;
int editingCol = -1;
WNDPROC oldEditProc = nullptr;

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
void ResizeListViewColumns(int totalWidth);
void StartEditCell(int row, int col);
void EndEditCell(bool save);
LRESULT CALLBACK EditControlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
