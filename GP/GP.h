#pragma once

#include <atomic>
#include <memory>

#include "IPlug_include_in_plug_hdr.h"



#define DB_TO_K(db)     (pow(10.0, (double)(db) / 20.0))
#define K_TO_DB(k)      (20.0 * log10(k))

const int kNumPresets = 1;

enum EParams
{
    kInputVolume = 0,
    kOutputVolume,
    kKnobABypass,
    kKnobAMix,
    kKnobBBypass,
    kKnobBMix,
    kNumParams
};

#if (defined(OS_WIN) || defined(OS_MAC)) && IPLUG_EDITOR
class GPGlintEditorHost;
#endif
#if defined(OS_WIN) && defined(APP_API)
class GPGlintTopLevelHost;
#endif

class GP final : public iplug::Plugin
{
public:


    float v_inputVolume = 0.f, v_outputVolume = 0.f;
    float v_knobAMix = 0.f, v_knobBMix = 0.f;
    bool  v_knobABypass = false, v_knobBBypass = false;

    // Atomic peaks written by the audio thread, read by the UI meter timer.
    std::atomic<float> mInPeakL{0.0f};
    std::atomic<float> mInPeakR{0.0f};
    std::atomic<float> mOutPeakL{0.0f};
    std::atomic<float> mOutPeakR{0.0f};

    GP(const iplug::InstanceInfo& info);
    ~GP();

    

#if IPLUG_EDITOR
    void* OpenWindow(void* pParent) override;
    void CloseWindow() override;
    void OnParentWindowResize(int width, int height) override;
    void SetScreenScale(float scale) override;
    void OnParamChangeUI(int paramIdx, iplug::EParamSource source = iplug::kUnknown) override;
    void OnReset() override;
    void updateAllParams();
    void ShowParamContextMenu(int paramIdx, float x, float y);
#endif

#if IPLUG_DSP // http://bit.ly/2S64BDd
    void ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames) override;
#endif

private:
    #if IPLUG_EDITOR
        #if defined(OS_WIN) || defined(OS_MAC)
            std::unique_ptr<GPGlintEditorHost> mGlintEditor;
        #endif
        #if defined(OS_WIN) && defined(APP_API)
            std::unique_ptr<GPGlintTopLevelHost> mTopLevelGlintHost;
        #endif
        std::unique_ptr<iplug::Timer> mMeterTimer;
    #endif
};
