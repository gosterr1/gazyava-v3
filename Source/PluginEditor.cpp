#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    const juce::Colour kLime (0xffb6f500), kRed (0xffe4002b), kBlue (0xff1e8bff);
    const float kPi = juce::MathConstants<float>::pi;
    const int W = 1360, H = 780;

    juce::String pct (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; }
    juce::String db  (double v) { return juce::String (v, 1) + " dB"; }
    juce::Image load (const void* d, int n) { return juce::ImageCache::getFromMemory (d, n); }

    const char* kModeDesc[5] = {
        "ЛАЙМ — классический ламповый буст: тёплый и плотный бас",
        "ТРОПИК — мягкая лента: округлый, тёплый, сочный низ",
        "ГРОМ — тяжёлый фазз: грязный жирный бас",
        "СИНИЙ — чистый холодный буст + саб-октава",
        "ЧЁРНЫЙ — жёсткий клип: агрессивный перегруз баса" };
}

// ---------------------------------------------------------------- knobs
LidKnob::LidKnob (juce::Image sprite, Fmt f) : src (std::move (sprite)), fmt (std::move (f))
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setMouseDragSensitivity (220);
}

void LidKnob::resized()
{
    const int d = juce::jmin (getWidth(), getHeight()) - 70;
    if (src.isValid() && d > 10) scaled = src.rescaled (d, d, juce::Graphics::highResamplingQuality);
}

void LidKnob::paint (juce::Graphics& g)
{
    auto c = getLocalBounds().toFloat().getCentre();
    const float rad = scaled.isValid() ? scaled.getWidth() * 0.5f : 100.f;
    const float p = (float) valueToProportionOfLength (getValue());
    const float a0 = juce::degreesToRadians (-135.f), a1 = juce::degreesToRadians (135.f), a = a0 + p * (a1 - a0);
    const float rr = rad + 14.f;

    g.setGradientFill (juce::ColourGradient (kLime.withAlpha (0.2f + 0.3f * p), c.x, c.y, kLime.withAlpha (0.f), c.x + rr + 20, c.y, true));
    g.fillEllipse (c.x - rr - 20, c.y - rr - 20, (rr + 20) * 2, (rr + 20) * 2);
    juce::Path track, val;
    track.addCentredArc (c.x, c.y, rr, rr, 0, a0, a1, true);
    val.addCentredArc   (c.x, c.y, rr, rr, 0, a0, a,  true);
    g.setColour (juce::Colour (0xff04100a));
    g.strokePath (track, juce::PathStrokeType (11.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setGradientFill (juce::ColourGradient (kBlue, c.x - rr, c.y, kRed, c.x + rr, c.y, false));
    g.strokePath (val, juce::PathStrokeType (8.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (scaled.isValid())
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - rad + 4, c.y - rad + 8, rad * 2, rad * 2);
        g.setColour (juce::Colours::white);
        g.drawImageTransformed (scaled, juce::AffineTransform::translation (c.x - rad, c.y - rad).followedBy (juce::AffineTransform::rotation (a, c.x, c.y)));
    }
    juce::Rectangle<float> pill (c.x - 52, c.y + rad * 0.55f, 104, 24);
    g.setColour (juce::Colours::black.withAlpha (0.72f)); g.fillRoundedRectangle (pill, 12.f);
    g.setColour (kLime); g.drawRoundedRectangle (pill, 12.f, 1.5f);
    g.setColour (juce::Colours::white); g.setFont (juce::Font (15.f, juce::Font::bold | juce::Font::italic));
    g.drawFittedText (RU ("ГАЗ ") + fmt (getValue()), pill.toNearestInt(), juce::Justification::centred, 1);
}

MiniKnob::MiniKnob (juce::String l, juce::Image sprite, Fmt f) : label (std::move (l)), src (std::move (sprite)), fmt (std::move (f))
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setMouseDragSensitivity (150);
}

void MiniKnob::resized()
{
    const int d = juce::jmin (getWidth(), getHeight() - 30) - 10;
    if (src.isValid() && d > 8) scaled = src.rescaled (d, d, juce::Graphics::highResamplingQuality);
}

void MiniKnob::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto tx = b.removeFromBottom (30.f);
    auto c = b.getCentre();
    const float rad = scaled.isValid() ? scaled.getWidth() * 0.5f : 20.f;
    const float p = (float) valueToProportionOfLength (getValue());
    const float a0 = juce::degreesToRadians (-135.f), a1 = juce::degreesToRadians (135.f), a = a0 + p * (a1 - a0);
    juce::Path track, val;
    track.addCentredArc (c.x, c.y, rad + 4, rad + 4, 0, a0, a1, true);
    val.addCentredArc   (c.x, c.y, rad + 4, rad + 4, 0, a0, a,  true);
    g.setColour (juce::Colour (0xff0a1a0f)); g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (kLime); g.strokePath (val, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (scaled.isValid())
    {
        g.setColour (juce::Colours::white);
        g.drawImageTransformed (scaled, juce::AffineTransform::translation (c.x - rad, c.y - rad).followedBy (juce::AffineTransform::rotation (a, c.x, c.y)));
    }
    g.setFont (juce::Font (11.f, juce::Font::bold));
    g.setColour (juce::Colours::white); g.drawFittedText (label, tx.removeFromTop (15.f).toNearestInt(), juce::Justification::centred, 1);
    g.setColour (kLime); g.drawFittedText (fmt (getValue()), tx.toNearestInt(), juce::Justification::centred, 1);
}

ModeCan::ModeCan (juce::String n, juce::Image sprite, juce::Colour glow) : label (std::move (n)), src (std::move (sprite)), glowC (glow)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ModeCan::resized()
{
    if (! src.isValid()) return;
    const int h = juce::jmax (10, getHeight() - 28);
    const int w = juce::roundToInt (src.getWidth() * (float) h / (float) src.getHeight());
    scaled = src.rescaled (w, h, juce::Graphics::highResamplingQuality);
}

void ModeCan::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto txt = b.removeFromBottom (28.f);
    if (scaled.isValid())
    {
        const int w = scaled.getWidth(), h = scaled.getHeight();
        const int x = (int) (b.getCentreX() - w * 0.5f), y = (int) (b.getBottom() - h);
        if (selected)
        {
            g.setGradientFill (juce::ColourGradient (glowC.withAlpha (0.75f), b.getCentreX(), y + h * 0.5f, glowC.withAlpha (0.f), b.getCentreX() + w * 1.25f, y + h * 0.5f, true));
            g.fillEllipse (b.getCentreX() - w * 1.25f, y + h * 0.5f - w * 1.25f, w * 2.5f, w * 2.5f);
        }
        g.setColour (juce::Colours::white.withAlpha (selected ? 1.f : 0.45f));
        g.drawImageAt (scaled, x, y);
    }
    g.setFont (juce::Font (14.f, juce::Font::bold | juce::Font::italic));
    g.setColour (selected ? glowC.brighter (0.5f) : juce::Colours::white.withAlpha (0.55f));
    g.drawFittedText (label, txt.toNearestInt(), juce::Justification::centred, 1);
}

void TubeMeter::tick (float rate)
{
    auto b = getLocalBounds().toFloat().reduced (4.f);
    const float top = b.getBottom() - b.getHeight() * level;
    const float n = rate / 30.f; int c = (int) n;
    if (rng.nextFloat() < n - (float) c) ++c;
    for (int i = 0; i < c && bubbles.size() < 240; ++i)
        bubbles.push_back ({ b.getX() + 5.f + rng.nextFloat() * (b.getWidth() - 10.f), b.getBottom() - 3.f,
                             1.5f + rng.nextFloat() * 3.8f, 1.2f + rng.nextFloat() * 2.2f + rate * 0.01f, rng.nextFloat() * 6.28f });
    for (auto& bb : bubbles) { bb.y -= bb.vy; bb.ph += 0.2f; bb.x += std::sin (bb.ph) * 0.35f; }
    bubbles.erase (std::remove_if (bubbles.begin(), bubbles.end(), [top] (const Bubble& bb) { return bb.y < top + 2.f; }), bubbles.end());
    repaint();
}

void TubeMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (4.f);
    juce::Path pill; pill.addRoundedRectangle (b, b.getWidth() * 0.5f);
    g.setColour (juce::Colours::white); g.fillPath (pill);
    g.saveState(); g.reduceClipRegion (pill);
    const float fh = b.getHeight() * level, top = b.getBottom() - fh;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4cc418), 0, top, juce::Colour (0xff17802a), 0, b.getBottom(), false));
    g.fillRect (b.getX(), top, b.getWidth(), fh);
    for (auto& bb : bubbles)
    {
        g.setColour (juce::Colours::white.withAlpha (0.25f)); g.fillEllipse (bb.x - bb.r, bb.y - bb.r, bb.r * 2, bb.r * 2);
        g.setColour (juce::Colours::white.withAlpha (0.85f)); g.drawEllipse (bb.x - bb.r, bb.y - bb.r, bb.r * 2, bb.r * 2, 1.2f);
    }
    g.restoreState();
    g.setColour (juce::Colours::black); g.strokePath (pill, juce::PathStrokeType (2.5f));
}

// ---------------------------------------------------------------- editor
MiniKnob* GazyavaEditor::addKnob (const char* id, const char* label, Fmt f, juce::Rectangle<int> r)
{
    knobs.push_back (std::make_unique<MiniKnob> (RU (label), imgLid, std::move (f)));
    auto* k = knobs.back().get();
    addAndMakeVisible (k);
    k->setBounds (r);
    sliderAtts.push_back (std::make_unique<SA> (proc.apvts, id, *k));
    return k;
}

void GazyavaEditor::addChoice (const char* id, std::vector<const char*> names, int cols, juce::Rectangle<int> area, int bw, int bh, int gap, int radio)
{
    auto grp = std::make_unique<ChoiceGroup>();
    auto* gp = grp.get();
    for (size_t i = 0; i < names.size(); ++i)
    {
        auto b = std::make_unique<juce::TextButton> (RU (names[i]));
        b->setClickingTogglesState (true); b->setRadioGroupId (radio);
        b->setBounds (area.getX() + (int) (i % (size_t) cols) * (bw + gap), area.getY() + (int) (i / (size_t) cols) * (bh + gap), bw, bh);
        b->onClick = [gp, i] { if (gp->btns[i]->getToggleState()) gp->att->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (*b);
        allButtons.push_back (b.get());
        gp->btns.push_back (std::move (b));
    }
    gp->att = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (id), [gp] (float v)
    { const int idx = juce::roundToInt (v); for (size_t j = 0; j < gp->btns.size(); ++j) gp->btns[j]->setToggleState ((int) j == idx, juce::dontSendNotification); });
    gp->att->sendInitialUpdate();
    groups.push_back (std::move (grp));
}

GazyavaEditor::GazyavaEditor (GazyavaProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      imgBig (load (BinaryData::can_big_png, BinaryData::can_big_pngSize)),
      imgSpk (load (BinaryData::speaker_png, BinaryData::speaker_pngSize)),
      imgCatL (load (BinaryData::manul_left_jpg, BinaryData::manul_left_jpgSize)),
      imgCatR (load (BinaryData::manul_right_jpg, BinaryData::manul_right_jpgSize)),
      imgLid (load (BinaryData::lid_png, BinaryData::lid_pngSize)),
      lid (load (BinaryData::lid_png, BinaryData::lid_pngSize), [] (double v) { return pct (v); })
{
    const int cx = W / 2;
    addAndMakeVisible (lid); lid.setBounds (cx - 140, 66, 280, 280);
    aLid = std::make_unique<SA> (proc.apvts, "drive", lid);
    addAndMakeVisible (tubeL); addAndMakeVisible (tubeR);
    tubeL.setBounds (284, 40, 60, 460); tubeR.setBounds (W - 344, 40, 60, 460);

    auto hz  = [] (double v) { return juce::String (juce::roundToInt (v)) + " Hz"; };
    auto ms  = [] (double v) { return juce::String (v, 1) + " ms"; };
    auto ms0 = [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; };
    // левый блок: бас-бустер
    auto L = [] (int col, int row) { return juce::Rectangle<int> (360 + col * 90, 92 + row * 86, 90, 86); };
    addKnob ("boost",  "БУСТ",     [] (double v) { return db (v * 18.0); }, L (0, 0));
    addKnob ("sub",    "САБ",      pct, L (1, 0));
    addKnob ("freq",   "ЧАСТОТА",  hz, L (0, 1));
    addKnob ("tight",  "ТАЙТ",     [] (double v) { return juce::String (juce::roundToInt (18.0 + v * 55.0)) + " Hz"; }, L (1, 1));
    addKnob ("crunch", "ХРУСТ",    pct, L (0, 2));
    addKnob ("ice",    "ЛЁД",      pct, L (1, 2));
    // правый блок: выход
    auto R = [] (int col, int row) { return juce::Rectangle<int> (820 + col * 90, 92 + row * 86, 90, 86); };
    addKnob ("in",       "ВХОД",     db, R (0, 0));
    addKnob ("width",    "ШИРИНА",   [] (double v) { return juce::String (juce::roundToInt (v * 200.0)) + "%"; }, R (1, 0));
    addKnob ("overload", "ПЕРЕГРУЗ", pct, R (0, 1));
    addKnob ("mix",      "МИКС",     pct, R (1, 1));
    addKnob ("out",      "ВЫХОД",    db, R (0, 2));
    limBtn.setButtonText (RU ("ЛИМИТЕР")); monoBtn.setButtonText (RU ("МОНО-БАС"));
    limBtn.setBounds (913, 272, 84, 26); monoBtn.setBounds (913, 304, 84, 26);
    for (auto* bt : { &limBtn, &monoBtn }) { bt->setClickingTogglesState (true); addAndMakeVisible (bt); allButtons.push_back (bt); }
    aLim = std::make_unique<BA> (proc.apvts, "limiter", limBtn); aMono = std::make_unique<BA> (proc.apvts, "monobass", monoBtn);

    // панели
    const int py = 542;
    auto C = [py] (int panel, int col, int row) { return juce::Rectangle<int> (360 + panel * 218 + col * 68, py + 26 + row * 100, 68, 94); };
    addKnob ("cthr",   "ПОРОГ",    db, C (0, 0, 0));
    addKnob ("cratio", "СТЕПЕНЬ",  [] (double v) { return juce::String (v, 1) + ":1"; }, C (0, 1, 0));
    addKnob ("catk",   "АТАКА",    ms, C (0, 2, 0));
    addKnob ("crel",   "РЕЛИЗ",    ms0, C (0, 0, 1));
    addKnob ("cgain",  "МЕЙКАП",   db, C (0, 1, 1));
    addKnob ("cmix",   "МИКС",     pct, C (0, 2, 1));
    addKnob ("rsize",  "РАЗМЕР",   pct, C (1, 0, 0));
    addKnob ("rdamp",  "ЗАТУХ.",   pct, C (1, 1, 0));
    addKnob ("rpre",   "ПРЕДЗАД.", ms0, C (1, 2, 0));
    addKnob ("rfizz",  "ФИЗЗ",     pct, C (1, 0, 1));
    addKnob ("rduck",  "ДАКИНГ",   pct, C (1, 1, 1));
    addKnob ("rmix",   "МИКС",     pct, C (1, 2, 1));
    addKnob ("sat",    "САТУР.",   pct, C (2, 0, 0));
    addKnob ("dist",   "ДИСТОРШН", pct, C (2, 0, 1));
    const int bx = 360 + 2 * 218 + 76;
    addChoice ("sattype",  { "ЛАМПА", "ЛЕНТА", "ТРАНС" }, 1, { bx, py + 30, 0, 0 }, 120, 26, 4, 81);
    addChoice ("disttype", { "КЛИП", "ФАЗЗ", "ФОЛД", "КРАШ" }, 2, { bx, py + 130, 0, 0 }, 58, 26, 4, 82);

    // банки-режимы
    const char* names[5] = { "ЛАЙМ", "ТРОПИК", "ГРОМ", "СИНИЙ", "ЧЁРНЫЙ" };
    juce::Image sprites[5] = { load (BinaryData::can_green_png, BinaryData::can_green_pngSize), load (BinaryData::can_baja_png, BinaryData::can_baja_pngSize),
                               load (BinaryData::can_purple_png, BinaryData::can_purple_pngSize), load (BinaryData::can_liberty_png, BinaryData::can_liberty_pngSize),
                               load (BinaryData::can_black_png, BinaryData::can_black_pngSize) };
    const juce::Colour glows[5] = { kLime, juce::Colour (0xff2de0c8), juce::Colour (0xffe0309a), juce::Colour (0xff2aa0ff), juce::Colour (0xffff4a2a) };
    modeAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter ("mode"), [this] (float v)
    { curMode = juce::jlimit (0, 4, juce::roundToInt (v)); for (size_t j = 0; j < modeCans.size(); ++j) modeCans[j]->setSelected ((int) j == curMode); repaint (360, 514, 640, 26); });
    for (int i = 0; i < 5; ++i)
    {
        auto mc = std::make_unique<ModeCan> (RU (names[i]), sprites[i], glows[i]);
        mc->onClick = [this, i] { modeAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (*mc); mc->setBounds (380 + i * 120, 366, 120, 150);
        modeCans.push_back (std::move (mc));
    }
    modeAtt->sendInitialUpdate();

    for (auto* bt : allButtons)
    {
        bt->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff08140c));
        bt->setColour (juce::TextButton::buttonOnColourId, kLime);
        bt->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        bt->setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    }
    setSize (W, H);
    startTimerHz (30);
}

void GazyavaEditor::mouseDown (const juce::MouseEvent& e)
{
    if (catRectL.contains (e.getPosition())) { proc.triggerMeow (0); meowAnim[0] = 1.f; }
    else if (catRectR.contains (e.getPosition())) { proc.triggerMeow (1); meowAnim[1] = 1.f; }
}

void GazyavaEditor::mouseMove (const juce::MouseEvent& e)
{
    const bool onCat = catRectL.contains (e.getPosition()) || catRectR.contains (e.getPosition());
    setMouseCursor (onCat ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void GazyavaEditor::timerCallback()
{
    auto toMeter = [] (float g) { return juce::jlimit (0.f, 1.f, (juce::Decibels::gainToDecibels (g, -48.f) + 48.f) / 48.f); };
    lvIn  = juce::jmax (toMeter (proc.inLevel.exchange (0.f)),  lvIn  * 0.88f);
    lvOut = juce::jmax (toMeter (proc.outLevel.exchange (0.f)), lvOut * 0.88f);
    const float bl = std::sqrt (juce::jlimit (0.f, 1.f, proc.bassLevel.exchange (0.f) * 0.8f));
    bassEnv = juce::jmax (bl, bassEnv * 0.78f);

    const float boostP = proc.apvts.getRawParameterValue ("boost")->load();
    const float rate = 6.f + boostP * 45.f + bassEnv * 140.f;
    tubeL.setLevel (0.16f + 0.84f * lvIn);  tubeR.setLevel (0.16f + 0.84f * lvOut);
    tubeL.tick (rate); tubeR.tick (rate);

    for (int i = 0; i < 2; ++i) { jx[i] = (rng.nextFloat() - 0.5f) * 2.f * bassEnv * 4.f; jy[i] = (rng.nextFloat() - 0.5f) * 2.f * bassEnv * 4.f; meowAnim[i] *= 0.86f; }
    for (auto& pf : puffs) { pf.p += pf.v; pf.v *= 0.97f; pf.r += 1.7f; pf.life -= 0.04f; }
    puffs.erase (std::remove_if (puffs.begin(), puffs.end(), [] (const Puff& pf) { return pf.life <= 0.f; }), puffs.end());
    if (bassEnv > 0.18f && puffs.size() < 140)
        for (int side = 0; side < 2; ++side)
        {
            auto r = side == 0 ? canL : canR;
            const float dir = side == 0 ? 1.f : -1.f;
            const int n = 1 + (int) (bassEnv * 3.f);
            for (int i = 0; i < n; ++i)
                puffs.push_back ({ { r.getCentreX() + dir * r.getWidth() * 0.35f, r.getY() + r.getHeight() * 0.40f + (rng.nextFloat() - 0.5f) * 40.f },
                                   { dir * (2.f + rng.nextFloat() * 5.f) * (0.5f + bassEnv), (rng.nextFloat() - 0.5f) * 3.f },
                                   8.f + rng.nextFloat() * 8.f, 1.f });
        }
    repaint (0, 0, 520, 540); repaint (W - 520, 0, 520, 540);
    repaint (catRectL.expanded (14)); repaint (catRectR.expanded (14));
}

void GazyavaEditor::renderBackground()
{
    bg = juce::Image (juce::Image::ARGB, W, H, true);
    juce::Graphics g (bg);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff07200f), 0, 0, juce::Colour (0xff010502), 0, (float) H, false)); g.fillAll();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1f7a2a).withAlpha (0.45f), W * 0.5f, 200, juce::Colours::transparentBlack, W * 0.5f, 520, true)); g.fillAll();
    g.setColour (kLime.withAlpha (0.6f)); g.drawRoundedRectangle (2.f, 2.f, (float) W - 4.f, (float) H - 4.f, 10.f, 2.f);

    auto head = [&g] (const juce::String& t, juce::Rectangle<int> r)
    { g.setColour (kLime); g.setFont (juce::Font (13.f, juce::Font::bold)); g.drawFittedText (t, r, juce::Justification::centred, 1); };
    head (RU ("БАС-БУСТЕР"), { 360, 70, 180, 20 });
    head (RU ("ВЫХОД"), { 820, 70, 180, 20 });
    head (RU ("РЕЖИМ БУСТЕРА — ТЫКАЙ В БАНКУ"), { 360, 348, 640, 18 });
    const char* titles[3] = { "КОМПРЕССОР", "РЕВЕРБ «ФИЗЗ»", "САТУРАЦИЯ + ДИСТОРШН" };
    for (int i = 0; i < 3; ++i)
    {
        juce::Rectangle<float> r (360.f + i * 218, 538.f, 204.f, 236.f);
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r, 10.f);
        g.setColour (kLime.withAlpha (0.45f)); g.drawRoundedRectangle (r, 10.f, 1.2f);
        head (RU (titles[i]), r.removeFromTop (24.f).toNearestInt());
    }
}

void GazyavaEditor::resized()
{
    const int cx = W / 2;
    canL = { 10, 34, 262, 478 }; canR = { W - 272, 34, 262, 478 };
    catRectL = { 10, 524, 262, 246 }; catRectR = { W - 272, 524, 262, 246 };
    bigS = imgBig.isValid() ? imgBig.rescaled (262, 478, juce::Graphics::highResamplingQuality) : juce::Image();
    const int D = (int) (canL.getWidth() * 0.80f);
    spkS = imgSpk.isValid() ? imgSpk.rescaled (D, D, juce::Graphics::highResamplingQuality) : juce::Image();
    catLS = imgCatL.isValid() ? imgCatL.rescaled (262, 246, juce::Graphics::highResamplingQuality) : juce::Image();
    catRS = imgCatR.isValid() ? imgCatR.rescaled (262, 246, juce::Graphics::highResamplingQuality) : juce::Image();

    juce::GlyphArrangement ga;
    ga.addFittedText (juce::Font (74.f, juce::Font::bold | juce::Font::italic), RU ("#ГАЗЯВА"), 0, 0, 560, 84, juce::Justification::centred, 1, 1.f);
    titlePath.clear(); ga.createPath (titlePath);
    titlePath.applyTransform (juce::AffineTransform::translation ((float) cx - 280.f, 0.f));
    renderBackground();
}

void GazyavaEditor::drawSideCan (juce::Graphics& g, juce::Rectangle<int> r, int side)
{
    const float ox = jx[side], oy = jy[side];
    const float cx = r.getCentreX() + ox, cy = r.getY() + r.getHeight() * 0.40f + oy;
    g.setGradientFill (juce::ColourGradient (kLime.withAlpha (0.08f + 0.5f * bassEnv), (float) r.getCentreX(), (float) r.getCentreY(),
                                             juce::Colours::transparentBlack, r.getCentreX() + r.getWidth() * 0.95f, (float) r.getCentreY(), true));
    g.fillEllipse (r.toFloat().expanded (r.getWidth() * 0.45f, r.getHeight() * 0.15f));
    g.setColour (juce::Colours::white);
    if (bigS.isValid()) g.drawImageAt (bigS, (int) (r.getX() + ox), (int) (r.getY() + oy));
    if (spkS.isValid())
    {
        const float D = (float) spkS.getWidth();
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (cx - D * 0.5f + 3, cy - D * 0.5f + 6, D, D);
        g.setColour (juce::Colours::white);
        g.drawImageAt (spkS, (int) (cx - D * 0.5f), (int) (cy - D * 0.5f));
        g.saveState();
        juce::Path clip; clip.addEllipse (cx - D * 0.34f, cy - D * 0.34f, D * 0.68f, D * 0.68f);
        g.reduceClipRegion (clip);
        g.setColour (juce::Colours::white);
        g.drawImageTransformed (spkS, juce::AffineTransform::translation (-D * 0.5f, -D * 0.5f).scaled (1.f + bassEnv * 0.32f).translated (cx, cy));
        g.restoreState();
        g.setColour (kLime.withAlpha (0.1f + bassEnv * 0.9f));
        g.drawEllipse (cx - D * 0.5f, cy - D * 0.5f, D, D, 1.5f + bassEnv * 3.5f);
    }
}

void GazyavaEditor::paint (juce::Graphics& g)
{
    if (bg.isValid()) { g.setColour (juce::Colours::white); g.drawImageAt (bg, 0, 0); }
    drawSideCan (g, canL, 0);
    drawSideCan (g, canR, 1);
    for (auto& pf : puffs)
    {
        const float a = juce::jlimit (0.f, 1.f, pf.life);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc8ff4a).withAlpha (0.55f * a), pf.p.x, pf.p.y,
                                                 juce::Colour (0xff5cc81a).withAlpha (0.f), pf.p.x + pf.r, pf.p.y, true));
        g.fillEllipse (pf.p.x - pf.r, pf.p.y - pf.r, pf.r * 2, pf.r * 2);
    }
    for (int i = 0; i < 2; ++i)
    {
        auto r = (i == 0 ? catRectL : catRectR).toFloat();
        const auto& img = i == 0 ? catLS : catRS;
        juce::Path clip; clip.addRoundedRectangle (r, 16.f);
        g.saveState(); g.reduceClipRegion (clip);
        g.setColour (juce::Colours::white);
        if (img.isValid()) g.drawImage (img, r.withSizeKeepingCentre (r.getWidth() * (1.f + 0.09f * meowAnim[i]), r.getHeight() * (1.f + 0.09f * meowAnim[i])), juce::RectanglePlacement::fillDestination);
        g.restoreState();
        g.setColour (kLime); g.strokePath (clip, juce::PathStrokeType (2.5f));
    }
    g.setColour (kRed);   g.strokePath (titlePath, juce::PathStrokeType (14.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), juce::AffineTransform::translation (5, 5));
    g.setColour (juce::Colours::black); g.strokePath (titlePath, juce::PathStrokeType (12.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4ffa0), 0, 10, juce::Colour (0xff6fd400), 0, 76, false)); g.fillPath (titlePath);

    g.setColour (kLime); g.setFont (juce::Font (13.f, juce::Font::italic));
    g.drawFittedText (RU (kModeDesc[curMode]), juce::Rectangle<int> (360, 516, 640, 20), juce::Justification::centred, 1);
}
