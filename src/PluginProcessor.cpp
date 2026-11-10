#include "PluginProcessor.h"
#include "PluginEditor.h"

fN0BAudioProcessor::fN0BAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

void fN0BAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    chain.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    inGainSmooth.reset (sampleRate, 0.02);
    outGainSmooth.reset (sampleRate, 0.02);
    inGainSmooth.setCurrentAndTargetValue (1.f);
    outGainSmooth.setCurrentAndTargetValue (1.f);
}

bool fN0BAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    // An effect: what comes in must match what goes out.
    return layouts.getMainInputChannelSet() == out;
}

int fN0BAudioProcessor::presetIndex() const
{
    if (auto* p = apvts.getRawParameterValue (ParamID::preset))
        return juce::jlimit (0, fn0b::kPresetCount - 1, juce::roundToInt (p->load()));
    return 0;
}

float fN0BAudioProcessor::knobValue() const
{
    if (auto* p = apvts.getRawParameterValue (ParamID::knob))
        return juce::jlimit (0.f, 1.f, p->load());
    return 0.f;
}

void fN0BAudioProcessor::applySmoothedGain (juce::AudioBuffer<float>& buffer,
                                           juce::SmoothedValue<float>& sm)
{
    const int n = buffer.getNumSamples();
    const int chans = buffer.getNumChannels();
    if (! sm.isSmoothing())
    {
        buffer.applyGain (sm.getCurrentValue());
        return;
    }
    for (int i = 0; i < n; ++i)
    {
        const float g = sm.getNextValue();
        for (int ch = 0; ch < chans; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }
}

void fN0BAudioProcessor::notePeak (std::atomic<float>& slot, const juce::AudioBuffer<float>& buffer)
{
    const float p = buffer.getMagnitude (0, buffer.getNumSamples());
    float cur = slot.load (std::memory_order_relaxed);
    while (p > cur && ! slot.compare_exchange_weak (cur, p, std::memory_order_relaxed))
        {}
}

void fN0BAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    float inDb = 0.f, outDb = 0.f;
    if (auto* p = apvts.getRawParameterValue (ParamID::inputGain))
        inDb = p->load();
    if (auto* p = apvts.getRawParameterValue (ParamID::outputGain))
        outDb = p->load();

    inGainSmooth.setTargetValue (juce::Decibels::decibelsToGain (inDb));
    outGainSmooth.setTargetValue (juce::Decibels::decibelsToGain (outDb));

    applySmoothedGain (buffer, inGainSmooth);
    notePeak (peakIn, buffer);

    chain.process (buffer, presetIndex(), knobValue());

    applySmoothedGain (buffer, outGainSmooth);
    notePeak (peakOut, buffer);
}

void fN0BAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void fN0BAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* fN0BAudioProcessor::createEditor()
{
    return new fN0BAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new fN0BAudioProcessor();
}
