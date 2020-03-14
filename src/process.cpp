#include "process.h"

using namespace XLib;

XLib::Process::Process( const XLib::maps_t& maps,
                        const std::string fullName,
                        pid_t pid )
 : _maps( maps ), _fullName( fullName ), _pid( pid )
{}

auto XLib::Process::refresh() -> void
{
    _maps = MemoryUtils::queryMaps( _pid );
}

auto XLib::Process::maps()
{
    refresh();
    return _maps;
}

auto XLib::Process::setFullName( const std::string& fullName ) -> void
{
    _fullName = fullName;
}

auto XLib::Process::fullName() const
{
    return _fullName;
}

auto XLib::Process::setPID( pid_t pid )
{
    _pid = pid;
    refresh();
}

auto XLib::Process::pid()
{
    return _pid;
}