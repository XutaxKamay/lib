#ifndef TIMER_H
#define TIMER_H

#include "types.h"

namespace XKLib
{
    template <typename T>
    concept floating_point = std::is_same<float, T>::value || std::
      is_same<double, T>::value;

    class Timer
    {
        public:
            auto start() -> void;
            auto end() -> void;
            auto nanos() -> uint16_t;
            auto micros() -> uint16_t;
            auto millis() -> uint16_t;
            auto seconds() -> uint64_t;
            auto difference() -> uint64_t;

        private:
            std::chrono::high_resolution_clock::time_point _start;
            std::chrono::high_resolution_clock::time_point _end;
            uint64_t _difference;
    };
};

#endif
