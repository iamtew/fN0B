#pragma once

#include "dsp/Chain.h"
#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>
#include <atomic>

class fN0BAudioProcessor : public juce::AudioProcessor
{
public:
    fN0BAudioProcessor();
    ~fN0BAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "fN0B"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    bool supportsMPE() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    int presetIndex() const;
    float knobValue() const;

    // Editor takes the peak (linear 0..) and zeros it. Audio thread only raises it.
    float takePeakIn()  { return peakIn.exchange (0.f, std::memory_order_relaxed); }
    float takePeakOut() { return peakOut.exchange (0.f, std::memory_order_relaxed); }

private:
    void applySmoothedGain (juce::AudioBuffer<float>&, juce::SmoothedValue<float>&);
    void notePeak (std::atomic<float>& slot, const juce::AudioBuffer<float>&);

    fn0b::Chain chain;
    juce::SmoothedValue<float> inGainSmooth, outGainSmooth;
    std::atomic<float> peakIn { 0.f }, peakOut { 0.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (fN0BAudioProcessor)
};
