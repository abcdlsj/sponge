#include "stream_reassembler.hh"

#include <cstddef>
#include <pthread.h>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _waiting_buffer({})
    , _unassembled_bytes_size(0)
    , _flag_eof(false)
    , _pos_eof(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::insert_buffer(const Segment &s) {
    if (_waiting_buffer.empty()) {
        _waiting_buffer.insert(s);
        _unassembled_bytes_size += s._data.size();
        return;
    }
    Segment c = s;
    size_t idx = c._index, sz = c._data.size();
    auto it = _waiting_buffer.lower_bound(s);
    if (it != _waiting_buffer.begin()) {
        it--;
        if (it->_index + it->_data.size() > idx) {
            if (idx + sz <= it->_index + it->_data.size())
                return;
            c._data = it->_data + c._data.substr(it->_index + it->_data.size() - idx);
            c._index = it->_index;
            idx = c._index, sz = c._data.size();
            _unassembled_bytes_size -= it->_data.size();
            _waiting_buffer.erase(it++);
        } else {
            it++;
        }
    }
    while (it != _waiting_buffer.end() && idx + sz > it->_index) {
        if (idx >= it->_index && idx + sz <= it->_index + it->_data.size())
            return;
        if (idx + sz < it->_index + it->_data.size()) {
            c._data += it->_data.substr(idx + sz - it->_index);
            sz = c._data.size();
        }
        _unassembled_bytes_size -= it->_data.size();
        _waiting_buffer.erase(it++);
    }
    _unassembled_bytes_size += c._data.size();
    _waiting_buffer.insert(c);
}

void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    auto s = Segment(index, data);
    auto first_unread_idx = _output.bytes_read();
    auto first_unassembled_idx = _output.bytes_written();
    auto first_unacceptable_idx = first_unread_idx + _capacity;

    if (index >= first_unacceptable_idx || index + data.size() < first_unassembled_idx)
        return;
    if (index + data.size() > first_unacceptable_idx) {
        s._data = s._data.substr(0, first_unacceptable_idx - index);
    }
    if (index <= first_unassembled_idx) {
        _output.write(s._data.substr(first_unassembled_idx - index));
        auto it = _waiting_buffer.begin();
        while (it->_index <= _output.bytes_written() && !_waiting_buffer.empty()) {
            if (it->_index + it->_data.size() > s._index + s._data.size()) {
                _output.write(it->_data.substr(_output.bytes_written() - it->_index));
            }
            _unassembled_bytes_size -= it->_data.size();
            _waiting_buffer.erase(it++);
        }
    } else {
        insert_buffer(s);
    }
    if (eof) {
        _flag_eof = true;
        _pos_eof = index + data.size();
    }
    if (_flag_eof && _output.bytes_written() == _pos_eof) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes_size; }

bool StreamReassembler::empty() const { return _unassembled_bytes_size == 0; }
