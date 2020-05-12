#include <iostream>

#include "midifile.hpp"


class MidiFile
{
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

public:
    MidiFile(const std::string& sFileName)
    {
        parseFile(sFilename)
    }
    bool parseFile(const std::string& sFileName)
    {
        // Open the MIDI file as a stream
        std::ifstream ifs;
        ifs.open(sFileName, std::fstream::in | std::ios::binary);
        if(!ifs.is_open())
            return false;

        // Swap bits in 32bit integer
        auto swap32 = [](uint32_t n)
        {
            return (((n >> 24) & 0xff) | ((n << 8) & 0xff0000) | ((n >> 8) & 0xff00) | ((n << 24) & 0xff000000));
        };

        auto swap16 = [](uint16_t n)
        {
            return ((n >> 8) | (n << 8));
        };

        auto readString = [&ifs](uint32_t nlength)
        {
            std::string s;
            for (uint32_t i =0; i <nLength; i++) s += ifs.get();
            return s;
        };

        auto readValue = [&ifs]()
        {
            uint32_t nValue =0;
            uint8_t nByte =0;

            // Read byte
            nValue =ifs.get();
            // Check MSB, if set, more bytes need reading
            if(nValue & 0x80)
            {
                nValue &=0x7F;

                do
                {
                    // Read next byte
                    nByte =ifs.get();
                    // Construct value by setting bottom 7 bits, then shifting 7 bits
                    nValue =(nValue << 7) | (nByte &0x7F);
                } while(nByte & 0x80);
            }
            // Return final construction (always a 32-bit unsigned int)
            return nValue;
        };

        // PARSE MIDI file

        uint32_t n32 =0;
        uint16_t n16 =0;

        // Read MIDI Header (4 bytes)
        ifs.read((char*)&n32, sizeof(uint32_t));
        uint32_t nFileID =swap32(n32);

        // Read MIDI Header Length (4 bytes)
        ifs.read((char*)&n32, sizeof(uint32_t));
        uint32_t nHeaderLength =swap32(n32);

        // Read MIDI Format information (2 bytes)
        ifs.read((char*)&n16, sizeof(uint16_t));
        uint16_t nFormat =swap16(n16);

        // Read MIDI Tracks count (2 bytes)
        ifs.read((char*)&n16, sizeof(uint16_t));
        uint16_t nTrackChunks =swap16(n16);

        // Read MIDI Divisions (s 2 bytes)
        ifs.read((char*)&n16, sizeof(uint16_t));
        uint16_t nDivision =swap16(n16);

        // Now for the actual data reading
        for(uint16_t nChunk =0; nChunk <nTrackChunks; nChunk++)
        {
            std::cout <<"====== NEW TRACK" <<std::endl;
            // Read Track Header (ID and Length, 8 bytes total)
            ifs.read((char*)&n32, sizeof(uint32_t));
            uint32_t nTrackID =swap32(n32);
            ifs.read((char*)&n32, sizeof(uint32_t));
            uint32_t nTrackLength =swap32(n32);

            bool bEndOfTrack =false;
            int8_t nPreviousStatus =0;

            vetTracks.push_back(MidiTrack());

            while(!ifs.eof() && !bEndOfTrack)
            {
                // Fundamentally all MIDI Events contain Timecode and Status byte
                uint32_t nStatusTimeDelta =0;
                uint8_t nStatus =0;

                // Read Timecode
                nStatusTimeDelta =readValue();
                nStatus =ifs.get();

                // Catch if MIDI run mode is on, to status byte is not repeated until it changes
                if (nStatus <0x80)
                {
                    nStatus =nPreviousStatus;
                    // Instruct the reader to go back one byte
                    ifs.seekg(-1, std::ios_base::cur);
                }

                if((nStatus & 0xF0) ==EventName::voiceNoteOff)
                {
                    nPreviousStatus =nStatus;
                    uint8_t nChannel =nStatus & 0x0F;
                    uint8_t nNoteID = ifs.get();
                    uint8_t nNoteVelocity = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::noteOff, nNoteID, nNoteVelocity, nStatusTimeDelta });
                }
                else if((nStatus & 0xF0) ==EventName::voiceNoteOn)
                {
                    nPreviousStatus =nStatus;
                    uint8_t nChannel =nStatus & 0x0F;
                    uint8_t nNoteID = ifs.get();
                    uint8_t nNoteVelocity = ifs.get();
                    if(nNoteVelocity ==0)
                        vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::noteOff, nNoteID, nNoteVelocity, nStatusTimeDelta });
                    else
                        vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::noteOn, nNoteID, nNoteVelocity, nStatusTimeDelta });
                }
                else if((nStatus & 0xF0) ==EventName::voiceAftertouch)
                {
                    nPreviousStatus =nStatus;
                    uint8_t nChannel =nStatus & 0x0F;
                    uint8_t nNoteID = ifs.get();
                    uint8_t nNoteVelocity = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
                }
                else if((nStatus & 0xF0) ==EventName::voiceControlChange)
                {
                    nPreviousStatus =nStatus;
                    uint8_t nChannel =nStatus & 0x0F;
                    uint8_t nNoteID = ifs.get();
                    uint8_t nControlValue = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
                }
                else if((nStatus & 0xF0) ==EventName::voiceProgramChange)
                {
                    nPreviousStatus =nStatus;
                    uint8_t nChannel =nStatus & 0x0F;
                    uint8_t nProgramID = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
                }
                else if((nStatus & 0xF0) ==EventName::voiceChannelPressure)
                {
                    nPreviousStatus =nStatus;
                    uint8_t nChannel =nStatus & 0x0F;
                    uint8_t nChannelPressure = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
                }
                else if((nStatus & 0xF0) ==EventName::voicePitchBend)
                {
                    nPreviousStatus =nStatus;
                    uint8_t nChannel =nStatus & 0x0F;
                    uint8_t nLS7B = ifs.get();
                    uint8_t nMS7B = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
                }
                else if((nStatus & 0xF0) ==EventName::systemExclusive)
                {

                } else
                {
                    std::cout <<"Unrecognized Status Byte: " <<nStatus <<std::endl;
                }
            }
        }
    }
};
