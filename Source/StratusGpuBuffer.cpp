#include "StratusGpuBuffer.h"
#include <functional>

namespace stratus {
    typedef std::function<void(void)> GpuBufferCommand;

    struct GpuBufferImpl {
        GpuBufferImpl(GpuBufferType type, const void * data, const size_t sizeBytes) 
            : _type(type),
              _bufferType(_ConvertBufferType(type)) {

            _bind = [this](){ glBindBuffer(_bufferType, _buffer); };
            _unbind = [this](){ glBindBuffer(_bufferType, 0); };

            glGenBuffers(1, &_buffer);
            _bind();
            glBufferData(_bufferType, sizeBytes, data, GL_STATIC_DRAW);
            _unbind();
        }

        ~GpuBufferImpl() {
            glDeleteBuffers(1, &_buffer);
        }

    void EnableAttribute(int32_t attribute, 
                         int32_t sizePerElem, 
                         GpuStorageType storage, 
                         bool normalized, 
                         uint32_t stride, 
                         uint32_t offset, 
                         uint32_t divisor = 0) {

        // If we exceed OpenGL's max of 4, we need to calculate a new stride that we
        // can use in the loop below
        if (sizePerElem > 4) {
            // Ex: for a 4x4 float matrix this will be 64 (16 * sizeof(float))
            const uint32_t totalSizeBytes = _CalculateSizeBytes(sizePerElem, storage);
            stride = stride + totalSizeBytes;
        }
        
        const auto enable = [this, attribute, sizePerElem, storage, normalized, stride, offset, divisor]() {
            // OpenGL caps each attrib to 4 elements, so if we have one that's larger
            // then we need treat it as multiple attribs
            for (int32_t i = 0, elem = 0; elem < sizePerElem; ++i, elem += 4) {
                const int32_t pos = attribute + i;
                const int32_t elemSize = (sizePerElem - elem) > 4 ? 4 : sizePerElem;
                const uint32_t totalSizeBytes = _CalculateSizeBytes(elemSize, storage);
                glEnableVertexAttribArray(pos);
                glVertexAttribPointer(
                    pos, 
                    elemSize,
                    _ConvertStorageType(storage),
                    normalized ? GL_TRUE : GL_FALSE,
                    stride, // Offset from one element to the next
                    (void *)(offset + i * totalSizeBytes) // one-time offset before reading first element
                );

                // If 0, data increments by 1 for each vertex
                glVertexAttribDivisor(pos, divisor);
            }
        };

        _enableAttributes.push_back(enable);
    }

    void Bind() const {
        _bind();
        for (auto& enable : _enableAttributes) enable();
    }

    void Unbind() const {
        _unbind();
    }

    private:
        static GLenum _ConvertBufferType(GpuBufferType type) {
            switch (type) {
            case GpuBufferType::PRIMITIVE_BUFFER: return GL_ARRAY_BUFFER;
            case GpuBufferType::INDEX_BUFFER: return GL_ELEMENT_ARRAY_BUFFER;
            }
        }

        static GLenum _ConvertStorageType(GpuStorageType type) {
            switch (type) {
            case GpuStorageType::BYTE: return GL_BYTE;
            case GpuStorageType::UNSIGNED_BYTE: return GL_UNSIGNED_BYTE;
            case GpuStorageType::SHORT: return GL_SHORT;
            case GpuStorageType::UNSIGNED_SHORT: return GL_UNSIGNED_SHORT;
            case GpuStorageType::INT: return GL_INT;
            case GpuStorageType::UNSIGNED_INT: return GL_UNSIGNED_INT;
            case GpuStorageType::FLOAT: return GL_FLOAT;
            }
        }

        static uint32_t _CalculateSizeBytes(int32_t sizePerElem, GpuStorageType type) {
            switch (type) {
            case GpuStorageType::BYTE:
            case GpuStorageType::UNSIGNED_BYTE: return sizePerElem * sizeof(uint8_t);
            case GpuStorageType::SHORT:
            case GpuStorageType::UNSIGNED_SHORT: return sizePerElem * sizeof(uint16_t);
            case GpuStorageType::INT:
            case GpuStorageType::UNSIGNED_INT: return sizePerElem * sizeof(uint32_t);
            case GpuStorageType::FLOAT: return sizePerElem * sizeof(float);
            }
        }

    private:
        const GpuBufferType _type;
        const GLenum _bufferType;
        GLuint _buffer;

        // Cached functions
        GpuBufferCommand _bind;
        GpuBufferCommand _unbind;
        std::vector<GpuBufferCommand> _enableAttributes;
    };

    GpuBuffer::GpuBuffer(GpuBufferType type, const void * data, const size_t sizeBytes)
        : _impl(std::make_shared<GpuBufferImpl>(type, data, sizeBytes)) {}

    void GpuBuffer::EnableAttribute(int32_t attribute, int32_t sizePerElem, GpuStorageType storage, bool normalized, uint32_t stride, uint32_t offset, uint32_t divisor) {
        _impl->EnableAttribute(attribute, sizePerElem, storage, normalized, stride, offset, divisor);
    }

    void GpuBuffer::Bind() const {
        _impl->Bind();
    }

    void GpuBuffer::Unbind() const {
        _impl->Unbind();
    }

    void GpuArrayBuffer::AddBuffer(const GpuBuffer& buffer) {
        _buffers.push_back(buffer);
    }

    void GpuArrayBuffer::Bind() const {
        for (auto& buffer : _buffers) buffer.Bind();
    }

    void GpuArrayBuffer::Unbind() const {
        for (auto& buffer : _buffers) buffer.Unbind();
    }
}