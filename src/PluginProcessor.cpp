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

void fN0BAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    chain.process (buffer, presetIndex(), knobValue());
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
