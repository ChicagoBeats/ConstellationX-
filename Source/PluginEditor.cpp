#include "PluginEditor.h"

namespace
{
const juce::Colour kBlack(0xff050507);
const juce::Colour kPanel(0xff0c0b0f);
const juce::Colour kPale(0xfff4edf0);
const juce::Colour kSteel(0xff8d8a93);

// Skin accents: Crimson (the approved default), Sapphire, Violet, Emerald, Amber.
const juce::Colour kSkins[5] = { juce::Colour(0xffff2638), juce::Colour(0xff2f8dff), juce::Colour(0xffb450ff),
                                 juce::Colour(0xff2ee86b), juce::Colour(0xffffb02e) };
const juce::Colour kGold(0xfff2b84a);

void drawCrown(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(r.getX(), r.getBottom());
    p.lineTo(r.getX(), r.getY() + r.getHeight() * 0.30f);
    p.lineTo(r.getX() + r.getWidth() * 0.25f, r.getY() + r.getHeight() * 0.62f);
    p.lineTo(r.getCentreX(), r.getY());
    p.lineTo(r.getX() + r.getWidth() * 0.75f, r.getY() + r.getHeight() * 0.62f);
    p.lineTo(r.getRight(), r.getY() + r.getHeight() * 0.30f);
    p.lineTo(r.getRight(), r.getBottom());
    p.closeSubPath();
    g.setColour(c);
    g.fillPath(p);
}

const juce::Colour kSilver(0xffdcdce4);

juce::Path chamfered(juce::Rectangle<float> r, float cut)
{
    juce::Path p;
    p.startNewSubPath(r.getX() + cut, r.getY());
    p.lineTo(r.getRight() - cut, r.getY());
    p.lineTo(r.getRight(), r.getY() + cut);
    p.lineTo(r.getRight(), r.getBottom() - cut);
    p.lineTo(r.getRight() - cut, r.getBottom());
    p.lineTo(r.getX() + cut, r.getBottom());
    p.lineTo(r.getX(), r.getBottom() - cut);
    p.lineTo(r.getX(), r.getY() + cut);
    p.closeSubPath();
    return p;
}

juce::ColourGradient chromeGradient(juce::Rectangle<float> b)
{
    juce::ColourGradient grad(juce::Colour(0xfff4f4f8), b.getX(), b.getY(),
                              juce::Colour(0xff55555d), b.getX(), b.getBottom(), false);
    grad.addColour(0.22, juce::Colour(0xff9494a0));
    grad.addColour(0.48, juce::Colour(0xff25252b));
    grad.addColour(0.74, juce::Colour(0xffa6a6b0));
    return grad;
}

void chromeStroke(juce::Graphics& g, const juce::Path& p, float thickness)
{
    g.setGradientFill(chromeGradient(p.getBounds()));
    g.strokePath(p, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void chromeFill(juce::Graphics& g, const juce::Path& p)
{
    g.setGradientFill(chromeGradient(p.getBounds()));
    g.fillPath(p);
}

// Red neon: three soft halo passes plus a bright core. w is the core-ish width.
void neonStroke(juce::Graphics& g, const juce::Path& p, juce::Colour c, float w, float intensity = 1.0f)
{
    const float widths[3] = { 2.6f, 1.8f, 1.25f };
    const float alphas[3] = { 0.10f, 0.18f, 0.30f };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour(c.withAlpha(alphas[i] * intensity));
        g.strokePath(p, juce::PathStrokeType(w * widths[i], juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour(c.brighter(0.35f).withAlpha(juce::jlimit(0.0f, 1.0f, 0.95f * intensity)));
    g.strokePath(p, juce::PathStrokeType(w * 0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void drawHexMesh(juce::Graphics& g, juce::Rectangle<float> area, float radius, juce::Colour colour)
{
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(area.toNearestInt());
    const float w = std::sqrt(3.0f) * radius, h = 1.5f * radius;
    juce::Path mesh;
    int row = 0;
    for (float y = area.getY() - radius; y < area.getBottom() + radius; y += h, ++row)
        for (float x = area.getX() - w + ((row & 1) ? w * 0.5f : 0.0f); x < area.getRight() + w; x += w)
            for (int k = 0; k < 6; ++k)
            {
                const float a = juce::MathConstants<float>::pi / 3.0f * (float) k + juce::MathConstants<float>::pi / 6.0f;
                const float px = x + std::cos(a) * radius * 0.9f, py = y + std::sin(a) * radius * 0.9f;
                if (k == 0) mesh.startNewSubPath(px, py); else mesh.lineTo(px, py);
                if (k == 5) mesh.closeSubPath();
            }
    g.setColour(colour);
    g.strokePath(mesh, juce::PathStrokeType(juce::jmax(0.6f, radius * 0.08f)));
}

// 2x2 twill weave tile, built at physical-pixel resolution.
juce::Image makeCarbonTile(int cell)
{
    cell = juce::jmax(3, cell);
    juce::Image tile(juce::Image::ARGB, cell * 2, cell * 2, true);
    juce::Graphics g(tile);
    for (int ty = 0; ty < 2; ++ty)
        for (int tx = 0; tx < 2; ++tx)
        {
            const juce::Rectangle<float> r((float) (tx * cell), (float) (ty * cell), (float) cell, (float) cell);
            const bool horizontal = ((tx + ty) & 1) == 0;
            juce::ColourGradient grad(juce::Colour(0xff09090c), r.getX(), r.getY(), juce::Colour(0xff09090c),
                                      horizontal ? r.getRight() : r.getX(), horizontal ? r.getY() : r.getBottom(), false);
            grad.addColour(0.5, horizontal ? juce::Colour(0xff26262e) : juce::Colour(0xff1b1b21));
            g.setGradientFill(grad);
            g.fillRect(r);
        }
    return tile;
}
} // namespace

// ============================== Look and feel ===============================

void ConstellationLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                                float pos, float startA, float endA, juce::Slider& s)
{
    const float R = ((float) juce::jmin(w, h) - 4.0f) * 0.5f;
    const auto c = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).getCentre();
    const float ang = startA + pos * (endA - startA);
    auto circle = [&](float r) { return juce::Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c); };

    // chrome bezel with a dark trough
    juce::Path bezel;
    bezel.addEllipse(circle(R * 0.98f));
    bezel.addEllipse(circle(R * 0.85f));
    bezel.setUsingNonZeroWinding(false);
    g.setGradientFill(chromeGradient(circle(R * 0.98f)));
    g.fillPath(bezel);
    g.setColour(juce::Colour(0xff020203));
    g.fillEllipse(circle(R * 0.85f));

    // red value arc with neon glow
    const float arcR = R * 0.74f;
    juce::Path track;
    track.addCentredArc(c.x, c.y, arcR, arcR, 0.0f, startA, endA, true);
    g.setColour(accent.withAlpha(0.18f));
    g.strokePath(track, juce::PathStrokeType(R * 0.08f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val;
    val.addCentredArc(c.x, c.y, arcR, arcR, 0.0f, startA, ang, true);
    neonStroke(g, val, accent, R * 0.08f);

    // glossy dome
    const auto dome = circle(R * 0.62f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff55545c), c.x - R * 0.25f, c.y - R * 0.35f,
                                           juce::Colour(0xff050506), c.x + R * 0.30f, c.y + R * 0.45f, true));
    g.fillEllipse(dome);
    g.setGradientFill(chromeGradient(dome));
    g.drawEllipse(dome, juce::jmax(1.0f, R * 0.018f));

    // pointer marker on the bezel
    juce::Path tri;
    tri.addTriangle(c.x, c.y - R * 0.80f, c.x - R * 0.045f, c.y - R * 0.95f, c.x + R * 0.045f, c.y - R * 0.95f);
    g.setColour(kPale);
    g.fillPath(tri, juce::AffineTransform::rotation(ang, c.x, c.y));

    g.setColour(kPale);
    g.setFont(juce::Font(R * 0.40f, juce::Font::bold));
    g.drawFittedText(s.getTextFromValue(s.getValue()), dome.reduced(R * 0.06f).toNearestInt(),
                     juce::Justification::centred, 1, 0.6f);
}

int ConstellationLookAndFeel::getSliderThumbRadius(juce::Slider& s)
{
    return (int) ((float) s.getHeight() * 0.38f);
}


void ConstellationLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                                                float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float tr = (float) getSliderThumbRadius(s);
    const float cy = (float) y + (float) h * 0.5f;
    const float th = (float) h * 0.22f;
    juce::Rectangle<float> track((float) x + tr * 0.3f, cy - th * 0.5f, (float) w - tr * 0.6f, th);

    juce::Path tp;
    tp.addRoundedRectangle(track, th * 0.5f);
    g.setColour(juce::Colour(0xff020203));
    g.fillPath(tp);
    chromeStroke(g, tp, juce::jmax(1.5f, th * 0.16f));

    const float centreX = track.getCentreX();
    const float x0 = juce::jmin(centreX, sliderPos), x1 = juce::jmax(centreX, sliderPos);
    g.setColour(accent.withAlpha(0.35f));
    g.fillRoundedRectangle(x0, track.getY() - th * 0.2f, x1 - x0, th * 1.4f, th * 0.7f);
    g.setColour(accent);
    g.fillRoundedRectangle(x0, track.getY() + th * 0.2f, x1 - x0, th * 0.6f, th * 0.3f);

    auto thumb = juce::Rectangle<float>(tr * 2.0f, tr * 2.0f).withCentre({ sliderPos, cy });
    g.setColour(accent.withAlpha(0.25f));
    g.fillEllipse(thumb.expanded(tr * 0.20f));
    g.setGradientFill(chromeGradient(thumb));
    g.fillEllipse(thumb);
    g.setColour(juce::Colour(0xff020203));
    g.fillEllipse(thumb.reduced(tr * 0.14f));
    auto dome = thumb.reduced(tr * 0.22f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4d4c54), dome.getCentreX() - tr * 0.25f, dome.getY(),
                                           juce::Colour(0xff050506), dome.getCentreX() + tr * 0.3f, dome.getBottom(), true));
    g.fillEllipse(dome);
    g.setColour(accent.withAlpha(0.9f));
    g.drawEllipse(dome, juce::jmax(1.2f, tr * 0.05f));
    drawCrown(g, juce::Rectangle<float>(tr * 0.95f, tr * 0.62f).withCentre(thumb.getCentre()), kSilver);
}

void ConstellationLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced(2.0f);
    const float rad = r.getHeight() * 0.5f;
    const bool on = b.getToggleState();
    g.setColour(juce::Colour(0xff040405));
    g.fillRoundedRectangle(r, rad);
    g.setColour(accent.withAlpha(highlighted ? 0.9f : 0.65f));
    g.drawRoundedRectangle(r, rad, 2.0f);

    auto left = r.withWidth(r.getWidth() * 0.5f).reduced(3.0f);
    auto right = left.withX(r.getCentreX());
    g.setFont(juce::Font(r.getHeight() * 0.42f, juce::Font::bold));
    if (on) { g.setColour(accent); g.fillRoundedRectangle(left, left.getHeight() * 0.5f); }
    else    { g.setColour(juce::Colour(0xff2a292f)); g.fillRoundedRectangle(right, right.getHeight() * 0.5f); }
    g.setColour(on ? kPale : kSteel);
    g.drawText("ON", left.toNearestInt(), juce::Justification::centred);
    g.setColour(on ? kSteel : kPale);
    g.drawText("OFF", right.toNearestInt(), juce::Justification::centred);
}


void ConstellationLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                                    bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.5f);
    const float rad = juce::jmin(r.getHeight() * 0.22f, 14.0f);
    juce::Path p;
    p.addRoundedRectangle(r, rad);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2a31), r.getX(), r.getY(),
                                           juce::Colour(0xff060607), r.getX(), r.getBottom(), false));
    g.fillPath(p);
    if (b.getToggleState())
    {
        g.setColour(accent.withAlpha(0.80f));
        g.fillPath(p);
    }
    else if (down || highlighted)
    {
        g.setColour(accent.withAlpha(down ? 0.55f : 0.25f));
        g.fillPath(p);
    }
    chromeStroke(g, p, 2.0f);
    g.setColour(accent.withAlpha(b.isEnabled() ? 0.45f : 0.15f));
    g.drawRoundedRectangle(r.reduced(3.5f), rad * 0.7f, 1.0f);
}

void ConstellationLookAndFeel::drawCornerResizer(juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    const float fw = (float) w, fh = (float) h;
    g.setColour(accent.withAlpha(isMouseDragging ? 1.0f : (isMouseOver ? 0.95f : 0.75f)));
    const float thickness = juce::jmax(1.5f, juce::jmin(fw, fh) * 0.07f);
    for (float f = 0.30f; f < 0.95f; f += 0.30f)
        g.drawLine(fw * f, fh, fw, fh * f, thickness);
}

juce::Font ConstellationLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font((float) buttonHeight * 0.42f, juce::Font::bold);
}

// ================================== Editor ==================================
ConstellationXVocalAudioProcessorEditor::ConstellationXVocalAudioProcessorEditor(
    ConstellationXVocalAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);

    styleKnob(depth, "depth");
    styleKnob(size, "size");
    styleKnob(decay, "decay");
    styleKnob(reflections, "reflections");
    styleKnob(width, "width");
    styleKnob(warmth, "warmth");
    styleKnob(air, "air");
    styleKnob(mix, "mix");
    styleKnob(output, "output");
    depth.setSliderStyle(juce::Slider::LinearHorizontal);

    motion.setColour(juce::ToggleButton::textColourId, kPale);
    addAndMakeVisible(motion);
    motionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "vfxMotion", motion);

    randomize.setColour(juce::TextButton::textColourOffId, kPale);
    randomize.setColour(juce::TextButton::textColourOnId, kPale);
    randomize.onClick = [this] { processor.randomize(); refreshButtons(); repaint(); };
    addAndMakeVisible(randomize);

    for (auto* b : { &saveBtn, &presetPrev, &presetNext, &slotA, &slotB, &undoTop, &redoTop,
                     &presetsBtn, &favoritesBtn, &abBtn, &undoBtn, &redoBtn })
    {
        b->setColour(juce::TextButton::textColourOffId, kPale);
        b->setColour(juce::TextButton::textColourOnId, kPale);
        addAndMakeVisible(*b);
    }
    for (auto* b : { &slotA, &slotB })
        b->setClickingTogglesState(false);

    saveBtn.onClick = [this] { processor.saveUserPreset(); refreshButtons(); repaint(); };
    presetPrev.onClick = [this] { processor.stepPreset(-1); repaint(); };
    presetNext.onClick = [this] { processor.stepPreset(1); repaint(); };
    slotA.onClick = [this] { processor.selectSlot(0); refreshButtons(); repaint(); };
    slotB.onClick = [this] { processor.selectSlot(1); refreshButtons(); repaint(); };
    abBtn.onClick = [this] { processor.selectSlot(1 - processor.getActiveSlot()); refreshButtons(); repaint(); };
    undoTop.onClick = undoBtn.onClick = [this] { doUndo(); };
    redoTop.onClick = redoBtn.onClick = [this] { doRedo(); };
    presetsBtn.onClick = [this] { showPresetsMenu(presetsBtn); };
    favoritesBtn.onClick = [this] { showFavoritesMenu(favoritesBtn); };

    for (auto* b : { &skinPrev, &skinNext })
    {
        b->setColour(juce::TextButton::textColourOffId, kPale);
        addAndMakeVisible(*b);
    }
    skinPrev.onClick = [this] { setSkin((currentSkin() + 4) % 5); };
    skinNext.onClick = [this] { setSkin((currentSkin() + 1) % 5); };

    setResizable(true, false);   // host may resize; the grip is our own (bottom-right, red)
    setResizeLimits(836, 470, 1920, 1080);
    getConstrainer()->setFixedAspectRatio(1672.0 / 941.0);
    resizer = std::make_unique<juce::ResizableCornerComponent>(this, getConstrainer());
    addAndMakeVisible(*resizer);
    resizer->setAlwaysOnTop(true);
    setSize(1254, 706);
    refreshButtons();

    lastTimeMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(40);
}

ConstellationXVocalAudioProcessorEditor::~ConstellationXVocalAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ConstellationXVocalAudioProcessorEditor::styleKnob(juce::Slider& s, const char* paramId)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    s.setMouseDragSensitivity(220);
    s.onValueChange = [this, &s] { if (s.isMouseOverOrDragging()) markDirty(); };
    addAndMakeVisible(s);
    attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, paramId, s));
    if (auto* rp = processor.apvts.getParameter(paramId))
        s.setDoubleClickReturnValue(true, rp->convertFrom0to1(rp->getDefaultValue()));
}

int ConstellationXVocalAudioProcessorEditor::currentSkin() const
{
    return juce::jlimit(0, 4, juce::roundToInt(processor.apvts.getRawParameterValue("skin")->load()));
}

void ConstellationXVocalAudioProcessorEditor::setSkin(int index)
{
    if (auto* p = processor.apvts.getParameter("skin"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1((float) juce::jlimit(0, 4, index)));
        p->endChangeGesture();
    }
}

juce::Rectangle<float> ConstellationXVocalAudioProcessorEditor::R(float x, float y, float w, float h) const
{
    const float s = scale();
    return { x * s, y * s, w * s, h * s };
}

juce::Rectangle<float> ConstellationXVocalAudioProcessorEditor::swatchRect(int i) const
{
    static const float xs[5] = { 120.0f, 198.0f, 276.0f, 353.0f, 427.0f };
    return R(xs[juce::jlimit(0, 4, i)], 855.0f, 70.0f, 50.0f);
}

void ConstellationXVocalAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (presetBoxRect().contains(e.position))
    {
        showPresetsMenu(*this);
        return;
    }
    for (int i = 0; i < 5; ++i)
        if (swatchRect(i).contains(e.position))
        {
            setSkin(i);
            return;
        }
}

void ConstellationXVocalAudioProcessorEditor::resized()
{
    depth.setBounds(R(470.0f, 495.0f, 730.0f, 105.0f).toNearestInt());
    size.setBounds(R(197.0f, 478.0f, 140.0f, 140.0f).toNearestInt());
    decay.setBounds(R(1336.0f, 478.0f, 140.0f, 140.0f).toNearestInt());

    static const float kx[6] = { 445.0f, 603.0f, 763.0f, 920.0f, 1077.0f, 1230.0f };
    juce::Slider* knobs[6] = { &reflections, &width, &warmth, &air, &mix, &output };
    for (int i = 0; i < 6; ++i)
        knobs[i]->setBounds(R(kx[i] - 56.0f, 736.0f - 56.0f, 112.0f, 112.0f).toNearestInt());

    motion.setBounds(R(70.0f, 712.0f, 175.0f, 58.0f).toNearestInt());
    randomize.setBounds(R(1468.0f, 715.0f, 150.0f, 54.0f).toNearestInt());
    skinPrev.setBounds(R(60.0f, 858.0f, 44.0f, 44.0f).toNearestInt());
    skinNext.setBounds(R(528.0f, 858.0f, 44.0f, 44.0f).toNearestInt());

    // header bar
    saveBtn.setBounds(R(575.0f, 18.0f, 72.0f, 46.0f).toNearestInt());
    presetPrev.setBounds(R(982.0f, 18.0f, 46.0f, 46.0f).toNearestInt());
    presetNext.setBounds(R(1036.0f, 18.0f, 46.0f, 46.0f).toNearestInt());
    slotA.setBounds(R(1215.0f, 18.0f, 52.0f, 46.0f).toNearestInt());
    slotB.setBounds(R(1275.0f, 18.0f, 52.0f, 46.0f).toNearestInt());
    undoTop.setBounds(R(1350.0f, 18.0f, 82.0f, 46.0f).toNearestInt());
    redoTop.setBounds(R(1440.0f, 18.0f, 82.0f, 46.0f).toNearestInt());

    // bottom-right bar
    presetsBtn.setBounds(R(680.0f, 846.0f, 175.0f, 64.0f).toNearestInt());
    favoritesBtn.setBounds(R(868.0f, 846.0f, 175.0f, 64.0f).toNearestInt());
    abBtn.setBounds(R(1056.0f, 846.0f, 140.0f, 64.0f).toNearestInt());
    undoBtn.setBounds(R(1208.0f, 846.0f, 140.0f, 64.0f).toNearestInt());
    redoBtn.setBounds(R(1362.0f, 846.0f, 150.0f, 64.0f).toNearestInt());

    if (resizer != nullptr)
    {
        const int grip = juce::jmax(22, juce::roundToInt(44.0f * scale()));
        resizer->setBounds(getWidth() - grip, getHeight() - grip, grip, grip);
    }
}

juce::Rectangle<float> ConstellationXVocalAudioProcessorEditor::presetBoxRect() const
{
    return R(660.0f, 16.0f, 305.0f, 50.0f);
}

void ConstellationXVocalAudioProcessorEditor::markDirty()
{
    dirty = true;
    lastChangeMs = juce::Time::getMillisecondCounterHiRes();
}

void ConstellationXVocalAudioProcessorEditor::doUndo()
{
    dirty = false;
    processor.undo();
    refreshButtons();
    repaint();
}

void ConstellationXVocalAudioProcessorEditor::doRedo()
{
    dirty = false;
    processor.redo();
    refreshButtons();
    repaint();
}

void ConstellationXVocalAudioProcessorEditor::refreshButtons()
{
    slotA.setToggleState(processor.getActiveSlot() == 0, juce::dontSendNotification);
    slotB.setToggleState(processor.getActiveSlot() == 1, juce::dontSendNotification);
    for (auto* b : { &undoTop, &undoBtn }) b->setEnabled(processor.canUndo());
    for (auto* b : { &redoTop, &redoBtn }) b->setEnabled(processor.canRedo());
}

void ConstellationXVocalAudioProcessorEditor::showPresetsMenu(juce::Component& target)
{
    juce::PopupMenu menu, factory, user;
    const juce::String current = processor.getPresetName();
    for (int i = 0; i < cx::kNumFactoryPresets; ++i)
        factory.addItem(1000 + i, cx::kFactoryPresets[i].name, true, current == cx::kFactoryPresets[i].name);

    const juce::StringArray users = processor.getUserPresetNames();
    if (users.isEmpty())
        user.addItem(1, "(none saved yet)", false, false);
    for (int i = 0; i < users.size(); ++i)
        user.addItem(2000 + i, users[i], true, current == users[i]);

    menu.addSubMenu("Factory", factory);
    menu.addSubMenu("User", user);
    menu.addSeparator();
    menu.addItem(10, "Save current as new user preset");
    menu.addItem(11, "Open preset folder");

    juce::Component::SafePointer<ConstellationXVocalAudioProcessorEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&target),
                       [safe, users](int result)
                       {
                           if (safe == nullptr || result == 0) return;
                           auto& proc = safe->processor;
                           if (result >= 1000 && result < 2000) proc.loadFactoryPreset(result - 1000);
                           else if (result >= 2000 && result < 3000 && result - 2000 < users.size())
                               proc.loadPresetByName(users[result - 2000]);
                           else if (result == 10) proc.saveUserPreset();
                           else if (result == 11) ConstellationXVocalAudioProcessor::getPresetDirectory().revealToUser();
                           safe->refreshButtons();
                           safe->repaint();
                       });
}

void ConstellationXVocalAudioProcessorEditor::showFavoritesMenu(juce::Component& target)
{
    juce::PopupMenu menu;
    const juce::String current = processor.getPresetName();
    const juce::StringArray favs = processor.getFavorites();
    if (favs.isEmpty())
        menu.addItem(1, "(no favorites yet)", false, false);
    for (int i = 0; i < favs.size(); ++i)
        menu.addItem(3000 + i, favs[i], true, current == favs[i]);
    menu.addSeparator();
    const bool loadable = processor.getAllPresetNames().contains(current);
    const bool isFav = processor.isFavorite(current);
    menu.addItem(20, isFav ? "Remove \"" + current + "\" from favorites" : "Add \"" + current + "\" to favorites", loadable);

    juce::Component::SafePointer<ConstellationXVocalAudioProcessorEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&target),
                       [safe, favs, current, isFav](int result)
                       {
                           if (safe == nullptr || result == 0) return;
                           auto& proc = safe->processor;
                           if (result >= 3000 && result - 3000 < favs.size()) proc.loadPresetByName(favs[result - 3000]);
                           else if (result == 20) proc.setFavorite(current, !isFav);
                           safe->refreshButtons();
                           safe->repaint();
                       });
}

void ConstellationXVocalAudioProcessorEditor::drawLabel(juce::Graphics& g, const juce::String& text,
                                                         float cxRef, float cyRef, float fontRef,
                                                         juce::Colour c, bool bold)
{
    const float s = scale();
    g.setColour(c);
    g.setFont(juce::Font(fontRef * s, bold ? juce::Font::bold : juce::Font::plain));
    g.drawText(text, juce::Rectangle<float>(300.0f * s, 40.0f * s).withCentre({ cxRef * s, cyRef * s }).toNearestInt(),
               juce::Justification::centred);
}


void ConstellationXVocalAudioProcessorEditor::fillCarbon(juce::Graphics& g, const juce::Path& p)
{
    g.setFillType(juce::FillType(carbonTile, juce::AffineTransform::scale(1.0f / layerScale)));
    g.fillPath(p);
}

void ConstellationXVocalAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> r)
{
    const float s = scale();
    const auto accent = lnf.accent;
    const auto path = chamfered(r, 20.0f * s).createPathWithRoundedCorners(4.0f * s);

    fillCarbon(g, path);
    g.setColour(juce::Colour(0xd0070709));
    g.fillPath(path);

    {   // red floor glow along the bottom edge
        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(path);
        g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.0f), r.getCentreX(), r.getY() + r.getHeight() * 0.55f,
                                               accent.withAlpha(0.38f), r.getCentreX(), r.getBottom(), false));
        g.fillRect(r);
    }
    chromeStroke(g, path, 3.5f * s);
    neonStroke(g, chamfered(r.reduced(6.0f * s), 16.0f * s).createPathWithRoundedCorners(3.0f * s), accent, 1.1f * s, 0.55f);
}


// Static chrome housing, ring and bolts (cached in the background layer).
void ConstellationXVocalAudioProcessorEditor::drawSpeakerHousing(juce::Graphics& g, juce::Point<float> c, float R)
{
    auto circle = [&](float r) { return juce::Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c); };
    const float deg = juce::MathConstants<float>::pi / 180.0f;

    juce::Path oct;
    for (int k = 0; k < 8; ++k)
    {
        const float a = (22.5f + 45.0f * (float) k) * deg;
        const float px = c.x + std::cos(a) * R, py = c.y + std::sin(a) * R;
        if (k == 0) oct.startNewSubPath(px, py); else oct.lineTo(px, py);
    }
    oct.closeSubPath();
    const auto housing = oct.createPathWithRoundedCorners(R * 0.10f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a292f), c.x, c.y - R, juce::Colour(0xff060607),
                                           c.x, c.y + R, false));
    g.fillPath(housing);
    chromeStroke(g, housing, R * 0.045f);

    juce::Path ring;
    ring.addEllipse(circle(R * 0.80f));
    ring.addEllipse(circle(R * 0.73f));
    ring.setUsingNonZeroWinding(false);
    chromeFill(g, ring);
    g.setColour(juce::Colour(0xff040405));
    g.fillEllipse(circle(R * 0.73f));

    for (int k = 0; k < 8; ++k)
    {
        const float a = (22.5f + 45.0f * (float) k) * deg;
        const juce::Point<float> p(c.x + std::cos(a) * R * 0.90f, c.y + std::sin(a) * R * 0.90f);
        const float br = R * 0.028f;
        const auto bolt = juce::Rectangle<float>(br * 2.0f, br * 2.0f).withCentre(p);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe6e6ee), p.x - br * 0.4f, p.y - br * 0.4f,
                                               juce::Colour(0xff17171b), p.x + br, p.y + br, true));
        g.fillEllipse(bolt);
    }
}

// Live part: halo, neon segments, rubber surround, cone and dust cap, driven by the real sub-band level.
void ConstellationXVocalAudioProcessorEditor::drawSpeaker(juce::Graphics& g, juce::Point<float> c,
                                                           float R, float level)
{
    const auto accent = lnf.accent;
    auto circle = [&](float r) { return juce::Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c); };
    const float deg = juce::MathConstants<float>::pi / 180.0f;

    g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.12f + 0.30f * level), c.x, c.y,
                                           accent.withAlpha(0.0f), c.x + R * 1.15f, c.y, true));
    g.fillEllipse(circle(R * 1.15f));

    // neon segments on the housing face pulse with the low-frequency level
    for (int k = 0; k < 8; ++k)
    {
        juce::Path arc;
        const float a0 = (45.0f * (float) k + 8.0f) * deg;
        arc.addCentredArc(c.x, c.y, R * 0.87f, R * 0.87f, 0.0f, a0, a0 + 29.0f * deg, true);
        neonStroke(g, arc, accent, R * 0.030f, 0.45f + 0.55f * level);
    }

    // glossy rubber surround
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a393f), c.x - R * 0.3f, c.y - R * 0.4f,
                                           juce::Colour(0xff030304), c.x + R * 0.3f, c.y + R * 0.4f, false));
    g.fillEllipse(circle(R * 0.70f));
    g.setColour(accent.withAlpha(0.35f));
    g.drawEllipse(circle(R * 0.66f), juce::jmax(1.0f, R * 0.012f));

    // cone with fine concentric grooves; excursion follows the sub-band level
    const float cone = R * 0.58f * (1.0f + 0.07f * level);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2f2e35), c.x - cone * 0.4f, c.y - cone * 0.5f,
                                           juce::Colour(0xff030304), c.x + cone * 0.5f, c.y + cone * 0.6f, true));
    g.fillEllipse(circle(cone));
    g.setColour(juce::Colour(0xff8a8892).withAlpha(0.22f));
    for (int i = 1; i <= 14; ++i)
        g.drawEllipse(circle(cone * ((float) i / 15.0f)), juce::jmax(0.8f, R * 0.005f));
    juce::Path sheen;
    sheen.addCentredArc(c.x, c.y, cone * 0.78f, cone * 0.78f, 0.0f, -70.0f * deg, -20.0f * deg, true);
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    g.strokePath(sheen, juce::PathStrokeType(R * 0.03f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // dust cap with silver crown
    const float cap = R * 0.24f * (1.0f + 0.10f * level);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff5a5960), c.x - cap * 0.4f, c.y - cap * 0.5f,
                                           juce::Colour(0xff0a0a0c), c.x + cap * 0.6f, c.y + cap * 0.7f, true));
    g.fillEllipse(circle(cap));
    g.setGradientFill(chromeGradient(circle(cap)));
    g.drawEllipse(circle(cap), juce::jmax(1.0f, R * 0.016f));
    drawCrown(g, juce::Rectangle<float>(cap * 1.0f, cap * 0.64f).withCentre(c), kSilver);
}

void ConstellationXVocalAudioProcessorEditor::drawVortex(juce::Graphics& g, juce::Rectangle<float> r)
{
    const float s = scale();
    const auto accent = lnf.accent;
    const bool moving = motion.getToggleState();
    const float depthValue = depthVis;   // eased Depth: -1 Forward (vortex contracts) .. +1 Back (vortex expands)
    const float level = juce::jlimit(0.0f, 1.0f, inLevel * 2.5f);

    juce::Path clip;
    clip.addRoundedRectangle(r, 22.0f * s);
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(clip);

    g.setGradientFill(juce::ColourGradient(accent.darker(0.85f), r.getCentreX(), r.getCentreY(), kBlack, r.getX(),
                                           r.getY(), true));
    g.fillRect(r);

    // expanding / contracting: slow breathing + depth (Back opens the vortex) + live input level
    const float breathe = moving ? 0.07f * std::sin((float) phase * 0.9f) : 0.0f;
    const float opening = juce::jlimit(0.28f, 1.0f,
                                       0.34f + 0.52f * (0.5f * (depthValue + 1.0f)) + breathe + 0.20f * level);
    const float rx = r.getWidth() * 0.5f * opening, ry = r.getHeight() * 0.5f * opening;
    const auto c = r.getCentre();
    const float ph = (float) phase;

    for (int arm = 0; arm < 5; ++arm)
    {
        juce::Path path;
        for (int i = 0; i <= 150; ++i)
        {
            const float t = (float) i / 150.0f;
            const float a = t * 7.0f + ph + (float) arm * juce::MathConstants<float>::twoPi / 5.0f;
            const float k = 0.04f + t * 0.94f;
            const float x = c.x + std::cos(a) * rx * k, y = c.y + std::sin(a) * ry * k;
            if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
        }
        g.setColour(accent.withAlpha(0.16f + 0.10f * level));
        g.strokePath(path, juce::PathStrokeType(8.0f * s));
        g.setColour(accent.brighter(0.5f).withAlpha(0.65f));
        g.strokePath(path, juce::PathStrokeType(1.8f * s));
    }

    for (int i = 0; i < 110; ++i)
    {
        const float a = (float) i * 2.39996f + ph * (0.3f + (float) (i % 4) * 0.08f);
        const float k = std::sqrt(((float) i + 1.0f) / 110.0f);
        const float sz = (1.0f + (float) (i % 3) * 0.7f) * 1.6f * s;
        g.setColour((i % 7 == 0 ? kPale : accent).withAlpha(0.45f + 0.4f * (moving ? 1.0f : 0.0f)));
        g.fillEllipse(c.x + std::cos(a) * rx * k, c.y + std::sin(a) * ry * k, sz, sz);
    }

    g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.55f), c.x, c.y, accent.withAlpha(0.0f),
                                           c.x + 40.0f * s * (1.0f + level), c.y, true));
    g.fillEllipse(c.x - 40.0f * s, c.y - 40.0f * s, 80.0f * s, 80.0f * s);
    g.setColour(kPale.withAlpha(0.9f));
    g.fillEllipse(c.x - 3.0f * s, c.y - 3.0f * s, 6.0f * s, 6.0f * s);

    g.setColour(accent.withAlpha(0.85f));
    g.drawRoundedRectangle(r.reduced(1.0f), 22.0f * s, 3.0f * s);
}

void ConstellationXVocalAudioProcessorEditor::ensureLayers()
{
    if (bgImage.isNull() || layerW != getWidth() || layerH != getHeight() || layerAccent != lnf.accent)
        renderLayers();
}

void ConstellationXVocalAudioProcessorEditor::renderLayers()
{
    layerW = getWidth();
    layerH = getHeight();
    layerAccent = lnf.accent;
    layerScale = layerW > 1400 ? 1.0f : 2.0f;   // supersample small windows for crisp edges
    const int pw = juce::jmax(1, juce::roundToInt((float) layerW * layerScale));
    const int ph = juce::jmax(1, juce::roundToInt((float) layerH * layerScale));
    carbonTile = makeCarbonTile(juce::roundToInt(4.0f * scale() * layerScale));

    bgImage = juce::Image(juce::Image::ARGB, pw, ph, true);
    {
        juce::Graphics g(bgImage);
        g.addTransform(juce::AffineTransform::scale(layerScale));
        renderBackground(g);
    }
    fgImage = juce::Image(juce::Image::ARGB, pw, ph, true);
    {
        juce::Graphics g(fgImage);
        g.addTransform(juce::AffineTransform::scale(layerScale));
        renderForeground(g);
    }
}

void ConstellationXVocalAudioProcessorEditor::renderBackground(juce::Graphics& g)
{
    const float s = scale();
    const auto accent = lnf.accent;
    const auto bounds = getLocalBounds().toFloat();

    g.fillAll(kBlack);
    juce::Path all;
    all.addRectangle(bounds);
    fillCarbon(g, all);

    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getCentreY(),
                                           juce::Colour(0xd8000000), bounds.getX(), bounds.getY(), true));
    g.fillRect(bounds);

    const auto v = R(380.0f, 85.0f, 910.0f, 420.0f);
    g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.30f), v.getCentreX(), v.getCentreY(),
                                           accent.withAlpha(0.0f), v.getCentreX() + 620.0f * s, v.getCentreY(), true));
    g.fillRect(bounds);

    drawHexMesh(g, R(8.0f, 110.0f, 150.0f, 380.0f), 9.0f * s, juce::Colour(0xff3a3a42).withAlpha(0.7f));
    drawHexMesh(g, R(1514.0f, 110.0f, 150.0f, 380.0f), 9.0f * s, juce::Colour(0xff3a3a42).withAlpha(0.7f));

    drawSpeakerHousing(g, { 175.0f * s, 310.0f * s }, 165.0f * s);
    drawSpeakerHousing(g, { 1495.0f * s, 310.0f * s }, 165.0f * s);

    chromeStroke(g, chamfered(bounds.reduced(8.0f * s), 40.0f * s).createPathWithRoundedCorners(6.0f * s), 7.0f * s);
    neonStroke(g, chamfered(bounds.reduced(17.0f * s), 34.0f * s).createPathWithRoundedCorners(5.0f * s),
               accent, 2.0f * s, 0.8f);
}

void ConstellationXVocalAudioProcessorEditor::renderForeground(juce::Graphics& g)
{
    const float s = scale();
    const auto accent = lnf.accent;

    // panels (drawn over the speakers where they overlap, as in the reference)
    drawPanel(g, R(112.0f, 450.0f, 260.0f, 190.0f));
    drawPanel(g, R(1300.0f, 450.0f, 260.0f, 190.0f));
    drawPanel(g, R(415.0f, 478.0f, 845.0f, 140.0f));
    drawPanel(g, R(362.0f, 648.0f, 945.0f, 150.0f));
    drawPanel(g, R(20.0f, 650.0f, 340.0f, 150.0f));
    drawPanel(g, R(1310.0f, 650.0f, 340.0f, 150.0f));
    drawPanel(g, R(20.0f, 812.0f, 620.0f, 112.0f));
    drawPanel(g, R(655.0f, 812.0f, 1005.0f, 112.0f));
    drawPanel(g, R(562.0f, 8.0f, 968.0f, 66.0f));

    // individual dark cells behind the six knobs
    static const float kx[6] = { 445.0f, 603.0f, 763.0f, 920.0f, 1077.0f, 1230.0f };
    for (int i = 0; i < 6; ++i)
    {
        const auto cell = chamfered(R(kx[i] - 74.0f, 653.0f, 148.0f, 141.0f), 14.0f * s).createPathWithRoundedCorners(3.0f * s);
        g.setColour(juce::Colour(0xa0000000));
        g.fillPath(cell);
        chromeStroke(g, cell, 1.6f * s);
    }

    // vortex window frame
    {
        const auto vr = R(380.0f, 85.0f, 910.0f, 420.0f);
        juce::Path outer, inner;
        outer.addRoundedRectangle(vr.expanded(3.0f * s), 24.0f * s);
        inner.addRoundedRectangle(vr.reduced(2.0f * s), 20.0f * s);
        chromeStroke(g, outer, 6.0f * s);
        neonStroke(g, inner, accent, 2.0f * s, 0.9f);
    }

    // title block
    drawCrown(g, R(68.0f, 22.0f, 84.0f, 56.0f), kGold);
    {
        const auto tr = R(170.0f, 14.0f, 340.0f, 56.0f);
        g.setFont(juce::Font(46.0f * s, juce::Font::bold | juce::Font::italic));
        g.setColour(accent.withAlpha(0.45f));
        g.drawText("CONSTELLATION", tr.translated(0.0f, 2.5f * s).toNearestInt(), juce::Justification::left);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffbfbfd), tr.getX(), tr.getY() + 6.0f * s,
                                               juce::Colour(0xff8c8c98), tr.getX(), tr.getBottom(), false));
        g.drawText("CONSTELLATION", tr.toNearestInt(), juce::Justification::left);

        const auto xr = R(452.0f, 10.0f, 60.0f, 60.0f);
        g.setFont(juce::Font(52.0f * s, juce::Font::bold | juce::Font::italic));
        for (int k = 0; k < 4; ++k)
        {
            g.setColour(accent.withAlpha(0.14f));
            g.drawText("X", xr.expanded((float) (k + 1) * 0.8f * s).toNearestInt(), juce::Justification::left);
        }
        g.setColour(accent);
        g.drawText("X", xr.toNearestInt(), juce::Justification::left);
    }
    drawLabel(g, "V   O   C   A   L", 325.0f, 66.0f, 22.0f, kPale, false);
    drawLabel(g, "CHICAGO BEATS AUDIO LABS", 322.0f, 88.0f, 15.0f, kPale.withAlpha(0.8f), false);

    // labels
    drawLabel(g, "SIZE", 267.0f, 464.0f, 24.0f, kPale);
    drawLabel(g, "NEAR", 180.0f, 608.0f, 19.0f, kPale);
    drawLabel(g, "FAR", 349.0f, 608.0f, 19.0f, kPale);
    drawLabel(g, "DECAY", 1391.0f, 464.0f, 24.0f, kPale);
    drawLabel(g, "SHORT", 1330.0f, 608.0f, 19.0f, kPale);
    drawLabel(g, "INFINITE", 1490.0f, 608.0f, 19.0f, kPale);
    drawLabel(g, "FORWARD", 560.0f, 548.0f, 25.0f, kPale);
    drawLabel(g, "BACK", 1122.0f, 548.0f, 25.0f, kPale);
    drawLabel(g, "VFX MOTION", 175.0f, 682.0f, 24.0f, kPale);
    drawLabel(g, "RANDOMIZER", 1490.0f, 682.0f, 24.0f, kPale);
    drawLabel(g, "SKIN SELECT", 226.0f, 834.0f, 24.0f, kPale);
    static const char* names[6] = { "REFLECTIONS", "WIDTH", "WARMTH", "AIR", "WET / DRY", "OUTPUT" };
    for (int i = 0; i < 6; ++i)
        drawLabel(g, names[i], kx[i], 667.0f, 22.0f, kPale);

    // direction arrows beside FORWARD / BACK
    {
        auto P = [s](float x, float y) { return juce::Point<float>(x * s, y * s); };
        juce::Path l, rr;
        l.addTriangle(P(484.0f, 548.0f), P(502.0f, 535.0f), P(502.0f, 561.0f));
        rr.addTriangle(P(1190.0f, 548.0f), P(1172.0f, 535.0f), P(1172.0f, 561.0f));
        neonStroke(g, l, accent, 1.5f * s, 0.7f);
        neonStroke(g, rr, accent, 1.5f * s, 0.7f);
        g.setColour(accent);
        g.fillPath(l);
        g.fillPath(rr);
    }

    // dice next to the randomizer button
    {
        auto die = R(1375.0f, 706.0f, 66.0f, 66.0f);
        juce::Path dp;
        dp.addRoundedRectangle(die, 10.0f * s);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2a31), die.getX(), die.getY(),
                                               juce::Colour(0xff060607), die.getX(), die.getBottom(), false));
        g.fillPath(dp);
        chromeStroke(g, dp, 2.5f * s);
        g.setColour(accent);
        const float dx = die.getWidth() * 0.26f, dr = die.getWidth() * 0.075f;
        const juce::Point<float> pts[5] = { { -dx, -dx }, { dx, -dx }, { 0, 0 }, { -dx, dx }, { dx, dx } };
        for (auto& pt : pts)
            g.fillEllipse(die.getCentreX() + pt.x - dr, die.getCentreY() + pt.y - dr, dr * 2.0f, dr * 2.0f);
    }

    // skin swatches (the layer is rebuilt when the skin changes)
    for (int i = 0; i < 5; ++i)
    {
        auto sw = swatchRect(i);
        const bool sel = (i == currentSkin());
        juce::Path sp;
        sp.addRoundedRectangle(sw, 6.0f * s);
        g.setGradientFill(juce::ColourGradient(kSkins[i].darker(0.6f), sw.getCentreX(), sw.getCentreY(),
                                               kBlack, sw.getX(), sw.getY(), true));
        g.fillPath(sp);
        g.setColour(kSkins[i].withAlpha(0.9f));
        g.fillEllipse(sw.getCentreX() - 7.0f * s, sw.getCentreY() - 7.0f * s, 14.0f * s, 14.0f * s);
        if (sel)
        {
            neonStroke(g, sp, kSkins[i], 1.8f * s, 0.9f);
            g.setColour(kPale);
            g.strokePath(sp, juce::PathStrokeType(2.0f * s));
        }
        else
        {
            chromeStroke(g, sp, 1.6f * s);
        }
    }
}

void ConstellationXVocalAudioProcessorEditor::paint(juce::Graphics& g)
{
    const float s = scale();
    const auto accent = lnf.accent;
    ensureLayers();

    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(bgImage, getLocalBounds().toFloat());

    // live artwork between the cached layers
    drawVortex(g, R(380.0f, 85.0f, 910.0f, 420.0f));
    drawSpeaker(g, { 175.0f * s, 310.0f * s }, 165.0f * s, juce::jlimit(0.0f, 1.0f, lowL * 4.0f));
    drawSpeaker(g, { 1495.0f * s, 310.0f * s }, 165.0f * s, juce::jlimit(0.0f, 1.0f, lowR * 4.0f));

    g.drawImage(fgImage, getLocalBounds().toFloat());

    // preset display (click to open the preset menu)
    {
        auto box = presetBoxRect();
        juce::Path bp;
        bp.addRoundedRectangle(box, 10.0f * s);
        g.setColour(juce::Colour(0xff030304));
        g.fillPath(bp);
        chromeStroke(g, bp, 1.8f * s);
        g.setColour(accent.withAlpha(0.35f));
        g.drawRoundedRectangle(box.reduced(3.0f * s), 8.0f * s, 1.0f * s);
        g.setColour(kPale);
        g.setFont(juce::Font(26.0f * s, juce::Font::bold));
        g.drawFittedText(processor.getPresetName(), R(670.0f, 18.0f, 285.0f, 32.0f).toNearestInt(),
                         juce::Justification::centred, 1, 0.6f);
        drawLabel(g, "PRESET", 812.0f, 57.0f, 14.0f, kSteel, false);
    }
}

void ConstellationXVocalAudioProcessorEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit(0.0, 0.1, (now - lastTimeMs) * 0.001);
    lastTimeMs = now;
    if (motion.getToggleState())
        phase += dt * 1.1;   // radians per second

    auto follow = [](float& v, float target) { v += (target > v ? 0.45f : 0.12f) * (target - v); };
    follow(lowL, processor.meterLowL.load());
    follow(lowR, processor.meterLowR.load());
    follow(inLevel, processor.meterIn.load());
    depthVis += 0.12f * (processor.apvts.getRawParameterValue("depth")->load() - depthVis);

    if (dirty && !juce::Desktop::getInstance().getMainMouseSource().isDragging()
        && juce::Time::getMillisecondCounterHiRes() - lastChangeMs > 300.0)
    {
        dirty = false;
        processor.commitHistory();
    }
    refreshButtons();

    const int skinNow = currentSkin();
    if (skinNow != shownSkin)
    {
        shownSkin = skinNow;
        lnf.accent = kSkins[skinNow];
        sendLookAndFeelChange();
        repaint();
        return;
    }

    // repaint only the animated regions (vortex + both subwoofers)
    repaint(R(380.0f, 85.0f, 910.0f, 420.0f).getSmallestIntegerContainer());
    repaint(R(0.0f, 100.0f, 350.0f, 400.0f).getSmallestIntegerContainer());
    repaint(R(1320.0f, 100.0f, 352.0f, 400.0f).getSmallestIntegerContainer());
}
