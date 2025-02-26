/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 1992-2013 Apogee Entertainment, LLC
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: GPL-2.0-or-later
*/


//
// Content path.
//


#include "bstone_content_path.h"

#include "bstone_exception.h"


namespace bstone
{


namespace detail
{


AssetPath make_gog_content_path();

AssetPath make_steam_content_path();


} // detail


namespace
{


class AssetPathException :
	public Exception
{
public:
	explicit AssetPathException(
		const char* message) noexcept
		:
		Exception{"CONTENT_PATH", message}
	{
	}
}; // AssetPathException


[[noreturn]]
void fail(
	const char* message)
{
	throw AssetPathException{message};
}

[[noreturn]]
void fail_nested(
	const char* message)
{
	std::throw_with_nested(AssetPathException{message});
}


} // namespace


AssetPath make_content_path(
	const ContentPathProvider type)
try
{
	switch (type)
	{
		case ContentPathProvider::gog:
			return detail::make_gog_content_path();

		case ContentPathProvider::steam:
			return detail::make_steam_content_path();

		default:
			fail("Unsupported provider.");
	}
}
catch (...)
{
	fail_nested(__func__);
}


} // bstone
