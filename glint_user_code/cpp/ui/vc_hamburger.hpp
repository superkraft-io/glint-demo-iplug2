#pragma once

/**
 * vc_hamburger.hpp
 * glint_gp — hamburger menu button for the header bar.
 *
 * Usage inside a flex component:
 *
 *   #include "app_ui/vc_hamburger.hpp"
 *   ...
 *   _c.add.make(32.f, 32.f, [](const glint_rect& r) { return new VCHamburger(r); });
 *   _c.add.make(36.f, 32.f, [](const glint_rect& r) { return new VCHamburger(r); });
 *
 * Clicking the button opens a native popup menu with two entries:
 *   • About             →  GPAboutModal
 *
 * Dev-tool actions (Inspect Element, Component Demos) remain wired to the
 * native system menu (Alt+Space) — see apps/gp/vcu_window.cpp.
 */

#include "glint/glint_core.hpp"
#include "glint/components/glint_builder.hpp"
#include "glint/components/glint_button.hpp"
#include "glint/platform/glint_platform.hpp"
#ifdef __APPLE__
#include "glint/platform/mac/glint_window_mac.hpp"
#endif
#include "glint_user_code/cpp/ui/about_modal.hpp"

// ── VCHamburger ──────────────────────────────────────────────────────────────
// Extends glint_button — hover/press state, OnMouseOver/Out, and OnMouseUp
// are all inherited. Clicking opens the host's native popup menu.

class VCHamburger : public glint_button
{
public:
  explicit VCHamburger(const glint_rect& r)
  {
    mRect = mPaintRECT = r;  // no label — three lines drawn instead
  }

  // Opens the GP context menu anchored to the bottom-left of 'anchor'.
  // Can be called from any element (e.g. logo image).
  static void openMenu(glint_element* anchor);

protected:
  void drawContent(glint_canvas& g) override
  {
    const glint_color col = _iconColor();
    const float cx = mRect.L + 10.f;
    const float cr = mRect.R - 10.f;
    const float mid = mRect.MH();
    for (int i = 0; i < 3; ++i)
      g.DrawLine(col, cx, mid - 5.f + i * 5.f,
                      cr, mid - 5.f + i * 5.f,
                 nullptr, 1.5f);
  }

  void DrawContentToCanvas(SkCanvas* canvas) override
  {
    if (!canvas) return;

    SkPaint paint;
    paint.setColor(skColor(_iconColor()));
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.5f);
    paint.setStrokeCap(SkPaint::kRound_Cap);

    const float cx = mRect.L + 10.f;
    const float cr = mRect.R - 10.f;
    const float mid = mRect.MH();
    for (int i = 0; i < 3; ++i)
    {
      const float y = mid - 5.f + i * 5.f;
      canvas->drawLine(cx, y, cr, y, paint);
    }
  }

  void OnMouseUp(float x, float y, const glint_mouse_mod& mod) override
  {
    const bool wasPressed = mIsPressed;
    glint_button::OnMouseUp(x, y, mod);  // handles press-release animation
    if (!wasPressed || !mRect.Contains(x, y)) return;
    openMenu(this);
  }

private:
  glint_color _iconColor() const
  {
    return mIsHovered ? glint_color(255, 255, 170, 0)     // #ffaa00
                      : glint_color(255, 255, 255, 255);  // #ffffff
  }
};

inline void VCHamburger::openMenu(glint_element* anchor)
  {
    if (!anchor || !anchor->mRoot) return;

    using P = std::pair<int, std::string>;
    const std::vector<P> items = {
      {2, "About"},
    };
    const std::vector<int> disabled;

    int screenX = 0, screenY = 0;
#ifdef __APPLE__
    if (anchor->mRoot->macWindow) {
      float scrollX = 0.f, scrollY = 0.f;
      for (glint_element* p = anchor->mParent; p; p = p->mParent)
        { scrollX += p->mScrollLeft; scrollY += p->mScrollTop; }
      const float cx = anchor->mRect.L - scrollX;
      const float cy = anchor->mRect.T  - scrollY;
      const RECT sr = anchor->mRoot->macWindow->contentRectToScreen(cx, cy, anchor->mRect.W(), anchor->mRect.H());
      screenX = sr.left;
      screenY = sr.bottom;
    }
#endif

    const int result = glint_platform::showContextMenu(screenX, screenY, items, disabled);
    switch (result)
    {
      case 2:
        GPAboutModal::open(&anchor->mRoot->mCanvas);
        break;
    }
    anchor->setDirty(false);
  }

