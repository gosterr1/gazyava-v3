#pragma once
#include "PluginProcessor.h"
#include <functional>

#define RU(s) juce::String::fromUTF8 (s)
using Fmt = std::function<juce::String (double)>;

// Большая ручка: настоящая крышка банки вращается вместе со значением
class LidKnob : public juce::Slider
{
public:
    LidKnob (juce::Image sprite, Fmt f);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    juce::Image src, scaled; Fmt fmt;
};

// Малая ручка: та же крышка, только меньше
class MiniKnob : public juce::Slider
{
public:
    MiniKnob (juce::String label, juce::Image sprite, Fmt f);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    juce::String label; juce::Image src, scaled; Fmt fmt;
};

// Банка-режим: клик выбирает режим бас-бустера
class ModeCan : public juce::Component
{
public:
    ModeCan (juce::String name, juce::Image sprite, juce::Colour glow);
    void setSelected (bool s) { selected = s; repaint(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    std::function<void()> onClick;
private:
    juce::String label; juce::Image src, scaled; juce::Colour glowC; bool selected = false;
};

// Колба с газировкой: чем больше баса, тем больше пузырьков
class TubeMeter : public juce::Component
{
public:
    void setLevel (float v) { level = v; }
    void tick (float spawnPerSec);
    void paint (juce::Graphics&) override;
private:
    struct Bubble { float x, y, r, vy, ph; };
    std::vector<Bubble> bubbles;
    juce::Random rng;
    float level = 0.f;
};

class GazyavaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GazyavaEditor (GazyavaProcessor&);
    ~GazyavaEditor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
private:
    struct Puff { juce::Point<float> p, v; float r, life; };
    struct ChoiceGroup { std::vector<std::unique_ptr<juce::TextButton>> btns; std::unique_ptr<juce::ParameterAttachment> att; };
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void drawSideCan (juce::Graphics&, juce::Rectangle<int>, int side);
    void renderBackground();
    MiniKnob* addKnob (const char* id, const char* label, Fmt f, juce::Rectangle<int> r);
    void addChoice (const char* id, std::vector<const char*> names, int cols, juce::Rectangle<int> area, int bw, int bh, int gap, int radio);

    GazyavaProcessor& proc;
    juce::Image imgBig, imgSpk, imgCatL, imgCatR, imgLid;
    juce::Image bigS, spkS, catLS, catRS, bg;
    juce::Rectangle<int> canL, canR, catRectL, catRectR;
    juce::Path titlePath;
    std::vector<Puff> puffs;
    juce::Random rng;
    float lvIn = 0.f, lvOut = 0.f, bassEnv = 0.f, jx[2] {}, jy[2] {}, meowAnim[2] {};
    int curMode = 0;

    LidKnob lid;
    TubeMeter tubeL, tubeR;
    std::vector<std::unique_ptr<MiniKnob>> knobs;
    std::vector<std::unique_ptr<ModeCan>> modeCans;
    std::vector<std::unique_ptr<ChoiceGroup>> groups;
    std::vector<juce::TextButton*> allButtons;
    juce::TextButton limBtn, monoBtn;

    std::unique_ptr<juce::ParameterAttachment> modeAtt;
    std::vector<std::unique_ptr<SA>> sliderAtts;
    std::unique_ptr<SA> aLid;
    std::unique_ptr<BA> aLim, aMono;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GazyavaEditor)
};
