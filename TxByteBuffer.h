#include <Arduino.h>

template <size_t SIZE>
class TxByteBuffer {
public:

    void begin() {
        _head = 0;
        _tail = 0;
    }

    bool push(uint8_t byte) {
        size_t next = (_head + 1) % SIZE;
        if (next == _tail)
            return false; // full

        _buffer[_head] = byte;
        _head = next;
        return true;
    }

    bool pop(uint8_t& byte) {
        if (_head == _tail)
            return false; // empty

        byte = _buffer[_tail];
        _tail = (_tail + 1) % SIZE;
        return true;
    }

    bool empty() const {
        return _head == _tail;
    }

    size_t freeSpace() const {
        return (SIZE + _tail - _head - 1) % SIZE;
    }

private:
    uint8_t _buffer[SIZE];
    size_t _head = 0;
    size_t _tail = 0;
};
