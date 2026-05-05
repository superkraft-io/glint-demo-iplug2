#pragma once

/**
 * knobControl.hpp  —  knob + bypass button composite  (user component)
 *
 * Wraps a glint_knob with a bypass_led_btn overlaid at the upper-left corner.
 *
 *   knobControl  (position: relative, flex column)
 *   ├── bypass_led_btn  (position: absolute, top: 0, left: 0)
 *   └── glint_knob       (normal flow)
 *
 * Usage:
 *   auto* ctrl = new knobControl();
 *   ctrl->knob->style.width  = 100.f;
 *   ctrl->knob->style.height = 100.f;
 *   ctrl->bypassBtn->onToggle = [](bool bypassed) { ... };
 *   parent->addChild(ctrl);
 */

#include "knob.hpp"
#include "bypass_led_btn.hpp"

class knobControl : public glint_element
{
public:
    // ── Public children ───────────────────────────────────────────────────────

    glint_element* title = nullptr;
    glint_knob* knob = nullptr;
    bypass_led_btn* bypassBtn = nullptr;

    knobControl()
    {
        classList.add("knob_control");

        align = "center top ttb";

        style.gap = 16.f;

        title = add.div([=](glint_component_style& _c) {
            _c.innerText = "Knob Control";
            _c.style.fontFamily = "Roboto";
            _c.style.fontWeight = 300;
            _c.style.fontSize = 16.f;
        });
        
        // knob: normal-flow child that determines the container's size
        knob = new glint_knob();
        knob->style.width = 100.f;
        knob->style.height = 100.f;
        knob->mInput_->style.width = 75.f;
        knob->style.marginTop = 14.f;
        knob->style.borderRadius = "50%";
        knob->style.boxShadow = "10px 10px 10px rgba(0, 0, 0, 0.5)";

        addChild(knob);
        
        // bypass button: absolutely positioned at the top-left corner
        bypassBtn = new bypass_led_btn();
        bypassBtn->classList.add("knob-control-bypass");
        addChild(bypassBtn);
    }
};
