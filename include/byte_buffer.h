#ifndef _PORGI_BYTE_BUFFER_H_
#define _PORGI_BYTE_BUFFER_H_

#include "easylogging++.h"

#include <cstddef>
#include <memory>
#include <string>

class ByteBuffer {
public:
    explicit ByteBuffer(size_t capacity = 0);
    explicit ByteBuffer(size_t size, uint8_t fill);
    explicit ByteBuffer(const void* buf, size_t size) : ByteBuffer(buf, size, size) { }
    explicit ByteBuffer(const void* buf, size_t size, size_t capacity);
    explicit ByteBuffer(const std::string& str) : ByteBuffer(str.c_str(), str.length()) { }

    ByteBuffer(const ByteBuffer& other) : ByteBuffer(other._data.get(), other._size, other._capacity) { }
    ByteBuffer(ByteBuffer&& other) { swap(other); }

    ByteBuffer& operator=(const ByteBuffer& rhs);
    ByteBuffer& operator=(ByteBuffer&& rhs) noexcept;
    bool operator==(const ByteBuffer& rhs) const;
    bool operator<(const ByteBuffer& rhs) const;

    uint8_t& operator[](size_t index) { return _data[index]; }
    uint8_t operator[](size_t index) const { return _data[index]; }

    uint8_t* begin() { return _data.get(); }
    const uint8_t* begin() const { return _data.get(); }
    uint8_t* end() { return _data.get() + _size; }
    const uint8_t* end() const { return _data.get() + _size; }

    uint8_t* data() { return _data.get(); }
    const uint8_t* data() const { return _data.get(); }
    std::string to_string() const { return std::string(reinterpret_cast<const char*>(data()), size()); }

    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }

    void append(const void* buf, size_t append_size);
    void append(const ByteBuffer& rhs) { append(rhs._data.get(), rhs._size); }

    void clear();

    template <typename... T>
    void append(ByteBuffer buffer, T&& ... buffers)
    {
        append(std::forward<ByteBuffer>(buffer));
        append(std::forward<T>(buffers)...);
    }

private:
    size_t _capacity{};
    size_t _size{};
    std::unique_ptr<uint8_t[]> _data;

    static void append(); 
    void allocate(size_t capacity);
    void extend(size_t new_capacity);
    void swap(ByteBuffer& rhs);
};

inline MAKE_LOGGABLE(ByteBuffer, buffer, os) {
    auto pb = std::make_unique<char[]>(buffer.size() + 1);

    std::memcpy(pb.get(), buffer.data(), buffer.size());
    pb[buffer.size()] = '\0';
    os << pb.get();

    return os;
}

namespace std {

  template <>
  struct hash<ByteBuffer>
  {
    std::size_t operator()(const ByteBuffer& buf) const
    {
        size_t result = 0;
        const size_t prime = 31;
        for (size_t i = 0; i < buf.size(); ++i) {
            result = buf.data()[i] + (result * prime);
        }
        return result;
    }
  };
}

#endif

