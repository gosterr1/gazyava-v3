#pragma once
#include <JuceHeader.h>

class GazyavaProcessor : public juce::AudioProcessor
{
public:
    GazyavaProcessor();
    ~GazyavaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Gazyava"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void triggerMeow (int which);   // 0 = левый манул, 1 = правый

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> inLevel { 0.f }, outLevel { 0.f }, bassLevel { 0.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static float shape (float x, float k, int type) noexcept;
    void updateTargets (bool snap);
    void loadMeow (int idx, const void* data, int size);
    float getP (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::dsp::LinkwitzRileyFilter<float> xover;
    juce::dsp::StateVariableTPTFilter<float> tone, revHp;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::Compressor<float> comp;
    juce::dsp::Reverb reverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 8192 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> revDelay { 16384 };
    juce::AudioBuffer<float> dryBuf, compBuf, revBuf;
    juce::SmoothedValue<float> sIn, sDrive, sBoost, sSub, sCrunch, sWidth, sSat, sDist, sOvl, sMix, sOut;
    double sampleRate = 44100.0;

    // состояние саб-генератора, "тайта", краша, физза
    float subEnv[2] {}, subL1[2] {}, subL2[2] {}, tz1[2] {}, tz2[2] {}, held[2] {};
    int   subSgn[2] { 1, 1 }, crushCnt[2] {};
    bool  subFlip[2] {};
    float fizzSm = 0.f, fizzTarget = 0.f, revLp[2] {}, duckEnv = 0.f;
    int   fizzCnt = 0;
    juce::Random rnd;

    // мяуканье
    juce::AudioBuffer<float> meow[2];
    double meowRate[2] { 44100.0, 44100.0 }, voicePos = 0.0, voiceStep = 1.0;
    std::atomic<int> meowReq { -1 };
    std::atomic<float> meowPitch { 1.f };
    int voice = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GazyavaProcessor)
};
