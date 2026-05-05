#pragma once

#if defined(OS_WIN) && IPLUG_EDITOR && defined(APP_API)

#include "GPGlintDocument.hpp"

#include "glint/platform/glint_window.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <memory>

namespace gp_glint {

inline bool standaloneTopLevelTestEnabled()
{
  char value[8] = {};
  const DWORD length = ::GetEnvironmentVariableA("GP_GLINT_TOPLEVEL_TEST", value, sizeof(value));
  return length > 0 && value[0] != '\0' && value[0] != '0';
}

} // namespace gp_glint

class GPGlintTopLevelWindow final : public glint_window
{
public:
  explicit GPGlintTopLevelWindow(GP& plugin)
    : mDocumentController(plugin)
  {
    mDocumentController.setRequestRedraw([this] {
      if (const HWND hwnd = mHWNDAtom.load())
        scheduleWindowRedraw(hwnd);
    });
  }

  bool Open(HWND standaloneParent)
  {
    if (isRunning())
      return mHWNDAtom.load() != nullptr;

    mStandaloneParent = standaloneParent;
    mCloseParentOnDestroy = true;
    mWindowDestroyed = false;

    startThread();

    const HWND hwnd = mHWNDAtom.load();
    if (hwnd != nullptr)
    {
      if (standaloneParent != nullptr)
        ::PostMessage(standaloneParent, WM_SYSCOMMAND, SC_MINIMIZE, 0);
      ::SetForegroundWindow(hwnd);
    }

    return hwnd != nullptr;
  }

  void Close()
  {
    if (!isRunning() || mWindowDestroyed)
      return;

    stopThread();
  }

  void CloseFromParent()
  {
    mCloseParentOnDestroy = false;
    mStandaloneParent = nullptr;
    Close();
  }

  void WaitForClose()
  {
    for (int i = 0; i < 3000 && isRunning(); ++i)
      ::Sleep(1);
  }

  bool IsOpen() const
  {
    return isRunning();
  }

  void SyncFromPlugin(int paramIdx)
  {
    mDocumentController.SyncFromPlugin(paramIdx);
  }

  void SetLevelMeters(float inL, float inR, float outL, float outR)
  {
    mDocumentController.SetLevelMeters(inL, inR, outL, outR);
  }

protected:
  const wchar_t* windowClassName() const override
  {
    return L"GPGlintTopLevelWindow";
  }

  const wchar_t* windowTitle() const override
  {
    return L"GP Glint Top-Level Test";
  }

  int defaultWidth() const override
  {
    return PLUG_WIDTH;
  }

  int defaultHeight() const override
  {
    return PLUG_HEIGHT;
  }

  void buildUI() override
  {
    if (mOwnRoot)
      mDocumentController.ConfigureDocument(*mOwnRoot);
  }

  void onDestroyed() override
  {
    mDocumentController.reset();
    mWindowDestroyed = true;

    if (!mCloseParentOnDestroy || mStandaloneParent == nullptr)
      return;

    const HWND standaloneParent = mStandaloneParent;
    mStandaloneParent = nullptr;
    mCloseParentOnDestroy = false;
    ::PostMessage(standaloneParent, WM_CLOSE, 0, 0);
  }

private:
  GPGlintDocumentController mDocumentController;
  HWND                       mStandaloneParent = nullptr;
  bool                       mCloseParentOnDestroy = false;
  bool                       mWindowDestroyed = false;
};

class GPGlintTopLevelHost final
{
public:
  explicit GPGlintTopLevelHost(GP& plugin)
    : mWindow(std::make_unique<GPGlintTopLevelWindow>(plugin))
  {
  }

  ~GPGlintTopLevelHost()
  {
    CloseFromParent();
  }

  void* Open(void* parent)
  {
    if (!mWindow)
      return nullptr;

    return mWindow->Open(static_cast<HWND>(parent)) ? parent : nullptr;
  }

  void CloseFromParent()
  {
    if (!mWindow)
      return;

    mWindow->CloseFromParent();
    mWindow->WaitForClose();
  }

  bool IsOpen() const
  {
    return mWindow && mWindow->IsOpen();
  }

  void SyncFromPlugin(int paramIdx)
  {
    if (mWindow)
      mWindow->SyncFromPlugin(paramIdx);
  }

private:
  std::unique_ptr<GPGlintTopLevelWindow> mWindow;
};

#endif