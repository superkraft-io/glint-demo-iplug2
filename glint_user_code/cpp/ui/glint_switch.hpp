#pragma once

/**
 * glint_switch.hpp  (user component — not part of the glint engine)
 * An iOS-style on/off toggle switch. Ported from glint_switch.js.
 *
 * Layout: this component is a flex-row container with padding 2px.
 *   [spacer] [thumb] [anti-spacer]
 *
 *   spacer.style.flexGrow = 0, anti.flexGrow = 1  → thumb at left  (OFF)
 *   spacer.style.flexGrow = 1, anti.flexGrow = 0  → thumb at right (ON)
 *
 *   totalGrow is always 1, so intermediate flex-grow values during the
 *   transition produce proportional thumb positions (smooth animation).
 *
 * Click is handled via element.addEventListener("click", ...) — identical to
 * the JS reference:
 *   this.element.onclick = () => { this.toggled = !this.toggled; }
 *
 * Usage:
 *   #include "source/app_ui/components/glint_switch.hpp"
 *
 *   auto* sw = new glint_switch();
 *   sw->trackOff = "#3c3c3c";
 *   sw->trackOn  = "#3399ff";
 *   sw->toggled  = false;
 *   sw->onChange = [](bool v) { ... };
 *   sw->size     = 26.f;   // container → 52×26, thumb → 22×22
 *   parent->addChild(sw);
 */

#include "glint/glint_element.hpp"

#include <functional>

class glint_switch : public glint_element
{
public:
  // ── Fields ────────────────────────────────────────────────────────────────
  bool     toggled  = false;
  float    size     = 0.f;   // shorthand: container → 2*size × size, thumb → (size-2*pad)²
  sk_color trackOff = glint_color(255,  60,  60,  60);
  sk_color trackOn  = glint_color(255,  51, 153, 255);
  sk_color thumb    = glint_color(255, 255, 255, 255);
  std::function<void(bool)> onChange;
  int tag = sk_no_tag;

  glint_switch()
  {
    add.div([this](auto& _c) {
      _c.style.transition = "flex-grow 0.2s";
      _c.style.width    = 0.f;
      _c.style.flexGrow = toggled ? 1.f : 0.f;
    }, &mSpacer);

    add.div([this](auto& _c) {
      _c.style.backgroundColor = thumb;
    }, &mThumb);

    add.div([this](auto& _c) {
      _c.style.transition = "flex-grow 0.2s";
      _c.style.width    = 0.f;
      _c.style.flexGrow = toggled ? 0.f : 1.f;
    }, &mAntiSpacer);

    element.addEventListener("click", [this](glint_event&) {
      toggled = !toggled;
      applyValue();
      setDirty(false);
      if (onChange) onChange(toggled);
    });

    _syncFromProps();
  }

  // ── Accessors ─────────────────────────────────────────────────────────────

  bool GetToggleValue() const   { return toggled; }
  void SetToggleValue(bool v, bool animate = true)
  {
    toggled = v;
    applyValue(animate);
    setDirty(false);
  }

  const char* typeName() const override { return "switch"; }

  // ── Reactive layout ───────────────────────────────────────────────────────

  void Draw(glint_canvas& g) override
  {
    _syncFromProps();
    glint_element::Draw(g);
  }

  void Layout(glint_canvas* g) override
  {
    _syncFromProps();

    glint_element::Layout(g);

    if (mThumb && size <= 0.f)
    {
      const float h = mThumb->mRect.H();
      if (h > 0.f)
      {
        mThumb->style.width        = h;
        mThumb->style.borderRadius = h / 2.f;
      }
    }
  }

  // ── Hit testing ───────────────────────────────────────────────────────────

  glint_element* HitTest(float x, float y) override
  {
    return GetPaintRECT().Contains(x, y) ? this : nullptr;
  }

private:
  glint_element* mSpacer     = nullptr;
  glint_element* mThumb      = nullptr;
  glint_element* mAntiSpacer = nullptr;

  void _syncFromProps()
  {
    if (size > 0.f)
    {
      style.width  = size * 2.f;
      style.height = size;
    }

    const float pad = 0.f;
    const float h   = (size > 0.f) ? size : mRect.H();
    const float td  = h - 2.f;

    style.display         = "flex";
    style.flexDirection   = "row";
    style.alignItems      = "stretch";
    style.paddingLeft     = pad;
    style.paddingRight    = pad;
    style.paddingTop      = pad;
    style.paddingBottom   = pad;
    style.borderRadius    = h / 2.f;
    style.borderColor     = glint_color(60, 255, 255, 255);
    style.borderWidth     = 1.f;
    style.backgroundColor = toggled ? trackOn : trackOff;
    style.transition      = "background-color 0.2s";

    if (mThumb)
    {
      mThumb->style.width           = td;
      mThumb->style.borderRadius    = td / 1.f;
      mThumb->style.backgroundColor = thumb;
    }
    if (mSpacer)
    {
      mSpacer->style.transition = "flex-grow 0.2s";
      mSpacer->style.flexGrow   = toggled ? 1.f : 0.f;
    }
    if (mAntiSpacer)
    {
      mAntiSpacer->style.transition = "flex-grow 0.2s";
      mAntiSpacer->style.flexGrow   = toggled ? 0.f : 1.f;
    }
  }

  void applyValue(bool animate = true)
  {
    const std::string rootTransition = style.transition;
    const std::string spacerTransition = mSpacer ? mSpacer->style.transition : std::string();
    const std::string antiSpacerTransition = mAntiSpacer ? mAntiSpacer->style.transition : std::string();

    if (!animate)
    {
      style.transition = "";
      if (mSpacer) mSpacer->style.transition = "";
      if (mAntiSpacer) mAntiSpacer->style.transition = "";
    }

    _syncFromProps();
    style.backgroundColor = toggled ? trackOn : trackOff;
    if (mSpacer)     mSpacer->style.flexGrow     = toggled ? 1.f : 0.f;
    if (mAntiSpacer) mAntiSpacer->style.flexGrow = toggled ? 0.f : 1.f;
    if (mThumb)      mThumb->style.backgroundColor = thumb;

    if (!animate)
    {
      style.transition = rootTransition;
      if (mSpacer) mSpacer->style.transition = spacerTransition;
      if (mAntiSpacer) mAntiSpacer->style.transition = antiSpacerTransition;
    }
  }
};

// Backward-compat alias
using sk_ui_switch = glint_switch;
