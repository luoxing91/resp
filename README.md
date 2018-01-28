# resp

 RESP(Redis Serialization Protocol) modern C++ Implementation

# Features
* RESP C++1y

# Supported Compilers
* Apple LLVM version 9.0.0 (clang-900.0.39.2)

# CMake Options

* RESP_BUILD_TEST: build resp's unit tests, default ON.
* RESP_BUILD_EXAMPLE: build resp's examples, default ON.

# Encode

```cpp
#include <resp/all.hpp>
#include <cassert>

int main()
{
  /// Create encoder.
  resp::encoder<resp::buffer> enc;
  
  /// Encode redis cmd SET key value.
  /**
  * @note Return at least one buffer, may multi, if multi, 
  *   user could use scatter-gather io to send them, this is for optimizing copy.
  */
  std::vector<resp::buffer> buffers = enc.encode("SET", "key", "value");
  /// buffers == "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n"
  assert(buffers.empty() == false);
}
```

# Decode

```cpp
#include <resp/all.hpp>
#include <cassert>
#include <cstdlib>

int main()
{
  /// Create decoder.
  resp::decoder dec;
  
  /// This is what reply from redis server we supposed for.
  char reply[64] = "+OK\r\n";
  size_t s = std::strlen(reply);
  
  /// Decode buffer from redis server's reply.
  /// It should be "OK".
  resp::result res = dec.decode(reply, s);
  assert(res == resp::completed);
  resp::unique_value rep = res.value();
  assert(rep.type() == resp::ty_string);
  assert(rep.string() == "OK");
}
```

# Other examples

Look for resp/examples.
