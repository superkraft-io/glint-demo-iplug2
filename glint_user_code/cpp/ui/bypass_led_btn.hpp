#pragma once

/**
 * bypass_led_btn.hpp  —  bypass toggle button  (user component)
 *
 * A small clickable row composed of:
 *   [LED image]  [BYPASS label]
 *
 * Clicking toggles bypassed_ and swaps the LED image between
 * led_off.png (default) and led_on.png.
 *
 * Usage:
 *   auto* btn = new bypass_led_btn();
 *   btn->onToggle = [](bool bypassed) { ... };
 *   parent->addChild(btn);
 *
 *   // or externally drive the state:
 *   btn->setBypassed(true);
 */

#include "glint/components/glint_builder.hpp"
#include <functional>

class bypass_led_btn : public glint_element
{
public:
    // ── Public API ────────────────────────────────────────────────────────────

    /** Current bypass state. */
    bool bypassed_ = false;

    /** Called whenever the bypass state toggles (via click). */
    std::function<void(bool)> onToggle;

    bypass_led_btn()
    {
        classList.add("bypass-led-btn");
        style.display       = "flex";
        style.flexDirection = "row";
        style.alignItems    = "center";
        style.gap           = 4.f;

        add.div([](auto& lbl) {
            lbl.className = "bypass-led-btn-label";
            lbl.innerText = "BYPASS";
        });

        
        mLedImg_ = add.image([](glint_image& img) {
            img.className = "bypass-led-btn-img";
            img.src       = "/img/led/led_off.png";
        });

        
        element.addEventListener("click", [this](glint_event&) {
            setBypassed(!bypassed_);
            if (onToggle) onToggle(bypassed_);
        });
    }

    /**
     * Programmatically set the bypass state.
     * Updates the LED image and marks the element dirty for redraw.
     */
    void setBypassed(bool v)
    {
        bypassed_ = v;
        if (mLedImg_)
            mLedImg_->SetSrc(bypassed_ ? "/img/led/led_on.png" : "/img/led/led_off.png");
        setDirty(false);
    }

private:
    glint_image* mLedImg_ = nullptr;
};
