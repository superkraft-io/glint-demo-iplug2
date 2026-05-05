#pragma once

/**
 * knob.hpp  —  glint_knob component  (user component, not part of the glint engine)
 *
 * A multi-layer rotary knob built from glint_element children:
 *
 *   Layer 1 — knob_face       : background image (absolute, 100 % × 100 %)
 *   Layer 2 — outer_glow      : masked outward glow halo behind the knob
 *   Layer 3 — knob_ridges     : Skia ridge ticks around the outer edge
 *   Layer 4 — knob_texture    : texture overlay  (mix-blend-mode: overlay, opacity 0.15)
 *   Layer 5 — inner_glow      : inset box-shadow ring  (#76c8ff)
 *   Layer 6 — value_indicator : Skia arc that renders the 270° value sweep
 *   Layer 7 — ticks           : Skia tick marks spanning ticksRadiusFrom → ticksRadiusTo
 *   Layer 8 — knob_value_input: editable numeric display (type=number, centered)
 *
 * Value range: [0, 1].
 * Interaction: vertical mouse drag (drag up = increase).  200 px = full range.
 * The numeric input also accepts direct keyboard entry and syncs back to value_.
 *
 * Arc geometry (Chrome / CSS convention):
 *   Start: 225° from North (= bottom-left, 7-o'clock position).
 *   End:   135° from North (= bottom-right, 5-o'clock position).
 *   Sweep: 270° clockwise.
 *   Track colour: #333333.  Value fill colour: #76c8ff.  Stroke width: 4 px.
 *
 * Usage:
 *   #include "source/app_ui/components/knob.hpp"
 *   // glint_builder.hpp (from glint.hpp) must be included first in the TU.
 *
 *   auto* k = new glint_knob();
 *   k->minValue = -100.f;
 *   k->maxValue = 100.f;
 *   k->value_   = 0.f;                         // initial value in [minValue, maxValue]
 *   k->onChange = [](float v) { ... };
 *   parent->addChild(k);
 */

// glint_builder.hpp brings in: glint_element, glint_document, glint_image,
// glint_input, glint_adder, ComponentAdd method bodies — one include is enough.
#include "glint/components/glint_builder.hpp"
#include "glint/utils/glint_utils.hpp"

#include <cmath>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
// SkCanvas::drawArc is declared in SkCanvas.h — no SkPath.h needed.

// ── Arc geometry constants ────────────────────────────────────────────────────
// Skia angle convention: 0° = East (right), increases clockwise.
// CSS angle convention:  0° = North (up),   increases clockwise.
// Conversion: skiaAngle = cssAngle − 90°
//
//   CSS 225° (bottom-left)  → Skia 135°   ← start
static constexpr float kKnobArcStartSkia  = 135.f;   // degrees, Skia coords
static constexpr float kKnobArcSweepTotal = 270.f;   // degrees, clockwise

// ── Private: ridge-tick child element ───────────────────────────────────────

class _glint_knob_ridges : public glint_element
{
public:
    const char* typeName() const override { return "div"; }

    void drawContent(glint_canvas& g) override
    {
        if (auto* canvas = static_cast<SkCanvas*>(g.GetDrawContext()))
            _drawRidges(canvas);
        (void)g;
    }

    void DrawContentToCanvas(SkCanvas* canvas) override { _drawRidges(canvas); }

private:
    static constexpr float kTickSpacingDegrees = 4.f;
    static constexpr float kTickLength = 3.f;
    static constexpr float kShadowTickOffsetDegrees = 1.f;

    static SkPoint _pointOnCircle(float centerX, float centerY, float radius, float angleDegrees)
    {
        const float angleRadians = angleDegrees * (3.14159265358979323846f / 180.f);
        return SkPoint::Make(
            centerX + radius * std::cos(angleRadians),
            centerY + radius * std::sin(angleRadians));
    }

    void _drawTick(SkCanvas* canvas, float centerX, float centerY, float innerRadius,
                   float outerRadius, float angleDegrees, SkColor color) const
    {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setColor(color);
        paint.setStrokeWidth(1.f);
        paint.setAntiAlias(true);

        const SkPoint start = _pointOnCircle(centerX, centerY, innerRadius, angleDegrees);
        const SkPoint end = _pointOnCircle(centerX, centerY, outerRadius, angleDegrees);
        canvas->drawLine(start.x(), start.y(), end.x(), end.y(), paint);
    }

    void _drawRidges(SkCanvas* canvas) const
    {
        if (!canvas) return;

        const float centerX = mPaintRECT.MW();
        const float centerY = mPaintRECT.MH();
        const float outerRadius = std::min(mPaintRECT.W(), mPaintRECT.H()) * 0.5f;
        const float innerRadius = outerRadius - kTickLength;
        if (innerRadius <= 0.f) return;

        const int tickCount = static_cast<int>(360.f / kTickSpacingDegrees);
        for (int tickIndex = 0; tickIndex < tickCount; ++tickIndex)
        {
            const float angle = kTickSpacingDegrees * static_cast<float>(tickIndex);
            _drawTick(canvas, centerX, centerY, innerRadius, outerRadius, angle,
                      SkColorSetRGB(0xFF, 0xFF, 0xFF));
            _drawTick(canvas, centerX, centerY, innerRadius, outerRadius,
                      angle + kShadowTickOffsetDegrees,
                      SkColorSetRGB(0x00, 0x00, 0x00));
        }
    }
};

struct glint_knob_tick
{
    glint_color color = glint_color(255, 128, 128, 128);
    float thickness = 1.f;
    float length = -1.f;
};

// ── Private: progress-tick child element ───────────────────────────────────

class _glint_knob_ticks : public glint_element
{
public:
    const std::vector<glint_knob_tick>* mTicks = nullptr;
    const float* mRadiusFrom = nullptr;
    const float* mRadiusTo = nullptr;

    const char* typeName() const override { return "div"; }

    void drawContent(glint_canvas& g) override
    {
        if (auto* canvas = static_cast<SkCanvas*>(g.GetDrawContext()))
            _drawTicks(canvas);
        (void)g;
    }

    void DrawContentToCanvas(SkCanvas* canvas) override { _drawTicks(canvas); }

private:
    static SkPoint _pointOnCircle(float centerX, float centerY, float radius, float angleDegrees)
    {
        const float angleRadians = angleDegrees * (3.14159265358979323846f / 180.f);
        return SkPoint::Make(
            centerX + radius * std::cos(angleRadians),
            centerY + radius * std::sin(angleRadians));
    }

    static SkColor _toSkColor(const glint_color& color)
    {
        return SkColorSetARGB(color.A, color.R, color.G, color.B);
    }

    void _drawTicks(SkCanvas* canvas) const
    {
        if (!canvas || !mTicks || mTicks->size() < 2) return;

        const float centerX = mPaintRECT.MW();
        const float centerY = mPaintRECT.MH();
        const float edgeRadius = std::min(mPaintRECT.W(), mPaintRECT.H()) * 0.5f;
        const float radiusFromOffset = mRadiusFrom ? *mRadiusFrom : -12.f;
        const float radiusToOffset = mRadiusTo ? *mRadiusTo : -6.f;
        const float radiusFrom = std::max(0.f, edgeRadius + radiusFromOffset);
        const float radiusTo = std::max(0.f, edgeRadius + radiusToOffset);
        const float defaultTickLength = std::fabs(radiusTo - radiusFrom);
        const float tickDirection = radiusTo >= radiusFrom ? 1.f : -1.f;
        const float tickCount = static_cast<float>(mTicks->size() - 1);
        const float degreesPerTick = kKnobArcSweepTotal / tickCount;

        for (std::size_t tickIndex = 0; tickIndex < mTicks->size(); ++tickIndex)
        {
            const glint_knob_tick& tick = (*mTicks)[tickIndex];
            const float angle = kKnobArcStartSkia
                + degreesPerTick * static_cast<float>(tickIndex);
            const float tickLength = tick.length >= 0.f ? tick.length : defaultTickLength;
            const float endRadius = std::max(0.f, radiusFrom + tickDirection * tickLength);
            const SkPoint start = _pointOnCircle(centerX, centerY, radiusFrom, angle);
            const SkPoint end = _pointOnCircle(centerX, centerY, endRadius, angle);

            SkPaint paint;
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setColor(_toSkColor(tick.color));
            paint.setStrokeWidth(tick.thickness);
            paint.setAntiAlias(true);
            canvas->drawLine(start.x(), start.y(), end.x(), end.y(), paint);
        }
    }
};

// ── Private: arc-drawing child element ───────────────────────────────────────
// Declared before glint_knob so the parent can hold a typed pointer.
// This is not part of the public API — it is an implementation detail.

class _glint_knob_value_indicator : public glint_element
{
public:
    const float* mValue = nullptr;   // points into parent's value_ field after constructor setup
    const float* mMinValue = nullptr;
    const float* mMaxValue = nullptr;
    const glint_color* mTrackColor = nullptr;
    const glint_color* mValueColor = nullptr;
    const float* mRadiusOffset = nullptr; // points into parent's arcRadiusOffset
    const float* mArcThickness = nullptr; // points into parent's arcThickness

    const char* typeName() const override { return "div"; }

    // ── glint_canvas draw path (normal render) ──────────────────────────────────
    void drawContent(glint_canvas& g) override
    {
        if (auto* canvas = static_cast<SkCanvas*>(g.GetDrawContext()))
            _drawArc(canvas);
        (void)g;
    }

    // ── Skia-direct draw path (inspector / offscreen) ─────────────────────────
    void DrawContentToCanvas(SkCanvas* canvas) override { _drawArc(canvas); }

private:
    void _drawArc(SkCanvas* canvas) const
    {
        if (!mValue || !canvas) return;

        const float cx = mPaintRECT.MW();
        const float cy = mPaintRECT.MH();
        // 0 = stroke sits on the knob's inside edge, positive moves outward,
        // negative moves inward toward the center.
        const float radiusOffset = mRadiusOffset ? *mRadiusOffset : -6.f;
        const float arcThickness = mArcThickness ? *mArcThickness : 4.f;
        const float r = std::min(mPaintRECT.W(), mPaintRECT.H()) * 0.5f
            - arcThickness * 0.5f + radiusOffset;
        if (r < 2.f) return;

        const SkRect arcOval = SkRect::MakeXYWH(cx - r, cy - r, r * 2.f, r * 2.f);

        // ── Track: full 270° sweep in dark grey ──────────────────────────────
        {
            SkPaint p;
            p.setStyle(SkPaint::kStroke_Style);
            const glint_color trackColor = mTrackColor
                ? *mTrackColor
                : glint_color(255, 0x33, 0x33, 0x33);
            p.setColor(SkColorSetARGB(trackColor.A, trackColor.R, trackColor.G, trackColor.B));
            p.setStrokeWidth(arcThickness);
            p.setAntiAlias(true);
            p.setStrokeCap(SkPaint::kRound_Cap);
            canvas->drawArc(arcOval, kKnobArcStartSkia, kKnobArcSweepTotal, false, p);
        }

        // ── Value fill: accent blue, proportional to the current range value ─
        const float minValue = mMinValue ? *mMinValue : 0.f;
        const float maxValue = mMaxValue ? *mMaxValue : 1.f;
        const float lowerBound = std::min(minValue, maxValue);
        const float upperBound = std::max(minValue, maxValue);
        const float span = upperBound - lowerBound;
        const float normalizedValue = span > 0.f
            ? std::max(0.f, std::min(1.f, (*mValue - lowerBound) / span))
            : 0.f;
        const bool spansZero = lowerBound < 0.f && upperBound > 0.f;
        const float baselineNormalized = spansZero
            ? std::max(0.f, std::min(1.f, (0.f - lowerBound) / span))
            : 0.f;
        const float valueStart = kKnobArcStartSkia + baselineNormalized * kKnobArcSweepTotal;
        const float valueSweep = (normalizedValue - baselineNormalized) * kKnobArcSweepTotal;
        if (std::fabs(valueSweep) > 0.001f)
        {
            SkPaint p;
            p.setStyle(SkPaint::kStroke_Style);
            const glint_color valueColor = mValueColor
                ? *mValueColor
                : glint_color(255, 0x76, 0xC8, 0xFF);
            p.setColor(SkColorSetARGB(valueColor.A, valueColor.R, valueColor.G, valueColor.B));
            p.setStrokeWidth(arcThickness);
            p.setAntiAlias(true);
            p.setStrokeCap(SkPaint::kRound_Cap);
            canvas->drawArc(arcOval, valueStart, valueSweep, false, p);
        }
    }
};

// ── glint_knob ─────────────────────────────────────────────────────────────────

class glint_knob : public glint_element
{
public:
    glint_image*   knobFace      = nullptr;
    glint_image*   mKnobTexture_ = nullptr;
    glint_element* mOuterGlow_wrapper = nullptr;
    glint_element* mOuterGlow = nullptr;
    glint_element* mInnerGlow = nullptr;
    glint_input*   mInput_    = nullptr;
    _glint_knob_value_indicator* mIndicator_    = nullptr;
    _glint_knob_ridges*           ridges        = nullptr;
    glint_element*             mArrowContainer_ = nullptr;
    glint_element*             arrow = nullptr;

    void addKnobTextureClass(const std::string& cls)
    {
        _appendClassNameToken(mPendingKnobTextureClasses, cls);
        if (mKnobTexture_) mKnobTexture_->classList.add(cls);
    }

    struct value_text_formatter
    {
        glint_knob* owner = nullptr;
        std::function<std::string(float)> fn;

        value_text_formatter& operator=(std::function<std::string(float)> value)
        {
            fn = std::move(value);
            if (owner) owner->_onValueTextHooksChanged();
            return *this;
        }

        explicit operator bool() const
        {
            return static_cast<bool>(fn);
        }

        std::string operator()(float value) const
        {
            return fn(value);
        }
    };

    struct value_text_parser
    {
        glint_knob* owner = nullptr;
        std::function<bool(const std::string&, float&)> fn;

        value_text_parser& operator=(std::function<bool(const std::string&, float&)> value)
        {
            fn = std::move(value);
            if (owner) owner->_onValueTextHooksChanged();
            return *this;
        }

        explicit operator bool() const
        {
            return static_cast<bool>(fn);
        }

        bool operator()(const std::string& text, float& outValue) const
        {
            return fn(text, outValue);
        }
    };

    // ── Public API ────────────────────────────────────────────────────────────

    /** Current value in [minValue, maxValue]. */
    float value_ = 0.f;

    /** Minimum allowed value for the knob. */
    float minValue = 0.f;

    /** Maximum allowed value for the knob. */
    float maxValue = 1.f;

    /** Value restored by knob double-click. */
    float defaultValue = 0.f;

    /** Formats the centered input text for the current knob value. */
    value_text_formatter formatValueText;

    /**
     * Parses user-entered text into a knob value.
     * Return true and write to outValue when parsing succeeds.
     */
    value_text_parser parseValueText;

    /**
     * Radial offset for the arc value indicator, in pixels.
     * 0   = the stroke sits exactly on the knob's inside edge.
     * > 0 = moves outward beyond the knob circle.
     * < 0 = moves inward toward the center.
     * Default -6 preserves the current visual placement.
     */
    float arcRadiusOffset = -6.f;

    /** Stroke width for the arc value indicator, in pixels. Default 4. */
    float arcThickness = 4.f;

    /** Stroke color for the full track arc. Default #333333. */
    glint_color arcTrackColor = glint_color(255, 0x33, 0x33, 0x33);

    /** Stroke color for the value indicator arc. Default #76c8ff. */
    glint_color arcValueColor = glint_color(255, 0x76, 0xC8, 0xFF);

    void setArcTrackColor(const glint_color& color)
    {
        if (_sameColor(arcTrackColor, color)) return;
        arcTrackColor = color;
        _onArcAppearanceChanged();
    }

    void setArcTrackColor(const std::string& cssColor)
    {
        setArcTrackColor(sk_color(cssColor.c_str()).value);
    }

    void setArcValueColor(const glint_color& color)
    {
        if (_sameColor(arcValueColor, color)) return;
        arcValueColor = color;
        _onArcAppearanceChanged();
    }

    void setArcValueColor(const std::string& cssColor)
    {
        setArcValueColor(sk_color(cssColor.c_str()).value);
    }

    /** Tick marks distributed across the knob arc sweep. */
    std::vector<glint_knob_tick> ticks = std::vector<glint_knob_tick>(11);

    /**
     * Tick start offset in pixels relative to the knob edge.
     * 0   = exactly on the knob edge.
     * > 0 = extends outward beyond the knob edge.
     * < 0 = moves inward toward the center.
     */
    float ticksRadiusFrom = -12.f;

    /**
     * Tick end offset in pixels relative to the knob edge.
     * 0   = exactly on the knob edge.
     * > 0 = extends outward beyond the knob edge.
     * < 0 = moves inward toward the center.
     */
    float ticksRadiusTo = -6.f;

    /** Called whenever value_ changes (drag or input entry). */
    std::function<void(float)> onChange;

    /**
     * Programmatically update the knob value using the current range.
     * This applies visuals immediately instead of waiting for a draw-time sync.
     */
    void setValue(float nextValue, bool syncInput = true)
    {
        const float clampedValue = _clampToRange(nextValue);
        const bool changed = clampedValue != value_;

        value_ = clampedValue;
        if (syncInput) _syncInput();
        _syncVisuals();
        if (changed && onChange) onChange(value_);
        setDirty(false);
    }

    glint_knob()
    {
        classList.add("glint_knob");

        formatValueText.owner = this;
        parseValueText.owner = this;

        for (auto& tick : ticks)
        {
            tick.color = glint_color(255, 128, 128, 128);
            tick.thickness = 1.f;
            tick.length = 6.f;
        }

        if (!ticks.empty())
        {
            ticks.front().length = 10.f;
            ticks.back().length = 10.f;

            const std::size_t midIndex = ticks.size() / 2;
            ticks[midIndex].length = 10.f;
            ticks[midIndex].color = glint_color(255, 255, 255, 255);
        }

        if (style.width.raw.empty()) style.width = 140.f;
        if (style.height.raw.empty()) style.height = 140.f;
        style.position = "relative";   // establish a positioned containing block
        align          = "center middle";

        // ── Layer 1: outer glow halo behind the knob ──────────────────────
        mOuterGlow_wrapper = add.div([this](glint_component_style& _c) {
            _c.className = "outer_glow";
            _c.align = "center middle";
            _c.style.position      = "absolute";
            _c.style.pointerEvents = "none";
            _c.style.zIndex        = -1;

            _c.add.div([this](glint_component_style& glow) {
                glow.className = "outer_glow_glow";
                glow.style.pointerEvents = "none";
            }, &mOuterGlow);
        });

        // ── Layer 2: knob face image ─────────────────────────────────────────
        knobFace = add.image([this](glint_image& _c) {
            _c.className = "knob_face";
            _c.style.position = "absolute";
            _c.style.width    = "100%";
            _c.style.height   = "100%";
            _c.src            = "/img/knob_face.png";
        });

        

        // ── Layer 3: texture overlay (mix-blend-mode: overlay, opacity: 0.15) ─
        mKnobTexture_ = add.image([this](glint_image& _c) {
            _c.className = "knob_texture";
        });
        _applyPendingKnobTextureClasses();

        // ── Layer 4: ridge ticks around the knob edge ──────────────────────
        {
            ridges            = new _glint_knob_ridges();
            ridges->className = "knob_ridges";
            ridges->style.position  = "absolute";
            ridges->style.width     = "100%";
            ridges->style.height    = "100%";
            ridges->style.pointerEvents = "none";
            ridges->style.mixBlendMode  = "luminosity";
            ridges->style.filter        = "contrast(0.3)";
            addChild(ridges);
        }


        // ── Layer 5: inner glow (CSS inset box-shadow) ───────────────────────
        mInnerGlow = add.div([](glint_component_style& _c) {
            _c.className = "inner_glow";
            _c.style.position      = "absolute";
            _c.style.width         = "100%";
            _c.style.height        = "100%";
            _c.style.pointerEvents = "none";
        });

        // Container for the triangle pointer and spacer divs.  This is needed to
        mArrowContainer_ = add.div([this](glint_component_style& _c) {
            _c.className = "arrow-container";
            _c.align     = "center middle";

            //triangle
            _c.add.div([](glint_component_style& triangle) {
                triangle.className = "arrow-left";
            }, &arrow);
            
            //spacer
            _c.add.div([](glint_component_style& _c) {
                _c.style.width = "100%";
            });
        });

        
        // ── Layer 6: arc value indicator (custom Skia drawContent) ───────────
        {
            auto* ind            = new _glint_knob_value_indicator();
            ind->mValue          = &value_;
            ind->mMinValue       = &minValue;
            ind->mMaxValue       = &maxValue;
            ind->mTrackColor     = &arcTrackColor;
            ind->mValueColor     = &arcValueColor;
            ind->mRadiusOffset   = &arcRadiusOffset;
            ind->mArcThickness   = &arcThickness;
            ind->className       = "value_indicator";
            ind->style.position      = "absolute";
            ind->style.width         = "100%";
            ind->style.height        = "100%";
            ind->style.pointerEvents = "none";
            addChild(ind);
            mIndicator_ = ind;
        }

        // ── Layer 7: progress ticks ─────────────────────────────────────────
        {
            auto* tickLayer          = new _glint_knob_ticks();
            tickLayer->mTicks        = &ticks;
            tickLayer->mRadiusFrom   = &ticksRadiusFrom;
            tickLayer->mRadiusTo     = &ticksRadiusTo;
            tickLayer->className     = "ticks";
            tickLayer->style.position      = "absolute";
            tickLayer->style.width         = "100%";
            tickLayer->style.height        = "100%";
            tickLayer->style.pointerEvents = "none";
            addChild(tickLayer);
        }

        // ── Layer 8: numeric value input (centered horizontally near bottom) ─
        // position:absolute so the builder will NOT inject a top value.
        // Horizontal centering and vertical placement are driven by knob.css.
        mInput_ = add.input([this](glint_input& _c) {
            _c.className      = "knob_value_input";
            _c.type           = _usesCustomText() ? "text" : "number";
            _c.min            = _rangeMin();
            _c.max            = _rangeMax();
            _c.setValue(_formatValueText(value_));
            // Sync value_ whenever the user edits the input field.
            _c.onChange = [this](const std::string& v) {
                float parsedValue = 0.f;
                if (_tryParseValueText(v, parsedValue))
                    _setValue(parsedValue, false);
            };
            _c.onSubmit = [this](const std::string& v) {
                float parsedValue = 0.f;
                if (_tryParseValueText(v, parsedValue)) _setValue(parsedValue);
                else _syncInput();
            };
            _c.onBlur = [this]() {
                _syncInput();
            };
        });

        addEventListener("wheel", [this](glint_event& e) {
            auto& wheel = static_cast<glint_wheel_event&>(e);
            wheel.preventDefault();

            const float baseStep = wheel.shiftKey ? _fineStepSize() : _stepSize();
            const float wheelUnits = wheel.deltaY / kWheelDeltaUnit;
            if (std::fabs(wheelUnits) <= 0.0001f) return;

            _setValue(value_ + wheelUnits * baseStep);
            e.stopPropagation();
        });

        addEventListener("dblclick", [this](glint_event& e) {
            _setValue(defaultValue);
            e.preventDefault();
            e.stopPropagation();
        });

        _setValue(value_);
    }

    // ── Mouse interaction: vertical drag ─────────────────────────────────────

    /**
     * Update value_ while the mouse is dragged.
     * Upward motion (negative dY) increases the value.
     * Shift applies a finer drag range without causing a mid-drag jump.
     */
    void OnMouseDrag(float /*x*/, float /*y*/, float /*dX*/, float dY,
                     const glint_mouse_mod& mod) override
    {
        const float stepPerPixel = mod.S ? _fineStepSize() : _stepSize();
        _setValue(value_ - dY * stepPerPixel);
    }

    void Draw(glint_canvas& g) override
    {
        _syncFromPublicState();
        glint_element::Draw(g);
    }

private:
    // ── Private fields ────────────────────────────────────────────────────────
    std::string                  mPendingKnobTextureClasses;
    bool                         mSyncInitialized = false;
    float                        mLastSyncedValue = 0.f;
    float                        mLastSyncedMinValue = 0.f;
    float                        mLastSyncedMaxValue = 1.f;
    float                        mLastSyncedArcRadiusOffset = -6.f;
    float                        mLastSyncedArcThickness = 4.f;
    float                        mLastSyncedTicksRadiusFrom = -12.f;
    float                        mLastSyncedTicksRadiusTo = -6.f;
    glint_color                       mLastSyncedArcTrackColor = glint_color(255, 0x33, 0x33, 0x33);
    glint_color                       mLastSyncedArcValueColor = glint_color(255, 0x76, 0xC8, 0xFF);
    std::vector<glint_knob_tick>  mLastSyncedTicks;

    static void _appendClassNameToken(std::string& dst, const std::string& cls)
    {
        if (cls.empty()) return;
        std::istringstream existing(dst);
        std::string token;
        while (existing >> token)
            if (token == cls) return;
        if (!dst.empty()) dst += ' ';
        dst += cls;
    }

    void _applyPendingKnobTextureClasses()
    {
        if (!mKnobTexture_ || mPendingKnobTextureClasses.empty()) return;
        std::istringstream classes(mPendingKnobTextureClasses);
        std::string token;
        while (classes >> token)
            mKnobTexture_->classList.add(token);
    }

    static constexpr float kWheelStep = 0.05f;
    static constexpr float kFineWheelStep = 0.01f;
    static constexpr float kWheelDeltaUnit = 40.f;
    static constexpr float kDragPixelsPerRange = 200.f;
    static constexpr float kFineDragPixelsPerRange = 1000.f;

    bool _usesCustomText() const
    {
        return static_cast<bool>(formatValueText) || static_cast<bool>(parseValueText);
    }

    void _onValueTextHooksChanged()
    {
        if (mInput_) _syncInput();
        setDirty(false);
    }

    void _onArcAppearanceChanged()
    {
        setDirty(false);
    }

    float _rangeMin() const
    {
        return std::min(minValue, maxValue);
    }

    float _rangeMax() const
    {
        return std::max(minValue, maxValue);
    }

    float _rangeSpan() const
    {
        return _rangeMax() - _rangeMin();
    }

    float _normalizedValue() const
    {
        const float span = _rangeSpan();
        if (span <= 0.f) return 0.f;
        return (value_ - _rangeMin()) / span;
    }

    float _stepSize() const
    {
        return _rangeSpan() / kDragPixelsPerRange;
    }

    float _fineStepSize() const
    {
        return _rangeSpan() / kFineDragPixelsPerRange;
    }

    float _clampToRange(float value) const
    {
        return std::max(_rangeMin(), std::min(_rangeMax(), value));
    }

    std::string _formatValueText(float value) const
    {
        if (formatValueText) return formatValueText(value);
        return _fmt(value);
    }

    bool _tryParseValueText(const std::string& text, float& outValue) const
    {
        if (parseValueText) return parseValueText(text, outValue);

        if (text.empty()) return false;
        char* end = nullptr;
        float v = std::strtof(text.c_str(), &end);
        if (end == text.c_str() || *end != '\0') return false;
        outValue = v;
        return true;
    }

    static std::string _colorToCssString(const glint_color& color)
    {
        std::ostringstream oss;
        oss << "rgba(" << color.R << ',' << color.G << ',' << color.B << ','
            << std::fixed << std::setprecision(4)
            << (static_cast<float>(color.A) / 255.f) << ')';
        return oss.str();
    }

    static bool _sameColor(const glint_color& lhs, const glint_color& rhs)
    {
        return lhs.A == rhs.A && lhs.R == rhs.R && lhs.G == rhs.G && lhs.B == rhs.B;
    }

    bool _sameTicks() const
    {
        if (mLastSyncedTicks.size() != ticks.size()) return false;
        for (std::size_t tickIndex = 0; tickIndex < ticks.size(); ++tickIndex)
        {
            const auto& lhs = mLastSyncedTicks[tickIndex];
            const auto& rhs = ticks[tickIndex];
            if (!_sameColor(lhs.color, rhs.color)) return false;
            if (lhs.thickness != rhs.thickness) return false;
            if (lhs.length != rhs.length) return false;
        }
        return true;
    }

    bool _inputOwnsFocus() const
    {
        return mRoot && mInput_ && mRoot->getFocusedNode() == mInput_;
    }

    void _captureSyncedState()
    {
        mSyncInitialized = true;
        mLastSyncedValue = value_;
        mLastSyncedMinValue = minValue;
        mLastSyncedMaxValue = maxValue;
        mLastSyncedArcTrackColor = arcTrackColor;
        mLastSyncedArcValueColor = arcValueColor;
        mLastSyncedArcRadiusOffset = arcRadiusOffset;
        mLastSyncedArcThickness = arcThickness;
        mLastSyncedTicksRadiusFrom = ticksRadiusFrom;
        mLastSyncedTicksRadiusTo = ticksRadiusTo;
        mLastSyncedTicks = ticks;
    }

    void _syncFromPublicState()
    {
        const bool visualsChanged = !mSyncInitialized
            || mLastSyncedValue != value_
            || mLastSyncedMinValue != minValue
            || mLastSyncedMaxValue != maxValue
            || !_sameColor(mLastSyncedArcTrackColor, arcTrackColor)
            || !_sameColor(mLastSyncedArcValueColor, arcValueColor)
            || mLastSyncedArcRadiusOffset != arcRadiusOffset
            || mLastSyncedArcThickness != arcThickness
            || mLastSyncedTicksRadiusFrom != ticksRadiusFrom
            || mLastSyncedTicksRadiusTo != ticksRadiusTo
            || !_sameTicks();

        if (!visualsChanged) return;

        value_ = _clampToRange(value_);
        _syncVisuals();

        if (!_inputOwnsFocus())
            _syncInput();

        _captureSyncedState();

        // Public fields like value_/minValue/maxValue are commonly assigned
        // after construction. This sync path updates child transforms during
        // Draw(), so request another frame to ensure the new computed styles
        // are applied instead of leaving the constructor-time transform visible.
        setDirty(false);
    }

    void _syncVisuals()
    {
        const auto formatDegrees = [](float degrees) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.6g", degrees);
            return std::string(buf);
        };
        const auto syncGlowMask = [&](glint_element* glow) {
            if (!glow) return;

            float angle = glint_utils::map(value_, minValue, maxValue, 0.f, kKnobArcSweepTotal)
                - 135.f;
            if (value_ < 0.f)
            {
                glow->style.transform = "scaleX(-1)";
                angle = 0.f - angle;
            }
            else
            {
                glow->style.transform = "none";
            }

            glow->style.mask = "conic-gradient(black 0deg, black "
                + formatDegrees(angle) + "deg, transparent "
                + formatDegrees(angle) + "deg)";
        };
        
        float normalized = _normalizedValue();
        
        const std::string rotation = std::string("rotate(")
            + formatDegrees(normalized * kKnobArcSweepTotal) + "deg)";

        if (mKnobTexture_) mKnobTexture_->style.transform = rotation;
        if (ridges) ridges->style.transform = rotation;

        const std::string rotationArrow = std::string("rotate(")
            + formatDegrees(normalized * kKnobArcSweepTotal - 45.f) + "deg)";
        if (mArrowContainer_) mArrowContainer_->style.transform = rotationArrow;

        syncGlowMask(mOuterGlow_wrapper);
        syncGlowMask(mInnerGlow);


        _captureSyncedState();
    }

    void _setValue(float nextValue, bool syncInput = true)
    {
        const float clampedValue = _clampToRange(nextValue);
        const bool changed = clampedValue != value_;

        if (!changed && mSyncInitialized)
            return;

        value_ = clampedValue;
        if (syncInput) _syncInput();
        _syncVisuals();
        if (changed && onChange) onChange(value_);
        setDirty(false);
    }

    // Pushes the current value_ into the input field's text.
    void _syncInput()
    {
        if (!mInput_) return;
        mInput_->type = _usesCustomText() ? "text" : "number";
        mInput_->min  = _rangeMin();
        mInput_->max  = _rangeMax();
        mInput_->setValueWithoutRedraw(_formatValueText(value_));
    }

    // Formats a [0, 1] float as "0.00" … "1.00".
    static std::string _fmt(float f)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << f;
        return oss.str();
    }
};
