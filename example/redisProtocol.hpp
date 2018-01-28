#pragma once 
#include <string>
#include <boost/asio/buffer.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/asio/buffer.hpp>

namespace redis{
using error_code = boost::system::error_code;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
//struct redis_body;

enum type{
    simple,
    errors,
    integers,
    bulkStrings,
    array,
};
using buffers_iterator = boost::asio::buffers_iterator<boost::asio::const_buffer>;
struct redis_base{
    virtual void decode(boost::asio::mutable_buffer& buffer) = 0;
};
template<char c>
struct redis_body :public redis_base{
    int findReturn(buffers_iterator const & start,
                   buffers_iterator const & end){
        if(*start != c){
            throw std::runtime_error{"request" +c };
        }
        
        auto pos = std::find(start, end, '\r');
        if(pos == end){
            throw std::runtime_error{"request \r" };
        }
        
        if(*(start+pos+1) != '\n'){
            throw std::runtime_error{"request \r\n" };
        }
        return pos;
    }
};

template<typename T> 
void append(boost::asio::mutable_buffer& buffer, T && v){
    //todo
}
struct redisArray:public redis_body<'*'>{
    std::vector<std::shared_ptr<redis_base>> a;
    void decode(boost::asio::mutable_buffer& buffer){
        append(buffer, "*");
        append(buffer, a.size());
        append(buffer, "\r\n");
        for(auto &re:a){
            re->decode(buffer);
        }
    }
    static std::shared_ptr<redis_base>
    encode(buffers_iterator start, buffers_iterator end){
        int index = findReturn(start, end);
        int64_t size = std::atoi(std::string(start+1, index));
        start += index+1;
        std::vector<std::shared_ptr<redis_base>> a;
        for(int i=0;i< size;i++){
            a.push_back(encode(start, end));
            start += findReturn(start, end) + 1;
        }
        return redisArray(std::move(a));
    }
};

template <char c>
struct redis_body_string:public redis_body<c>{
    std::string str;
    explicit
    redis_body_string(buffers_iterator start, buffers_iterator end)
            :str(start, end){}
    void decode(boost::asio::mutable_buffer& buffer){
        append(buffer, c);
        append(buffer, str);
        append(buffer, "\r\n");
    }
    void encode(buffers_iterator start, buffers_iterator end){
        str  = std::string(start+1, start+pos);
    }
};

using redis_body_simple = redis_body_string<'+'> ;
using redis_body_error = redis_body_string<'-'> ;


template<>
struct redis_body<'$'>{
    std::string str;
    redis_body_bulk(std::string && str)
            :str(start+1, end){}
    void decode(boost::asio::mutable_buffer& buffer){
        append(buffer, '$');
        append(buffer, str.size());
        append(buffer, "\r\n");
        
        append(buffer, str);
        append(buffer, "\r\n");
    }
    static redis_body encode(buffers_iterator start, buffers_iterator end){
        redis_body<'$'> ex;
        return ex;
    }
};
struct redis_body_null_bulk_string :public redis_body<'$'>{
    void decode(boost::asio::mutable_buffer& buffer){
        append(buffer, "$-1\r\n");  
    }
};

template<>
struct redis_body<':'>{
    redis_body(buffers_iterator start, buffers_iterator end){
    }
};
using redis_body_integer = redis_body<':'>;

template<char c>
std::shared_ptr<redis_base> encode_(buffers_iterator start, buffers_iterator end);

std::shared_ptr<redis_base> encode(buffers_iterator start, buffers_iterator end){
    if(start != end){
        throw std::runtime_error{"unknow action"};
    }
    if(*start == '+'){
        return encode_<'+'>(start+1, end);
    }else if(*start == '-'){
        return encode_<'-'>(start+1, end);
    }else if(*start == ':'){
        return encode_<':'>(start+1, end);
    }else if(*start == '$'){
        return encode_<'$'>(start+1, end);
    }else if(*start == '*'){
        return encode_<'*'>(start+1, end);
    }else{
        throw std::runtime_error{"unknow action"};
    }
}

template<char c>
std::shared_ptr<redis_base> encode(boost::asio::const_buffer& b){
    return encode<c>(buffers_begin(b), buffers_end(b));
}


static uint64_t lineEndPosition(buffers_iterator start, buffers_iterator end){
    auto p = std::find(start, end , '\r');
    if(p == end){
        throw std::runtime_error{"find not '\r'"};
    }
    return p-start;
}

std::shared_ptr<redis_base>
encode_array(buffers_iterator start,
             buffers_iterator end){
    uint64_t len = lineEndPosition(start, end);
    int64_t size = std::atoi(std::string(start, start+len).c_str());
    start += len+1;
    std::vector<std::shared_ptr<redis_body>> a;
    for(int i= 0; i< size; i++){
        len = lineEndPosition(start, end);
        a.push_back(encode(start, start+len));
        start += len+1;
    }
    return std::shared_ptr<redis_body>(new redis_body_array(std::move(a)));
}

std::shared_ptr<redis_body>
encode_simple(buffers_iterator start ,
              buffers_iterator end){
    return std::shared_ptr<redis_body>
            (new redis_body_simple(start, start+lineEndPosition(start, end)));
}

    
static std::shared_ptr<redis_body>
make_bulk(buffers_iterator start,
          buffers_iterator end){
    uint64_t len = lineEndPosition(start, end);
    uint64_t i = std::atoi(std::string(start, start+len).c_str());
    start = start+len+1;
    if(i == -1){
        return std::shared_ptr<redis_body>( new redis_body_null_bulk_string());
    }
    return std::shared_ptr<redis_body>( new redis_body_bulk(start, start+ i));
}
    
static  std::shared_ptr<redis_body>
redis_body_integer(buffers_iterator start,
                   buffers_iterator end){
    std::string i = std::string(start, start+lineEndPosition(start, end));
    return redis_body_integer(std::atoi(i)); // int64_t;
}
    static redis_body redis_body_error(buffers_iterator start,
                                       buffers_iterator end){
        return redis_body(start, start+line_size(start, end), true)
    }
    redis_body(buffers_iterator start,
               buffers_iterator end, bool isError)
            :isError_(isError), message(start, end){
    }

void write(tcp::socket& socket, redis_body& body){
    
    boost::system::error_code ignored_error;
    boost::asio::write(socket, boost::asio::buffer(body.dump()),
                       boost::asio::transfer_all(), ignored_error);
    
}
void read(tcp::socket& socket, redis_body& body){
    boost::array<char, 128> buf;
    boost::system::error_code error;
    size_t len = socket.read_some(boost::asio::buffer(buf), error);
    if(error == boost::asio::error::eof){
        std::cout << "endl";
    }else if(error){
        throw boost::system::system_error(error); 
    }
    std::cout.write(buf.data(), len);
}

}
