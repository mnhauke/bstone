/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/

#include <cassert>
#include <algorithm>
#include "SDL.h"
#include "bstone_char_conv.h"
#include "bstone_exception.h"
#include "bstone_memory_pool_1x.h"
#include "bstone_sys_logger.h"
#include "bstone_sys_sdl_detail.h"
#include "bstone_sys_sdl_exception.h"
#include "bstone_sys_sdl_gl_mgr.h"
#include "bstone_sys_sdl_limits.h"
#include "bstone_sys_sdl_mouse_mgr.h"
#include "bstone_sys_sdl_video_mgr.h"
#include "bstone_sys_sdl_window_mgr.h"

namespace bstone {
namespace sys {

namespace {

class SdlVideoMgr final : public VideoMgr
{
public:
	SdlVideoMgr(Logger& logger);
	SdlVideoMgr(const SdlVideoMgr&) = delete;
	SdlVideoMgr& operator=(const SdlVideoMgr&) = delete;
	~SdlVideoMgr() override;

	static void* operator new(std::size_t count);
	static void operator delete(void* ptr) noexcept;

private:
	using DisplayModeCache = DisplayMode[limits::max_display_modes];

private:
	Logger& logger_;
	DisplayModeCache display_mode_cache_{};

private:
	DisplayMode do_get_current_display_mode() override;
	Span<const DisplayMode> do_get_display_modes() override;

	GlMgrUPtr do_make_gl_mgr() override;
	MouseMgrUPtr do_make_mouse_mgr() override;
	WindowMgrUPtr do_make_window_mgr() override;

private:
	static void log_int(int value, std::string& message);
	static void log_rect(const SDL_Rect& rect, std::string& message);

	void log_drivers(std::string& message);
	static void log_display_mode(const SDL_DisplayMode& mode, std::string& message);
	void log_displays(std::string& message);
	void log_info() noexcept;

	static DisplayMode map_display_mode(const SDL_DisplayMode& sdl_display_mode) noexcept;
};

// ==========================================================================

using SdlVideoMgrPool = MemoryPool1XT<SdlVideoMgr>;

SdlVideoMgrPool sdl_video_mgr_pool{};

// ==========================================================================

SdlVideoMgr::SdlVideoMgr(Logger& logger)
try
	:
	logger_{logger}
{
	logger_.log_information("<<< Start up SDL video manager.");

	sdl_ensure_result(SDL_InitSubSystem(SDL_INIT_VIDEO));
	log_info();

	logger_.log_information(">>> SDL video manager started up.");
}
BSTONE_STATIC_THROW_NESTED_FUNC

SdlVideoMgr::~SdlVideoMgr()
{
	logger_.log_information("Shut down SDL video manager.");

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void* SdlVideoMgr::operator new(std::size_t count)
try
{
	return sdl_video_mgr_pool.allocate(count);
}
BSTONE_STATIC_THROW_NESTED_FUNC

void SdlVideoMgr::operator delete(void* ptr) noexcept
{
	sdl_video_mgr_pool.deallocate(ptr);
}

DisplayMode SdlVideoMgr::do_get_current_display_mode()
try
{
	auto sdl_display_mode = SDL_DisplayMode{};
	sdl_ensure_result(SDL_GetCurrentDisplayMode(0, &sdl_display_mode));
	return map_display_mode(sdl_display_mode);
}
BSTONE_STATIC_THROW_NESTED_FUNC

Span<const DisplayMode> SdlVideoMgr::do_get_display_modes()
try
{
	const auto count = std::min(SDL_GetNumDisplayModes(0), limits::max_display_modes);

	auto sdl_display_mode = SDL_DisplayMode{};

	for (auto i = 0; i < count; ++i)
	{
		sdl_ensure_result(SDL_GetDisplayMode(0, i, &sdl_display_mode));
		display_mode_cache_[i] = map_display_mode(sdl_display_mode);
	}

	return Span<const DisplayMode>{display_mode_cache_, count};
}
BSTONE_STATIC_THROW_NESTED_FUNC

GlMgrUPtr SdlVideoMgr::do_make_gl_mgr()
{
	return make_sdl_gl_mgr(logger_);
}

MouseMgrUPtr SdlVideoMgr::do_make_mouse_mgr()
{
	return make_sdl_mouse_mgr(logger_);
}

WindowMgrUPtr SdlVideoMgr::do_make_window_mgr()
{
	return make_sdl_window_mgr(logger_);
}

void SdlVideoMgr::log_int(int value, std::string& message)
{
	char chars[11];
	const auto digit_count = char_conv::to_chars(value, make_span(chars), 10);
	message.append(chars, static_cast<std::size_t>(digit_count));
}

void SdlVideoMgr::log_rect(const SDL_Rect& rect, std::string& message)
{
	message += "(x: ";
	log_int(rect.x, message);
	message += "; y: ";
	log_int(rect.y, message);
	message += "; w: ";
	log_int(rect.w, message);
	message += "; h: ";
	log_int(rect.h, message);
	message += ')';
}

void SdlVideoMgr::log_drivers(std::string& message)
{
	//
	const auto current_driver = SDL_GetCurrentVideoDriver();

	message += "Current driver: \"";
	message += (current_driver != nullptr ? current_driver : "???");
	message += '"';
	detail::sdl_log_eol(message);

	//
	const auto driver_count = SDL_GetNumVideoDrivers();

	if (driver_count == 0)
	{
		message = "No built-in drivers.";
		detail::sdl_log_eol(message);
		return;
	}

	message = "Built-in drivers:";
	detail::sdl_log_eol(message);

	for (auto i = decltype(driver_count){}; i < driver_count; ++i)
	{
		const auto sdl_driver_name = SDL_GetVideoDriver(i);

		message += "  \"";
		message += (sdl_driver_name != nullptr ? sdl_driver_name : "???");
		message += '"';
		detail::sdl_log_eol(message);
	}
}

void SdlVideoMgr::log_display_mode(const SDL_DisplayMode& mode, std::string& message)
{
	log_int(mode.w, message);
	message += 'x';
	log_int(mode.h, message);
	message += ' ';
	log_int(mode.refresh_rate, message);
	message += " Hz";
}

void SdlVideoMgr::log_displays(std::string& message)
{
	//
	const auto display_count = SDL_GetNumVideoDisplays();

	if (display_count == 0)
	{
		message = "No displays.";
		detail::sdl_log_eol(message);
		return;
	}

	message = "Displays:";
	detail::sdl_log_eol(message);

	for (auto i = decltype(display_count){}; i < display_count; ++i)
	{
		auto sdl_spec = SDL_AudioSpec{};
		const auto sdl_display_name = SDL_GetDisplayName(i);

		//
		message += "  ";
		log_int(i + 1, message);
		message += ". \"";
		message += (sdl_display_name != nullptr ? sdl_display_name : "???");
		message += '"';
		detail::sdl_log_eol(message);

		//
		message += "  Bounds: ";
		auto bounds = SDL_Rect{};

		if (SDL_GetDisplayBounds(i, &bounds) == 0)
		{
			log_rect(bounds, message);
		}
		else
		{
			message += "???";
		}

		detail::sdl_log_eol(message);

#if SDL_VERSION_ATLEAST(2, 0, 5)
		//
		message += "  Usable bounds: ";
		auto usable_bounds = SDL_Rect{};

		if (SDL_GetDisplayUsableBounds(i, &usable_bounds) == 0)
		{
			log_rect(usable_bounds, message);
		}
		else
		{
			message += "???";
		}

		detail::sdl_log_eol(message);
#endif

#if SDL_VERSION_ATLEAST(2, 0, 9)
		//
		message += "  Orientation: ";

		switch (SDL_GetDisplayOrientation(i))
		{
			case SDL_ORIENTATION_LANDSCAPE: message += "landscape"; break;
			case SDL_ORIENTATION_LANDSCAPE_FLIPPED: message += "landscape flipped"; break;
			case SDL_ORIENTATION_PORTRAIT: message += "portrait"; break;
			case SDL_ORIENTATION_PORTRAIT_FLIPPED: message += "portrait flipped"; break;
			default: message += "???"; break;
		}

		detail::sdl_log_eol(message);
#endif

		//
		auto current_mode = SDL_DisplayMode{};

		message += "  Current mode: ";

		if (SDL_GetCurrentDisplayMode(i, &current_mode) == 0)
		{
			log_display_mode(current_mode, message);
		}
		else
		{
			message += "???";
		}

		detail::sdl_log_eol(message);

		//
		const auto mode_count = SDL_GetNumDisplayModes(i);

		if (mode_count > 0)
		{
			message = "  Modes:";
			detail::sdl_log_eol(message);

			for (auto j = decltype(mode_count){}; j < mode_count; ++j)
			{
				auto mode = SDL_DisplayMode{};

				if (SDL_GetDisplayMode(i, j, &mode) == 0)
				{
					message += "    ";
					log_display_mode(mode, message);
					detail::sdl_log_eol(message);
				}
			}
		}
		else
		{
			message = "  No modes.";
			detail::sdl_log_eol(message);
		}
	}
}

void SdlVideoMgr::log_info() noexcept
try
{
	auto message = std::string{};
	message.reserve(4096);

	log_drivers(message);
	log_displays(message);

	logger_.log_information(message);
}
catch (...) {}

DisplayMode SdlVideoMgr::map_display_mode(const SDL_DisplayMode& sdl_display_mode) noexcept
{
	auto display_mode = DisplayMode{};
	display_mode.width = sdl_display_mode.w;
	display_mode.height = sdl_display_mode.h;
	display_mode.refresh_rate = sdl_display_mode.refresh_rate;
	return display_mode;
}

} // namespace

// ==========================================================================

VideoMgrUPtr make_sdl_video_mgr(Logger& logger)
try
{
	return std::make_unique<SdlVideoMgr>(logger);
}
BSTONE_STATIC_THROW_NESTED_FUNC

} // namespace sys
} // namespace bstone
