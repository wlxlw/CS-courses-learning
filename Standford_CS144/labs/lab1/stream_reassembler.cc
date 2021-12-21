#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

//template <typename... Targs>
//void DUMMY_CODE(Targs &&... /* unused */) {}
#include<iostream>
using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity):_output(capacity),_capacity(capacity),_win(), _win_s(), _ack(0), _win_s_true_cnt(0),_eof_index(-1) {
  //_output(capacity);
  //_capacity(capacity);
 // _output = ByteStream(capacity);
 // _capacity = capacity;
  //_ack = 0;
  //_win_s_true_cnt = 0 ;
  // for(size_t i=0;i<capacity;i++){
  //   _win.push_back(" ");
  //   _win_s.push_back(false);
  // }
  printf("Test begin capacity: %zu\n",capacity);
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
   
    if(eof){
      _eof_index = index+data.size();
      if(_ack == _eof_index){
      _output.end_input();
      }
    }
    if(data.size()==0)
      return;
    //DUMMY_CODE(data, index, eof);
    size_t end_index = index+data.size()-1;
    size_t data_start_index;
    //size_t data_end_index;
    size_t stream_start_index;
    size_t stream_end_index;
    
    // 0         data.size()-1
    //index      end_index
    //     _ack               _ack+_win.size()-1
    if(end_index>=_ack){//有交集
      stream_start_index = max(_ack,index);
      // stream_end_index = min(_ack+_win.size()-1,end_index);
      stream_end_index = min(_ack+_capacity-_output.buffer_size()-1,end_index);
      while(_win.size() < _capacity-_output.buffer_size()){
        _win.push_back(" ");
        _win_s.push_back(false);
      }

//printf("%zu %zu\n",_ack+_capacity-_output.buffer_size()-1,end_index);
//printf("%zu %zu %zu %zu\n",_ack,_capacity,_output.buffer_size(),_output.bytes_read());
cout<<"**input**string \""<<data<<"\" ack "<<_ack<<" eof "<<eof<<" index "<<index<<endl;
printf("  stream_start_index:%zu stream_end_index:%zu\n",stream_start_index,stream_end_index);
//printf("bytestream %zu win %zu\n",_output.buffer_size(),_win.size());
      if(stream_end_index>=stream_start_index){
        data_start_index = stream_start_index-index;
        //data_end_index = stream_end_index-end_index+data.size()-1;

        for(size_t i=0;i<=stream_end_index-stream_start_index;i++){
          if(_win_s[i+stream_start_index-_ack]==false){
          _win[i+stream_start_index-_ack][0] = data[i+data_start_index];
          _win_s[i+stream_start_index-_ack] = true;
          _win_s_true_cnt++;
          }
        }
        while(_win_s.size()>=1 && _win_s[0]){
printf("  bytestream write ");
cout<<"\""<<_win.front()<<"\"";
          _output.write(_win.front());
          _win.pop_front();
          _win_s.pop_front();
          _ack++;
          _win_s_true_cnt--;
printf(" ack %zu\n",_ack);        
        }
      }
    }
printf("  ack %zu _eof_index %zu\n", _ack,_eof_index);
    if(_ack == _eof_index){
      _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _win_s_true_cnt; }

bool StreamReassembler::empty() const {
  if(_win_s_true_cnt==0 && _output.buffer_empty())//接收窗口没有读取数据，且_output为空
    return true;
  else 
    return false;
}
