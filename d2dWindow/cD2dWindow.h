// cD2dWindow.h
#pragma once

class cVidFrame;

class cD2dWindow {
public:
  cD2dWindow() {}
  virtual ~cD2dWindow() {}

  void initialise (wchar_t* title, int width, int height, bool fullScreen = false);

  // gets
  D2D1_SIZE_F getClientF() { return clientF; }

  ID2D1DeviceContext* getDeviceContext() { return mDeviceContext.Get(); }

  IDWriteTextFormat* getTextFormat() { return textFormat; }
  IDWriteTextFormat* getTextFormatSize (int fontSize);
  ID2D1SolidColorBrush* getBlueBrush() { return blueBrush; }
  ID2D1SolidColorBrush* getBlackBrush() { return blackBrush; }
  ID2D1SolidColorBrush* getDimGreyBrush() { return dimGreyBrush; }
  ID2D1SolidColorBrush* getGreyBrush() { return greyBrush; }
  ID2D1SolidColorBrush* getWhiteBrush() { return whiteBrush; }
  ID2D1SolidColorBrush* getYellowBrush() { return yellowBrush; }
  //{{{
  D2D1_BITMAP_PROPERTIES getBitmapProperties() {
    return { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE, 96.0f, 96.0f };
    }
  //}}}

  bool getMouseDown() { return mMouseDown; }

  ID3D11Device* getD3d11Device() { return mD3device.Get(); }
  IDWriteFactory* getDwriteFactory() { return mDWriteFactory; }

  bool getFullScreen() { return mFullScreen; }
  void toggleFullScreen();
  void setExit() { mExit = true; }

  void setChangeRate (int changeRate) { mChangeRate = changeRate; }
  void changed() { mChanged = true; }

  ID2D1Bitmap* makeBitmap (cVidFrame* yuvFrame, ID2D1Bitmap*& bitmap, uint64_t& bitmapPts);

  LRESULT wndProc (HWND hWnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
  void messagePump();

  // public static var
  static cD2dWindow* mD2dWindow;
  HWND mHWND = 0;

protected:
  virtual bool onKey(int key) { return false; }
  virtual void onMouseWheel (int delta) {}
  virtual void onMouseProx (bool inClient, int x, int y) {}
  virtual void onMouseDown (bool right, int x, int y) {};
  virtual void onMouseMove (bool right, int x, int y, int xInc, int yInc) {}
  virtual void onMouseUp (bool right, bool mouseMoved, int x, int y) {}
  virtual void onDraw (ID2D1DeviceContext* dc) = 0;
  //{{{
  int keyInc() {
  // control+shift = 1hour
  // shift   = 5min
  // control = 1min
    return mControlKeyDown ? (mShiftKeyDown ? 60*60 : 60) : (mShiftKeyDown ? 5*60 : 1);
    }
  //}}}
  //{{{  protected vars
  int mKeyDown = 0;

  bool mShiftKeyDown = false;
  bool mControlKeyDown = false;

  bool mMouseDown = false;
  bool mRightDown = false;

  bool mMouseMoved = false;
  bool mDownConsumed = false;

  int mDownMousex = 0;
  int mDownMousey = 0;

  int mLastMousex = 0;
  int mLastMousey = 0;

  int mProxMousex = 0;
  int mProxMousey = 0;
  //}}}

private:
  void createDeviceResources();
  void createSizedResources();
  void createDirect2d();

  void onResize();
  void onRender();

  // private vars
  bool mExit = false;
  bool mChanged = false;
  int mChangeRate = 0;
  bool mMouseTracking= false;
  //{{{  deviceIndependentResources
  ComPtr<ID2D1Factory1> mD2D1Factory;
  IDWriteFactory* mDWriteFactory = nullptr;
  //}}}
  //{{{  deviceResources
  ComPtr<ID3D11Device> mD3device;
  ComPtr<ID3D11Device1> mD3dDevice1;
  ComPtr<ID3D11DeviceContext1> mD3dContext1;
  ComPtr<IDXGIDevice> mDxgiDevice;
  //}}}
  //{{{  sizedResources
  D2D1_SIZE_U client;
  D2D1_SIZE_F clientF;

  ComPtr<IDXGISwapChain1> mSwapChain;
  ComPtr<ID2D1Bitmap1> mD2dTargetBitmap;

  ComPtr<ID2D1DeviceContext> mDeviceContext;

  IDWriteTextFormat* textFormat = nullptr;
  IDWriteTextFormat* textFormatSize = nullptr;
  ID2D1SolidColorBrush* blueBrush = nullptr;
  ID2D1SolidColorBrush* blackBrush = nullptr;
  ID2D1SolidColorBrush* dimGreyBrush = nullptr;
  ID2D1SolidColorBrush* greyBrush = nullptr;
  ID2D1SolidColorBrush* whiteBrush = nullptr;
  ID2D1SolidColorBrush* yellowBrush = nullptr;
  //}}}
  //{{{  fullScreen
  bool mFullScreen = false;
  RECT mScreenRect;
  DWORD mScreenStyle;
  //}}}

  std::thread mRenderThread;
  };
