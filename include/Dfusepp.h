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

#include "Crc.h"
#include <array>
#include <string>
#include <vector>

// See http://rc.fdr.hu/UM0391.pdf for more details

#if defined(DFUSEPP_IMAGE_ELEMENT_VERSION)
#define DFUSEPP_IMAGE_ELEMENT_VERSION_SIZE 4
#else
#define DFUSEPP_IMAGE_ELEMENT_VERSION_SIZE 0
#endif

namespace Dfusepp {

struct Prefix {
    union {
        struct __attribute__((packed)) {
            std::array<char, 5> signature;
            uint8_t version;
            uint32_t dfuImageSize;
            uint8_t targets;
        } Value;
        std::array<uint8_t, 11> data {};
    };
};

struct TargetPrefix {
    union {
        struct __attribute__((packed)) {
            std::array<char, 6> signature;
            uint8_t alternateSetting;
            uint32_t targetNamed;
            std::array<char, 255> targetName;
            uint32_t targetSize;
            uint32_t elements;
        } Value;
        std::array<uint8_t, 274> data {};
    };
};

struct ImageElement {
    union {
        struct __attribute__((packed)) {
            uint32_t address;
            uint32_t size;
#if defined(DFUSEPP_IMAGE_ELEMENT_VERSION)
            uint8_t versionMajor;
            uint8_t versionMinor;
            uint16_t versionPatch;
#endif
        } Value;
        std::array<uint8_t, 8 + DFUSEPP_IMAGE_ELEMENT_VERSION_SIZE> data {};
    };
    uint32_t offset;
};

struct Suffix {
    union {
        struct __attribute__((packed)) {
            uint16_t version;
            uint16_t productId;
            uint16_t vendorId;
            uint16_t dfu;
            std::array<char, 3> signature;
            uint8_t length;
            uint32_t crc;
        } Value;
        std::array<uint8_t, 16> data {};
    };
};

class Dfusepp {
public:
    Dfusepp() = default;
    virtual ~Dfusepp() = default;

    /// @brief Adds data to be processed
    /// @param data The data buffer to be added
    /// @param offset The offset to add the data at
    /// @param size The size of the data to be added
    /// @return A bool with the result
    virtual bool addData(const uint8_t* data, uint32_t offset, size_t size)
    {
        for (size_t index = offset; index < (offset + size); ++index) {
            if (index < sizeof(Prefix::Value)) {
                m_prefix.data[index] = data[index - offset];
            } else if (m_targetPrefixIndex < sizeof(TargetPrefix::Value)) {
                m_targetPrefix.data[m_targetPrefixIndex] = data[index - offset];
                m_targetPrefixIndex++;
            } else if (m_imageElements.size() < m_targetPrefix.Value.elements) {
                if (!m_imageElementIndex)
                    m_imageElement.offset = index + sizeof(ImageElement::Value);

                if (m_imageElementIndex < sizeof(ImageElement::Value)) {
                    m_imageElement.data[m_imageElementIndex] = data[index - offset];
                    m_imageElementIndex++;
                    if (m_imageElementIndex == (sizeof(ImageElement::Value) - 1))
                        m_imageElement.Value.size -= DFUSEPP_IMAGE_ELEMENT_VERSION_SIZE;
                } else if (m_imageElementIndex == (sizeof(ImageElement::Value) + m_imageElement.Value.size - 1)) {
                    m_imageElements.push_back(m_imageElement);
                    std::fill(m_imageElement.data.begin(), m_imageElement.data.end(), 0);
                    m_imageElementIndex = 0;
                } else
                    m_imageElementIndex++;
            } else if (m_suffixIndex < sizeof(Suffix::Value)) {
                m_suffix.data[m_suffixIndex] = data[index - offset];
                m_suffixIndex++;
            } else
                return false;

            // The CRC field should not be part of the CRC calculation
            if (m_suffixIndex <= (sizeof(Suffix::Value) - sizeof(uint32_t)))
                m_crc = calculateCrc(m_crc, data[index - offset]);
        }

        return true;
    }

    /// Returns the version
    /// @return A uint16_t with the version
    virtual uint16_t version() const
    {
        return m_suffix.Value.version;
    }

    /// Returns the product ID
    /// @return A uint16_t with the product ID
    virtual uint16_t productId() const
    {
        return m_suffix.Value.productId;
    }

    /// Returns the vendor ID
    /// @return A uint16_t with the vendor ID
    virtual uint16_t vendorId() const
    {
        return m_suffix.Value.vendorId;
    }

    /// Returns if the DfuSe prefix is valid
    /// @return A bool with the result
    virtual bool prefixValid() const
    {
        return m_prefix.Value.signature == s_prefixSignature;
    }

    /// Returns if the DfuSe file is valid
    /// @return A bool with the result
    virtual bool valid() const
    {
        return (prefixValid() && (m_suffix.Value.signature == s_suffixSignature) && (m_crc == m_suffix.Value.crc));
    }

    /// Returns the detected target name
    /// @return A std::string with the target name
    virtual std::string targetName() const
    {
        return std::string(m_targetPrefix.Value.targetName.data());
    }

    /// Returns the detected images
    /// @return A std::vector with ImageElement's
    virtual std::vector<ImageElement> images() const
    {
        return m_imageElements;
    }

    /// Returns the total image size in bytes
    /// @return A size_t with the size
    virtual size_t size() const
    {
        return m_prefix.Value.dfuImageSize + sizeof(Suffix::Value);
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
