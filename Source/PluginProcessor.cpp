#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    const int kModeShape[5] = { 0, 1, 2, 4, 5 };   // лайм / тропик / гром / синий / чёрный
    const int kSatShape[3]  = { 0, 1, 4 };
}

juce::AudioProcessorValueTreeState::ParameterLayout GazyavaProcessor::createLayout()
{
    using NR = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&p] (const char* id, const char* name, NR r, float def)
    { p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def)); };
    auto skew = [] (float lo, float hi, float step, float centre) { NR r (lo, hi, step); r.setSkewForCentre (centre); return r; };

    add ("in",       "Input",        NR (-12.f, 12.f, 0.01f), 0.f);
    add ("drive",    "Drive",        NR (0.f, 1.f, 0.001f), 0.4f);
    add ("boost",    "Bass Boost",   NR (0.f, 1.f, 0.001f), 0.4f);
    add ("sub",      "Sub",          NR (0.f, 1.f, 0.001f), 0.f);
    add ("freq",     "Crossover",    skew (40.f, 300.f, 0.1f, 100.f), 100.f);
    add ("tight",    "Tight",        NR (0.f, 1.f, 0.001f), 0.3f);
    add ("crunch",   "Crunch",       NR (0.f, 1.f, 0.001f), 0.2f);
    add ("ice",      "Ice Tone",     NR (0.f, 1.f, 0.001f), 0.f);
    add ("width",    "Width",        NR (0.f, 1.f, 0.001f), 0.5f);
    add ("overload", "Overload",     NR (0.f, 1.f, 0.001f), 0.f);
    add ("mix",      "Mix",          NR (0.f, 1.f, 0.001f), 1.f);
    add ("out",      "Output",       NR (-18.f, 6.f, 0.01f), 0.f);
    add ("sat",      "Saturation",   NR (0.f, 1.f, 0.001f), 0.f);
    add ("dist",     "Distortion",   NR (0.f, 1.f, 0.001f), 0.f);
    add ("cthr",     "Comp Thresh",  NR (-40.f, 0.f, 0.1f), -14.f);
    add ("cratio",   "Comp Ratio",   skew (1.f, 20.f, 0.1f, 4.f), 3.f);
    add ("catk",     "Comp Attack",  skew (1.f, 100.f, 0.1f, 20.f), 10.f);
    add ("crel",     "Comp Release", skew (20.f, 500.f, 1.f, 120.f), 120.f);
    add ("cgain",    "Comp Makeup",  NR (0.f, 12.f, 0.1f), 0.f);
    add ("cmix",     "Comp Mix",     NR (0.f, 1.f, 0.001f), 0.f);
    add ("rsize",    "Reverb Size",  NR (0.f, 1.f, 0.001f), 0.5f);
    add ("rdamp",    "Reverb Damp",  NR (0.f, 1.f, 0.001f), 0.5f);
    add ("rpre",     "Reverb Predelay", NR (0.f, 100.f, 0.1f), 20.f);
    add ("rfizz",    "Reverb Fizz",  NR (0.f, 1.f, 0.001f), 0.4f);
    add ("rduck",    "Reverb Duck",  NR (0.f, 1.f, 0.001f), 0.5f);
    add ("rmix",     "Reverb Mix",   NR (0.f, 1.f, 0.001f), 0.f);
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Bass Mode",
                 juce::StringArray { "Lime", "Tropic", "Thunder", "Blue", "Black" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "sattype", 1 }, "Sat Type",
                 juce::StringArray { "Tube", "Tape", "Transformer" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "disttype", 1 }, "Dist Type",
                 juce::StringArray { "Hard", "Fuzz", "Fold", "Crush" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "limiter", 1 }, "Limiter", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "monobass", 1 }, "Mono Bass", false));
    return { p.begin(), p.end() };
}

GazyavaProcessor::GazyavaProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GAZYAVA", createLayout())
{
    loadMeow (0, BinaryData::meow1_wav, BinaryData::meow1_wavSize);
    loadMeow (1, BinaryData::meow2_wav, BinaryData::meow2_wavSize);
}

void GazyavaProcessor::loadMeow (int idx, const void* data, int size)
{
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatReader> r (wav.createReaderFor (new juce::MemoryInputStream (data, (size_t) size, false), true));
    if (r == nullptr) return;
    meow[idx].setSize (1, (int) r->lengthInSamples);
    r->read (&meow[idx], 0, (int) r->lengthInSamples, 0, true, false);
    meowRate[idx] = r->sampleRate;
}

void GazyavaProcessor::triggerMeow (int which)
{
    meowPitch.store (0.93f + rnd.nextFloat() * 0.16f);
    meowReq.store (which);
}

bool GazyavaProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float GazyavaProcessor::shape (float x, float k, int type) noexcept
{
    switch (type)
    {
        case 0:  return (std::tanh (k * x + 0.2f) - 0.19738f) / std::tanh (k * 0.9f);                        // лампа
        case 1:  return (std::atan (k * x * 1.2f + 0.15f) - 0.14889f) / std::atan (k * 1.2f);                // лента
        case 2:  { const float a = 1.f - std::exp (-std::abs (x) * k * 1.4f);                                // фазз
                   return (x < 0 ? -a : a) / (1.f - std::exp (-k * 1.4f)); }
        case 3:  return std::sin (x * (1.f + k * 0.35f) * 1.2f);                                             // фолд
        case 4:  { const float g = 0.6f + k * 0.35f; return std::tanh (x * g) / std::tanh (g); }             // мягкий/чистый
        default: return juce::jlimit (-1.f, 1.f, x * (1.f + k * 0.8f)) * 0.9f;                               // жёсткий клип
    }
}

void GazyavaProcessor::updateTargets (bool snap)
{
    auto set = [snap] (juce::SmoothedValue<float>& s, float v)
    { if (snap) s.setCurrentAndTargetValue (v); else s.setTargetValue (v); };
    set (sIn,     juce::Decibels::decibelsToGain (getP ("in")));
    set (sDrive,  getP ("drive"));
    set (sBoost,  juce::Decibels::decibelsToGain (getP ("boost") * 18.f));
    set (sSub,    getP ("sub"));
    set (sCrunch, getP ("crunch"));
    set (sWidth,  getP ("width") * 2.f);
    set (sSat,    getP ("sat"));
    set (sDist,   getP ("dist"));
    set (sOvl,    getP ("overload"));
    set (sMix,    getP ("mix"));
    set (sOut,    juce::Decibels::decibelsToGain (getP ("out")));
}

void GazyavaProcessor::prepareToPlay (double sr, int spb)
{
    sampleRate = sr;
    const double osr = sr * 4.0;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) spb, 2 };
    juce::dsp::ProcessSpec ospec { osr, (juce::uint32) (spb * 4), 2 };

    oversampling.initProcessing ((size_t) spb);
    oversampling.reset();
    xover.prepare (ospec);
    tone.prepare (spec);   tone.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    revHp.prepare (spec);  revHp.setType (juce::dsp::StateVariableTPTFilterType::highpass); revHp.setCutoffFrequency (180.f);
    limiter.prepare (spec); limiter.setThreshold (-0.5f); limiter.setRelease (60.f);
    comp.prepare (spec);
    reverb.prepare (spec);
    revDelay.prepare (spec);

    const int lat = (int) std::round (oversampling.getLatencyInSamples());
    setLatencySamples (lat);
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) lat);
    dryBuf.setSize (2, spb); compBuf.setSize (2, spb); revBuf.setSize (2, spb);

    for (auto* s : { &sIn, &sDrive, &sBoost, &sSub, &sCrunch, &sWidth, &sSat, &sDist, &sOvl }) s->reset (osr, 0.02);
    for (auto* s : { &sMix, &sOut }) s->reset (sr, 0.03);
    updateTargets (true);
    for (int c = 0; c < 2; ++c) { subEnv[c] = subL1[c] = subL2[c] = tz1[c] = tz2[c] = held[c] = revLp[c] = 0.f; subSgn[c] = 1; }
}

void GazyavaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0) return;

    updateTargets (false);
    const int  mode     = juce::jlimit (0, 4, juce::roundToInt (getP ("mode")));
    const int  satT     = juce::jlimit (0, 2, juce::roundToInt (getP ("sattype")));
    const int  distT    = juce::jlimit (0, 3, juce::roundToInt (getP ("disttype")));
    const bool monoBass = getP ("monobass") > 0.5f;
    const bool limOn    = getP ("limiter") > 0.5f;
    const float ice = getP ("ice"), tight = getP ("tight"), distP = getP ("dist");
    xover.setCutoffFrequency (getP ("freq"));

    const float inPk = juce::jmax (buffer.getMagnitude (0, 0, n), buffer.getMagnitude (1, 0, n));
    inLevel.store (juce::jmax (inLevel.load(), inPk));

    dryBuf.setSize (2, n, false, false, true);
    for (int c = 0; c < 2; ++c)
    {
        auto* src = buffer.getReadPointer (c);
        auto* d = dryBuf.getWritePointer (c);
        for (int i = 0; i < n; ++i) { dryDelay.pushSample (c, src[i]); d[i] = dryDelay.popSample (c); }
    }

    // ---------- нелинейная часть на 4x oversampling ----------
    const float osr = (float) (sampleRate * 4.0), twoPi = juce::MathConstants<float>::twoPi;
    const float aSub  = 1.f - std::exp (-twoPi * 70.f / osr);
    const float aEnvA = 1.f - std::exp (-1.f / (0.002f * osr)), aEnvR = 1.f - std::exp (-1.f / (0.08f * osr));
    const float aTight = 1.f - std::exp (-twoPi * (18.f + tight * 55.f) / osr);
    const int   crushN = 1 + (int) (distP * 14.f);
    const float crushLv = std::pow (2.f, 12.f - distP * 9.f);
    const int   mShape = kModeShape[mode], sShape = kSatShape[satT];

    juce::dsp::AudioBlock<float> blk (buffer);
    auto sub = blk.getSubsetChannelBlock (0, 2).getSubBlock (0, (size_t) n);
    auto up = oversampling.processSamplesUp (sub);
    float* ch[2] = { up.getChannelPointer (0), up.getChannelPointer (1) };
    const int nu = (int) up.getNumSamples();
    float bassPk = 0.f;

    for (int i = 0; i < nu; ++i)
    {
        const float inG = sIn.getNextValue(), dr = sDrive.getNextValue(), bg = sBoost.getNextValue(), sb = sSub.getNextValue(),
                    cr = sCrunch.getNextValue(), wd = sWidth.getNextValue(), sa = sSat.getNextValue(), di = sDist.getNextValue(),
                    ov = sOvl.getNextValue();
        const float k = (1.f + dr * 14.f) * (mode == 2 ? 1.2f : 1.f);
        const float subAmt = juce::jmin (1.f, sb + (mode == 3 ? 0.45f : 0.f));
        float lo[2], hi[2];
        for (int c = 0; c < 2; ++c) xover.processSample (c, ch[c][i] * inG, lo[c], hi[c]);
        if (monoBass) lo[0] = lo[1] = 0.5f * (lo[0] + lo[1]);
        { const float m = 0.5f * (hi[0] + hi[1]), s = 0.5f * (hi[0] - hi[1]) * wd; hi[0] = m + s; hi[1] = m - s; }

        for (int c = 0; c < 2; ++c)
        {
            const float x = lo[c];
            // саб-октава: делитель частоты по переходам через ноль
            const float ax = std::abs (x);
            subEnv[c] += (ax - subEnv[c]) * (ax > subEnv[c] ? aEnvA : aEnvR);
            const int ns = x > 0.002f ? 1 : (x < -0.002f ? -1 : subSgn[c]);
            if (ns == 1 && subSgn[c] == -1) subFlip[c] = ! subFlip[c];
            subSgn[c] = ns;
            subL1[c] += aSub * ((subFlip[c] ? 1.f : -1.f) * subEnv[c] - subL1[c]);
            subL2[c] += aSub * (subL1[c] - subL2[c]);

            float s = shape (x * bg, k, mShape) + subL2[c] * subAmt * 1.2f;   // буст ДО сатуратора: уровень ограничен
            tz1[c] += aTight * (s - tz1[c]); s -= tz1[c];                       // тайт: срез инфранизов
            tz2[c] += aTight * (s - tz2[c]); s -= tz2[c];
            bassPk = juce::jmax (bassPk, std::abs (s));

            float h = hi[c];
            if (cr > 0.001f) { const float hs = shape (h * (1.f + cr * 3.f), 1.f + cr * 10.f, 0); h += (hs - h) * cr; }
            float y = s + h;

            if (sa > 0.001f) { const float ws = shape (y * (1.f + sa * 1.5f), 1.f + sa * 6.f, sShape); y += (ws - y) * sa; }

            if (di > 0.001f)
            {
                float d;
                switch (distT)
                {
                    case 0:  d = shape (y, 1.f + di * 30.f, 5); break;
                    case 1:  d = shape (y * (1.f + di * 4.f), 1.f + di * 18.f, 2); break;
                    case 2:  d = shape (y, 1.f + di * 18.f, 3); break;
                    default: if (++crushCnt[c] >= crushN) { crushCnt[c] = 0; held[c] = y; }
                             d = std::round (held[c] * crushLv) / crushLv; break;
                }
                y += (d - y) * juce::jmin (1.f, di * 2.5f);
            }
            if (ov > 0.001f)
            {
                const float z = juce::jlimit (-1.f, 1.f, y * (1.f + ov * 18.f));
                y += (z * 0.85f - y) * juce::jmin (1.f, ov * 3.f);
            }
            ch[c][i] = y;
        }
    }
    bassLevel.store (juce::jmax (bassLevel.load(), bassPk));
    oversampling.processSamplesDown (sub);

    // ---------- тон ----------
    if (ice > 0.01f)
    {
        tone.setCutoffFrequency (juce::jmin (20000.f * std::pow (0.075f, ice), 0.45f * (float) sampleRate));
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        tone.process (ctx);
    }

    // ---------- компрессор (параллельный) ----------
    const float cmix = getP ("cmix");
    if (cmix > 0.001f)
    {
        comp.setThreshold (getP ("cthr")); comp.setRatio (getP ("cratio"));
        comp.setAttack (getP ("catk"));    comp.setRelease (getP ("crel"));
        compBuf.setSize (2, n, false, false, true);
        for (int c = 0; c < 2; ++c) compBuf.copyFrom (c, 0, buffer, c, 0, n);
        juce::dsp::AudioBlock<float> cb (compBuf);
        juce::dsp::ProcessContextReplacing<float> cctx (cb);
        comp.process (cctx);
        const float mk = juce::Decibels::decibelsToGain (getP ("cgain"));
        for (int c = 0; c < 2; ++c)
        {
            auto* o = buffer.getWritePointer (c); auto* cp = compBuf.getReadPointer (c);
            for (int i = 0; i < n; ++i) o[i] = o[i] * (1.f - cmix) + cp[i] * mk * cmix;
        }
    }

    // ---------- реверб "физз": газированный хвост + дакинг ----------
    const float rmix = getP ("rmix");
    if (rmix > 0.001f)
    {
        revBuf.setSize (2, n, false, false, true);
        for (int c = 0; c < 2; ++c) revBuf.copyFrom (c, 0, buffer, c, 0, n);
        juce::dsp::AudioBlock<float> rb (revBuf);
        juce::dsp::ProcessContextReplacing<float> rctx (rb);
        revHp.process (rctx);                                     // бас в реверб не уходит
        revDelay.setDelay (getP ("rpre") * 0.001f * (float) sampleRate);
        for (int c = 0; c < 2; ++c)
        {
            auto* r = revBuf.getWritePointer (c);
            for (int i = 0; i < n; ++i) { revDelay.pushSample (c, r[i]); r[i] = revDelay.popSample (c); }
        }
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = 0.3f + getP ("rsize") * 0.7f; rp.damping = getP ("rdamp");
        rp.wetLevel = 0.4f; rp.dryLevel = 0.f; rp.width = 1.f; rp.freezeMode = 0.f;
        reverb.setParameters (rp);
        reverb.process (rctx);

        const float fz = getP ("rfizz"), dk = getP ("rduck");
        for (int i = 0; i < n; ++i)
        {
            if (--fizzCnt <= 0) { fizzCnt = (int) ((float) sampleRate / (900.f + rnd.nextFloat() * 1800.f)); fizzTarget = rnd.nextFloat(); }
            fizzSm += 0.2f * (fizzTarget - fizzSm);
            const float gate = 1.f - fz * 0.85f * fizzSm;
            const float ain = std::abs (buffer.getSample (0, i)) + std::abs (buffer.getSample (1, i));
            duckEnv += (ain - duckEnv) * (ain > duckEnv ? 0.02f : 0.0008f);
            const float dg = 1.f - dk * juce::jlimit (0.f, 1.f, duckEnv * 1.5f);
            for (int c = 0; c < 2; ++c)
            {
                float r = revBuf.getSample (c, i);
                revLp[c] += 0.15f * (r - revLp[c]);
                r = (r + fz * 0.8f * (r - revLp[c])) * gate;       // физз: хруст/пузыри в хвосте
                buffer.addSample (c, i, r * rmix * dg * 1.4f);
            }
        }
    }

    // ---------- микс / выход ----------
    for (int i = 0; i < n; ++i)
    {
        const float m = sMix.getNextValue(), og = sOut.getNextValue();
        for (int c = 0; c < 2; ++c)
        {
            auto* o = buffer.getWritePointer (c);
            o[i] = (dryBuf.getSample (c, i) * (1.f - m) + o[i] * m) * og;
        }
    }
    if (limOn) { juce::dsp::ProcessContextReplacing<float> ctx (sub); limiter.process (ctx); }

    // ---------- мяу ----------
    const int req = meowReq.exchange (-1);
    if (req >= 0 && req < 2 && meow[req].getNumSamples() > 2)
    { voice = req; voicePos = 0.0; voiceStep = meowRate[req] / sampleRate * (double) meowPitch.load(); }
    if (voice >= 0)
    {
        const int len = meow[voice].getNumSamples();
        const float* src = meow[voice].getReadPointer (0);
        for (int i = 0; i < n; ++i)
        {
            const int ip = (int) voicePos;
            if (ip >= len - 1) { voice = -1; break; }
            const float fr = (float) (voicePos - ip), v = (src[ip] * (1.f - fr) + src[ip + 1] * fr) * 0.9f;
            buffer.addSample (0, i, v); buffer.addSample (1, i, v);
            voicePos += voiceStep;
        }
    }

    const float outPk = juce::jmax (buffer.getMagnitude (0, 0, n), buffer.getMagnitude (1, 0, n));
    outLevel.store (juce::jmax (outLevel.load(), outPk));
}

juce::AudioProcessorEditor* GazyavaProcessor::createEditor() { return new GazyavaEditor (*this); }

void GazyavaProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void GazyavaProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GazyavaProcessor(); }
