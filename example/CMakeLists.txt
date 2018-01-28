#
# This file is part of the CMake build system for RESP
#
# Copyright (c) 2016 Nous Xiong (348944179 at qq dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# See https://github.com/nousxiong/resp for latest version.
#

# RESP base example.
function(add_demo value)
add_executable (${value} ${value}.cc)
target_link_libraries(${value} boost_system.a)
install (TARGETS ${value} RUNTIME DESTINATION bin)
endfunction(add_demo)

add_demo (base)
add_demo(demo)
# RESP advance example.
add_demo (reuse)

# RESP advance example.
add_demo (pipeline)

# RESP advance example.
add_demo (incomplete)

# RESP advance example.
add_demo (array)
