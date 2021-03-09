// The MIT License (MIT)

// Copyright (c) 2021 Bang & Olufsen a/s

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <array>
#include <string>

// See http://rc.fdr.hu/UM0391.pdf for more details

namespace Dfusepp {

struct DfusePrefix {
    union {
        struct __attribute__((packed)) {
            std::array<char, 5> m_signature;
            uint8_t m_version;
            uint32_t m_dfuImageSize;
            uint8_t m_targets;
        } Value;
        std::array<uint8_t, 11> m_data {};
    };
};

struct DfuseTargetPrefix {
    union {
        struct __attribute__((packed)) {
            std::array<char, 6> m_signature;
            uint8_t m_alternateSetting;
            uint32_t m_targetNamed;
            std::array<char, 255> m_targetName;
            uint32_t m_targetSize;
            uint32_t m_elements;
        } Value;
        std::array<uint8_t, 274> m_data {};
    };
};

struct DfuseImageElement {
    union {
        struct __attribute__((packed)) {
            uint32_t m_address;
            uint32_t m_size;
        } Value;
        std::array<uint8_t, 8> m_data {};
    };
    uint32_t m_offset;
};

struct DfuseSuffix {
    union {
        struct __attribute__((packed)) {
            uint16_t m_version;
            uint16_t m_productId;
            uint16_t m_vendorId;
            uint16_t m_dfu;
            std::array<char, 3> m_signature;
            uint8_t m_length;
            uint32_t m_crc;
        } Value;
        std::array<uint8_t, 16> m_data {};
    };
};

class Dfusepp {
public:
    //! @brief Constructs the instance
    Dfusepp()
    {
        generateCrcTable();
    }

    //! @brief Adds data to be processed
    //! @param data The data buffer to be added
    //! @param offset The offset to add the data at
    //! @param size The size of the data to be added
    //! @return A bool with the result
    bool addData(const uint8_t* data, uint32_t offset, size_t size)
    {
        for (size_t index = offset; index < (offset + size); ++index) {
            if (index < sizeof(DfusePrefix::m_data)) {
                m_prefix.m_data[index] = data[index - offset];
            } else if (m_targetPrefixIndex < sizeof(DfuseTargetPrefix::m_data)) {
                m_targetPrefix.m_data[m_targetPrefixIndex] = data[index - offset];
                m_targetPrefixIndex++;
            } else if (m_imageElements.size() < m_targetPrefix.Value.m_elements) {
                if (!m_imageElementIndex)
                    m_imageElement.m_offset = index;

                if (m_imageElementIndex < sizeof(DfuseImageElement::m_data)) {
                    m_imageElement.m_data[m_imageElementIndex] = data[index - offset];
                    m_imageElementIndex++;
                } else if (m_imageElementIndex == (sizeof(DfuseImageElement::m_data) + m_imageElement.Value.m_size - 1)) {
                    m_imageElements.push_back(m_imageElement);
                    std::fill(m_imageElement.m_data.begin(), m_imageElement.m_data.end(), 0);
                    m_imageElementIndex = 0;
                } else
                    m_imageElementIndex++;
            } else if (m_suffixIndex < sizeof(DfuseSuffix::m_data)) {
                m_suffix.m_data[m_suffixIndex] = data[index - offset];
                m_suffixIndex++;
            } else
                return false;

            // The CRC field should not be part of the CRC calculation
            if (m_suffixIndex <= (sizeof(DfuseSuffix) - sizeof(uint32_t)))
                m_crc = calculateCrc(m_crc, data[index - offset]);
        }

        return true;
    }

    //! Returns the version
    //! @return A uint16_t with the version
    uint16_t version()
    {
        return m_suffix.Value.m_version;
    }

    //! Returns the product ID
    //! @return A uint16_t with the product ID
    uint16_t productId()
    {
        return m_suffix.Value.m_productId;
    }

    //! Returns the vendor ID
    //! @return A uint16_t with the vendor ID
    uint16_t vendorId()
    {
        return m_suffix.Value.m_vendorId;
    }

    //! Returns if the DfuSe file is valid
    //! @return A bool with the result
    bool valid()
    {
        return ((m_prefix.Value.m_signature == s_prefixSignature) && (m_suffix.Value.m_signature == s_suffixSignature) && (m_crc == m_suffix.Value.m_crc));
    }

    //! Returns the detected target name
    //! @return A std::string with the target name
    std::string targetName()
    {
        return std::string(m_targetPrefix.Value.m_targetName.data());
    }

    //! Returns the detected images
    //! @return A std::vector with DfuseImageElement's
    std::vector<DfuseImageElement> images() const
    {
        return m_imageElements;
    }

    //! @brief Destructs the instance
    virtual ~Dfusepp() = default;

private:
    void generateCrcTable()
    {
        uint32_t crc;

        for (uint16_t i = 0; i < m_crcTable.size(); i++) {
            crc = i;
            for (uint16_t j = 8; j > 0; j--) {
                (crc & 1) ? crc = (crc >> 1) ^ 0xedb88320 : crc >>= 1;
                m_crcTable[i] = crc;
            }
        }
    }

    uint32_t calculateCrc(uint32_t crc, uint32_t value)
    {
        return ((crc >> 8) & 0x00FFFFFF) ^ m_crcTable[(crc ^ value) & 0xFF];
    }

    std::array<char, 5> s_prefixSignature = { 'D', 'f', 'u', 'S', 'e' };
    std::array<char, 3> s_suffixSignature = { 'U', 'F', 'D' };

    DfusePrefix m_prefix;
    DfuseTargetPrefix m_targetPrefix;
    DfuseSuffix m_suffix;
    std::vector<DfuseImageElement> m_imageElements;
    DfuseImageElement m_imageElement;
    std::array<uint32_t, 256> m_crcTable;
    uint32_t m_imageElementIndex { 0 };
    uint32_t m_targetPrefixIndex { 0 };
    uint32_t m_suffixIndex { 0 };
    uint32_t m_crc { 0xFFFFFFFF };
};

} // namespace Dfusepp