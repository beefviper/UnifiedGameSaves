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
const wchar_t* SAVES_FILE = L"UnifiedGameSaves.txt";

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND hListView;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    AddGameDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    RemoveGameDlg(HWND, UINT, WPARAM, LPARAM);

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
