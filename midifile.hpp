#ifndef midifile_hpp
#define midifile_hpp


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
    uint8_t                 mMinNote =64;
};

class MidiFile
{
public;
    MidiFile()
}

#endif
