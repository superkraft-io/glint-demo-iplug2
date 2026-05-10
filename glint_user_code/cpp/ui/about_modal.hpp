#pragma once

/**
 * about_modal.hpp
 * GlintPlug — About modal overlay.
 *
 * Ported from view_to_replicate/frontend/sk_ui/sk_ui_about/sk_ui_about.js + .css
 *
 * Layout (horizontal flex card):
 *
 *   ┌─────────────────────────────────────────────────────────────────[×]─┐
 *   │  [mellow_glow + glint logo]                                         │
 *   │  GlintPlug                                                          │
 *   │  Version 1.0.0                                                      │
 *   │  ·                                                                  │
 *   │  Powered by  Superkraft                                             │
 *   │  Created with  Rezonant                                             │
 *   └─────────────────────────────────────────────────────────────────────┘
 *
 * Open/close animation: card fades-in with scale(0.7→1) + opacity(0→1).
 *
 * Static API:
 *   GPAboutModal::open(&mRoot->mCanvas);  // create-on-demand + show
 *   GPAboutModal::close();                // hide + animate out
 *   GPAboutModal::isOpen();               // query
 */

#include "glint/glint_core.hpp"
#include "glint/components/glint_builder.hpp"
#include "gp_platform_utils.hpp"
#include "config.h"

inline constexpr const char* GP_ABOUT_VERSION = PLUG_VERSION_STR;

// ── GPAboutModal ──────────────────────────────────────────────────────────────
class GPAboutModal : public glint_element
{
public:
  // ── Static lifecycle ─────────────────────────────────────────────────────
  static GPAboutModal* sCurrent;

  ~GPAboutModal() override
  {
    if (sCurrent == this) sCurrent = nullptr;
  }

  static void open(glint_element* root)
  {
    if (!sCurrent)
    {
      auto* m = new GPAboutModal();
      root->addChild(m);
      sCurrent = m;
    }
    // Reset card computedStyle to the closed values so the transition engine
    // sees a real delta and fires the animation even on repeated opens.
    // (On second open, computedStyle may already be at the open values if the
    // close transition didn't complete before display:none hid the element.)
    
    sCurrent->style.pointerEvents = "";

    if (sCurrent->mCard)
    {
      sCurrent->mCard->computedStyle.opacity   = 0.f;
      sCurrent->mCard->computedStyle.transform = "scale(0.7)";
      sCurrent->mCard->computedStyle.filter    = "blur(10px) brightness(0)";
    }

    // Animate backdrop in
    sCurrent->style.display  = "flex";
    sCurrent->style.opacity  = 1.f;
    // Animate card in
    if (sCurrent->mCard)
    {
      sCurrent->mCard->style.opacity   = 1.f;
      sCurrent->mCard->style.transform = "scale(1)";
      sCurrent->mCard->style.filter    = "blur(0px) brightness(1)";
    }
    sCurrent->setDirty(false);
  }

  static void close()
  {
    if (!sCurrent) return;
    // Animate card out
    if (sCurrent->mCard)
    {
      sCurrent->mCard->style.opacity   = 0.f;
      sCurrent->mCard->style.transform = "scale(0.7)";
      sCurrent->mCard->style.filter    = "blur(10px) brightness(0)";
    }
    // Fade backdrop out — hide after transition finishes
    sCurrent->style.opacity = 0.f;
    sCurrent->setDirty(false);

    // Hide after the longest transition duration (500 ms from backdrop fade).
    // TODO: hook into transitionend for a cleaner teardown.
    sCurrent->style.pointerEvents = "none";
    sCurrent->setDirty(false);
  }

  static bool isOpen()
  {
    return sCurrent && sCurrent->style.display != "none";
  }

  // ── Constructor ───────────────────────────────────────────────────────────
  GPAboutModal()
  {
    className = "about-modal";  // all styles live in about_modal.css
    add.div([this](auto& _c) {
      _c.className = "about-modal-card";
    }, &mCard);

    if (!mCard) return;

    mCard->add.button([](glint_button& b) {
      b.className       = "about-modal-close-btn";
      b.align           = "center middle";
      
      b.add.div([](glint_component_style& _c) {
        _c.style.width         = 16.f;
        _c.style.height        = 16.f;
        _c.style.filter        = "brightness(0) invert(1)";
        _c.style.pointerEvents = "none";  // let clicks fall through to button
        _c.style.background = "url(/img/svg/close.svg)";
        _c.style.backgroundSize = "contain";
      });

      b.onClick         = []{ GPAboutModal::close(); };
    });

    // ── LEFT column ──────────────────────────────────────────────────────────
    mCard->add.div([](auto& _c) {
      _c.className = "about-modal-left-col";

      // ── Logo container — glow behind, logo on top ─────────────────────
      _c.add.div([](auto& _c) {
        _c.className = "about-modal-logo-container";
        _c.align     = "center middle";

        // Mellow glow — absolute, behind the logo
        /*_c.add.image([](auto& _c) {
          _c.className = "about-modal-glow";
          _c.src       = "/img/mellow_glow.png";
        });*/

         // Mellow glow rotating
        _c.add.img([](glint_image& _c) {
          _c.className = "about-modal-glow-rotating";
          _c.src       = "/img/mellow_glow.png";
        });

        // Square logo — above the glow via z-index
        _c.add.img([](glint_image& _c) {
          _c.className = "about-modal-logo";
          _c.src       = "/img/glint.png";
        });
      });

      // ── "GlintPlug" title row ──────────────────────────────────
      _c.add.div([](auto& _c) {
        _c.className = "about-modal-title-row";

        _c.add.div([](auto& _c) {
          _c.className = "about-modal-title-main";
          _c.innerText = "GlintPlug";
        });
      });

      // ── Version ───────────────────────────────────────────────────────────
      _c.add.div([](auto& _c) {
        _c.className = "about-modal-version";
        _c.innerText = std::string("Version ") + GP_ABOUT_VERSION;
      });

      // ── Spacer ────────────────────────────────────────────────────────────
      _c.add.spacer();

      _c.add.div([](glint_component_style& footer) {
        footer.classList.add("footer");
        footer.align = "center middle ttb";

        footer.add.div([](glint_component_style& label) {
          label.classList.add("footer-made-in");
          label.innerText = "Powered by";
          label.style.fontSize = 10.f;
        });

        footer.add.div([](glint_component_style& icons) {
          icons.align = "center middle";
          icons.style.gap = 4.f;

          icons.add.img([](glint_image& icon) {
            icon.classList.add("powered-by-superkraft-icon");
            icon.src = "/img/superkraft.png";
          });

          icons.add.img([](glint_image& icon) {
            icon.classList.add("powered-by-rezonant-icon");
            icon.src = "/img/rezonant.png";
          });

          icons.add.img([](glint_image& icon) {
            icon.classList.add("powered-by-glint-icon");
            icon.src = "/img/glint.png";
          });
        });
      });
    });

    add.div([](auto& _c) {
      _c.className = "about-modal-copyright-text-container";
      _c.align = "center middle fullwidth";
    
      _c.add.div([](auto& _c) {
        _c.className = "about-modal-copyright-text";
        _c.innerText = "Copyright © 2026 Superkraft · All rights reserved ·";
      });

      _c.add.div([](auto& _c) {
        _c.className = "about-modal-copyright-text-url";
        _c.innerText = "https://superkraft.io";

        _c.addEventListener("mouseup", [](glint_event& e) {
          auto& me = static_cast<glint_mouse_event&>(e);
          if (me.button != 0) return;
          gp_open_url("https://superkraft.io");
        });
      });
    });
  }

protected:
  glint_element* mCard = nullptr;
};

// Out-of-line static definition — one TU only (this header is included once).
inline GPAboutModal* GPAboutModal::sCurrent = nullptr;
