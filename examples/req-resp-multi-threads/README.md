# hello-parodus

Example illustrating simple event interaction with parodus.

[![Apache V2 License](http://img.shields.io/badge/license-Apache%20V2-blue.svg)](https://github.com/Comcast/libparodus/blob/master/LICENSE.txt)

# Building Instructions

```
mkdir build
cd build
cmake ..
make
```

# Running the application.
1. Ensure parodus is running first successfully.
2. Start the application after building, like so - ```./src/hello-parodus -p <parodus local URL> -c <URL to receive parodus response>``` 
3. Command line options:
4. -n <parallel threads>	default is 1
5. -a request delay option for all threads is random (0 .. 1048575) usecs
6. -n no delays
7. -1 request delay option for thread 1, other threads no delay
8. -2 request delay for all threads except thread 1
