#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <wininet.h>
#include <gdiplus.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace std;
using namespace Gdiplus;

// =========================================================================
//  PRO CUSTOM API / VERCEL ENDPOINT CONFIGURATION
// =========================================================================
// When deployed to Vercel, replace SERVER_HOST with your Vercel domain (e.g. L"op-license.vercel.app")
wstring SERVER_HOST   = L"127.0.0.1";
int     SERVER_PORT   = 3000; // Use 443 for HTTPS Vercel deployment, 3000 for local dev
wstring SERVER_PATH   = L"/api/validate";
bool    USE_HTTPS     = false; // Set to true when using Vercel HTTPS

// Color Palette (Pro Dark Theme)
const COLORREF COLOR_BG          = RGB(13, 15, 24);     // #0D0F18 Deep Obsidian
const COLORREF COLOR_HEADER      = RGB(21, 25, 40);     // #151928 Header Bar
const COLORREF COLOR_INPUT_BG    = RGB(30, 34, 54);     // #1E2236 Input field
const COLORREF COLOR_BTN_NORMAL  = RGB(88, 101, 242);   // #5865F2 Pro Accent
const COLORREF COLOR_BTN_HOVER   = RGB(114, 137, 218);  // Hover Accent
const COLORREF COLOR_TEXT_WHITE  = RGB(255, 255, 255);
const COLORREF COLOR_TEXT_MUTED  = RGB(120, 128, 160);
const COLORREF COLOR_SUCCESS     = RGB(16, 185, 129);   // Emerald
const COLORREF COLOR_ERROR       = RGB(239, 68, 68);    // Crimson Red

// String Conversion Utilities
string WStringToString(const wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size, NULL, NULL);
    return str;
}

wstring StringToWString(const string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size);
    return wstr;
}

// Get HWID
string GetHWID() {
    DWORD volSerial = 0;
    GetVolumeInformationA("C:\\", NULL, 0, &volSerial, NULL, NULL, NULL, 0);
    char compName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(compName);
    GetComputerNameA(compName, &size);

    stringstream ss;
    ss << compName << "-" << hex << volSerial;
    return ss.str();
}

// AppData Key Storage
string GetKeyFilePath() {
    char appDataPath[MAX_PATH];
    if (GetEnvironmentVariableA("APPDATA", appDataPath, MAX_PATH) > 0) {
        string dir = string(appDataPath) + "\\HollowLauncher";
        CreateDirectoryA(dir.c_str(), NULL);
        return dir + "\\license.key";
    }
    return "license.key";
}

string LoadSavedKey() {
    ifstream file(GetKeyFilePath());
    if (file.is_open()) {
        string key;
        getline(file, key);
        return key;
    }
    return "";
}

void SaveKey(const string& key) {
    ofstream file(GetKeyFilePath());
    if (file.is_open()) {
        file << key;
    }
}

// JSON Parser helper
string GetJsonValue(const string& json, const string& key) {
    string target = "\"" + key + "\":";
    size_t pos = json.find(target);
    if (pos == string::npos) return "";

    size_t start = pos + target.length();
    while (start < json.length() && (json[start] == ' ' || json[start] == '"')) start++;

    size_t end = start;
    while (end < json.length() && json[end] != '"' && json[end] != ',' && json[end] != '}') end++;

    return json.substr(start, end - start);
}

// HTTP POST Request to Next.js API
string HttpPostJson(const string& jsonBody) {
    HINTERNET hSession = InternetOpenA("HollowLauncherClient", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hSession) return "";

    string hostStr = WStringToString(SERVER_HOST);
    HINTERNET hConnect = InternetConnectA(hSession, hostStr.c_str(), (INTERNET_PORT)SERVER_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hSession);
        return "";
    }

    DWORD flags = INTERNET_FLAG_RELOAD;
    if (USE_HTTPS) flags |= INTERNET_FLAG_SECURE;

    string pathStr = WStringToString(SERVER_PATH);
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", pathStr.c_str(), NULL, NULL, NULL, flags, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return "";
    }

    string headers = "Content-Type: application/json\r\n";
    BOOL sent = HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)jsonBody.c_str(), (DWORD)jsonBody.length());
    if (!sent) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return "";
    }

    char buffer[2048] = { 0 };
    DWORD bytesRead = 0;
    string responseStr = "";

    while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        responseStr += buffer;
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);
    return responseStr;
}

// License Key Validation against Next.js API
bool ValidateKeyWithNextAPI(const string& key, wstring& outMessage) {
    string hwid = GetHWID();
    string jsonPayload = "{\"key\":\"" + key + "\",\"hwid\":\"" + hwid + "\"}";

    string apiResp = HttpPostJson(jsonPayload);
    if (apiResp.empty()) {
        outMessage = L"Connection Error: Unable to connect to License API server.";
        return false;
    }

    string successStr = GetJsonValue(apiResp, "success");
    string msgStr = GetJsonValue(apiResp, "message");

    if (successStr == "true") {
        SaveKey(key);
        outMessage = L"License Valid! " + StringToWString(msgStr);
        return true;
    } else {
        outMessage = StringToWString(msgStr.empty() ? "Invalid or expired license key." : msgStr);
        return false;
    }
}

void RunHidden(const string& exePath) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    CreateProcessA(
        NULL,
        (LPSTR)exePath.c_str(),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void LaunchPayloads() {
    RunHidden("IntelService.exe");
    RunHidden("IntelHelper.exe");
}

// Global UI Handles & State
HWND hKeyEdit;
HWND hStatusLabel;
HWND hLoginBtn;
HWND hCloseBtn;
HWND hMinBtn;

bool g_Authenticated = false;
bool g_BtnHover = false;
bool g_CloseHover = false;
bool g_MinHover = false;
int  g_StatusType = 0; // 0: Normal, 1: Success, 2: Error
ULONG_PTR g_GdiplusToken;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFontEdit  = CreateFontW(16, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hFontStatus= CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        // Custom Close Button
        hCloseBtn = CreateWindowW(L"BUTTON", L"X", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 442, 10, 26, 26, hWnd, (HMENU)201, NULL, NULL);
        // Custom Minimize Button
        hMinBtn   = CreateWindowW(L"BUTTON", L"-", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 410, 10, 26, 26, hWnd, (HMENU)202, NULL, NULL);

        // Edit Control for License Key
        hKeyEdit = CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 35, 115, 410, 36, hWnd, (HMENU)101, NULL, NULL);
        SendMessageW(hKeyEdit, WM_SETFONT, (WPARAM)hFontEdit, TRUE);

        string savedKey = LoadSavedKey();
        if (!savedKey.empty()) {
            SetWindowTextW(hKeyEdit, StringToWString(savedKey).c_str());
        }

        // Custom Login Button
        hLoginBtn = CreateWindowW(L"BUTTON", L"LOG IN & LAUNCH", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 35, 172, 410, 44, hWnd, (HMENU)102, NULL, NULL);

        // Status Message Label
        hStatusLabel = CreateWindowW(L"STATIC", L"Ready. Enter license key to authenticate.", WS_VISIBLE | WS_CHILD, 35, 232, 410, 40, hWnd, (HMENU)103, NULL, NULL);
        SendMessageW(hStatusLabel, WM_SETFONT, (WPARAM)hFontStatus, TRUE);

        TRACKMOUSEEVENT tme = { sizeof(tme) };
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        TrackMouseEvent(&tme);

        break;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtl = (HWND)lParam;

        if (hCtl == hKeyEdit) {
            SetTextColor(hdc, COLOR_TEXT_WHITE);
            SetBkColor(hdc, COLOR_INPUT_BG);
            static HBRUSH hbrInput = CreateSolidBrush(COLOR_INPUT_BG);
            return (LRESULT)hbrInput;
        }
        if (hCtl == hStatusLabel) {
            SetBkMode(hdc, TRANSPARENT);
            if (g_StatusType == 1)      SetTextColor(hdc, COLOR_SUCCESS);
            else if (g_StatusType == 2) SetTextColor(hdc, COLOR_ERROR);
            else                        SetTextColor(hdc, COLOR_TEXT_MUTED);

            static HBRUSH hbrBg = CreateSolidBrush(COLOR_BG);
            return (LRESULT)hbrBg;
        }
        break;
    }

    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hWnd, &pt);
            if (pt.y <= 48 && pt.x < 400) {
                return HTCAPTION;
            }
        }
        return hit;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        HDC hdc = pDIS->hDC;
        RECT rc = pDIS->rcItem;

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        if (pDIS->CtlID == 102) { // Login Button
            COLORREF btnColor = (pDIS->itemState & ODS_SELECTED) ? RGB(70, 80, 200) :
                                (g_BtnHover ? COLOR_BTN_HOVER : COLOR_BTN_NORMAL);

            SolidBrush brush(Color(255, GetRValue(btnColor), GetGValue(btnColor), GetBValue(btnColor)));
            GraphicsPath path;
            int radius = 10;
            path.AddArc(rc.left, rc.top, radius * 2, radius * 2, 180, 90);
            path.AddArc(rc.right - radius * 2, rc.top, radius * 2, radius * 2, 270, 90);
            path.AddArc(rc.right - radius * 2, rc.bottom - radius * 2, radius * 2, radius * 2, 0, 90);
            path.AddArc(rc.left, rc.bottom - radius * 2, radius * 2, radius * 2, 90, 90);
            path.CloseFigure();

            graphics.FillPath(&brush, &path);

            FontFamily fontFamily(L"Segoe UI");
            Font font(&fontFamily, 12, FontStyleBold, UnitPixel);
            SolidBrush textBrush(Color(255, 255, 255, 255));
            StringFormat stringFormat;
            stringFormat.SetAlignment(StringAlignmentCenter);
            stringFormat.SetLineAlignment(StringAlignmentCenter);

            RectF rectF((REAL)rc.left, (REAL)rc.top, (REAL)(rc.right - rc.left), (REAL)(rc.bottom - rc.top));
            graphics.DrawString(L"LOG IN & LAUNCH", -1, &font, rectF, &stringFormat, &textBrush);
            return TRUE;
        }
        else if (pDIS->CtlID == 201) { // Close X Button
            Color bgCol = g_CloseHover ? Color(255, 239, 68, 68) : Color(255, 21, 25, 40);
            SolidBrush brush(bgCol);
            graphics.FillRectangle(&brush, (INT)rc.left, (INT)rc.top, (INT)(rc.right - rc.left), (INT)(rc.bottom - rc.top));

            FontFamily fontFamily(L"Segoe UI");
            Font font(&fontFamily, 11, FontStyleBold, UnitPixel);
            SolidBrush textBrush(Color(255, 255, 255, 255));
            StringFormat stringFormat;
            stringFormat.SetAlignment(StringAlignmentCenter);
            stringFormat.SetLineAlignment(StringAlignmentCenter);

            RectF rectF((REAL)rc.left, (REAL)rc.top, (REAL)(rc.right - rc.left), (REAL)(rc.bottom - rc.top));
            graphics.DrawString(L"X", -1, &font, rectF, &stringFormat, &textBrush);
            return TRUE;
        }
        else if (pDIS->CtlID == 202) { // Minimize Button
            Color bgCol = g_MinHover ? Color(255, 30, 34, 54) : Color(255, 21, 25, 40);
            SolidBrush brush(bgCol);
            graphics.FillRectangle(&brush, (INT)rc.left, (INT)rc.top, (INT)(rc.right - rc.left), (INT)(rc.bottom - rc.top));

            FontFamily fontFamily(L"Segoe UI");
            Font font(&fontFamily, 11, FontStyleBold, UnitPixel);
            SolidBrush textBrush(Color(255, 180, 190, 220));
            StringFormat stringFormat;
            stringFormat.SetAlignment(StringAlignmentCenter);
            stringFormat.SetLineAlignment(StringAlignmentCenter);

            RectF rectF((REAL)rc.left, (REAL)rc.top, (REAL)(rc.right - rc.left), (REAL)(rc.bottom - rc.top));
            graphics.DrawString(L"-", -1, &font, rectF, &stringFormat, &textBrush);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == 201) {
            PostQuitMessage(0);
        }
        else if (LOWORD(wParam) == 202) {
            ShowWindow(hWnd, SW_MINIMIZE);
        }
        else if (LOWORD(wParam) == 102) { // Login
            wchar_t keyBuffer[256] = { 0 };
            GetWindowTextW(hKeyEdit, keyBuffer, 256);
            wstring wkey = keyBuffer;
            string key = WStringToString(wkey);

            if (key.empty()) {
                g_StatusType = 2;
                SetWindowTextW(hStatusLabel, L"Error: Please enter a license key.");
                InvalidateRect(hStatusLabel, NULL, TRUE);
                break;
            }

            g_StatusType = 0;
            SetWindowTextW(hStatusLabel, L"Connecting to License API server...");
            InvalidateRect(hStatusLabel, NULL, TRUE);
            UpdateWindow(hWnd);

            wstring apiMsg;
            if (ValidateKeyWithNextAPI(key, apiMsg)) {
                g_StatusType = 1;
                SetWindowTextW(hStatusLabel, (L"Success: " + apiMsg).c_str());
                InvalidateRect(hStatusLabel, NULL, TRUE);
                UpdateWindow(hWnd);
                Sleep(800);
                g_Authenticated = true;
                DestroyWindow(hWnd);
            } else {
                g_StatusType = 2;
                SetWindowTextW(hStatusLabel, (L"Error: " + apiMsg).c_str());
                InvalidateRect(hStatusLabel, NULL, TRUE);
            }
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        SolidBrush bgBrush(Color(255, 13, 15, 24));
        graphics.FillRectangle(&bgBrush, 0, 0, 480, 320);

        SolidBrush headerBrush(Color(255, 21, 25, 40));
        graphics.FillRectangle(&headerBrush, 0, 0, 480, 46);

        FontFamily fontFam(L"Segoe UI");
        Font titleFont(&fontFam, 12, FontStyleBold, UnitPixel);
        SolidBrush titleBrush(Color(255, 255, 255, 255));
        graphics.DrawString(L"HOLLOW LAUNCHER", -1, &titleFont, PointF(18, 14), &titleBrush);

        Font subtitleFont(&fontFam, 10, FontStyleRegular, UnitPixel);
        SolidBrush subTitleBrush(Color(255, 120, 128, 160));
        graphics.DrawString(L"Next.js License API Client", -1, &subtitleFont, PointF(150, 15), &subTitleBrush);

        Font labelFont(&fontFam, 11, FontStyleBold, UnitPixel);
        SolidBrush labelBrush(Color(255, 180, 190, 220));
        graphics.DrawString(L"LICENSE KEY", -1, &labelFont, PointF(35, 92), &labelBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_GdiplusToken, &gdiplusStartupInput, NULL);

    string savedKey = LoadSavedKey();
    if (!savedKey.empty()) {
        wstring statusMsg;
        if (ValidateKeyWithNextAPI(savedKey, statusMsg)) {
            LaunchPayloads();
            GdiplusShutdown(g_GdiplusToken);
            return 0;
        }
    }

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"ProNextLauncherClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassExW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 480;
    int winH = 320;
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;

    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName,
        L"Hollow Launcher",
        WS_POPUP | WS_VISIBLE,
        posX, posY, winW, winH,
        NULL, NULL, hInst, NULL
    );

    HRGN hRgn = CreateRoundRectRgn(0, 0, winW, winH, 14, 14);
    SetWindowRgn(hWnd, hRgn, TRUE);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    GdiplusShutdown(g_GdiplusToken);

    if (g_Authenticated) {
        LaunchPayloads();
    }

    return 0;
}