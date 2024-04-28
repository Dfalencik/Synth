#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Sampler.h"
#include "SynthVoice.h"
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

NewProjectAudioProcessor::NewProjectAudioProcessor()
: AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    initializeSampleDirectory();  // Correctly placed within the constructor body
}


juce::AudioProcessorValueTreeState::ParameterLayout NewProjectAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Adding parameters to audio processor
    layout.add(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("filterCutoff", "Filter Cutoff", 20.0f, 20000.0f, 2000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("filterResonance", "Filter Resonance", 0.1f, 10.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("lfoRate", "LFO Rate", 0.1f, 20.0f, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("lfoDepth", "LFO Depth", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("attack", "Attack", 0.1f, 5.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", 0.1f, 5.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain", 0.0f, 1.0f, 0.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("release", "Release", 0.1f, 5.0f, 1.5f));
    
    return layout;  // Return the fully configured layout
}


void NewProjectAudioProcessor::initializeSampleDirectory() {
    juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    juce::File sampleDir = appDataDir.getChildFile("NewProject/SAMPLES");
    if (!sampleDir.exists()) {
        sampleDir.createDirectory();
    }
    // Continue with your file operations
}




NewProjectAudioProcessor::~NewProjectAudioProcessor() {
    // Perform any necessary cleanup if required
}

const juce::String NewProjectAudioProcessor::getName() const {
    return "NewProjectAudioProcessor";
}

bool NewProjectAudioProcessor::acceptsMidi() const {
    return true;
}

bool NewProjectAudioProcessor::producesMidi() const {
    return false;
}

bool NewProjectAudioProcessor::isMidiEffect() const {
    return false;
}

double NewProjectAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int NewProjectAudioProcessor::getNumPrograms() {
    return 1;
}

int NewProjectAudioProcessor::getCurrentProgram() {
    return 0;
}

void NewProjectAudioProcessor::setCurrentProgram(int index) {}

const juce::String NewProjectAudioProcessor::getProgramName(int index) {
    return {};
}

void NewProjectAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

bool NewProjectAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Example: Only supporting stereo input and output
    if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() &&
        (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled() ||
         layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo())) {
        return true;
    }
    return false;
}

void NewProjectAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // Save the current state as XML
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NewProjectAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // Load the state from XML
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}




void NewProjectAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Add checks to ensure positive sampleRate and samplesPerBlock
    jassert(sampleRate > 0 && samplesPerBlock > 0);
    if (sampleRate <= 0 || samplesPerBlock <= 0) {
        DBG("Invalid sampleRate or samplesPerBlock");
        return;
    }
    wavetableSynth.prepareToPlay(sampleRate, samplesPerBlock);
    sampler.prepareToPlay(sampleRate, samplesPerBlock);
}

void NewProjectAudioProcessor::releaseResources() {
    wavetableSynth.releaseResources();
    sampler.releaseResources();
}

void NewProjectAudioProcessor::setWaveform(int type) {
    if (type >= 0 && type < static_cast<int>(WavetableSynthesizer::Waveform::NumWaveforms)) {
        wavetableSynth.setWaveform(static_cast<WavetableSynthesizer::Waveform>(type));
    } else {
        DBG("Invalid waveform type specified");
    }
}

void NewProjectAudioProcessor::setSynthVolume(float volume) {
    wavetableSynth.setVolume(volume);
}

void NewProjectAudioProcessor::setSampleVolume(float volume) {
    sampler.setVolume(volume);
}

void NewProjectAudioProcessor::setUnisonSize(int size) {
    wavetableSynth.setUnisonSize(size);  
}

void NewProjectAudioProcessor::setDetuneAmount(float amount) {
    wavetableSynth.setDetuneAmount(amount);
}

// Ensure all parameters are retrieved safely.
void NewProjectAudioProcessor::setFilterCutoff(float cutoff) {
    auto* param = apvts.getParameter("filterCutoff");
    if (param) {
        const float normalizedCutoff = param->convertTo0to1(juce::jlimit(20.0f, 20000.0f, cutoff));
        param->setValueNotifyingHost(normalizedCutoff);
    } else {
        DBG("Filter Cutoff parameter not found!");
    }
}


void NewProjectAudioProcessor::setLFORate(float rate) {
    auto& parameter = *apvts.getParameter("lfoRate");
    parameter.setValueNotifyingHost(parameter.convertTo0to1(rate));
}

void NewProjectAudioProcessor::setLFODepth(float depth) {
    // Retrieve the parameter from the AudioProcessorValueTreeState by ID
    auto* param = apvts.getParameter("lfoDepth");
    if (param) {
        // Convert the actual value to a normalized range (0 to 1) if necessary and set it
        param->setValueNotifyingHost(param->convertTo0to1(depth));
    }
}

void NewProjectAudioProcessor::setFilterResonance(float resonance) {
    // Retrieve the parameter from the AudioProcessorValueTreeState by ID
    auto* param = apvts.getParameter("filterResonance");
    if (param) {
        // Convert the actual value to a normalized range (0 to 1) if necessary and set it
        param->setValueNotifyingHost(param->convertTo0to1(resonance));
    }
}


// Implementation
void NewProjectAudioProcessor::triggerNoteOn(int noteNumber, float velocity) {
    for (auto& voice : synthVoices) {
        if (!voice.isActive()) {
            voice.startNote(noteNumber, velocity); // Example parameters for attack and release
            break;
        }
    }
}


void NewProjectAudioProcessor::updateSynthVoiceADSR(float attack, float decay, float sustain, float release) {
    // Loop through each voice and update ADSR settings
    for (auto& voice : synthVoices) {  // Use reference to avoid pointer if not needed
        voice.updateADSR(attack, decay, sustain, release);
    }
}

std::vector<juce::File> NewProjectAudioProcessor::getSampleFiles() const {
    return sampleFiles;
}

// This function processes the audio block
void NewProjectAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    auto* mixParam = apvts.getRawParameterValue("mix");
    float mixLevel = mixParam ? mixParam->load() : 0.5f;

    juce::AudioBuffer<float> synthBuffer(buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> sampleBuffer(buffer.getNumChannels(), buffer.getNumSamples());
    synthBuffer.clear();
    sampleBuffer.clear();

    // Handle MIDI events
    for (const auto midiMessage : midiMessages) {
        const auto message = midiMessage.getMessage();
        handleMidiEvent(message);
    }

    // Render audio
    renderAudio(synthBuffer, sampleBuffer, buffer, mixLevel, midiMessages);

    // Apply chorus
    juce::dsp::AudioBlock<float> block(buffer);
    chorus.process(juce::dsp::ProcessContextReplacing<float>(block));

    // Apply reverb to each channel
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        reverb.processStereo(channelData, channelData, buffer.getNumSamples());
    }
}


// This function renders audio
void NewProjectAudioProcessor::renderAudio(juce::AudioBuffer<float>& synthBuffer, juce::AudioBuffer<float>& sampleBuffer, juce::AudioBuffer<float>& mainBuffer, float mixLevel, juce::MidiBuffer& midiMessages) {
    // Render synthesizer and sampler buffers
    wavetableSynth.renderNextBlock(synthBuffer, midiMessages, 0, mainBuffer.getNumSamples());
    sampler.renderNextBlock(sampleBuffer, midiMessages, 0, mainBuffer.getNumSamples());

    // Mix down synth and sample buffers to the main buffer
    for (int channel = 0; channel < mainBuffer.getNumChannels(); ++channel) {
        mainBuffer.addFrom(channel, 0, synthBuffer, channel, 0, mainBuffer.getNumSamples(), mixLevel);
        mainBuffer.addFrom(channel, 0, sampleBuffer, channel, 0, mainBuffer.getNumSamples(), 1.0f - mixLevel);
    }
}


void NewProjectAudioProcessor::handleMidiEvent(const juce::MidiMessage& message) {
    if (message.isNoteOn()) {
        int noteNumber = message.getNoteNumber();
        float velocity = message.getFloatVelocity();
        wavetableSynth.handleNoteOn(noteNumber, velocity);
        sampler.handleNoteOn(noteNumber, velocity);
    } else if (message.isNoteOff()) {
        int noteNumber = message.getNoteNumber();
        float velocity = message.getFloatVelocity();
        wavetableSynth.handleNoteOff(noteNumber, velocity);
        sampler.handleNoteOff(noteNumber, velocity);
    }
    // Add additional handling for other types of MIDI messages if needed
}



void NewProjectAudioProcessor::scanSamplesDirectory(const juce::String& path) {
    juce::File directory(path);
    if (directory.exists() && directory.isDirectory()) {
        // Initialize the ranged directory iterator to iterate through WAV files
        juce::RangedDirectoryIterator iter(directory, true, "*.wav");

        // Use a range-based for loop to process each file
        for (auto& entry : iter) {
            const auto& file = entry.getFile();  // Access the file from the DirectoryEntry object
            if (file.existsAsFile()) { // Ensure the item is a file
                sampleFiles.push_back(file); // Add the file to the list
            }
        }
    }
}

void NewProjectAudioProcessor::loadSample(const juce::String& path) {
    juce::File file(path);
    if (file.existsAsFile()) {
        currentSampleFile = file;
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

        if (reader) {
            sampleBuffer.setSize(reader->numChannels, static_cast<int>(reader->lengthInSamples));
            reader->read(&sampleBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
            samplePosition = 0;
        } else {
            DBG("Failed to read sample");
        }
    } else {
        DBG("File does not exist: " + path);
    }
}



juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor() {
    return new NewProjectAudioProcessorEditor(*this);
}

void NewProjectAudioProcessor::setVolume(float volume) {
    // Implementation might involve setting a volume parameter or directly adjusting an audio buffer
    this->volume = volume;  // Assume 'volume' is a float member variable of NewProjectAudioProcessor
}

bool NewProjectAudioProcessor::hasEditor() const {
    return true;
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new NewProjectAudioProcessor();
}
