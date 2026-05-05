#include "GP.h"
#include "IPlug_include_in_plug_src.h"
#include "GPGlintEditor.hpp"
#include "GPGlintTopLevelHost.hpp"
#ifdef OS_WIN
// (no proprietary includes)
#endif

#if defined(OS_WIN) && defined(APP_API)
static bool UseTopLevelStandaloneTest()
{
  return gp_glint::standaloneTopLevelTestEnabled();
}
#endif


GP::GP(const iplug::InstanceInfo& info)
: iplug::Plugin(info, iplug::MakeConfig(kNumParams, kNumPresets))
{
    GetParam(kInputVolume)->InitDouble("Input Volume", 0., -24., 24., 0.1, "dB");
    GetParam(kOutputVolume)->InitDouble("Output Volume", 0., -24., 24., 0.1, "dB");
        
    GetParam(kKnobABypass)->InitBool("Pan Bypass", false);
    GetParam(kKnobAMix)->InitDouble("Pan", 0., -100., 100.0, 0.1, "%");

    GetParam(kKnobBBypass)->InitBool("Crush Bypass", false);
    GetParam(kKnobBMix)->InitDouble("Bit Crush", 0., 0., 100.0, 0.1, "%");
}

GP::~GP(){
};

#if IPLUG_EDITOR
    void* GP::OpenWindow(void* pParent)
    {
        #if defined(OS_WIN)
            #if defined(APP_API)
                if (UseTopLevelStandaloneTest())
                {
                    if (!mTopLevelGlintHost)
                    mTopLevelGlintHost = std::make_unique<GPGlintTopLevelHost>(*this);

                    void* view = mTopLevelGlintHost->Open(pParent);
                    if (view)
                    OnUIOpen();

                    return view;
                }
            #endif

            if (!mGlintEditor)
                mGlintEditor = std::make_unique<GPGlintEditorHost>(*this);

            void* view = mGlintEditor->Open(pParent);
            if (view)
            {
                #if defined(APP_API)
                    mGlintEditor->OnParentResized(PLUG_WIDTH, PLUG_HEIGHT);
                #endif
                mMeterTimer = std::unique_ptr<iplug::Timer>(
                    iplug::Timer::Create([this](iplug::Timer&) {
                        if (mGlintEditor)
                            mGlintEditor->SetLevelMeters(
                                mInPeakL.load(std::memory_order_relaxed),
                                mInPeakR.load(std::memory_order_relaxed),
                                mOutPeakL.load(std::memory_order_relaxed),
                                mOutPeakR.load(std::memory_order_relaxed));
                    }, 33)
                );
                OnUIOpen();
            }

            return view;
            #elif defined(OS_MAC)
                if (!mGlintEditor)
                    mGlintEditor = std::make_unique<GPGlintEditorHost>(*this);

                void* view = mGlintEditor->Open(pParent);
                if (view)
                {
                    mMeterTimer = std::unique_ptr<iplug::Timer>(
                        iplug::Timer::Create([this](iplug::Timer&) {
                            if (mGlintEditor)
                                mGlintEditor->SetLevelMeters(
                                    mInPeakL.load(std::memory_order_relaxed),
                                    mInPeakR.load(std::memory_order_relaxed),
                                    mOutPeakL.load(std::memory_order_relaxed),
                                    mOutPeakR.load(std::memory_order_relaxed));
                        }, 33)
                    );
                    OnUIOpen();
                }

                return view;
            #else
            return IEditorDelegate::OpenWindow(pParent);
        #endif
    }

    void GP::CloseWindow()
    {
        mMeterTimer.reset();
        #if defined(OS_WIN)
            #if defined(APP_API)
                if (mTopLevelGlintHost)
                    mTopLevelGlintHost->CloseFromParent();
            #endif

            if (mGlintEditor)
            {
                mGlintEditor->Close();
                mGlintEditor.reset();
            }
        #elif defined(OS_MAC)
            if (mGlintEditor)
            {
                mGlintEditor->Close();
                mGlintEditor.reset();
            }
        #endif
    IEditorDelegate::CloseWindow();
    }

    void GP::OnParentWindowResize(int width, int height)
    {
        #if defined(OS_WIN)
            if (mGlintEditor)
                mGlintEditor->OnParentResized(width, height);
        #elif defined(OS_MAC)
            if (mGlintEditor)
                mGlintEditor->OnParentResized(width, height);
        #endif
    }

    void GP::SetScreenScale(float scale)
    {
        // scale is the device pixel ratio reported by the host (e.g. 2.0 at 200%).
        // VST3/CLAP hosts treat getSize() as physical pixels, so we must report
        // PLUG_WIDTH*scale x PLUG_HEIGHT*scale to get the correctly-sized parent HWND.
        if (scale <= 0.f) scale = 1.f;
        const int physW = static_cast<int>(std::lround(static_cast<float>(PLUG_WIDTH)  * scale));
        const int physH = static_cast<int>(std::lround(static_cast<float>(PLUG_HEIGHT) * scale));
        SetEditorSize(physW, physH);
        // If the editor is already open, ask the host to resize the window too.
        // AU (IPlugAU) does not override EditorResize, so skip it for that target.
        #ifndef AU_API
        if (mGlintEditor)
            EditorResize(physW, physH);
        #endif
    }

    void GP::OnParamChangeUI(int paramIdx, iplug::EParamSource source)
    {
        iplug::Plugin::OnParamChangeUI(paramIdx, source);

        #if defined(OS_WIN)
            #if defined(APP_API)
                if (mTopLevelGlintHost)
                    mTopLevelGlintHost->SyncFromPlugin(paramIdx);
            #endif

            if (mGlintEditor)
                mGlintEditor->SyncFromPlugin(paramIdx);
        #elif defined(OS_MAC)
            if (mGlintEditor)
                mGlintEditor->SyncFromPlugin(paramIdx);
        #endif

        updateAllParams();
    }

    void GP::OnReset() {
        updateAllParams();
    }

    void GP::updateAllParams() {
        v_inputVolume  = static_cast<float>(GetParam(kInputVolume)->Value());
        v_outputVolume = static_cast<float>(GetParam(kOutputVolume)->Value());
        v_knobAMix     = static_cast<float>(GetParam(kKnobAMix)->Value());
        v_knobBMix     = static_cast<float>(GetParam(kKnobBMix)->Value());
        v_knobABypass  = GetParam(kKnobABypass)->Bool();
        v_knobBBypass  = GetParam(kKnobBBypass)->Bool();
    }

    void GP::ShowParamContextMenu(int paramIdx, float x, float y)
    {
#if defined(VST3_API) || defined(VST3C_API)
        using namespace Steinberg;
        using namespace Steinberg::Vst;

        IComponentHandler* handler = GetComponentHandler();
        IPlugView*         view    = GetView();
        if (!handler || !view) return;

        FUnknownPtr<IComponentHandler3> handler3(handler);
        if (!handler3) return;

        ParamID paramId = static_cast<ParamID>(paramIdx);
        Steinberg::Vst::IContextMenu* menu = handler3->createContextMenu(view, &paramId);
        if (!menu) return;

        menu->popup(static_cast<UCoord>(x), static_cast<UCoord>(y));
        menu->release();
#endif
    }
#endif

#if IPLUG_DSP
    void GP::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames)
    {
        const int nCh = NInChansConnected();
        const float inGain  = static_cast<float>(DB_TO_K(v_inputVolume));
        const float outGain = static_cast<float>(DB_TO_K(v_outputVolume));

        // Pan (Knob A): balance law, -100% = full left, +100% = full right
        const float pan   = v_knobAMix / 100.f;
        const float panGL = (v_knobABypass || pan <= 0.f) ? 1.f : (1.f - pan);
        const float panGR = (v_knobABypass || pan >= 0.f) ? 1.f : (1.f + pan);

        // Bit crush (Knob B): 0% = full resolution (24-bit), 100% = 1-bit
        const float drive      = v_knobBMix / 100.f;                  // 0.0 to 1.0
        const float crushBits  = 24.f - drive * 23.f;                 // 24 → 1 bits
        const float crushSteps = powf(2.f, crushBits - 1.f);

        float inPeakL = 0.f, inPeakR = 0.f, outPeakL = 0.f, outPeakR = 0.f;

        for (int i = 0; i < nFrames; i++) {
            float l = static_cast<float>(inputs[0][i]) * inGain;
            float r = (nCh > 1) ? static_cast<float>(inputs[1][i]) * inGain : l;
            inPeakL = std::max(inPeakL, fabsf(l));
            inPeakR = std::max(inPeakR, fabsf(r));

            // Pan
            l *= panGL;
            r *= panGR;

            // Bit crush
            if (!v_knobBBypass && drive > 0.f) {
                l = roundf(l * crushSteps) / crushSteps;
                r = roundf(r * crushSteps) / crushSteps;
            }

            const float outL = l * outGain;
            const float outR = r * outGain;
            outputs[0][i] = outL;
            if (nCh > 1) outputs[1][i] = outR;
            outPeakL = std::max(outPeakL, fabsf(outL));
            outPeakR = std::max(outPeakR, fabsf(outR));
        }

        mInPeakL.store(inPeakL,  std::memory_order_relaxed);
        mInPeakR.store(inPeakR,  std::memory_order_relaxed);
        mOutPeakL.store(outPeakL, std::memory_order_relaxed);
        mOutPeakR.store(outPeakR, std::memory_order_relaxed);
    }
#endif
