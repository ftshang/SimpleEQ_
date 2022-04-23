/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height);

    g.setColour(juce::Colour(36, 35, 55));
    g.fillEllipse(bounds);

    g.setColour(juce::Colour(56, 31, 155));
    g.drawEllipse(bounds, 1.f);

    if (auto* rotslidewlab = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        juce::Path p;

        juce::Rectangle<float> rec;
        rec.setLeft(center.getX() - 2);
        rec.setRight(center.getX() + 2);
        rec.setTop(bounds.getY());
        rec.setBottom(center.getY() - rotslidewlab->getTextHeight() * 1.5);

        p.addRoundedRectangle(rec, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngleRadians = juce::jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(juce::AffineTransform().rotated(sliderAngleRadians, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rotslidewlab->getTextHeight());
        auto text = rotslidewlab->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        rec.setSize(strWidth + 4, rotslidewlab->getTextHeight() + 2);
        rec.setCentre(center);

        g.setColour(juce::Colours::black);
        g.fillRect(rec);

        g.setColour(juce::Colours::white);
        g.drawFittedText(text, rec.toNearestInt(), juce::Justification::centred, 1);
    }
}



// ======================================================================== //

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    auto startAngle = juce::degreesToRadians(180.f + 45.f);
    auto endAngle = juce::degreesToRadians(180.f - 45.f) + juce::MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    //g.setColour(juce::Colours::red);
    //g.drawRect(getLocalBounds());
    //g.setColour(juce::Colours::yellow);
    //g.drawRect(sliderBounds);

    getLookAndFeel().drawRotarySlider(g, 
                                      sliderBounds.getX(), 
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(), 
                                      sliderBounds.getHeight(), 
                                      juce::jmap(getValue(), 
                                      range.getStart(), 
                                      range.getEnd(), 
                                      0.0, 1.0), 
                                      startAngle,
                                      endAngle, 
                                      *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(juce::Colour(36, 35, 55));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        auto angle = juce::jmap(pos, 0.f, 1.f, startAngle, endAngle);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, angle);

        juce::Rectangle<float> rec;
        auto str = labels[i].label;
        rec.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        rec.setCentre(c);
        rec.setY(rec.getY() + getTextHeight());

        g.drawFittedText(str, rec.toNearestInt(), juce::Justification::centred, 1);
    }

}
juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;

    juce::Rectangle<int> rec;
    rec.setSize(size, size);
    rec.setCentre(bounds.getCentreX(), 0);
    rec.setY(2);

    return rec;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        return choiceParam->getCurrentChoiceName();
    }

    juce::String str;
    bool addKiloHertz = false;
    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();
        if (val > 999.9f)
        {
            val /= 1000.f;
            addKiloHertz = true;
        }

        str = juce::String(val, (addKiloHertz ? 2 : 0));
    }
    else 
    {
        jassertfalse;
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addKiloHertz)
        {
            str << "k";
        }
        str << suffix;
    }

    return str;
}
// ===================================================================================== //
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    updateChain();
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        //update the monochain
        updateChain();
        // signal a repaint
        repaint();
    }
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.audProcValTS);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::aqua);
    auto responseArea = getLocalBounds();

    auto width = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> magnitudes;

    magnitudes.resize(width);

    for (int i = 0; i < width; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(static_cast<double>(i) / static_cast<double>(width), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak>())
        {
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!lowcut.isBypassed<0>())
        {
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!lowcut.isBypassed<1>())
        {
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!lowcut.isBypassed<2>())
        {
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!lowcut.isBypassed<3>())
        {
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!highcut.isBypassed<0>())
        {
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!highcut.isBypassed<1>())
        {
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!highcut.isBypassed<2>())
        {
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highcut.isBypassed<3>())
        {
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        magnitudes[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(magnitudes.front()));

    for (size_t i = 1; i < magnitudes.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(magnitudes[i]));
    }

    g.setColour(Colours::indigo);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

    g.setColour(Colours::black);
    g.strokePath(responseCurve, PathStrokeType(2.f));

}


//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor(SimpleEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    peakFreqSlider(*audioProcessor.audProcValTS.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.audProcValTS.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.audProcValTS.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.audProcValTS.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.audProcValTS.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.audProcValTS.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.audProcValTS.getParameter("HighCut Slope"), "dB/Oct"),

    responseCurveComponent(audioProcessor),
    peakFreqSliderAttachment(audioProcessor.audProcValTS, "Peak Freq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.audProcValTS, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.audProcValTS, "Peak Quality", peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.audProcValTS, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.audProcValTS, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.audProcValTS, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.audProcValTS, "HighCut Slope", highCutSlopeSlider)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    peakFreqSlider.labels.add({ 0.f, "20Hz" });
    peakFreqSlider.labels.add({ 1.f, "20kHz" });
    peakGainSlider.labels.add({ 0.f, "-24dB" });
    peakGainSlider.labels.add({ 1.f, "24dB" });
    peakQualitySlider.labels.add({ 0.f, "0.1" });
    peakQualitySlider.labels.add({ 1.f, "10.0" });
    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20kHz" });
    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20kHz" });
    lowCutSlopeSlider.labels.add({ 0.0f, "12" });
    lowCutSlopeSlider.labels.add({ 1.f, "48" });
    highCutSlopeSlider.labels.add({ 0.0f, "12" });
    highCutSlopeSlider.labels.add({ 1.f, "48" });

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    setSize (600, 400);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{

}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::aqua);

}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    float hRatio = 20.f / 100.f; //JUCE_LIVE_CONSTANT(25) / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(5);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);

    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };

}