/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/

#include "SDL_mouse.h"
#include "bstone_exception.h"
#include "bstone_memory_pool_1x.h"
#include "bstone_sys_sdl_exception.h"
#include "bstone_sys_sdl_mouse_mgr.h"

namespace bstone {
namespace sys {

namespace {

class SdlMouseMgr final : public MouseMgr
{
public:
	SdlMouseMgr(Logger& logger);
	~SdlMouseMgr() override;

	static void* operator new(std::size_t count);
	static void operator delete(void* ptr) noexcept;

private:
	Logger& logger_;

private:
	void do_set_relative_mode(bool is_enable) override;
};

// ==========================================================================

using SdlMouseMgrPool = MemoryPool1XT<SdlMouseMgr>;

SdlMouseMgrPool sdl_mouse_mgr_pool{};

// ==========================================================================

SdlMouseMgr::SdlMouseMgr(Logger& logger)
try
	:
	logger_{logger}
{
	logger_.log_information("Start up SDL mouse manager.");
}
BSTONE_STATIC_THROW_NESTED_FUNC

SdlMouseMgr::~SdlMouseMgr()
{
	logger_.log_information("Shut down SDL mouse manager.");
}

void* SdlMouseMgr::operator new(std::size_t count)
try
{
	return sdl_mouse_mgr_pool.allocate(count);
}
BSTONE_STATIC_THROW_NESTED_FUNC

void SdlMouseMgr::operator delete(void* ptr) noexcept
{
	sdl_mouse_mgr_pool.deallocate(ptr);
}

void SdlMouseMgr::do_set_relative_mode(bool is_enable)
{
	sdl_ensure_result(SDL_SetRelativeMouseMode(is_enable ? SDL_TRUE : SDL_FALSE));
}

} // namespace

// ==========================================================================

MouseMgrUPtr make_sdl_mouse_mgr(Logger& logger)
{
	return std::make_unique<SdlMouseMgr>(logger);
}

} // namespace sys
} // namespace bstone
