#ifndef PCH_H
#define PCH_H

/* OS specific stuffs */
#ifndef WINDOWS
    #include <dlfcn.h>
    #include <fcntl.h>
    #include <link.h>

    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/uio.h>
    #include <sys/wait.h>

    #include <linux/limits.h>
#else
    #include <dbghelp.h>
    #include <tlhelp32.h>
    #include <windows.h>
#endif

/* std */
#include <algorithm>
#include <array>
#include <bitset>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <concepts>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <random>
#include <regex>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

#include <unistd.h>

/* CryptoPP */
#include <vendor/cryptopp/aes.h>
#include <vendor/cryptopp/modes.h>
#include <vendor/cryptopp/osrng.h>
#include <vendor/cryptopp/randpool.h>
#include <vendor/cryptopp/rdrand.h>
#include <vendor/cryptopp/rng.h>
#include <vendor/cryptopp/rsa.h>
#include <vendor/cryptopp/sha.h>
#include <vendor/cryptopp/zlib.h>

/* SIMD */
#include <immintrin.h>

#endif