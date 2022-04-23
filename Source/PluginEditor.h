/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeel : public juce::LookAndFeel_V4
{
    virtual void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slida) override
    {

    }
};


struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) : param{ &rap }, suffix{ unitSuffix }, juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, 
        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        setLookAndFeel(&looknFeel);
    }

    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }


    void paint(juce::Graphics& g) override
    {

    }

    juce::Rectangle<int> getSliderBounds() const;

    int getTextHeight() const
    {
        return 14;
    }

    juce::String getDisplayString() const;

private:
    LookAndFeel looknFeel;

    juce::RangedAudioParameter* param;
    juce::String suffix;
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

    RotarySliderWithLabels peakFreqSlider;
    RotarySliderWithLabels peakGainSlider;
    RotarySliderWithLabels peakQualitySlider;
    RotarySliderWithLabels lowCutFreqSlider;
    RotarySliderWithLabels highCutFreqSlider;
    RotarySliderWithLabels lowCutSlopeSlider;
    RotarySliderWithLabels highCutSlopeSlider;

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
