#pragma once

#if IPLUG_EDITOR

#include "GP.h"

#include "glint/glint_standalone.hpp"

#if defined(GLINT_BUNDLE_DEEP) || defined(GLINT_BUNDLE_SHALLOW)
  #include "glint_bundle/glint_bundle_library.hpp"
#endif

#include "glint_user_code/cpp/ui/knob.hpp"
#include "glint_user_code/cpp/ui/knobControl.hpp"
#include "glint_user_code/cpp/ui/bypass_led_btn.hpp"
#include "glint_user_code/cpp/ui/vc_hamburger.hpp"
#include "glint_user_code/cpp/ui/vuMeter.hpp"

#include <ctime>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace gp_glint {

inline std::filesystem::path repoRootDir()
{
  return std::filesystem::path(__FILE__).parent_path() / "..";
}

inline std::filesystem::path stylesDir()
{
  return repoRootDir() / "glint_user_code" / "web" / "styles";
}

inline std::filesystem::path assetsDir()
{
  return repoRootDir() / "glint_user_code" / "web";
}

inline void syncKnobAVisuals(glint_knob& knob, float value)
{
  if (value < 0.f)
  {
    knob.setArcValueColor("#ff0606");
    if (knob.mOuterGlow)
      knob.mOuterGlow->style.backgroundColor = "#ff0606";
    if (knob.mInnerGlow)
      knob.mInnerGlow->style.boxShadow = "inset 0px -3px 8px 2px #ff0606";
  }
  else
  {
    knob.setArcValueColor("#1998ff");
    if (knob.mOuterGlow)
      knob.mOuterGlow->style.backgroundColor = "#1998ff";
    if (knob.mInnerGlow)
      knob.mInnerGlow->style.boxShadow = "inset 0px -3px 8px 2px #1998ff";
  }
}

inline void syncKnobBVisuals(glint_knob& knob, float value)
{
  if (value < 0.f)
  {
    knob.setArcValueColor("#ff0606");
    if (knob.mOuterGlow)
      knob.mOuterGlow->style.backgroundColor = "#ff0606";
    if (knob.mInnerGlow)
      knob.mInnerGlow->style.boxShadow = "inset 0px -3px 8px 2px #ff0606";
  }
  else
  {
    knob.setArcValueColor("#41F699");
    if (knob.mOuterGlow)
      knob.mOuterGlow->style.backgroundColor = "#41F699";
    if (knob.mInnerGlow)
      knob.mInnerGlow->style.boxShadow = "inset 0px -3px 8px 2px #41F699";
  }
}

inline void loadAppStyles(glint_document& document)
{
  document.loadStylesheet("/styles/main.css");
  document.loadStylesheet("/styles/main_ui.css");
  document.loadStylesheet("/styles/second_stylesheet.css");
  document.loadStylesheet("/styles/third_stylesheet.css");
  document.loadStylesheet("/styles/about_modal.css");
  document.loadStylesheet("/styles/license_form.css");
  document.loadStylesheet("/styles/knob.css");
}

inline knobControl* addKnobCluster(glint_component_style& parent,
                                   const char*            label,
                                   const char*            id,
                                   float                  value,
                                   float                  minValue,
                                   float                  maxValue)
{
  auto* control = new knobControl();
  auto* knob = control->knob;

  knob->id = id;
  knob->arcThickness = 3.f;
  knob->arcRadiusOffset = 3.5f;
  knob->ticksRadiusFrom = 4.f;
  knob->ticksRadiusTo = 8.f;
  knob->minValue = minValue;
  knob->maxValue = maxValue;
  knob->setValue(value);
  knob->mInput_->style.backgroundColor = "#000000a5";
  knob->mInput_->style.border = "none";
  
  knob->arrow->style.filter = "drop-shadow(0px 0px 1px #000000)";

  //knob->knobFace->style.filter = "saturate(0.4) brightness(1)";
  knob->knobFace->src = "/img/knob_face_light.png";

  for (auto& tick : knob->ticks)
  {
    tick.color = glint_color(255, 128, 128, 128);
    tick.thickness = 1.f;
    tick.length = 4.f;
  }

  knob->ticks.front().length = 10.f;
  knob->ticks[knob->ticks.size() / 2].length = 10.f;
  knob->ticks.back().length = 10.f;
  knob->ticks[knob->ticks.size() / 2].color = glint_color(255, 255, 255, 255);

  parent.add.div([=](glint_component_style& group) {
    group.style.display = "flex";
    group.style.flexDirection = "column";
    group.style.alignItems = "center";
    group.style.gap = 10.f;
    group.add.attach(control);
  });

  control->title->innerText = label;
  knob->addKnobTextureClass("main_knob");
  return control;
}

inline glint_knob* addIoLevelKnob(glint_component_style& parent,
                                  const char*            id,
                                  const char*            label,
                                  float                  marginBottom = 0.f,
                                  float                  minVal       = -24.f,
                                  float                  maxVal       = 24.f,
                                  float                  defaultVal   = 0.f,
                                  std::shared_ptr<GPVuMeter>* outMeter = nullptr)
{
  auto* knob = new glint_knob();
  knob->id = id;
  knob->defaultValue = defaultVal;
  knob->minValue = minVal;
  knob->maxValue = maxVal;
  knob->arcThickness = 2.f;
  knob->arcRadiusOffset = 0.f;
  knob->style.width = 32.f;
  knob->style.height = 32.f;
  knob->setArcTrackColor(glint_color(0, 0, 0, 1));
  knob->setArcValueColor("#ffffff");
  knob->setValue(defaultVal);
  knob->ticks.clear();
  knob->knobFace->style.filter = "brightness(0.62) saturate(0)";
  knob->mInput_->style.display = "none";
  knob->ridges->style.display = "none";

  knob->arrow->style.width = 6.f;
  knob->arrow->style.height = 6.f;
  knob->arrow->style.marginRight = 10.f;
  knob->arrow->style.borderRadius = 12.f;
  knob->arrow->style.backgroundColor = "white";
  knob->arrow->className = "";

  if (knob->mOuterGlow_wrapper) knob->mOuterGlow_wrapper->style.display = "none";
  if (knob->mOuterGlow) knob->mOuterGlow->style.display = "none";
  if (knob->mInnerGlow) knob->mInnerGlow->style.display = "none";

  parent.add.div([=](glint_component_style& group) {

    group.add.div([=](glint_component_style& empty) {
      empty.style.width = 1.f;
      empty.style.height = 10.f;
    });

    group.style.position = "relative";
    group.align = "center middle";
    group.style.display = "flex";
    group.style.flexDirection = "column";
    group.style.alignItems = "center";
    group.style.gap = 10.f;
    group.style.width  = 32.f;
    group.style.height = 214.f;   // meter(100) + gap(6) + knob(32) + gap(6) + label(14)
    if (marginBottom > 0.f)
      group.style.marginBottom = marginBottom;

    if (outMeter)
    {
      GPVuMeterConfig cfg;
      cfg.numSegments = 20;
      cfg.segGap      = 1.5f;
      cfg.segments.assign(14, { "#22c55e", "#0a2e14", "#86efac" });                    // green  (0–13)  → up to -12 dBFS
      cfg.segments.insert(cfg.segments.end(), 4, { "#eab308", "#2a1f00", "#fde047" }); // yellow (14–17) → -12 to -4 dBFS
      cfg.segments.insert(cfg.segments.end(), 2, { "#ef4444", "#3b0c0c", "#fca5a5" }); // red    (18–19) → -4 to  0 dBFS

      const auto meterState = std::make_shared<GPVuMeter>(std::move(cfg));
      *outMeter = meterState;

      group.add.div([meterState](glint_component_style& meter) {
        meter.style.display = "block";
        meter.style.width = 25.f;
        meter.style.height = 151.f;
        meter.draw = [meterState](glint_canvas& g, const glint_rect& rect) {
          meterState->Draw(g, rect);
        };
      });
    }

    group.add.attach(knob);

    group.add.div([=](glint_component_style& text) {
      text.innerText = label;
      text.style.color = "#ffffff";
      text.style.fontSize = 10.f;
      text.style.fontFamily = "Roboto";
      text.style.fontWeight = 500.f;
      text.style.userSelect = "none";
      text.style.bottom = 10.f;
      text.style.position = "absolute";
      text.style.pointerEvents = "none";
    });
  });

  return knob;
}

} // namespace gp_glint

class GPGlintDocumentController final
{
public:
  explicit GPGlintDocumentController(GP& plugin)
    : mPlugin(plugin)
  {
  }

  void setRequestRedraw(std::function<void()> requestRedraw)
  {
    mRequestRedraw = std::move(requestRedraw);
  }

  void reset()
  {
    mSyncingFromPlugin = false;
    mCanvas             = nullptr;
    mKnobAKnob          = nullptr;
    mKnobBKnob         = nullptr;
    mInputKnob          = nullptr;
    mOutputKnob         = nullptr;
    mInputMeter         = nullptr;
    mOutputMeter        = nullptr;
    mKnobABypassBtn = nullptr;
    mKnobBBypassBtn    = nullptr;
  }

  // Called from the UI-thread meter timer to push live level data.
  void SetLevelMeters(float inL, float inR, float outL, float outR)
  {
    if (mInputMeter)  mInputMeter->SetLevel(inL, inR);
    if (mOutputMeter) mOutputMeter->SetLevel(outL, outR);
    if (mRequestRedraw) mRequestRedraw();
  }

  void SyncFromPlugin(int paramIdx)
  {
    // --- Bypass buttons (non-knob) ------------------------------------------
    if (paramIdx == kKnobABypass && mKnobABypassBtn)
    {
      mKnobABypassBtn->setBypassed(mPlugin.GetParam(kKnobABypass)->Value() > 0.5);
      if (mRequestRedraw) mRequestRedraw();
      return;
    }
    if (paramIdx == kKnobBBypass && mKnobBBypassBtn)
    {
      mKnobBBypassBtn->setBypassed(mPlugin.GetParam(kKnobBBypass)->Value() > 0.5);
      if (mRequestRedraw) mRequestRedraw();
      return;
    }

    // --- Knob params --------------------------------------------------------
    glint_knob* targetKnob = nullptr;
    float nextValue = 0.f;

    if (paramIdx == kKnobAMix && mKnobAKnob)
    {
      targetKnob = mKnobAKnob;
      nextValue = static_cast<float>(mPlugin.GetParam(kKnobAMix)->Value());
    }
    else if (paramIdx == kKnobBMix && mKnobBKnob)
    {
      targetKnob = mKnobBKnob;
      nextValue = static_cast<float>(mPlugin.GetParam(kKnobBMix)->Value());
    }
    else if (paramIdx == kInputVolume && mInputKnob)
    {
      targetKnob = mInputKnob;
      nextValue = static_cast<float>(mPlugin.GetParam(kInputVolume)->Value());
    }
    else if (paramIdx == kOutputVolume && mOutputKnob)
    {
      targetKnob = mOutputKnob;
      nextValue = static_cast<float>(mPlugin.GetParam(kOutputVolume)->Value());
    }
    else
    {
      return;
    }

    mSyncingFromPlugin = true;
    targetKnob->setValue(nextValue);

    if (paramIdx == kKnobAMix)
      gp_glint::syncKnobAVisuals(*targetKnob, nextValue);
    else if (paramIdx == kKnobBMix)
      gp_glint::syncKnobBVisuals(*targetKnob, nextValue);

    mSyncingFromPlugin = false;

    if (mRequestRedraw)
      mRequestRedraw();
  }

  void ConfigureDocument(glint_document& document)
  {
    mKnobAKnob = nullptr;
    mKnobBKnob = nullptr;

    document.name = "glint_gp";
    document.mCanvas.style.backgroundColor = "#111111";
    mCanvas = &document.mCanvas;

    document.add.div([this](glint_component_style& app) {
      app.classList.add("app-root");

      app.add.img([](glint_image& logo) {
        logo.classList.add("app-background");
        logo.src = "/img/background.png";
      });
      
      app.add.div([](glint_component_style& header) {
        header.classList.add("header");

        header.add.img([](glint_image& logo) {
          logo.classList.add("header-logo");
          logo.src = "/img/glint.png";
          logo.addEventListener("mouseup", [](glint_event& e) {
            auto& me = static_cast<glint_mouse_event&>(e);
            if (me.button != 0) return;
            VCHamburger::openMenu(e.target);
          });
        });


        header.add.div([](glint_component_style& titleWrap) {
          titleWrap.classList.add("header-title");

          titleWrap.add.div([](glint_component_style& title) {
            title.classList.add("header-title-name");
            title.innerText = "GlintPlug";
          });
        });

        header.add.spacer();

        header.add.div([](glint_component_style& badge) {
          badge.classList.add("latency-badge");

          badge.style.background = "url(/img/latency_badge_256.png)";

        });
      });

      app.add.div([this](glint_component_style& content) {
        content.classList.add("main-content");
        content.style.position = "relative";
        content.style.display = "flex";
        content.style.flexDirection = "row";
        content.style.alignItems = "center";
        content.style.justifyContent = "center";

        content.add.div([this](glint_component_style& controls) {
          controls.classList.add("controls-strip");
          controls.align = "left top fill";

          auto* panControl = gp_glint::addKnobCluster(
            controls,
            "PAN",
            "main_knob_a",
            static_cast<float>(mPlugin.GetParam(kKnobAMix)->Value()),
            -100.f,
            100.f);
          auto* panKnob = panControl->knob;
          panKnob->setArcTrackColor("#15242e");
          gp_glint::syncKnobAVisuals(*panKnob, panKnob->value_);
          panKnob->onChange = [this, panKnob](float value) {
            gp_glint::syncKnobAVisuals(*panKnob, value);

            if (mSyncingFromPlugin)
              return;

            mPlugin.SendParameterValueFromUI(kKnobAMix, mPlugin.GetParam(kKnobAMix)->ToNormalized(value));
          };
          panKnob->formatValueText = [](float value) -> std::string {
            if (value == 0.f)
              return "CENTER";
            if (value < 0.f)
              return "Left " + std::to_string(static_cast<int>(0.f - value)) + "%";
            return "Right " + std::to_string(static_cast<int>(value)) + "%";
          };
          panKnob->addEventListener("mousedown", [this](glint_event& e) {
            auto& me = static_cast<glint_mouse_event&>(e);
            if (me.button != 2) return;
            e.stopPropagation();
            mPlugin.ShowParamContextMenu(kKnobAMix, me.clientX, me.clientY);
          });
          mKnobAKnob = panKnob;
          panControl->bypassBtn->setBypassed(mPlugin.GetParam(kKnobABypass)->Value() > 0.5);
          panControl->bypassBtn->onToggle = [this](bool bypassed) {
            mPlugin.SendParameterValueFromUI(kKnobABypass,
              mPlugin.GetParam(kKnobABypass)->ToNormalized(bypassed ? 1.0 : 0.0));
          };
          panControl->bypassBtn->addEventListener("mousedown", [this](glint_event& e) {
            auto& me = static_cast<glint_mouse_event&>(e);
            if (me.button != 2) return;
            e.stopPropagation();
            mPlugin.ShowParamContextMenu(kKnobABypass, me.clientX, me.clientY);
          });
          mKnobABypassBtn = panControl->bypassBtn;

          auto* crushControl = gp_glint::addKnobCluster(
            controls,
            "CRUSH",
            "main_knob_b",
            static_cast<float>(mPlugin.GetParam(kKnobBMix)->Value()),
            0.f,
            100.f);
          auto* crushKnob = crushControl->knob;
          crushKnob->setArcTrackColor("#0c341f");
          gp_glint::syncKnobBVisuals(*crushKnob, crushKnob->value_);
          crushKnob->onChange = [this, crushKnob](float value) {
            gp_glint::syncKnobBVisuals(*crushKnob, value);

            if (mSyncingFromPlugin)
              return;

            mPlugin.SendParameterValueFromUI(kKnobBMix, mPlugin.GetParam(kKnobBMix)->ToNormalized(value));
          };
          crushKnob->formatValueText = [](float value) -> std::string {
            if (value == 0.f)
              return "CLEAN";
            const int bits = static_cast<int>(24.f - (value / 100.f) * 23.f);
            return std::to_string(bits) + " bit";
          };
          crushKnob->addEventListener("mousedown", [this](glint_event& e) {
            auto& me = static_cast<glint_mouse_event&>(e);
            if (me.button != 2) return;
            e.stopPropagation();
            mPlugin.ShowParamContextMenu(kKnobBMix, me.clientX, me.clientY);
          });
          mKnobBKnob = crushKnob;
          crushControl->bypassBtn->setBypassed(mPlugin.GetParam(kKnobBBypass)->Value() > 0.5);
          crushControl->bypassBtn->onToggle = [this](bool bypassed) {
            mPlugin.SendParameterValueFromUI(kKnobBBypass,
              mPlugin.GetParam(kKnobBBypass)->ToNormalized(bypassed ? 1.0 : 0.0));
          };
          crushControl->bypassBtn->addEventListener("mousedown", [this](glint_event& e) {
            auto& me = static_cast<glint_mouse_event&>(e);
            if (me.button != 2) return;
            e.stopPropagation();
            mPlugin.ShowParamContextMenu(kKnobBBypass, me.clientX, me.clientY);
          });
          mKnobBBypassBtn = crushControl->bypassBtn;
        });

        content.add.div([this](glint_component_style& ioKnobs) {
          ioKnobs.classList.add("controlSection_ioKnobs");

          ioKnobs.add.div([](glint_component_style& label) {
            label.style.textAlign = "center";
            label.innerText = "VOLUME";
            label.style.color = "#c5c5c5";
            label.style.fontSize = 10.f;
            label.style.fontFamily = "Roboto";
            label.style.fontWeight= 600.f;
            label.style.userSelect = "none";
            label.style.width = "96%";
            label.style.height = 10.f;
            label.style.position = "absolute";
            label.style.top = 4.f;
          });

          mInputKnob = gp_glint::addIoLevelKnob(
            ioKnobs, "input_volume_knob", "IN", 0.f,
            static_cast<float>(mPlugin.GetParam(kInputVolume)->GetMin()),
            static_cast<float>(mPlugin.GetParam(kInputVolume)->GetMax()),
            static_cast<float>(mPlugin.GetParam(kInputVolume)->Value()),
            &mInputMeter);
          mInputKnob->onChange = [this](float value) {
            if (!mSyncingFromPlugin)
              mPlugin.SendParameterValueFromUI(kInputVolume,
                mPlugin.GetParam(kInputVolume)->ToNormalized(value));
          };
          mInputKnob->addEventListener("mousedown", [this](glint_event& e) {
            auto& me = static_cast<glint_mouse_event&>(e);
            if (me.button != 2) return;
            e.stopPropagation();
            mPlugin.ShowParamContextMenu(kInputVolume, me.clientX, me.clientY);
          });

          mOutputKnob = gp_glint::addIoLevelKnob(
            ioKnobs, "output_volume_knob", "OUT", 0.f,
            static_cast<float>(mPlugin.GetParam(kOutputVolume)->GetMin()),
            static_cast<float>(mPlugin.GetParam(kOutputVolume)->GetMax()),
            static_cast<float>(mPlugin.GetParam(kOutputVolume)->Value()),
            &mOutputMeter);
          mOutputKnob->onChange = [this](float value) {
            if (!mSyncingFromPlugin)
              mPlugin.SendParameterValueFromUI(kOutputVolume,
                mPlugin.GetParam(kOutputVolume)->ToNormalized(value));
          };
          mOutputKnob->addEventListener("mousedown", [this](glint_event& e) {
            auto& me = static_cast<glint_mouse_event&>(e);
            if (me.button != 2) return;
            e.stopPropagation();
            mPlugin.ShowParamContextMenu(kOutputVolume, me.clientX, me.clientY);
          });
        });
      });

      app.add.div([](glint_component_style& footer) {
        footer.classList.add("footer");
      });
    });

    const std::filesystem::path cssDirectory = gp_glint::stylesDir();
    const std::filesystem::path webAssetsDirectory = gp_glint::assetsDir();

    document.onRequest = [cssDirectory, webAssetsDirectory](glint_resource_request& request) {
      #if defined(GLINT_BUNDLE_DEEP) || defined(GLINT_BUNDLE_SHALLOW)
        static glint_bundle::glint_bundle_library sBundle;
        if (sBundle.dispatch(request))
            return;

        const std::string bundlePath = request.pathname.empty() ? request.url : request.pathname;
        request.error(404, "Bundled resource not found: " + bundlePath);
        return;
      #else

      const std::string& url = request.url;
      const bool isCss = url.size() >= 4 && url.compare(url.size() - 4, 4, ".css") == 0;

      if (isCss)
      {
        const auto path = cssDirectory / std::filesystem::path(request.pathname).filename();
        request.fromFile(path.string());
        return;
      }

      std::string pathname = request.pathname;
      while (!pathname.empty() && (pathname.front() == '/' || pathname.front() == '\\'))
        pathname.erase(pathname.begin());

      request.fromFile((webAssetsDirectory / pathname).string());
#endif
    };

    gp_glint::loadAppStyles(document);
  }

private:
  GP&                  mPlugin;
  std::function<void()> mRequestRedraw;
  bool                  mSyncingFromPlugin = false;
  glint_element*        mCanvas           = nullptr;
  glint_knob*           mKnobAKnob        = nullptr;
  glint_knob*           mKnobBKnob       = nullptr;
  glint_knob*           mInputKnob        = nullptr;
  glint_knob*           mOutputKnob       = nullptr;
  std::shared_ptr<GPVuMeter> mInputMeter;
  std::shared_ptr<GPVuMeter> mOutputMeter;
  bypass_led_btn*       mKnobABypassBtn = nullptr;
  bypass_led_btn*       mKnobBBypassBtn    = nullptr;
};

#endif