#pragma once

namespace Lzs
{
    inline std::vector<unsigned char> Decompress(const std::vector<unsigned char>& compressed)
    {
        if (compressed.size() < 4)
        {
            abort();
        }

        const unsigned int inputBufferSize = static_cast<unsigned int>(compressed.size());
        const unsigned int input_length =
            (((compressed[0] & 0xFF) << 0) |
            ((compressed[1] & 0xFF) << 8) |
            ((compressed[2] & 0xFF) << 16) |
            ((compressed[3] & 0xFF) << 24)) + 4;

        if (input_length != inputBufferSize)
        {
            abort();
        }

        unsigned int extract_size = (inputBufferSize + 255) & ~255;
        std::vector<unsigned char> extract_buffer(extract_size);



        unsigned int input_offset = 4;
        unsigned int output_offset = 0;
        unsigned char control_byte = 0;
        unsigned char control_bit = 0;

        while (input_offset < inputBufferSize)
        {
            if (control_bit == 0)
            {
                control_byte = compressed[input_offset++];
                control_bit = 8;
            }

            if (control_byte & 1)
            {
                extract_buffer[output_offset++] = compressed[input_offset++];

                if (output_offset == extract_size)
                {
                    extract_size += 256;
                    extract_buffer.resize(extract_size);
                }
            }
            else
            {
                const unsigned char reference1 = compressed[input_offset++];
                const unsigned char reference2 = compressed[input_offset++];

                const unsigned short int reference_offset = reference1 | ((reference2 & 0xF0) << 4);

                const unsigned char reference_length = (reference2 & 0xF) + 3;

                int real_offset = output_offset - ((output_offset - 18 - reference_offset) & 0xFFF);

                for (int j = 0; j < reference_length; ++j)
                {
                    if (real_offset < 0)
                    {
                        extract_buffer[output_offset++] = 0;
                    }
                    else
                    {
                        extract_buffer[output_offset++] = extract_buffer[real_offset];
                    }

                    if (output_offset == extract_size)
                    {
                        extract_size += 256;
                        extract_buffer.resize(extract_size);
                    }

                    ++real_offset;
                }
            }

            control_byte >>= 1;
            control_bit--;
        }

        // Truncate to exact size
        extract_buffer.resize(output_offset);
        return extract_buffer;
    }
}
