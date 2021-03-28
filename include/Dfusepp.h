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
#include <vector>
#include "Crc.h"

#if defined (DFUSEPP_IMAGE_ELEMENT_VERSION)
#define DFUSEPP_IMAGE_ELEMENT_VERSION_SIZE 4
#else
#define DFUSEPP_IMAGE_ELEMENT_VERSION_SIZE 0
#endif

// See http://rc.fdr.hu/UM0391.pdf for more details

namespace Dfusepp {

struct Prefix {
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

struct TargetPrefix {
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

struct ImageElement {
    union {
        struct __attribute__((packed)) {
            uint32_t m_address;
            uint32_t m_size;
#if defined (DFUSEPP_IMAGE_ELEMENT_VERSION)
            uint8_t m_versionMajor;
            uint8_t m_versionMinor;
            uint16_t m_versionPatch;
#endif
        } Value;
        std::array<uint8_t, 8 + DFUSEPP_IMAGE_ELEMENT_VERSION_SIZE> m_data {};
    };
    uint32_t m_offset;
};

struct Suffix {
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
    Dfusepp() = default;

    //! @brief Adds data to be processed
    //! @param data The data buffer to be added
    //! @param offset The offset to add the data at
    //! @param size The size of the data to be added
    //! @return A bool with the result
    bool addData(const uint8_t* data, uint32_t offset, size_t size)
    {
        for (size_t index = offset; index < (offset + size); ++index) {
            if (index < sizeof(Prefix::Value)) {
                m_prefix.m_data[index] = data[index - offset];
            } else if (m_targetPrefixIndex < sizeof(TargetPrefix::Value)) {
                m_targetPrefix.m_data[m_targetPrefixIndex] = data[index - offset];
                m_targetPrefixIndex++;
            } else if (m_imageElements.size() < m_targetPrefix.Value.m_elements) {
                if (!m_imageElementIndex)
                    m_imageElement.m_offset = index + sizeof(ImageElement::Value);

                if (m_imageElementIndex < sizeof(ImageElement::Value)) {
                    m_imageElement.m_data[m_imageElementIndex] = data[index - offset];
                    m_imageElementIndex++;
#if defined (DFUSEPP_IMAGE_ELEMENT_VERSION)
                    if (m_imageElementIndex == (sizeof(ImageElement::Value) - 1))
                        m_imageElement.Value.m_size -= DFUSEPP_IMAGE_ELEMENT_VERSION_SIZE;
#endif
                } else if (m_imageElementIndex == (sizeof(ImageElement::Value) + m_imageElement.Value.m_size - 1)) {
                    m_imageElements.push_back(m_imageElement);
                    std::fill(m_imageElement.m_data.begin(), m_imageElement.m_data.end(), 0);
                    m_imageElementIndex = 0;
                } else
                    m_imageElementIndex++;
            } else if (m_suffixIndex < sizeof(Suffix::Value)) {
                m_suffix.m_data[m_suffixIndex] = data[index - offset];
                m_suffixIndex++;
            } else
                return false;

            // The CRC field should not be part of the CRC calculation
            if (m_suffixIndex <= (sizeof(Suffix::Value) - sizeof(uint32_t)))
                m_crc = calculateCrc(m_crc, data[index - offset]);
        }

        return true;
    }

    //! Returns the version
    //! @return A uint16_t with the version
    uint16_t version() const
    {
        return m_suffix.Value.m_version;
    }

    //! Returns the product ID
    //! @return A uint16_t with the product ID
    uint16_t productId() const
    {
        return m_suffix.Value.m_productId;
    }

    //! Returns the vendor ID
    //! @return A uint16_t with the vendor ID
    uint16_t vendorId() const
    {
        return m_suffix.Value.m_vendorId;
    }

    //! Returns if the DfuSe file has a valid prefix
    //! @return A bool with the result
    bool prefixValid() const
    {
        return m_prefix.Value.m_signature == s_prefixSignature;
    }

    //! Returns if the DfuSe file is valid
    //! @return A bool with the result
    bool valid() const
    {
        return (prefixValid() && (m_suffix.Value.m_signature == s_suffixSignature) && (m_crc == m_suffix.Value.m_crc));
    }

    //! Returns the detected target name
    //! @return A std::string with the target name
    std::string targetName() const
    {
        return std::string(m_targetPrefix.Value.m_targetName.data());
    }

    //! Returns the detected images
    //! @return A std::vector with ImageElement's
    std::vector<ImageElement> images() const
    {
        return m_imageElements;
    }

    //! Returns the total image size in bytes
    size_t size() const
    {
        return m_prefix.Value.m_dfuImageSize + sizeof(Suffix::Value);
    }

private:
    static constexpr std::array<char, 5> s_prefixSignature = { 'D', 'f', 'u', 'S', 'e' };
    static constexpr std::array<char, 3> s_suffixSignature = { 'U', 'F', 'D' };
    Prefix m_prefix;
    TargetPrefix m_targetPrefix;
    Suffix m_suffix;
    std::vector<ImageElement> m_imageElements;
    ImageElement m_imageElement;
    uint32_t m_imageElementIndex { 0 };
    uint32_t m_targetPrefixIndex { 0 };
    uint32_t m_suffixIndex { 0 };
    uint32_t m_crc { 0xFFFFFFFF };
};

} // namespace Dfusepp
