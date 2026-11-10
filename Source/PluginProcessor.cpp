#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace Orange;

juce::AudioProcessorValueTreeState::ParameterLayout OrangeSynthProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < pCount; ++i)
    {
        const auto& d = kParams[i];
        juce::NormalisableRange<float> range (d.minV, d.maxV, d.step);
        if (d.centre > 0.0f)
            range.setSkewForCentre (d.centre);

        juce::String name = d.name;
        if (d.group == 2) name = "Amp " + name;
        if (d.group == 3) name = "Filter " + name;

        auto attrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            const float a = std::fabs (v);
            return juce::String (v, a >= 100.0f ? 0 : (a >= 10.0f ? 1 : 2));
        });

        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { d.id, 1 },
                                                                 name, range, d.defV, attrs));
    }
    return layout;
}

OrangeSynthProcessor::OrangeSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < pCount; ++i)
        raw[i] = apvts.getRawParameterValue (kParams[i].id);

    loadPreset (0);
}

void OrangeSynthProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    lfoPhase = 0.0f;
    for (auto& v : voices) v.kill();
}

bool OrangeSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void OrangeSynthProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, kNumPresets - 1, index);
    currentProgram = index;
    for (int i = 0; i < pCount; ++i)
        if (auto* p = apvts.getParameter (kParams[i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (kPresets[index].v[i]));
}

void OrangeSynthProcessor::setCurrentProgram (int index) { loadPreset (index); }

const juce::String OrangeSynthProcessor::getProgramName (int index)
{
    if (index < 0 || index >= kNumPresets) return {};
    return juce::String (kPresets[index].category) + ": " + kPresets[index].name;
}

void OrangeSynthProcessor::noteOn (int note, float velocity)
{
    pendingRelease[note] = false;

    Voice* target = nullptr;
    for (auto& v : voices)                       // retrigger same note
        if (v.note == note && v.active()) { target = &v; break; }
    if (target == nullptr)
        for (auto& v : voices)
            if (! v.active()) { target = &v; break; }
    if (target == nullptr)                       // steal the oldest
    {
        target = &voices[0];
        for (auto& v : voices) if (v.age < target->age) target = &v;
    }

    const bool noGlide = *raw[pGlide] < 0.0005f;
    target->start (note, velocity, lastFreq, noGlide, ++noteCounter);
    lastFreq = target->targetFreq;
}

void OrangeSynthProcessor::noteOff (int note)
{
    for (auto& v : voices)
        if (v.note == note && v.held)
        {
            if (sustainDown) pendingRelease[note] = true;
            else v.release();
        }
}

void OrangeSynthProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                      noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())                noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())             bendSemis = ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * 2.0f;
    else if (m.isController() && m.getControllerNumber() == 64)
    {
        sustainDown = m.getControllerValue() >= 64;
        if (! sustainDown)
            for (auto& v : voices)
                if (v.held && v.note >= 0 && pendingRelease[v.note]) { v.release(); pendingRelease[v.note] = false; }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        for (auto& v : voices) { if (m.isAllSoundOff()) v.kill(); else v.release(); }
}

void OrangeSynthProcessor::renderRange (float* left, float* right, int start, int end, const Patch& patch)
{
    const float sr = (float) sampleRate;
    const float lfoInc = patch[pLfoRate] / sr;
    const float vol = patch[pVolume];

    for (int s = start; s < end; ++s)
    {
        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        const float lfo = std::sin (6.2831853f * lfoPhase);

        float sum = 0.0f;
        for (auto& v : voices)
            if (v.active())
                sum += v.render (patch, sr, lfo, bendSemis);

        const float out = std::tanh (sum * vol * 2.0f);
        left[s] = out;
        if (right != nullptr) right[s] = out;
    }
}

void OrangeSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    Patch patch;
    for (int i = 0; i < pCount; ++i)
        patch.v[i] = raw[i]->load();

    const int n = buffer.getNumSamples();
    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    int pos = 0;
    for (const auto metadata : midi)
    {
        const int evPos = juce::jlimit (0, n, metadata.samplePosition);
        renderRange (left, right, pos, evPos, patch);
        pos = evPos;
        handleMidi (metadata.getMessage());
    }
    renderRange (left, right, pos, n, patch);
}

juce::AudioProcessorEditor* OrangeSynthProcessor::createEditor()
{
    return new OrangeSynthEditor (*this);
}

void OrangeSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OrangeSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType (apvts.state.getType()))
        {
            currentProgram = juce::jlimit (0, kNumPresets - 1, (int) tree.getProperty ("program", 0));
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrangeSynthProcessor();
}
