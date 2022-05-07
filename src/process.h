#ifndef PROCESS_H
#define PROCESS_H

#include "exception.h"
#include "memorymap.h"
#include "memoryutils.h"
#include "patternbyte.h"
#include "processbase.h"
#include "processmemoryarea.h"
#include "processmemorymap.h"
#include "runnabletask.h"

namespace XKLib
{
    class Process : public ProcessBase
    {
      public:
        static inline constexpr process_id_t INVALID_PID    = -1;
        static inline constexpr std::size_t TASK_STACK_SIZE = 0x10000;

      public:
        static auto self() -> Process;
        static auto find(const std::string& name) -> Process;
        static auto ProcessName(const process_id_t pid) -> std::string;

      public:
        Process();
        explicit Process(const process_id_t pid);

      public:
        auto tasks() const -> tasks_t;
        auto mmap() const -> const ProcessMemoryMap&;
        auto search(PatternByte& patternByte) const -> void;

      public:
        auto mmap() -> ProcessMemoryMap&;

      public:
        template <std::size_t N = TASK_STACK_SIZE>
        auto createTask(const ptr_t routineAddress) -> RunnableTask<N>
        {
            return RunnableTask<N>(*this, routineAddress);
        }

        auto read(const auto address, const std::size_t size) const
          -> bytes_t
        {
            return _mmap.read(address, size);
        }

        auto write(const auto address, const bytes_t& bytes) const -> void
        {
            _mmap.write(address, bytes);
        }

        auto write(const auto address,
                   const auto ptr,
                   const std::size_t size) const -> void
        {
            _mmap.write(address, ptr, size);
        }

      public:
        auto allocArea(const auto address,
                       const std::size_t size,
                       const mapf_t flags) -> ptr_t
        {
            return _mmap.allocArea<decltype(address)>(address,
                                                      size,
                                                      flags);
        }

        auto freeArea(const auto address, const std::size_t size) -> void
        {
            _mmap.freeArea<decltype(address)>(address, size);
        }

        auto protectMemoryArea(const auto address,
                               const std::size_t size,
                               const mapf_t flags) -> void
        {
            _mmap.protectMemoryArea(address, size, flags);
        }

        auto forceWrite(const auto address, const bytes_t& bytes) -> void
        {
            _mmap.forceWrite(address, bytes);
        }

        auto forceWrite(const auto address,
                        const auto ptr,
                        const std::size_t size) -> void
        {
            _mmap.forceWrite(address, ptr, size);
        }

      private:
        std::string _full_name;
        ProcessMemoryMap _mmap;
    };
}

#endif
