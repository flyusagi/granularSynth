/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             WavetableSynthTutorial
 version:          3.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Wavetable synthesiser.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             Component
 mainClass:        MainContentComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
//! [WavetableOscillator top]
class WavetableOscillator
{
public:
    WavetableOscillator (const juce::AudioSampleBuffer& wavetableToUse)
        : wavetable (wavetableToUse),
          tableSize (wavetable.getNumSamples() - 1)
    {
        jassert (wavetable.getNumChannels() == 1);
    }
//! [WavetableOscillator top]

//! [WavetableOscillator setFrequency]
    void setFrequency (float frequency, float sampleRate)
    {
        auto tableSizeOverSampleRate = (float) tableSize / sampleRate;
        tableDelta = frequency * tableSizeOverSampleRate;
    }
//! [WavetableOscillator setFrequency]

//! [WavetableOscillator getNextSample]
    forcedinline float getNextSample() noexcept
    {
        auto index0 = (unsigned int) currentIndex;
        auto index1 = index0 + 1;
//! [WavetableOscillator getNextSample]

        auto frac = currentIndex - (float) index0;

        auto* table = wavetable.getReadPointer (0);
        auto value0 = table[index0];
        auto value1 = table[index1];

        auto currentSample = value0 + frac * (value1 - value0);

        if ((currentIndex += tableDelta) > (float) tableSize)
            currentIndex -= (float) tableSize;

        return currentSample;
    }

//! [WavetableOscillator bottom]
private:
    const juce::AudioSampleBuffer& wavetable;
    const int tableSize;
    float currentIndex = 0.0f, tableDelta = 0.0f;
};
//! [WavetableOscillator bottom]

//==============================================================================
class MainContentComponent   : public juce::AudioAppComponent
                               
{
public:
    MainContentComponent()
    {
        addAndMakeVisible(openButton);
        openButton.setButtonText("FileOpen...");
        openButton.onClick = [this]{ openButtonClicked();};

        addAndMakeVisible(clearButton);
        clearButton.setButtonText("clear");
        clearButton.onClick=[this]{clearButtonClicked();};

        formatManager.registerBasicFormats();

        setSize (400, 400);
        setAudioChannels (0, 2); // no inputs, two outputs

    }

    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    void releaseResources()override
    {

    }

    void resized() override
    {
        openButton .setBounds (10, 10, getWidth() - 20, 20);
        clearButton.setBounds (10, 40, getWidth() - 20, 20);
    }

 /*   void timerCallback() override
    {
        auto cpu = deviceManager.getCpuUsage() * 100;
        cpuUsageText.setText (juce::String (cpu, 6) + " %", juce::dontSendNotification);
    } */

//! [MainContentComponent createWavetable top]
/*    void createWavetable()
    {
        sineTable.setSize (1, (int) tableSize + 1);
        auto* samples = sineTable.getWritePointer (0);


         auto angleDelta = juce::MathConstants<double>::twoPi / (double) (tableSize - 1);
         auto currentAngle = 0.0;

        for (unsigned int i = 0; i < tableSize; ++i)
        {
            auto sample = std::sin (currentAngle);
            samples[i] = (float) sample;
            currentAngle += angleDelta;
        }


        samples[tableSize] = samples[0];
    } */
//! [MainContentComponent createWavetable bottom]

    void prepareToPlay (int, double sampleRate) override
    {
/*        auto numberOfOscillators = 200;

        for (auto i = 0; i < numberOfOscillators; ++i)
        {
            auto* oscillator = new WavetableOscillator (sineTable);

            auto midiNote = juce::Random::getSystemRandom().nextDouble() * 36.0 + 48.0;
            auto frequency = 440.0 * pow (2.0, (midiNote - 69.0) / 12.0);

            oscillator->setFrequency ((float) frequency, (float) sampleRate);
            oscillators.add (oscillator);
        }

        level = 0.25f / (float) numberOfOscillators; */
    }



    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        auto* leftBuffer  = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
        auto* rightBuffer = bufferToFill.buffer->getWritePointer (1, bufferToFill.startSample);

        bufferToFill.clearActiveBufferRegion();

        for (auto oscillatorIndex = 0; oscillatorIndex < oscillators.size(); ++oscillatorIndex)
        {
            auto* oscillator = oscillators.getUnchecked (oscillatorIndex);

            for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
            {
                auto levelSample = oscillator->getNextSample() * level;

                leftBuffer[sample]  += levelSample;
                rightBuffer[sample] += levelSample;
            }
        }
    }

private:

 void openButtonClicked()
    {
        shutdownAudio();                                                                            // [1]

        chooser = std::make_unique<juce::FileChooser> ("Select a Wave file shorter than 2 seconds to play...",
                                                       juce::File{},
                                                       "*.wav");
        auto chooserFlags = juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();

            if (file == juce::File{})
                return;

            std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file)); // [2]

            if (reader.get() != nullptr)
            {
                auto duration = (float) reader->lengthInSamples / reader->sampleRate;               // [3]

                if (duration < 2)
                {
                    sineTable.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);  // [4]
                    reader->read (&fileBuffer,                                                      // [5]
                                  0,                                                                //  [5.1]
                                  (int) reader->lengthInSamples,                                    //  [5.2]
                                  0,                                                                //  [5.3]
                                  true,                                                             //  [5.4]
                                  true);                                                            //  [5.5]
                    position = 0;                                                                   // [6]
                    setAudioChannels (0, (int) reader->numChannels);                                // [7]
                }
                else
                {
                    // handle the error that the file is 2 seconds or longer..
                }
            }
        });
    }

    void clearButtonClicked()
    {
        shutdownAudio();
    }

 /* juce::Label cpuUsageLabel;
    juce::Label cpuUsageText;        CPU処理関連    */
    
    // ファイル関連
    juce::TextButton openButton;
    juce::TextButton clearButton;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::AudioFormatManager formatManager;
    juce::AudioSampleBuffer fileBuffer;
    int position;
    //ファイル関連

    const unsigned int tableSize = 1 << 7;
    float level = 0.0f;

    juce::AudioSampleBuffer sineTable;
    juce::OwnedArray<WavetableOscillator> oscillators;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
