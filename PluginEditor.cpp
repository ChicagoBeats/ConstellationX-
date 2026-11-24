#include "PluginEditor.h"

namespace
{
const juce::Colour bg(0xff09090d);
const juce::Colour panel(0xff151116);
const juce::Colour red(0xffff2638);
const juce::Colour pale(0xfff4edf0);
}

ConstellationXVocalAudioProcessorEditor::ConstellationXVocalAudioProcessorEditor(
    ConstellationXVocalAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setResizable(true, true);
    setResizeLimits(900, 560, 1800, 1120);
    setSize(1200, 740);

    styleSlider(depth, "DEPTH");
    styleSlider(size, "SIZE");
    styleSlider(decay, "DECAY");
    styleSlider(reflections, "REFLECTIONS");
    styleSlider(width, "WIDTH");
    styleSlider(warmth, "WARMTH");
    styleSlider(air, "AIR");
    styleSlider(mix, "WET / DRY");
    styleSlider(output, "OUTPUT");

    auto attach = [this](juce::Slider& s, const char* id)
    {
        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, id, s));
    };
    attach(depth, "depth"); attach(size, "size"); attach(decay, "decay");
    attach(reflections, "reflections"); attach(width, "width"); attach(warmth, "warmth");
    attach(air, "air"); attach(mix, "mix"); attach(output, "output");

    motion.setColour(juce::ToggleButton::textColourId, pale);
    motion.setColour(juce::ToggleButton::tickColourId, red);
    addAndMakeVisible(motion);
    motionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "vfxMotion", motion);

    randomize.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff65101b));
    randomize.setColour(juce::TextButton::textColourOffId, pale);
    randomize.onClick = [this]
    {
        const char* ids[] = { "depth", "size", "decay", "reflections", "width",
                              "warmth", "air", "mix" };
        for (auto* id : ids)
        {
            if (auto* param = processor.apvts.getParameter(id))
            {
                const float value = rng.nextFloat();
                param->beginChangeGesture();
                param->setValueNotifyingHost(value);
                param->endChangeGesture();
            }
        }
    };
    addAndMakeVisible(randomize);

    skin.addItem("CRIMSON BLACK", 1);
    skin.addItem("BLACK DIAMOND", 2);
    skin.addItem("CHROME RED", 3);
    skin.addItem("ELECTRIC BLUE", 4);
    skin.setSelectedId(1);
    addAndMakeVisible(skin);

    startTimerHz(30);
}

void ConstellationXVocalAudioProcessorEditor::styleSlider(juce::Slider& s, const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
    s.setColour(juce::Slider::rotarySliderFillColourId, red);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff493039));
    s.setColour(juce::Slider::thumbColourId, pale);
    s.setColour(juce::Slider::textBoxTextColourId, pale);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(s);
}

void ConstellationXVocalAudioProcessorEditor::drawSpeaker(
    juce::Graphics& g, juce::Rectangle<float> r, float level)
{
    g.setColour(juce::Colour(0xff260d14));
    g.fillEllipse(r);
    g.setColour(red.withAlpha(0.8f));
    g.drawEllipse(r, 4.0f);
    auto cone = r.reduced(r.getWidth() * (0.16f + 0.035f * level));
    g.setColour(juce::Colour(0xff25252c));
    g.fillEllipse(cone);
    g.setColour(juce::Colour(0xff77717a));
    g.drawEllipse(cone, 2.0f);
    auto cap = cone.reduced(cone.getWidth() * 0.30f);
    g.setColour(juce::Colour(0xff08080b));
    g.fillEllipse(cap);
    g.setColour(red.withAlpha(0.7f));
    g.drawEllipse(cap, 2.0f);
}

void ConstellationXVocalAudioProcessorEditor::drawVortex(
    juce::Graphics& g, juce::Rectangle<float> r)
{
    const float depthValue = processor.apvts.getRawParameterValue("depth")->load();
    const float opening = 0.48f + 0.36f * (1.0f - depthValue) * 0.5f;
    const float motionAmount = motion.getToggleState() ? 1.0f : 0.0f;
    const float phase = animationPhase * motionAmount;
    const auto centre = r.getCentre();
    const float maxRadius = juce::jmin(r.getWidth(), r.getHeight()) * opening;

    g.setColour(juce::Colour(0xff100b12));
    g.fillEllipse(r);

    for (int arm = 0; arm < 5; ++arm)
    {
        juce::Path path;
        for (int i = 0; i <= 180; ++i)
        {
            const float t = i / 180.0f;
            const float angle = t * 7.0f + phase + arm * juce::MathConstants<float>::twoPi / 5.0f;
            const float radius = maxRadius * (0.04f + t * 0.94f);
            const float x = centre.x + std::cos(angle) * radius;
            const float y = centre.y + std::sin(angle) * radius * 0.55f;
            if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
        }
        g.setColour(red.withAlpha(0.18f));
        g.strokePath(path, juce::PathStrokeType(7.0f));
        g.setColour(juce::Colour(0xffff6670).withAlpha(0.62f));
        g.strokePath(path, juce::PathStrokeType(1.6f));
    }

    for (int i = 0; i < 95; ++i)
    {
        const float a = i * 2.39996f + phase * (0.3f + (i % 4) * 0.08f);
        const float radius = maxRadius * std::sqrt((i + 1.0f) / 95.0f);
        const float x = centre.x + std::cos(a) * radius;
        const float y = centre.y + std::sin(a) * radius * 0.55f;
        const float star = 1.0f + (i % 3) * 0.65f;
        g.setColour((i % 7 == 0 ? pale : red).withAlpha(0.45f + 0.4f * motionAmount));
        g.fillEllipse(x, y, star, star);
    }

    g.setColour(juce::Colour(0xffff4555).withAlpha(0.16f));
    g.fillEllipse(centre.x - 18.0f, centre.y - 18.0f, 36.0f, 36.0f);
    g.setColour(pale.withAlpha(0.9f));
    g.fillEllipse(centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
}

void ConstellationXVocalAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff39101b));
    g.drawRoundedRectangle(bounds.reduced(5.0f), 15.0f, 3.0f);

    g.setColour(pale);
    g.setFont(juce::Font(25.0f, juce::Font::bold));
    g.drawText("CONSTELLATIONX", 28, 14, 330, 34, juce::Justification::left);
    g.setColour(red);
    g.drawText("VOCAL", 30, 47, 160, 22, juce::Justification::left);
    g.setColour(juce::Colour(0xffc9aeb5));
    g.setFont(10.0f);
    g.drawText("CHICAGO BEATS AUDIO LABS", 30, 70, 250, 16, juce::Justification::left);

    auto centre = juce::Rectangle<float>(getWidth() * 0.24f, 104.0f,
                                         getWidth() * 0.52f, getHeight() * 0.48f);
    drawVortex(g, centre);
    drawSpeaker(g, { 24.0f, 132.0f, 190.0f, 190.0f }, 0.5f + 0.5f * std::sin(animationPhase));
    drawSpeaker(g, { (float)getWidth() - 214.0f, 132.0f, 190.0f, 190.0f },
                0.5f + 0.5f * std::sin(animationPhase + 0.8f));

    g.setColour(pale);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("FORWARD", getWidth()/2 - 210, getHeight()/2 + 30, 90, 24, juce::Justification::centred);
    g.drawText("BACK", getWidth()/2 + 120, getHeight()/2 + 30, 90, 24, juce::Justification::centred);

    g.setColour(juce::Colour(0xff3b2028));
    g.drawHorizontalLine(getHeight() - 112, 24.0f, (float)getWidth() - 24.0f);
    g.setColour(juce::Colour(0xffa98c94));
    g.setFont(10.0f);
    g.drawText("CONSTELLATIONX VOCAL  •  SPATIAL DEPTH PROCESSOR",
               28, getHeight() - 25, getWidth() - 56, 16, juce::Justification::centred);
}

void ConstellationXVocalAudioProcessorEditor::resized()
{
    const int w = getWidth(), h = getHeight();
    depth.setBounds(w/2 - 105, h/2 + 58, 210, 105);
    size.setBounds(48, h/2 - 12, 112, 112);
    decay.setBounds(w - 160, h/2 - 12, 112, 112);

    const int y = h - 205;
    const int knobW = 94;
    const int start = (w - knobW * 6) / 2;
    juce::Slider* knobs[] = { &reflections, &width, &warmth, &air, &mix, &output };
    for (int i = 0; i < 6; ++i)
        knobs[i]->setBounds(start + i * knobW, y, knobW, 116);

    motion.setBounds(30, h - 82, 150, 30);
    randomize.setBounds(w - 180, h - 82, 150, 34);
    skin.setBounds(w/2 - 105, h - 82, 210, 28);
}

void ConstellationXVocalAudioProcessorEditor::timerCallback()
{
    if (motion.getToggleState())
        animationPhase += 0.035f;
    repaint();
}
