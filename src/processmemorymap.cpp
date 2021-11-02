#include "processmemorymap.h"
#include "process.h"
#include <regex>
#include <utility>

#ifndef WINDOWS
    #include <fstream>
    #include <unistd.h>

    #include <sys/file.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/types.h>
#else
    #include <windows.h>
#endif

using namespace XKLib;

ProcessMemoryMap::ProcessMemoryMap(ProcessBase process)
 : _process_base(process)
{
    refresh();
}

auto ProcessMemoryMap::refresh() -> void
{
    if (_process_base.id() == Process::INVALID_PID)
    {
        return;
    }

    _areas.clear();

#ifndef WINDOWS
    std::ifstream file_memory_map(
      "/proc/" + std::to_string(_process_base.id()) + "/maps");
    std::string line;

    if (!file_memory_map.is_open())
    {
        XLIB_EXCEPTION("Couldn't open /proc/"
                       + std::to_string(_process_base.id()) + "/maps");
    }

    while (std::getline(file_memory_map, line))
    {
        uintptr_t start {}, end {};
        std::string prot = "????";
        std::string name = "unknown";

        /* abcd-1000abcd rwxp %x %i:%i %i %s */
        /* 7ff1b37fb000-7ff1b3da9000 r--p 00000000 fe:00 1841992 [ ]+
         * /usr/lib/locale/locale-archive */

        /**
         * This is maybe faster, so I'm just commenting it out, though it
         * assumes that /proc/pid/maps is always correct.. Indeed it
         * should be. I don't know why it shouldn't!
         * Better to be safe than sorry.
         */

//        std::sscanf(line.c_str(),
//                    "%p-%p %c%c%c",
//                    view_as<ptr_t*>(&start),
//                    view_as<ptr_t*>(&end),
//                    &prot[0],
//                    &prot[1],
//                    &prot[2]);

        constexpr auto REGEX_HEX_NUMBER = "[0-9a-f]+";
        constexpr auto REGEX_PROT       = "[r-][w-][x-]p";
        constexpr auto REGEX_NAME = "[0-9a-f]+-[0-9a-f]+ [r-][w-][x-]p "
                                    "[0-9a-f]+ "
                                    "[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f] "
                                    "[0-9]+[ ]+";

        std::smatch match;
        std::regex regex_hex_number(REGEX_HEX_NUMBER);

        if (std::regex_search(line, match, regex_hex_number))
        {
            std::stringstream(match[0].str()) >> std::hex >> start;

            auto suffix = match.suffix().str();

            if (std::regex_search(suffix, match, regex_hex_number))
            {
                std::stringstream(match[0].str()) >> std::hex >> end;

                suffix = match.suffix().str();

                std::regex regex_prot(REGEX_PROT);

                if (std::regex_search(suffix, match, regex_prot))
                {
                    prot = match[0].str();
                }
                else
                {
                    XLIB_EXCEPTION("Could not find memory protections");
                }
            }
            else
            {
                XLIB_EXCEPTION("Could not find end address");
            }
        }
        else
        {
            XLIB_EXCEPTION("Could not find start address");
        }

        if (prot.size() != 4)
        {
            XLIB_EXCEPTION("Memory protection should have matched 4 "
                           "characters");
        }

        std::regex regex_name(REGEX_NAME);

        /* Sometimes there's no name */
        if (std::regex_search(line, match, regex_name))
        {
            name = line.substr(match[0].str().size(), line.size());
        }

        auto is_on = [](char prot)
        {
            if (prot == '-')
            {
                return false;
            }

            return true;
        };

        auto area = std::make_shared<ProcessMemoryArea>(_process_base);
        area->initProtectionFlags(
          (is_on(prot[0]) ? MemoryArea::ProtectionFlags::READ : 0)
          | (is_on(prot[1]) ? MemoryArea::ProtectionFlags::WRITE : 0)
          | (is_on(prot[2]) ? MemoryArea::ProtectionFlags::EXECUTE : 0));
        area->setAddress(view_as<ptr_t>(start));
        area->setSize(end - start);
        area->setName(name);

        _areas.push_back(std::move(area));
    }

    file_memory_map.close();

#else
    auto process_handle = OpenProcess(PROCESS_QUERY_INFORMATION,
                                      false,
                                      _process_base.id());

    if (process_handle == nullptr)
    {
        XLIB_EXCEPTION("Couldn't open process from pid: "
                       + std::to_string(_process_base.id()));
    }

    MEMORY_BASIC_INFORMATION info;
    data_t bs;
    char module_path[MAX_PATH];

    for (bs = nullptr;
         VirtualQueryEx(process_handle, bs, &info, sizeof(info))
         == sizeof(info);
         bs += info.RegionSize)
    {
        auto area = std::make_shared<ProcessMemoryArea>(_process_base);
        area->setAddress(bs);
        area->setSize(info.RegionSize);
        area->initProtectionFlags(
          ProcessMemoryArea::ProtectionFlags::ToOwn(info.Protect));

        if (GetModuleFileNameA(view_as<HMODULE>(info.AllocationBase),
                               module_path,
                               sizeof(module_path)))
        {
            area->setName(module_path);
        }
        else
        {
            area->setName("unknown");
        }

        _areas.push_back(std::move(area));
    }

    CloseHandle(process_handle);
#endif
}
