#pragma once

/**
 * gp_platform_utils.hpp
 * Thin cross-platform helpers for GlintPlug UI code.
 */

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <shellapi.h>
#elif defined(__APPLE__)
  #include <TargetConditionals.h>
  #if TARGET_OS_OSX
    #include <CoreFoundation/CoreFoundation.h>
  #endif
#endif

// Opens a URL in the OS default browser.
// On Windows: ShellExecuteW
// On macOS:   LSOpenCFURLRef  (no ObjC / AppKit required)
// Other platforms: no-op (add as needed).
inline void gp_open_url(const char* url)
{
  if (!url || !*url) return;

#if defined(_WIN32)
  // Convert UTF-8 → UTF-16 for ShellExecuteW.
  const int wlen = ::MultiByteToWideChar(CP_UTF8, 0, url, -1, nullptr, 0);
  if (wlen <= 0) return;
  auto* wurl = new wchar_t[wlen];
  ::MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, wlen);
  ::ShellExecuteW(nullptr, L"open", wurl, nullptr, nullptr, SW_SHOWNORMAL);
  delete[] wurl;

#elif defined(__APPLE__) && TARGET_OS_OSX
  CFStringRef cfStr = CFStringCreateWithCString(nullptr, url, kCFStringEncodingUTF8);
  if (!cfStr) return;
  CFURLRef cfURL = CFURLCreateWithString(nullptr, cfStr, nullptr);
  CFRelease(cfStr);
  if (!cfURL) return;
  LSOpenCFURLRef(cfURL, nullptr);
  CFRelease(cfURL);
#endif
}
