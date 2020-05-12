
class MidiFile
{
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
        }
    }
};
