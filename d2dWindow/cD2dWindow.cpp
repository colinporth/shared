// cD2dWindow.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <wrl.h>
#define _USE_MATH_DEFINES

#include <locale>
#include <codecvt>
#include <thread>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <DXGI1_2.h>
#include <d2d1helper.h>
#include <dwrite.h>

using namespace Microsoft::WRL;
using namespace D2D1;

#include "cD2dWindow.h"

#pragma comment(lib,"d3d11")
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dwrite.lib")

using namespace D2D1;
//}}}

#include "../video/cVidFrame.h"

// static var init
cD2dWindow* cD2dWindow::mD2dWindow = NULL;
int mFontSize = 0;
//{{{
LRESULT CALLBACK WndProc (HWND hWnd, unsigned int msg, WPARAM wparam, LPARAM lparam) {

  return cD2dWindow::mD2dWindow->wndProc (hWnd, msg, wparam, lparam);
  }
//}}}

// public
//{{{
void cD2dWindow::initialise (char* windowTitle, int width, int height, bool fullScreen) {
// windowTitle is wchar_t* rather than wstring

  mD2dWindow = this;

  WNDCLASSEX wndclass = { sizeof (WNDCLASSEX),
                          CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
                          WndProc,
                          0, 0,
                          GetModuleHandle (0),
                          LoadIcon (0,IDI_APPLICATION),
                          LoadCursor (0,IDC_ARROW),
                          NULL,
                          0,
                          "windowClass",
                          LoadIcon (0,IDI_APPLICATION) };

  if (RegisterClassEx (&wndclass))
    mHWND = CreateWindowEx (0,
                            "windowClass",
                            windowTitle,
                            WS_OVERLAPPEDWINDOW,
                            20, 20, width+4, height+32,
                            0, 0,
                            GetModuleHandle(0), 0);

  if (mHWND) {
    //SetWindowLong (hWnd, GWL_STYLE, 0);
    if (fullScreen)
      toggleFullScreen();
    ShowWindow (mHWND, SW_SHOWDEFAULT);
    UpdateWindow (mHWND);

    createDirect2d();

    mRenderThread = std::thread ([=]() { onRender(); } );
    mRenderThread.detach();
    }
  }
//}}}

//{{{
IDWriteTextFormat* cD2dWindow::getTextFormatSize (int fontSize) {

  if (fontSize != mFontSize) {
    if (textFormatSize)
      textFormatSize->Release();
    mDWriteFactory->CreateTextFormat (L"FreeSans", NULL,
                                      DWRITE_FONT_WEIGHT_REGULAR,
                                      DWRITE_FONT_STYLE_NORMAL,
                                      DWRITE_FONT_STRETCH_NORMAL,
                                      float(fontSize), L"en-us", &textFormatSize);
    textFormatSize->SetWordWrapping (DWRITE_WORD_WRAPPING_WRAP);
    mFontSize = fontSize;
    }

  return textFormatSize;
  }
//}}}

//{{{
void cD2dWindow::toggleFullScreen() {

  mFullScreen = !mFullScreen;

  if (mFullScreen) {
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(monitorInfo);

    HMONITOR hMonitor = MonitorFromWindow (mHWND, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo (hMonitor, &monitorInfo);

    WINDOWINFO wndInfo;
    wndInfo.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo (mHWND, &wndInfo);

    mScreenRect = wndInfo.rcWindow;
    mScreenStyle = wndInfo.dwStyle;

    SetWindowLong (mHWND, GWL_STYLE, WS_POPUP);
    SetWindowPos (mHWND, HWND_NOTOPMOST,
                  monitorInfo.rcMonitor.left , monitorInfo.rcMonitor.top,
                  abs(monitorInfo.rcMonitor.left - monitorInfo.rcMonitor.right),
                  abs(monitorInfo.rcMonitor.top - monitorInfo.rcMonitor.bottom),
                  SWP_SHOWWINDOW);
    }
  else {
    AdjustWindowRectEx (&mScreenRect, 0, 0, 0);
    SetWindowLong (mHWND, GWL_STYLE, mScreenStyle);
    SetWindowPos (mHWND, HWND_NOTOPMOST,
                  mScreenRect.left , mScreenRect.top ,
                  abs(mScreenRect.right - mScreenRect.left), abs(mScreenRect.bottom - mScreenRect.top),
                  SWP_SHOWWINDOW);
    }
  }
//}}}
//{{{
ID2D1Bitmap* cD2dWindow::makeBitmap (cVidFrame* vidFrame, ID2D1Bitmap*& bitmap, uint64_t& bitmapPts) {

  if (vidFrame) {
    if (vidFrame->mPts != bitmapPts) {
      bitmapPts = vidFrame->mPts;
      if (bitmap)  {
        auto pixelSize = bitmap->GetPixelSize();
        if ((pixelSize.width != vidFrame->mWidth) || (pixelSize.height != vidFrame->mHeight)) {
          bitmap->Release();
          bitmap = nullptr;
          }
        }
      if (!bitmap) // create bitmap
        mDeviceContext->CreateBitmap (SizeU(vidFrame->mWidth, vidFrame->mHeight), getBitmapProperties(), &bitmap);
      bitmap->CopyFromMemory (
        &RectU (0, 0, vidFrame->mWidth, vidFrame->mHeight), vidFrame->getBgra(), vidFrame->mWidth * 4);
      }
    }
  else if (bitmap) {
    bitmap->Release();
    bitmap = nullptr;
    }

  return bitmap;
  }
//}}}

//{{{
LRESULT cD2dWindow::wndProc (HWND hWnd, unsigned int msg, WPARAM wparam, LPARAM lparam) {

  switch (msg) {
    //{{{
    case WM_SIZE: {
      onResize();
      return 0;
      }
    //}}}
    //{{{
    case WM_KEYDOWN:
      mKeyDown++;

      if (wparam == 0x10)
        mShiftKeyDown = true;
      if (wparam == 0x11)
        mControlKeyDown = true;

      if (onKey ((int)wparam))
        PostQuitMessage (0) ;

      return 0;
    //}}}
    //{{{
    case WM_KEYUP:
      mKeyDown--;

      if (wparam == 0x10)
        mShiftKeyDown = false;
      if (wparam == 0x11)
        mControlKeyDown = false;

      return 0;
    //}}}

    case WM_LBUTTONDOWN:
    //{{{
    case WM_RBUTTONDOWN: {

      if (!mMouseDown)
        SetCapture (hWnd);

      mMouseDown = true;
      mMouseMoved = false;

      mDownMousex = LOWORD(lparam);
      mDownMousey = HIWORD(lparam);
      mRightDown = (msg == WM_RBUTTONDOWN);

      mLastMousex = mDownMousex;
      mLastMousey = mDownMousey;

      onMouseDown (mRightDown, mDownMousex, mDownMousey);

      return 0;
      }
    //}}}

    case WM_LBUTTONUP:
    //{{{
    case WM_RBUTTONUP: {

      int x = LOWORD(lparam);
      int y = HIWORD(lparam);

      mLastMousex = x;
      mLastMousey = y;
      onMouseUp (mRightDown, mMouseMoved, x, y);

      if (mMouseDown)
        ReleaseCapture();
      mMouseDown = false;

      return 0;
      }
    //}}}

    //{{{
    case WM_MOUSEMOVE: {
      short x = (short)(lparam & 0xFFFF);
      short y = (short)((lparam >> 16) & 0xFFFF);

      if (mMouseDown) {
        mMouseMoved = true;
        onMouseMove (mRightDown, x, y, x - mLastMousex, y - mLastMousey);
        mLastMousex = x;
        mLastMousey = y;
        }

      else {
        mProxMousex = x;
        mProxMousey = y;
        onMouseProx (true, x, y);
        }

      if (!mMouseTracking) {
        TRACKMOUSEEVENT tme;
         tme.cbSize = sizeof(TRACKMOUSEEVENT);
         tme.dwFlags = TME_LEAVE;
         tme.hwndTrack = hWnd;

         if (TrackMouseEvent(&tme))
           mMouseTracking = true;
         }

      return 0;
      }
    //}}}
    //{{{
    case WM_MOUSEWHEEL: {
      short delta = GET_WHEEL_DELTA_WPARAM(wparam);
      onMouseWheel (delta);

      return 0;
      }
    //}}}
    //{{{
    case WM_MOUSELEAVE: {
      onMouseProx (false, 0, 0);
      mMouseTracking =  false;
      return 0;
      }
    //}}}
    //{{{
    case WM_DESTROY: {
      PostQuitMessage(0) ;
      return 0;
      }
    //}}}

    //case WM_PAINT:
    //  ValidateRect (hWnd, NULL);
    //  return 0;
    //case WM_DISPLAYCHANGE:
    //  return 0;

    default:
      return DefWindowProc (hWnd, msg, wparam, lparam);
    }
  }
//}}}
//{{{
void cD2dWindow::messagePump() {

  MSG msg;
  while (!mExit && GetMessage (&msg, NULL, 0, 0)) {
    TranslateMessage (&msg);
    DispatchMessage (&msg);
    }
  }
//}}}

// private
//{{{
void cD2dWindow::createDeviceResources() {
// create DX11 API device object, get D3Dcontext

  UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

  D3D_FEATURE_LEVEL featureLevels[] = {
    D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1 };

  ComPtr<ID3D11DeviceContext> mD3context;
  D3D11CreateDevice (nullptr,                   // specify null to use the default adapter
                     D3D_DRIVER_TYPE_HARDWARE,
                     0,
                     creationFlags,             // optionally set debug and Direct2D compatibility flags
                     featureLevels,             // list of feature levels this app can support
                     ARRAYSIZE(featureLevels),  // number of possible feature levels
                     D3D11_SDK_VERSION,
                     &mD3device,                // returns the Direct3D device created
                     nullptr,//&mFeatureLevel,  // returns feature level of device created
                     &mD3context);              // returns the device immediate context

  mD3device.As (&mD3dDevice1);
  mD3context.As (&mD3dContext1);
  mD3device.As (&mDxgiDevice);

  // create D2D1device from DXGIdevice for D2Drendering.
  ComPtr<ID2D1Device> mD2dDevice;
  mD2D1Factory->CreateDevice (mDxgiDevice.Get(), &mD2dDevice);

  // create D2Ddevice_context from D2D1device
  mD2dDevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &mDeviceContext);
  }
//}}}
//{{{
void cD2dWindow::createSizedResources() {

  RECT rect;
  GetClientRect (mHWND, &rect);
  client.width = rect.right - rect.left;
  client.height = rect.bottom - rect.top;
  clientF.width = float(client.width);
  clientF.height = float(client.height);

  if (mSwapChain != nullptr) {
    HRESULT hr = mSwapChain->ResizeBuffers (2, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED) {
      mSwapChain = nullptr;
      createDeviceResources();
      return;
      }
    }
  else {
    // get DXGIDevice from D3dDevice
    ComPtr<IDXGIDevice1> mDxgiDevice1;
    mD3dDevice1.As (&mDxgiDevice1);

    // get DXGIAdapter from DXGIdevice
    ComPtr<IDXGIAdapter> mDxgiAdapter;
    mDxgiDevice1->GetAdapter (&mDxgiAdapter);

    // get DXGIFactory2 from DXGIadapter
    ComPtr<IDXGIFactory2> mDxgiFactory;
    mDxgiAdapter->GetParent (IID_PPV_ARGS (&mDxgiFactory));

    // create swapChain
    //{{{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};

    swapChainDesc.Width = 0;                           // use automatic sizing
    swapChainDesc.Height = 0;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format

    swapChainDesc.Stereo = false;

    swapChainDesc.SampleDesc.Count = 1;                // don't use multi-sampling
    swapChainDesc.SampleDesc.Quality = 0;

    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;                     // use double buffering to enable flip

    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // all apps must use this SwapEffect
    swapChainDesc.Flags = 0;
    //}}}
    mDxgiFactory->CreateSwapChainForHwnd (
      mD3dDevice1.Get(), mHWND, &swapChainDesc, nullptr, nullptr, &mSwapChain);
    mDxgiDevice1->SetMaximumFrameLatency(1);
    }

  //  get D2DtargetBitmap from DXGIbackbuffer, set as mD2dContext D2DrenderTarget.
  D2D1_BITMAP_PROPERTIES1 d2d1_bitmapProperties;
  d2d1_bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
  d2d1_bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
  d2d1_bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
  d2d1_bitmapProperties.colorContext = NULL;
  mD2D1Factory->GetDesktopDpi (&d2d1_bitmapProperties.dpiX, &d2d1_bitmapProperties.dpiY);

  // get DXGIbackbuffer surface pointer from swapchain
  ComPtr<IDXGISurface> dxgiBackBuffer;
  mSwapChain->GetBuffer (0, IID_PPV_ARGS (&dxgiBackBuffer));
  //ComPtr<ID3D11Texture2D> backBuffer;
  //mSwapChain->GetBuffer (0, IID_PPV_ARGS (&backBuffer));
  mDeviceContext->CreateBitmapFromDxgiSurface (dxgiBackBuffer.Get(), &d2d1_bitmapProperties, &mD2dTargetBitmap);

  // set Direct2D render target.
  mDeviceContext->SetTarget (mD2dTargetBitmap.Get());

  // Grayscale text anti-aliasing is recommended for all Metro style apps.
  mDeviceContext->SetTextAntialiasMode (D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
  }
//}}}
//{{{
void cD2dWindow::createDirect2d() {

  // create D2D1Factory
  D2D1_FACTORY_OPTIONS fo = {};
  #ifdef _DEBUG
  fo.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
  #endif
  D2D1CreateFactory (D2D1_FACTORY_TYPE_MULTI_THREADED, fo, mD2D1Factory.GetAddressOf());

  // create DWriteFactory
  DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                       __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&mDWriteFactory));

  createDeviceResources();
  createSizedResources();

  // create Consolas textFormat using DWriteFactory
  mDWriteFactory->CreateTextFormat (L"FreeSans", NULL,
                                    DWRITE_FONT_WEIGHT_REGULAR,
                                    DWRITE_FONT_STYLE_NORMAL,
                                    DWRITE_FONT_STRETCH_NORMAL,
                                    16.0f, L"en-us", &textFormat);
  textFormat->SetWordWrapping (DWRITE_WORD_WRAPPING_WRAP);

  // create some solid brushes
  mDeviceContext->CreateSolidColorBrush (ColorF (ColorF::CornflowerBlue), &blueBrush);
  mDeviceContext->CreateSolidColorBrush (ColorF (0x000000), &blackBrush);
  mDeviceContext->CreateSolidColorBrush (ColorF (0x303030), &dimGreyBrush);
  mDeviceContext->CreateSolidColorBrush (ColorF (0x606060), &greyBrush);
  mDeviceContext->CreateSolidColorBrush (ColorF (0xFFFFFF), &whiteBrush);
  mDeviceContext->CreateSolidColorBrush (ColorF (0xFFFF00), &yellowBrush);
  }
//}}}

//{{{
void cD2dWindow::onResize() {

  RECT rect;
  GetClientRect (mHWND, &rect);

  if (((rect.bottom - rect.top) != client.width) || ((rect.right - rect.left) != client.height)) {
    if (mDeviceContext) {
      mDeviceContext->SetTarget (nullptr);
      mD2dTargetBitmap = nullptr;
      createSizedResources();

      onResize (mDeviceContext.Get());
      }
    }
  }
//}}}
//{{{
void cD2dWindow::onRender() {

  // wait for target bitmap
  while (mD2dTargetBitmap == nullptr)
    Sleep (16);

  int changeCount = 0;
  while (mDeviceContext) {
    if (mChanged || (mChangeRate && (changeCount >= mChangeRate))) {
      mChanged = false;
      changeCount = 0;

      mDeviceContext->BeginDraw();
      mDeviceContext->Clear (ColorF (0x000000));
      onDraw (mDeviceContext.Get());
      mDeviceContext->EndDraw();

      if (mSwapChain->Present (1, 0) == DXGI_ERROR_DEVICE_REMOVED) {
        mSwapChain = nullptr;
        createDeviceResources();
        }
      }
    else {
      Sleep (16);
      if (mChangeRate > 0)
        changeCount++;
      }
    }
  }
//}}}
