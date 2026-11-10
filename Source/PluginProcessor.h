#pragma once
#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Voice.h"

class OrangeSynthProcessor : public juce::AudioProcessor
{
public:
    OrangeSynthProcessor();
    ~OrangeSynthProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Orange Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return Orange::kNumPresets; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadPreset (int index);

    juce::AudioProcessorValueTreeState apvts;
    int currentProgram = 0;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleMidi (const juce::MidiMessage& m);
    void renderRange (float* left, float* right, int start, int end, const Orange::Patch& patch);
    void noteOn (int note, float velocity);
    void noteOff (int note);

    static constexpr int numVoices = 16;
    std::array<Orange::Voice, numVoices> voices;
    std::atomic<float>* raw[Orange::pCount] {};

    double sampleRate = 44100.0;
    float lfoPhase = 0.0f;
    float bendSemis = 0.0f;
    float lastFreq = 440.0f;
    bool sustainDown = false;
    bool pendingRelease[128] {};
    unsigned long long noteCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrangeSynthProcessor)
};
