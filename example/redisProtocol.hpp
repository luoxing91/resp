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
using buffers_iterator = boost::asio::buffers_iterator<boost::asio::const_buffer>;
struct redis_base{
    virtual void decode(boost::asio::mutable_buffer& buffer) = 0;
    //virtual void encode(boost::asio::const_buffer& buffer) = 0;
};

template<char c>
struct redis_body :public redis_base{
    static buffers_iterator findReturn(buffers_iterator const & start,
                                   buffers_iterator const & end){
        std::stringstream req;
        req << "request" << c;
        if(*start != c){
            throw std::runtime_error{req.str()};
        }
        auto pos = std::find(start, end, '\r');
    
        if(pos == end){
            throw std::runtime_error{"request \r" };
        }
    
    if(*(pos+1) != '\n'){
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
    using Store = std::vector<std::shared_ptr<redis_base>>;
    Store store;
    redisArray(Store && b):store(std::move(b)){}
    void decode(boost::asio::mutable_buffer& buffer){
        //boost::asio::mutable_buffer buffer;
        append(buffer, "*");
        append(buffer, store.size());
        append(buffer, "\r\n");
        for(auto &re:store){
            re->decode(buffer);
        }
    }
    
    static std::shared_ptr<redis_base>
    encode(buffers_iterator start, buffers_iterator end){
        buffers_iterator index = redisArray::findReturn(start, end);
        int64_t size = std::atoi(std::string(start+1, index).c_str());
        start = index+1;
        std::vector<std::shared_ptr<redis_base>> a;
        for(int i=0;i< size;i++){
            a.push_back(encode(start, end));
            start = findReturn(start, end) + 1;
        }
        return std::shared_ptr<redis_base>(new redisArray(std::move(a)));
    }
};

template <char c>
struct redisString:public redis_body<c>{
    std::string str;
    explicit
    redisString(std::string&& _str)
            :str(_str){}
    void decode(boost::asio::mutable_buffer& buffer){
        append(buffer, c);
        append(buffer, str);
        append(buffer, "\r\n");
    }
    static std::shared_ptr<redis_base>
    encode(buffers_iterator start, buffers_iterator end){
        buffers_iterator index = redisString::findReturn(start, end); 
        return std::shared_ptr<redis_base>(new redisString(std::string(start+1, index)));
    }
};

using redisSimple = redisString<'+'> ;
using redisError = redisString<'-'> ;



struct redisBulk:public redis_body<'$'>{
    std::string str;
    redisBulk(std::string && str_)
            :str(std::move(str_)){}
    void decode(boost::asio::mutable_buffer& buffer){
        append(buffer, '$');
        append(buffer, str.size());
        append(buffer, "\r\n");
        
        append(buffer, str);
        append(buffer, "\r\n");
    }
    
    static std::shared_ptr<redis_base> encode(buffers_iterator start, buffers_iterator end){
        buffers_iterator index  = redisBulk::findReturn(start, end);
        start = index+2;
        index =  redisBulk::findReturn(start, end);
        return std::shared_ptr<redis_base>(new redisBulk(std::string(start, index)));
    }
};
struct redisInteger:public redis_body<':'>{
    redisInteger(int && i):i_(i){}
    int i_;
    static std::shared_ptr<redis_base> encode(buffers_iterator start, buffers_iterator end){
        
        return std::shared_ptr<redis_base>
                (new redisInteger(std::atoi
                                  (std::string(start, findReturn(start,end)).c_str())));
                 
    }
    void decode(boost::asio::mutable_buffer& buffer){
         append(buffer, ':');
         append(buffer, i_); 
         append(buffer, "\r\n");
    }
};

struct redis_body_null_bulk_string :public redis_body<'$'>{
    void decode(boost::asio::mutable_buffer& buffer){
        append(buffer, "$-1\r\n");  
    }
};

using redis_body_integer = redis_body<':'>;

std::shared_ptr<redis_base> encode(buffers_iterator start, buffers_iterator end){
    if(start != end){
        throw std::runtime_error{"unknow action"};
    }
    if(*start == '+'){
        return redisSimple::encode(start, end);
    }else if(*start == '-'){
        return redisError::encode(start, end);
    }else if(*start == ':'){
        return redisInteger::encode(start, end);
    }else if(*start == '$'){
        return redisBulk::encode(start, end);
    }else if(*start == '*'){
        return redisBulk::encode(start, end);
    }else{
        throw std::runtime_error{"unknow action"};
    }
}



struct redis{
    std::shared_ptr<redis_base> data_;
    void write(tcp::socket& socket){
        boost::system::error_code ignored_error;
        boost::asio::mutable_buffer buffer;
        data_->decode(buffer);
        
        boost::asio::write(socket, buffer,
                       boost::asio::transfer_all(), ignored_error);
    }
    static redis read(tcp::socket& socket){
        boost::array<char, 128> buf;
        boost::system::error_code error;
        size_t len = socket.read_some(boost::asio::buffer(buf), error);
        if(error == boost::asio::error::eof){
            std::cout << "endl";
        }else if(error){
            throw boost::system::system_error(error); 
        }
        std::cout.write(buf.data(), len);
        redis r;
        return r;
    }

};



}
