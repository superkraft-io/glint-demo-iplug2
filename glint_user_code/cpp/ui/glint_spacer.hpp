#pragma once

/**
 * glint_spacer.hpp  (user component — not part of the glint engine)
 * A flex-grow spacer — invisible child that expands to fill all remaining
 * main-axis space inside a flex container.
 *
 * Usage:
 *   #include "source/app_ui/components/glint_spacer.hpp"
 *
 *   parent->addChild(new glint_spacer());   // fills remaining flex space
 *
 * Note: the engine builder's add.spacer() creates an equivalent plain
 * glint_element with flexGrow=1 and does not require this class.
 * Use this class when you need the named type (e.g. for isinstance checks
 * or when constructing outside a builder context).
 */

#include "glint/glint_element.hpp"

// ── glint_spacer ─────────────────────────────────────────────────────────────
class glint_spacer : public glint_element
{
public:
  glint_spacer()
  {
    style.flexGrow = 1.f;
    style.width    = 0.f;
    style.height   = 0.f;
  }

  const char* typeName() const override { return "spacer"; }
  const char* tagName()  const override { return "div"; }

  // Spacers are invisible and should not intercept mouse events.
  glint_element* HitTest(float, float) override { return nullptr; }
};

// Backward-compat alias
using sk_ui_spacer = glint_spacer;
