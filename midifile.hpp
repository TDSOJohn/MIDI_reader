#ifndef midifile_hpp
#define midifile_hpp

#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <array>


struct MidiEvent
{
    enum class Type
    {
        noteOff,
        noteOn,
        other
    } event;
    
    uint8_t nKey =0;
    uint8_t nVelocity =0;
    uint32_t nWallTick =0;
    uint32_t nDeltaTick =0;
};

struct MidiNote
{
    uint8_t                 nKey =0;
    uint8_t                 nVelocity =0;
    uint32_t                nStartTime =0;
    uint32_t                nDuration =0;
};

struct MidiTrack
{
    std::string             sName;
    std::string             sInstrument;
    std::vector<MidiEvent>  vecEvents;
    std::vector<MidiNote>   vecNotes;
    uint8_t                 nMaxNote =64;
    uint8_t                 nMinNote =64;
};

class MidiFile
{
public:
                    MidiFile(const std::string& sFileName);

private:
    bool            parseFile(const std::string& sFileName);

public:
    std::vector<MidiTrack> vecTracks;
    uint32_t m_nTempo =0;
    uint32_t m_nBPM =0;

private:
    enum EventName : uint8_t
    {
        // Last 4 bits are for channel number
        voiceNoteOff =0x80,
        voiceNoteOn =0x90,
        voiceAftertouch =0xA0,
        voiceControlChange =0xB0,
        voiceProgramChange =0xC0,
        voiceChannelPressure =0xD0,
        voicePitchBend =0xE0,
        systemExclusive =0xF0
    };

    enum metaEventName : uint8_t
    {
        metaSequence = 0x00,
        metaText = 0x01,
        metaCopyright = 0x02,
        metaTrackName = 0x03,
        metaInstrumentName = 0x04,
        metaLyrics = 0x05,
        metaMarker = 0x06,
        metaCuePoint = 0x07,
        metaChannelPrefix = 0x20,
        metaEndOfTrack = 0x2F,
        metaSetTempo = 0x51,
        metaSMPTEOffset = 0x54,
        metaTimeSignature = 0x58,
        metaKeySignature = 0x59,
        metaSequencerSpecific = 0x7F
    };
};

#endif
