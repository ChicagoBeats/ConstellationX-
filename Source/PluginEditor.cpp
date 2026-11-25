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
} // namespace

// ============================== Look and feel ===============================
void ConstellationLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                                float pos, float startA, float endA, juce::Slider& s)
{
    const float d = (float) juce::jmin(w, h) - 4.0f;
    const auto c = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).getCentre();
    const float ringR = d * 0.5f * 0.92f;
    const float ang = startA + pos * (endA - startA);

    juce::Path track;
    track.addCentredArc(c.x, c.y, ringR, ringR, 0.0f, startA, endA, true);
    g.setColour(accent.withAlpha(0.20f));
    g.strokePath(track, juce::PathStrokeType(d * 0.05f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc(c.x, c.y, ringR, ringR, 0.0f, startA, ang, true);
    g.setColour(accent.withAlpha(0.30f));
    g.strokePath(val, juce::PathStrokeType(d * 0.11f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(accent);
    g.strokePath(val, juce::PathStrokeType(d * 0.05f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto body = juce::Rectangle<float>(d * 0.70f, d * 0.70f).withCentre(c);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3b3a41), c.x, body.getY(),
                                           juce::Colour(0xff040405), c.x, body.getBottom(), false));
    g.fillEllipse(body);
    g.setColour(juce::Colour(0xffa09da6).withAlpha(0.55f));
    g.drawEllipse(body, juce::jmax(1.0f, d * 0.012f));

    const float r0 = body.getWidth() * 0.5f * 0.80f, r1 = body.getWidth() * 0.5f * 0.97f;
    g.setColour(kPale);
    g.drawLine(c.x + std::sin(ang) * r0, c.y - std::cos(ang) * r0,
               c.x + std::sin(ang) * r1, c.y - std::cos(ang) * r1, juce::jmax(1.5f, d * 0.025f));

    g.setColour(kPale);
    g.setFont(juce::Font(d * 0.22f, juce::Font::bold));
    g.drawFittedText(s.getTextFromValue(s.getValue()), body.reduced(body.getWidth() * 0.08f).toNearestInt(),
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
    const float th = (float) h * 0.20f;
    juce::Rectangle<float> track((float) x + tr * 0.3f, cy - th * 0.5f, (float) w - tr * 0.6f, th);

    g.setColour(juce::Colour(0xff030304));
    g.fillRoundedRectangle(track, th * 0.5f);
    g.setColour(accent.withAlpha(0.45f));
    g.drawRoundedRectangle(track, th * 0.5f, 1.5f);

    const float centreX = track.getCentreX();
    const float x0 = juce::jmin(centreX, sliderPos), x1 = juce::jmax(centreX, sliderPos);
    g.setColour(accent.withAlpha(0.35f));
    g.fillRoundedRectangle(x0, track.getY() - th * 0.2f, x1 - x0, th * 1.4f, th * 0.7f);
    g.setColour(accent);
    g.fillRoundedRectangle(x0, track.getY() + th * 0.2f, x1 - x0, th * 0.6f, th * 0.3f);

    auto thumb = juce::Rectangle<float>(tr * 2.0f, tr * 2.0f).withCentre({ sliderPos, cy });
    g.setColour(accent.withAlpha(0.25f));
    g.fillEllipse(thumb.expanded(tr * 0.18f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff34333a), thumb.getCentreX(), thumb.getY(),
                                           juce::Colour(0xff020203), thumb.getCentreX(), thumb.getBottom(), false));
    g.fillEllipse(thumb);
    g.setColour(accent);
    g.drawEllipse(thumb, juce::jmax(1.5f, tr * 0.07f));
    drawCrown(g, juce::Rectangle<float>(tr * 0.95f, tr * 0.62f).withCentre(thumb.getCentre()), kGold);
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
    const float rad = r.getHeight() * 0.5f;
    g.setColour(juce::Colour(0xff060607));
    g.fillRoundedRectangle(r, rad);
    if (b.getToggleState())
    {
        g.setColour(accent.withAlpha(0.85f));
        g.fillRoundedRectangle(r, rad);
    }
    else if (down || highlighted)
    {
        g.setColour(accent.withAlpha(down ? 0.55f : 0.25f));
        g.fillRoundedRectangle(r, rad);
    }
    g.setColour(accent.withAlpha(0.85f));
    g.drawRoundedRectangle(r, rad, 2.0f);
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
        knobs[i]->setBounds(R(kx[i] - 62.0f, 741.0f - 62.0f, 124.0f, 124.0f).toNearestInt());

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

void ConstellationXVocalAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> r)
{
    const float s = scale();
    g.setColour(kPanel);
    g.fillRoundedRectangle(r, 18.0f * s);
    g.setColour(lnf.accent.withAlpha(0.18f));
    g.drawRoundedRectangle(r.reduced(5.0f * s), 14.0f * s, 4.0f * s);
    g.setColour(lnf.accent.withAlpha(0.75f));
    g.drawRoundedRectangle(r, 18.0f * s, 2.0f * s);
}

void ConstellationXVocalAudioProcessorEditor::drawSpeaker(juce::Graphics& g, juce::Point<float> c,
                                                           float radius, float level)
{
    const auto accent = lnf.accent;
    auto circle = [&](float rr) { return juce::Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(c); };

    g.setColour(accent.withAlpha(0.08f + 0.22f * level));
    g.fillEllipse(circle(radius * 1.10f));

    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff55545b), c.x, c.y - radius, juce::Colour(0xff0a0a0c),
                                           c.x, c.y + radius, false));
    g.fillEllipse(circle(radius));
    g.setColour(accent.withAlpha(0.85f));
    g.drawEllipse(circle(radius * 0.97f), radius * 0.045f);
    g.setColour(juce::Colour(0xff060608));
    g.fillEllipse(circle(radius * 0.86f));
    g.setColour(accent.withAlpha(0.35f));
    g.drawEllipse(circle(radius * 0.80f), radius * 0.02f);

    // cone: excursion follows the real sub-band level
    const float cone = radius * 0.70f * (1.0f + 0.07f * level);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2a30), c.x, c.y - cone, juce::Colour(0xff050506),
                                           c.x, c.y + cone, false));
    g.fillEllipse(circle(cone));
    g.setColour(juce::Colour(0xff6c6a73).withAlpha(0.6f));
    for (int i = 1; i <= 4; ++i)
        g.drawEllipse(circle(cone * (1.0f - 0.17f * (float) i)), juce::jmax(1.0f, radius * 0.008f));
    g.setColour(juce::Colour(0xff09090b));
    g.fillEllipse(circle(radius * 0.26f * (1.0f + 0.10f * level)));
    g.setColour(accent.withAlpha(0.7f));
    g.drawEllipse(circle(radius * 0.26f * (1.0f + 0.10f * level)), radius * 0.02f);
    drawCrown(g, juce::Rectangle<float>(radius * 0.26f, radius * 0.17f).withCentre(c), kGold);
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

void ConstellationXVocalAudioProcessorEditor::paint(juce::Graphics& g)
{
    const float s = scale();
    const auto accent = lnf.accent;
    g.fillAll(kBlack);

    // outer frame
    g.setColour(accent.withAlpha(0.25f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(6.0f * s), 20.0f * s, 5.0f * s);
    g.setColour(accent.withAlpha(0.8f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(6.0f * s), 20.0f * s, 1.5f * s);

    // title block
    drawCrown(g, R(68.0f, 22.0f, 84.0f, 56.0f), kGold);
    g.setColour(kPale);
    g.setFont(juce::Font(46.0f * s, juce::Font::bold | juce::Font::italic));
    g.drawText("CONSTELLATION", R(170.0f, 14.0f, 340.0f, 56.0f).toNearestInt(), juce::Justification::left);
    g.setColour(accent);
    g.setFont(juce::Font(52.0f * s, juce::Font::bold | juce::Font::italic));
    g.drawText("X", R(452.0f, 10.0f, 60.0f, 60.0f).toNearestInt(), juce::Justification::left);
    drawLabel(g, "V   O   C   A   L", 325.0f, 66.0f, 22.0f, kPale, false);
    drawLabel(g, "CHICAGO BEATS AUDIO LABS", 322.0f, 88.0f, 15.0f, kPale.withAlpha(0.8f), false);

    // animated vortex + dual subwoofers (all driven by live audio meters)
    drawVortex(g, R(380.0f, 85.0f, 910.0f, 420.0f));
    drawSpeaker(g, { 175.0f * s, 310.0f * s }, 165.0f * s, juce::jlimit(0.0f, 1.0f, lowL * 4.0f));
    drawSpeaker(g, { 1495.0f * s, 310.0f * s }, 165.0f * s, juce::jlimit(0.0f, 1.0f, lowR * 4.0f));

    // panels
    drawPanel(g, R(112.0f, 450.0f, 260.0f, 190.0f));
    drawPanel(g, R(1300.0f, 450.0f, 260.0f, 190.0f));
    drawPanel(g, R(415.0f, 478.0f, 845.0f, 140.0f));
    drawPanel(g, R(362.0f, 648.0f, 945.0f, 150.0f));
    drawPanel(g, R(20.0f, 650.0f, 340.0f, 150.0f));
    drawPanel(g, R(1310.0f, 650.0f, 340.0f, 150.0f));
    drawPanel(g, R(20.0f, 812.0f, 620.0f, 112.0f));
    drawPanel(g, R(655.0f, 812.0f, 1005.0f, 112.0f));
    drawPanel(g, R(562.0f, 8.0f, 968.0f, 66.0f));

    // preset display (click to open the preset menu)
    {
        auto box = presetBoxRect();
        g.setColour(juce::Colour(0xff030304));
        g.fillRoundedRectangle(box, 10.0f * s);
        g.setColour(accent.withAlpha(0.6f));
        g.drawRoundedRectangle(box, 10.0f * s, 1.5f * s);
        g.setColour(kPale);
        g.setFont(juce::Font(26.0f * s, juce::Font::bold));
        g.drawFittedText(processor.getPresetName(), R(670.0f, 18.0f, 285.0f, 32.0f).toNearestInt(),
                         juce::Justification::centred, 1, 0.6f);
        drawLabel(g, "PRESET", 812.0f, 57.0f, 14.0f, kSteel, false);
    }

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
    static const float xs[6] = { 445.0f, 603.0f, 763.0f, 920.0f, 1077.0f, 1230.0f };
    for (int i = 0; i < 6; ++i)
        drawLabel(g, names[i], xs[i], 667.0f, 22.0f, kPale);

    // direction arrows beside FORWARD / BACK
    {
        auto P = [s](float x, float y) { return juce::Point<float>(x * s, y * s); };
        juce::Path l, rr;
        l.addTriangle(P(484.0f, 548.0f), P(502.0f, 535.0f), P(502.0f, 561.0f));
        rr.addTriangle(P(1190.0f, 548.0f), P(1172.0f, 535.0f), P(1172.0f, 561.0f));
        g.setColour(accent);
        g.fillPath(l);
        g.fillPath(rr);
    }

    // dice next to the randomizer button
    {
        auto die = R(1375.0f, 706.0f, 66.0f, 66.0f);
        g.setColour(juce::Colour(0xff0a0a0c));
        g.fillRoundedRectangle(die, 10.0f * s);
        g.setColour(accent);
        g.drawRoundedRectangle(die, 10.0f * s, 2.5f * s);
        const float dx = die.getWidth() * 0.26f, dr = die.getWidth() * 0.075f;
        const juce::Point<float> pts[5] = { { -dx, -dx }, { dx, -dx }, { 0, 0 }, { -dx, dx }, { dx, dx } };
        for (auto& pt : pts)
            g.fillEllipse(die.getCentreX() + pt.x - dr, die.getCentreY() + pt.y - dr, dr * 2.0f, dr * 2.0f);
    }

    // skin swatches
    for (int i = 0; i < 5; ++i)
    {
        auto sw = swatchRect(i);
        const bool sel = (i == currentSkin());
        g.setGradientFill(juce::ColourGradient(kSkins[i].darker(0.6f), sw.getCentreX(), sw.getCentreY(),
                                               kBlack, sw.getX(), sw.getY(), true));
        g.fillRoundedRectangle(sw, 6.0f * s);
        g.setColour(kSkins[i].withAlpha(0.9f));
        g.fillEllipse(sw.getCentreX() - 7.0f * s, sw.getCentreY() - 7.0f * s, 14.0f * s, 14.0f * s);
        g.setColour(sel ? kPale : kSkins[i].withAlpha(0.6f));
        g.drawRoundedRectangle(sw, 6.0f * s, sel ? 3.0f * s : 1.5f * s);
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
