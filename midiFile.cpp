
#include "midiFile.hpp"


MidiFile::MidiFile(const std::string& sFileName)
{
    parseFile(sFileName);
}

bool MidiFile::parseFile(const std::string& sFileName)
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

    auto readString = [&ifs](uint32_t nLength)
    {
        std::string s;
        for (uint32_t i = 0; i <nLength; i++) s += ifs.get();
        return s;
    };

    auto readValue = [&ifs]()
    {
        uint32_t nValue = 0;
        uint8_t nByte = 0;

        // Read byte
        nValue = ifs.get();
        // Check MSB, if set, more bytes need reading
        if(nValue & 0x80)
        {
            nValue &=0x7F;

            do
            {
                // Read next byte
                nByte = ifs.get();
                // Construct value by setting bottom 7 bits, then shifting 7 bits
                nValue = (nValue << 7) | (nByte &0x7F);
            } while(nByte & 0x80);
        }
        // Return final construction (always a 32-bit unsigned int)
        return nValue;
    };

    // PARSE MIDI file
    uint32_t n32 = 0;
    uint16_t n16 = 0;

    // Read MIDI Header (4 bytes)
    ifs.read((char*)&n32, sizeof(uint32_t));
    uint32_t nFileID = swap32(n32);

    // Read MIDI Header Length (4 bytes)
    ifs.read((char*)&n32, sizeof(uint32_t));
    uint32_t nHeaderLength = swap32(n32);

    // Read MIDI Format information (2 bytes)
    ifs.read((char*)&n16, sizeof(uint16_t));
    uint16_t nFormat = swap16(n16);

    // Read MIDI Tracks count (2 bytes)
    ifs.read((char*)&n16, sizeof(uint16_t));
    uint16_t nTrackChunks = swap16(n16);

    // Read MIDI Divisions (s 2 bytes)
    ifs.read((char*)&n16, sizeof(uint16_t));
    uint16_t nDivision = swap16(n16);

    // Now for the actual data reading
    for(uint16_t nChunk = 0; nChunk < nTrackChunks; nChunk++)
    {
        std::cout << "====== NEW TRACK" << std::endl;
        // Read Track Header (ID and Length, 8 bytes total)
        ifs.read((char*)&n32, sizeof(uint32_t));
        uint32_t nTrackID = swap32(n32);
        ifs.read((char*)&n32, sizeof(uint32_t));
        uint32_t nTrackLength = swap32(n32);

        bool bEndOfTrack = false;
        int8_t nPreviousStatus = 0;

        vecTracks.push_back(MidiTrack());

        while(!ifs.eof() && !bEndOfTrack)
        {
            // Fundamentally all MIDI Events contain Timecode and Status byte
            uint32_t nStatusTimeDelta = 0;
            uint8_t nStatus = 0;

            // Read Timecode
            nStatusTimeDelta = readValue();
            nStatus = ifs.get();

            // Catch if MIDI run mode is on, to status byte is not repeated until it changes
            if (nStatus < 0x80)
            {
                nStatus = nPreviousStatus;
                // Instruct the reader to go back one byte
                ifs.seekg(-1, std::ios_base::cur);
            }

            if((nStatus & 0xF0) == EventName::voiceNoteOff)
            {
                nPreviousStatus = nStatus;
                uint8_t nChannel = nStatus & 0x0F;
                uint8_t nNoteID = ifs.get();
                uint8_t nNoteVelocity = ifs.get();
                vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::noteOff, nNoteID, nNoteVelocity, nStatusTimeDelta });
            }
            else if((nStatus & 0xF0) == EventName::voiceNoteOn)
            {
                nPreviousStatus = nStatus;
                uint8_t nChannel = nStatus & 0x0F;
                uint8_t nNoteID = ifs.get();
                uint8_t nNoteVelocity = ifs.get();
                if(nNoteVelocity == 0)
                    vecTracks[nChunk].vecEvents.push_back( MidiEvent(MidiEvent::Type::noteOff, nNoteID, nNoteVelocity, nStatusTimeDelta));
                else
                    vecTracks[nChunk].vecEvents.push_back( MidiEvent(MidiEvent::Type::noteOn, nNoteID, nNoteVelocity, nStatusTimeDelta));
            }
            else if((nStatus & 0xF0) == EventName::voiceAftertouch)
            {
                nPreviousStatus = nStatus;
                uint8_t nChannel = nStatus & 0x0F;
                uint8_t nNoteID = ifs.get();
                uint8_t nNoteVelocity = ifs.get();
                vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
            }
            else if((nStatus & 0xF0) == EventName::voiceControlChange)
            {
                nPreviousStatus = nStatus;
                uint8_t nChannel = nStatus & 0x0F;
                uint8_t nNoteID = ifs.get();
                uint8_t nControlValue = ifs.get();
                vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
            }
            else if((nStatus & 0xF0) == EventName::voiceProgramChange)
            {
                nPreviousStatus = nStatus;
                uint8_t nChannel = nStatus & 0x0F;
                uint8_t nProgramID = ifs.get();
                vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
            }
            else if((nStatus & 0xF0) == EventName::voiceChannelPressure)
            {
                nPreviousStatus = nStatus;
                uint8_t nChannel = nStatus & 0x0F;
                uint8_t nChannelPressure = ifs.get();
                vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
            }
            else if((nStatus & 0xF0) == EventName::voicePitchBend)
            {
                nPreviousStatus = nStatus;
                uint8_t nChannel = nStatus & 0x0F;
                uint8_t nLS7B = ifs.get();
                uint8_t nMS7B = ifs.get();
                vecTracks[nChunk].vecEvents.push_back({ MidiEvent::Type::other });
            }
            else if((nStatus & 0xF0) == EventName::systemExclusive)
            {
                nPreviousStatus =0;

                if(nStatus == 0xF0)
                    std::cout << "system message begins: " << readString(readValue()) << std::endl;
                else if(nStatus == 0xF7)
                    std::cout << "system message ends: " << readString(readValue()) << std::endl;
                else if(nStatus == 0xFF)
                {
                    // Meta Message
                    uint8_t nType = ifs.get();
                    uint8_t nLength = ReadValue();

                    switch (nType)
                    {
                        case metaSequence:
                            std::cout << "Sequence Number: " << ifs.get() << ifs.get() << std::endl;
                            break;
                        case metaText:
                            std::cout << "Text: " << readString(nLength) << std::endl;
                            break;
                        case metaCopyright:
                            std::cout << "Copyright: " << readString(nLength) << std::endl;
                            break;
                        case metaTrackName:
                            vecTracks[nChunk].sName = readString(nLength);
                            std::cout << "Track Name: " << vecTracks[nChunk].sName << std::endl;
                            break;
                        case metaInstrumentName:
                            vecTracks[nChunk].sInstrument = ReadString(nLength);
                            std::cout << "Instrument Name: " << vecTracks[nChunk].sInstrument << std::endl;
                            break;
                        case metaLyrics:
                            std::cout << "Lyrics: " << readString(nLength) << std::endl;
                            break;
                        case metaMarker:
                            std::cout << "Marker: " << readString(nLength) << std::endl;
                            break;
                        case metaCuePoint:
                            std::cout << "Cue: " << readString(nLength) << std::endl;
                            break;
                        case metaChannelPrefix:
                            std::cout << "Prefix: " << ifs.get() << std::endl;
                            break;
                        case metaEndOfTrack:
                            bEndOfTrack = true;
                            break;
                        case metaSetTempo:
                            // microseconds per quarter note
                            if (m_nTempo == 0)
							{
								(m_nTempo |= (ifs.get() << 16));
								(m_nTempo |= (ifs.get() << 8));
								(m_nTempo |= (ifs.get() << 0));
								m_nBPM = (60000000 / m_nTempo);
								std::cout << "Tempo: " << m_nTempo << " (" << m_nBPM << "bpm)" << std::endl;
							}
							break;
                        case metaSMPTEOffset:
							std::cout << "SMPTE: H:" << ifs.get() << " M:" << ifs.get() << " S:" << ifs.get() << " FR:" << ifs.get() << " FF:" << ifs.get() << std::endl;
							break;
						case metaTimeSignature:
							std::cout << "Time Signature: " << ifs.get() << "/" << (2 << ifs.get()) << std::endl;
							std::cout << "ClocksPerTick: " << ifs.get() << std::endl;

							// A MIDI "Beat" is 24 ticks, so specify how many 32nd notes constitute a beat
							std::cout << "32per24Clocks: " << ifs.get() << std::endl;
							break;
						case metaKeySignature:
							std::cout << "Key Signature: " << ifs.get() << std::endl;
							std::cout << "Minor Key: " << ifs.get() << std::endl;
							break;
						case metaSequencerSpecific:
							std::cout << "Sequencer Specific: " << ReadString(nLength) << std::endl;
							break;
						default:
							std::cout << "Unrecognised MetaEvent: " << nType << std::endl;
                    }
                }
            } else
            {
                std::cout << "Unrecognized Status Byte: " << nStatus << std::endl;
            }
        }
    }

    // convert time events to notes
    for(auto& track : vecTracks)
    {
        uint32_t nWallTime = 0;
        std::list<MidiNote> listNotesBeingProcessed;

        for(auto& event : track.vecEvents)
        {
            nWallTime += event.nDeltaTick;

            if(event.event == MidiEvent::Type::noteOn)
                listNotesBeingProcessed.push_back({ event.nKey, event.nVelocity, nWallTime, 0 });

            if(event.event == MidiEvent::Type::noteOff)
            {
                auto note = std::find_if(listNotesBeingProcessed.begin(), listNotesBeingProcessed.end(), [&](const MidiNote& n) { return n.nKey == event.nKey; });

                if (note != listNotesBeingProcessed.end())
				{
					note->nDuration = nWallTime - note->nStartTime;
					track.vecNotes.push_back(*note);
					track.nMinNote = std::min(track.nMinNote, note->nKey);
					track.nMaxNote = std::max(track.nMaxNote, note->nKey);
					listNotesBeingProcessed.erase(note);
				}
            }
        }
    }
    return true;
}
