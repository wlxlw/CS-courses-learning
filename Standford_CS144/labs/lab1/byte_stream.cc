#include "byte_stream.hh"
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):_buf(),_end_input(false),_bytes_written(0),_bytes_read(0),_capacity(capacity) {}



size_t ByteStream::write(const string &data) {
    size_t cnt = 0;
     size_t len = min(remaining_capacity(), data.size());
    for (size_t i = 0; i< len; i++) {//不能在循环中判断i<remaining_capacity()
        _buf.push_back(data[i]);
        _bytes_written++;
        cnt++;
    }
    return cnt;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    string ans = "";
    for (size_t i = 0; i < len&& i<_buf.size(); i++) {
        ans += _buf[i];
    }
    return ans;
}


//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t now_buf_size = _buf.size();//不能再循环中直接i<_buf,size()
    for (size_t i = 0; i < len && i<now_buf_size; i++) {
        _bytes_read++;    
	_buf.pop_front();
    }
}


//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // 0DUMMY_CODE(len);
    string ans = peek_output(len);
    pop_output(len);
    return ans;
}

void ByteStream::end_input() { _end_input = true; }

bool ByteStream::input_ended() const {return _end_input;}

size_t ByteStream::buffer_size() const { return _buf.size(); }

bool ByteStream::buffer_empty() const {
    return _buf.empty();
}

bool ByteStream::eof() const {
    if (buffer_empty()&&input_ended())
        return true;
    else
        return false;
}

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { 
	return _capacity - _buf.size();
}
