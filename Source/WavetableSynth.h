#pragma once
#include <random>
#include <cmath>

constexpr int grainSize = 44100 / 2 ;
constexpr int numOsc = 5;


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

        for (int i=0; i < numOsc; ++i){
            grain_oscs[i].cnt = (grainSize / numOsc) * i;
        }



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
    {}

    static float calcMado(float i, float j) {
        const double PI = 3.1415;
        return 0.5 - 0.5 * cos(2. * PI * i / (j-1));
    } 

    struct GrainOscillator{
        std::random_device rnd;
        float* rightBuffer;
        int cnt = 0;
        juce::AudioSampleBuffer grainTable;

        GrainOscillator(){
            grainTable.setSize(1, (int)grainSize + 1);
        }

        void getNext (const juce::AudioSourceChannelInfo& bufferToFill, juce::AudioSampleBuffer& fileBuffer){
            int signalVecSize = bufferToFill.buffer->getNumSamples();
            auto* grainBuffer = grainTable.getWritePointer (0,0);

            /*
            cnt 0 -> grainSize の区間で、こんなかんじの音量になるようにする
                  ______
               ／        \
              /           \
            _/             \_



            ハン窓 = 0.5-0.5cos2pi(t/T)
            */

            auto* leftBuffer  = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
            
            if(bufferToFill.buffer->getNumChannels() > 1){
                rightBuffer = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);
            }
          
            for (auto i = 0; i < signalVecSize; ++i)
            {
                leftBuffer[i] += grainBuffer[cnt] * calcMado(cnt, grainSize);
                
                if (bufferToFill.buffer->getNumChannels() > 1) {
                    rightBuffer[i] += grainBuffer[cnt] * calcMado(cnt, grainSize);
                }

                if(cnt == grainSize)
                {
                    auto start = rnd() % (fileBuffer.getNumSamples());
                    grainTable.copyFrom( 0,
                                        0,
                                        fileBuffer,
                                        0,
                                        start,
                                        grainTable.getNumSamples()); //grainTableにファイルをコピーする（どこからコピーするかはランダム）
                    std::cout << i << " | " << start << std::endl;
                    cnt = 0;
                }else{
                    ++cnt;
                }
            }
        }
    };


    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        if (!fileBufferReady || grain_oscs[0].grainTable.getNumChannels() != 1){
            return;
        }

        bufferToFill.clearActiveBufferRegion();

        for(int i=0; i < numOsc; ++i){
          
            grain_oscs[i].getNext(bufferToFill, fileBuffer);
        }


    }

private:

 void openButtonClicked()
    {
        shutdownAudio();                                                                            // [1]

        chooser = std::make_unique<juce::FileChooser> ("Select a Wave file",
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
                fileBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);  // [4]
                reader->read (&fileBuffer,                                                      // [5]
                                0,                                                                //  [5.1]
                                (int) reader->lengthInSamples,                                    //  [5.2]
                                0,                                                                //  [5.3]
                                true,                                                             //  [5.4]
                                true);                                                            //  [5.5]
                position = 0;                                                                   // [6]
                setAudioChannels (0, (int) reader->numChannels);                                // [7]
                fileBufferReady = true;
                
               
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
    bool fileBufferReady = false;

    float level = 0.0f;

    double rate;
    int startSample;

    // GrainOsc
    std::array<GrainOscillator, numOsc> grain_oscs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
