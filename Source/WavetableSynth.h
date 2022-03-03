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
#include <random> 

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

        std::srand(time(NULL));

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
        grainTable.makeCopyOf(&fileBuffer);
    } */ 
//! [MainContentComponent createWavetable bottom]

    void prepareToPlay (int, double sampleRate) override
    {
        rate = sampleRate;
    }



    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        if (grainTable.getNumChannels() != 1) return;

        /*
            前提：
            ・grainTableが存在する。長さはtableSize+1
            ・grainTableCounterは、前回（grainTable分）どこまで再生したかを覚えておくためのカウンタ
            ・fileBufferにオーディオファイルが一本まるごと読み込まれている
            ・読み込みオーディオデータは、以下の条件を満たす。
                ・空白期間がない。
                ・単一のピッチである。
                ・そこそこ長いデータ（4秒以上くらいはあるといいかも）

            シグナルベクタ分のオーディオサンプルを生成してbufferToFillに書き込む。

            大まかな流れ：
            fileBufferからtableSize分読み込む。（最初のwavロード時にも1回読み込む）
            tableSize分再生して、再生しきったらfileBufferのランダムな位置からtableSize分コピーしてくる。
            これを繰り返す。

            sample     | 0 -> 1023 | 0 -> 1024 | 0 -> 1024 |
            sineBuffer | 0 -> 1023 | 1024 -> 2047 | 2048 -> ... | ... | ->tableSize (ロードし直し) 0 -> 
        */
        auto* leftBuffer  = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
       // auto* rightBuffer = bufferToFill.buffer->getWritePointer (1, bufferToFill.startSample);

        int signalVecSize = bufferToFill.buffer->getNumSamples();
        bufferToFill.clearActiveBufferRegion();

        auto* sineBuffer = grainTable.getWritePointer (0,0);

        for (auto sample = 0; sample < signalVecSize; ++sample)
        {
            leftBuffer[sample]  = sineBuffer[grainTableCounter];

            // [1] これを書く
            startSample = rand() % (grainTable.getNumSamples() - signalVecSize);
            // ランダムに設定、エリアの制約は必要（後ろとかだとバッファ突き抜けちゃう）


            // ここまでできたら、動作を確認して一度コミットする

            // [2]
            // それぞれのグレイン（grainTable）をフェードさせながらつないでいく
            // 位相が半分ずれた二つのtableを用意して、クロスフェードさせながらつなぐ
            /*
                こんなかんじ：
                ts = tableSize
                grainTable[0] | 0 -----> ts | 0 ------------------> ts | 
                grainTable[1] |     | 0 -------------> ts| 0 -----------------> ts |
                片方が真ん中くらいを再生している間に、もう片方が切り替え＆ロードを行う
                それぞれのtableは別個にデータを持つ
            */

            // [3]
            // grainTableを20個くらいにしてみる -> ここまでやると、だいぶグラニュラーっぽい音がでるはず！
            // grainTableに読み込むサンプル区間の長さをパラメータで制御できるようにしてみる
            
           
            if(grainTableCounter == tableSize)
            {
                grainTable.copyFrom( 0,
                                    0,
                                    fileBuffer,
                                    0,
                                    startSample,
                                    grainTable.getNumSamples());

                grainTableCounter = 0;
            }else{
                ++grainTableCounter;
            }
        }
    }

private:
 void createWaveTable()
    {
        auto numberOfOscillators = 200;

        for (auto i = 0; i < numberOfOscillators; ++i)
        {
            auto* oscillator = new WavetableOscillator (grainTable);

            auto midiNote = juce::Random::getSystemRandom().nextDouble() * 36.0 + 48.0;
            auto frequency = 440.0 * pow (2.0, (midiNote - 69.0) / 12.0);

            oscillator->setFrequency ((float) frequency, (float) rate);
            oscillators.add (oscillator);
        }

        level = 0.25f / (float) numberOfOscillators;
    }

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
                    fileBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);  // [4]
                    reader->read (&fileBuffer,                                                      // [5]
                                  0,                                                                //  [5.1]
                                  (int) reader->lengthInSamples,                                    //  [5.2]
                                  0,                                                                //  [5.3]
                                  true,                                                             //  [5.4]
                                  true);                                                            //  [5.5]
                    position = 0;                                                                   // [6]
                    setAudioChannels (0, (int) reader->numChannels);                                // [7]
                
                    grainTable.setSize(1, (int)tableSize + 1);

                    grainTable.copyFrom(0,
                        0,
                        fileBuffer,
                        0,
                        0,
                        grainTable.getNumSamples());

                    createWaveTable();
                    
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

    const int tableSize = 44100 / 4;
    float level = 0.0f;

    double rate;

    juce::AudioSampleBuffer grainTable;
    int grainTableCounter = 0;
    int startSample;

    juce::OwnedArray<WavetableOscillator> oscillators;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
