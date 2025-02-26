/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/

#include <SDL_messagebox.h>
#include "bstone_exception.h"
#include "bstone_sys_message_box.h"
#include "bstone_sys_sdl_exception.h"

namespace bstone {
namespace sys {

void show_message_box(
	const char* title,
	const char* message,
	MessageBoxType type)
try
{
	auto sdl_flags = Uint32{};

	switch (type)
	{
		case MessageBoxType::error: sdl_flags |= SDL_MESSAGEBOX_ERROR; break;
		case MessageBoxType::information: sdl_flags |= SDL_MESSAGEBOX_INFORMATION; break;
		case MessageBoxType::warning: sdl_flags |= SDL_MESSAGEBOX_WARNING; break;
		default: BSTONE_STATIC_THROW("Unknown type.");
	}

	sdl_ensure_result(SDL_ShowSimpleMessageBox(sdl_flags, title, message, nullptr));
}
BSTONE_STATIC_THROW_NESTED_FUNC

int show_message_box(const MessageBoxDescriptor& descriptor)
try
{
	auto sdl_message_box_flags = Uint32{};

	switch (descriptor.type)
	{
		case MessageBoxType::error: sdl_message_box_flags |= SDL_MESSAGEBOX_ERROR; break;
		case MessageBoxType::information: sdl_message_box_flags |= SDL_MESSAGEBOX_INFORMATION; break;
		case MessageBoxType::warning: sdl_message_box_flags |= SDL_MESSAGEBOX_WARNING; break;
		default: BSTONE_STATIC_THROW("Unknown type.");
	}

	constexpr auto max_buttons = 8;

	if (descriptor.buttons.get_size() > max_buttons)
	{
		BSTONE_STATIC_THROW("Too many buttons.");
	}

	SDL_MessageBoxButtonData sdl_buttons[max_buttons];
	auto sdl_button = sdl_buttons;

	for (const auto& button : descriptor.buttons)
	{
		auto sdl_button_flags = Uint32{};

		if ((button.flags & MessageBoxButtonFlags::default_for_escape_key) != MessageBoxButtonFlags{})
		{
			sdl_button_flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
		}

		if ((button.flags & MessageBoxButtonFlags::default_for_return_key) != MessageBoxButtonFlags{})
		{
			sdl_button_flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
		}

		*sdl_button = SDL_MessageBoxButtonData{};
		sdl_button->flags = sdl_button_flags;
		sdl_button->buttonid = button.id;
		sdl_button->text = button.text;
		++sdl_button;
	}

	auto sdl_message_box = SDL_MessageBoxData{};
	sdl_message_box.flags = sdl_message_box_flags;
	sdl_message_box.title = descriptor.title;
	sdl_message_box.message = descriptor.message;
	sdl_message_box.numbuttons = static_cast<int>(descriptor.buttons.get_size());
	sdl_message_box.buttons = sdl_buttons;

	auto sdl_button_id = 0;
	sdl_ensure_result(SDL_ShowMessageBox(&sdl_message_box, &sdl_button_id));
	return sdl_button_id;
}
BSTONE_STATIC_THROW_NESTED_FUNC

} // namespace sys
} // namespace bstone
