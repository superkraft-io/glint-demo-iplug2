#pragma once

#if IPLUG_EDITOR

#include "GP.h"
#include "GPGlintDocument.hpp"
#include "glint/platform/glint_view.hpp"

#if defined(OS_WIN)
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <windowsx.h>
#endif

class GPGlintEditorHost final
{
public:
  explicit GPGlintEditorHost(GP& plugin);
  ~GPGlintEditorHost();

  void* Open(void* parent);
  void Close();
  void OnParentResized(int width, int height);
  void SyncFromPlugin(int paramIdx);
  void SetLevelMeters(float inL, float inR, float outL, float outR);


private:
  GPGlintDocumentController         mDocumentController;
  std::unique_ptr<glint::glint_view> mView;
};

inline GPGlintEditorHost::GPGlintEditorHost(GP& plugin)
  : mDocumentController(plugin)
{
  mDocumentController.setRequestRedraw([this] {
    if (mView)
      mView->requestRedraw();
  });
}

inline GPGlintEditorHost::~GPGlintEditorHost()
{
  Close();
}

inline void* GPGlintEditorHost::Open(void* parent)
{
  if (mView)
    return mView->nativeHandle();

  glint::glint_view_options options{};
  options.parent = parent;
  options.width = PLUG_WIDTH;
  options.height = PLUG_HEIGHT;
  options.clearColor = SkColorSetARGB(255, 0x11, 0x11, 0x11);
  options.onDocumentCreated = [this](glint_document& document) {
    mDocumentController.ConfigureDocument(document);
  };

  mView = glint::createView(options);
  return mView ? mView->nativeHandle() : nullptr;
}

inline void GPGlintEditorHost::Close()
{
  mDocumentController.reset();
  mView.reset();
}

inline void GPGlintEditorHost::OnParentResized(int width, int height)
{
  if (!mView)
    return;

  mView->resize(width, height);
}

inline void GPGlintEditorHost::SyncFromPlugin(int paramIdx)
{
  mDocumentController.SyncFromPlugin(paramIdx);
}

inline void GPGlintEditorHost::SetLevelMeters(float inL, float inR, float outL, float outR)
{
  mDocumentController.SetLevelMeters(inL, inR, outL, outR);
}

#endif
