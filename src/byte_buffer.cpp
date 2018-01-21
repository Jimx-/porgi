#include "byte_buffer.h"

#include <algorithm>
#include <cstring>

ByteBuffer::ByteBuffer(size_t capacity) : _capacity(capacity), _size(0)
{
    allocate(capacity);
}

ByteBuffer::ByteBuffer(size_t size, uint8_t fill) : _capacity(size), _size(size)
{
    allocate(_capacity);
    std::memset(_data.get(), fill, size);
}

ByteBuffer::ByteBuffer(const void* buf, size_t size, size_t capacity) : _capacity(capacity), _size(size)
{
    if (size > capacity) {
        throw std::runtime_error("size cannot be greater than capacity");
    }

    allocate(capacity);
    std::memcpy(_data.get(), buf, size);
}

ByteBuffer& ByteBuffer::operator=(const ByteBuffer& rhs)
{
    _size = rhs._size;
    _capacity = rhs._capacity;

    allocate(_capacity);
    std::memcpy(_data.get(), rhs._data.get(), _size);
    return *this;
}

ByteBuffer& ByteBuffer::operator=(ByteBuffer&& rhs) noexcept
{
    swap(rhs);
    return *this;
}

bool ByteBuffer::operator==(const ByteBuffer& rhs) const
{
    return _size == rhs._size && memcmp(data(), rhs.data(), _size) == 0;
}

bool ByteBuffer::operator<(const ByteBuffer& rhs) const
{
    if (size() < rhs.size()) return true;
    if (size() > rhs.size()) return false;

    return memcmp(data(), rhs.data(), size()) < 0;
}

void ByteBuffer::append(const void* buf, size_t append_size)
{
    if (append_size == 0) return;

    size_t new_size = append_size + _size;
    if (new_size > _capacity) {
        extend(new_size);
    }

    std::memcpy(end(), buf, append_size);
    _size = new_size;
}

void ByteBuffer::clear()
{
    _data.reset(nullptr);
    _size = 0;
    _capacity = 0;
}

void ByteBuffer::allocate(size_t capacity)
{
    _data.reset(new uint8_t[capacity]);
}

void ByteBuffer::extend(size_t new_capacity)
{
    if (new_capacity > _capacity && new_capacity < 2 * _capacity)
        new_capacity = 2 * _capacity;

    auto new_buf = new uint8_t[new_capacity];

    if (_size != 0) {
        std::memcpy(new_buf, _data.get(), _size);
    }
    _data.reset(new_buf);

    _capacity = new_capacity;
}

void ByteBuffer::swap(ByteBuffer &rhs)
{
    std::swap(_capacity, rhs._capacity);
    std::swap(_size, rhs._size);
    _data.swap(rhs._data);
}
