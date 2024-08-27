# grpc-two-way

## Concept

In a two-way grpc setup, the server acts, in a way, as the middleware to the clients.
Each client connects to the server and provides to the server its own port number.
The server uses the client IP address and provided port number to connect to the client's own gRPC server.

The server stores all of its clients in a list and upon a failure, assumes the client is down.
It is up to the client to re-establish connection. The provided examples cover that as well.


## Requirements
1. C++ Compiler (g++, clang, msbuild) with C++ 14 or higher support. 


### Examples

### Requirements
1. Python 3.6 or higher
2. g++ with C++ 14 or higher support
3. CMake
4. make

### Build
1. Create Python Virtual Environment (this can be skipped if you don't mind installing conan on your main pip)
```
python3 -m venv .venv
```

2. Activate Python Virtual Environment (this can be skipped if you don't mind installing conan on your main pip)
```
. .venv/bin/activate
```

3. Install conan
```
pip install conan==2.2.2
```

4. Detect build environment
```
conan profile detect --force
```

5. Install dependencies
```
conan install . --output-folder=build --build=missing
```

6. Run CMake
```
cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
```

7. Run Make
```
make -j 4
```

### Run

1. Run server
```
build/chat_server
```

2. Run Client 1 (5558 is the port the client 2 will bind)
```
build/chat_client 5558
```

3. Run client 2 (778 is the port the client 2 will bind)
```
build/chat_client 7778
```

For each client, input a username and press enter.
Afterwards, you can type and press enter to send messages.
