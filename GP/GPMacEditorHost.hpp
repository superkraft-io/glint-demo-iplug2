#pragma once

#include "GP.h"

#if defined(OS_MAC) && IPLUG_EDITOR

class GPMacEditorHost final
{
public:
  explicit GPMacEditorHost(GP& plugin);
  ~GPMacEditorHost();

  void* Open(void* parent);
  void Close();
  void OnParentResized(int width, int height);
  void SyncFromPlugin(int paramIdx);

private:
  GP&  mPlugin;
  void* mController = nullptr;
};

#endif