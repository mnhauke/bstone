/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/

#include "bstone_exception.h"
#include "bstone_memory_pool_1x.h"
#include "bstone_sys_sdl_window.h"
#include "bstone_sys_sdl_window_mgr.h"

namespace bstone {
namespace sys {

namespace {

class SdlWindowMgr final : public WindowMgr
{
public:
	SdlWindowMgr(Logger& logger);
	~SdlWindowMgr() override;

	static void* operator new(std::size_t count);
	static void operator delete(void* ptr) noexcept;

private:
	Logger& logger_;

private:
	WindowUPtr do_make_window(const WindowInitParam& param) override;
};

// ==========================================================================

using SdlWindowMgrPool = MemoryPool1XT<SdlWindowMgr>;

SdlWindowMgrPool sdl_window_mgr_pool{};

// ==========================================================================

SdlWindowMgr::SdlWindowMgr(Logger& logger)
	:
	logger_{logger}
{
	logger_.log_information("Start up SDL window manager.");
}

SdlWindowMgr::~SdlWindowMgr()
{
	logger_.log_information("Shut down SDL window manager.");
}

void* SdlWindowMgr::operator new(std::size_t count)
try
{
	return sdl_window_mgr_pool.allocate(count);
}
BSTONE_STATIC_THROW_NESTED_FUNC

void SdlWindowMgr::operator delete(void* ptr) noexcept
{
	sdl_window_mgr_pool.deallocate(ptr);
}

WindowUPtr SdlWindowMgr::do_make_window(const WindowInitParam& param)
{
	return make_sdl_window(logger_, param);
}

} // namespace

// ==========================================================================

WindowMgrUPtr make_sdl_window_mgr(Logger& logger)
{
	return std::make_unique<SdlWindowMgr>(logger);
}

} // namespace sys
} // namespace bstone
