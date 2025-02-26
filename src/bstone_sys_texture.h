/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/

#if !defined(BSTONE_SYS_TEXTURE_INCLUDED)
#define BSTONE_SYS_TEXTURE_INCLUDED

#include <memory>
#include "bstone_sys_r2_rect.h"
#include "bstone_sys_pixel_format.h"
#include "bstone_sys_texture_lock.h"

namespace bstone {
namespace sys {

enum class TextureBlendMode
{
	none = 0,
	blend,
};

enum class TextureAccess
{
	none = 0,
	streaming,
};

// ==========================================================================

class Texture
{
public:
	Texture() = default;
	virtual ~Texture() = default;

	void set_blend_mode(TextureBlendMode mode);
	void copy(const R2Rect* texture_rect, const R2Rect* target_rect);

	TextureLockUPtr make_lock();
	TextureLockUPtr make_lock(R2Rect rect);

private:
	virtual void do_set_blend_mode(TextureBlendMode mode) = 0;
	virtual void do_copy(const R2Rect* texture_rect, const R2Rect* target_rect) = 0;

	virtual TextureLockUPtr do_make_lock(const R2Rect* rect) = 0;
};

// ==========================================================================

struct TextureInitParam
{
	PixelFormat pixel_format;
	TextureAccess access;
	int width;
	int height;
};

using TextureUPtr = std::unique_ptr<Texture>;

} // namespace sys
} // namespace bstone

#endif // BSTONE_SYS_TEXTURE_INCLUDED
