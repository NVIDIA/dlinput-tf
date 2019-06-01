#!/bin/bash

# check and install msgpack-c 

filename="/tmp/test_msgpack.cc"
execfile="/tmp/test_msgpack"
tmpfile=$(mktemp ${filename})

content="
#include <msgpack.hpp>
#include <sstream>

int main(void)
{
    msgpack::type::tuple<int, bool> src(1, true);
    std::stringstream buffer;
    msgpack::pack(buffer, src);
    return 0;
}
"

echo "$content" > $filename
if g++ -o $execfile $filename; then 
    echo "msgpack installed";
else 
    git clone https://github.com/msgpack/msgpack-c.git
    cd msgpack-c
    cmake .
    make
    # sudo make install
    cd ..
    rm -rf msgpack-c  
fi

rm -rf "$execfile"
rm -rf "$tmpfile"