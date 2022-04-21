/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {

    }
};

struct ResponseCurveComponent : public juce::Component, public juce::AudioProcessorParameter::Listener, public juce::Timer
{

    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent();

    virtual void parameterValueChanged(int parameterIndex, float newValue) override;

    virtual void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override
    {

    }

    virtual void timerCallback() override;

    virtual void paint(juce::Graphics& g) override;

private:
    SimpleEQAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{ false };

    MonoChain monoChain;

};

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    // Constructor
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);

    // Destructor
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;

    CustomRotarySlider peakFreqSlider;
    CustomRotarySlider peakGainSlider;
    CustomRotarySlider peakQualitySlider;
    CustomRotarySlider lowCutFreqSlider;
    CustomRotarySlider highCutFreqSlider;
    CustomRotarySlider lowCutSlopeSlider;
    CustomRotarySlider highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    std::vector<juce::Component*> getComps();

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment;
    Attachment peakGainSliderAttachment;
    Attachment peakQualitySliderAttachment;
    Attachment lowCutFreqSliderAttachment;
    Attachment highCutFreqSliderAttachment;
    Attachment lowCutSlopeSliderAttachment;
    Attachment highCutSlopeSliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
