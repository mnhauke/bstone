/*
BStone: A Source port of
Blake Stone: Aliens of Gold and Blake Stone: Planet Strike

Copyright (c) 1992-2013 Apogee Entertainment, LLC
Copyright (c) 2013-2019 Boris I. Bendovsky (bibendovsky@hotmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the
Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


//
// Renderer interface.
//


#ifndef BSTONE_RENDERER_INCLUDED
#define BSTONE_RENDERER_INCLUDED


#include <string>
#include <vector>
#include "bstone_sprite.h"
#include "bstone_vecn.h"


namespace bstone
{


enum class RendererKind
{
	none,
	opengl,
}; // RendererKind

enum class RendererPath
{
	none,

	autodetect,

	ogl_1_x,
}; // RendererPath

enum class RendererCommandId
{
	none,

	set_depth_state,
	set_viewport,
	set_2d,
	set_3d,
	enable_blending,
	enable_depth_write,

	draw_quads,
}; // RendererCommandId


class RendererInitializeWindowParam
{
public:
	bool is_visible_;
	bool is_borderless_;
	bool is_fullscreen_desktop_;
	bool is_positioned_;

	int x_;
	int y_;

	int width_;
	int height_;

	std::string title_utf8_;
}; // RendererInitializeWindowParam

class RendererInitializeParam
{
public:
	RendererPath renderer_path_;
	RendererInitializeWindowParam window_;
}; // RendererInitializeParam


class RendererColor32
{
public:
	std::uint8_t r_;
	std::uint8_t g_;
	std::uint8_t b_;
	std::uint8_t a_;


	constexpr RendererColor32()
		:
		r_{},
		g_{},
		b_{},
		a_{}
	{
	}

	constexpr RendererColor32(
		const std::uint8_t r,
		const std::uint8_t g,
		const std::uint8_t b,
		const std::uint8_t a)
		:
		r_{r},
		g_{g},
		b_{b},
		a_{a}
	{
	}


	std::uint8_t* get_data()
	{
		return reinterpret_cast<std::uint8_t*>(this);
	}

	const std::uint8_t* get_data() const
	{
		return reinterpret_cast<const std::uint8_t*>(this);
	}


	constexpr std::uint32_t to_u32() const
	{
		return (r_ << 24) | (g_ << 16) | (b_ << 8) | a_;
	}

	constexpr bool operator==(
		const RendererColor32& rhs) const
	{
		return r_ == rhs.r_ && g_ == rhs.g_ && b_ == rhs.b_ && a_ == rhs.a_;
	}
}; // RendererColor32

using RendererColor32Ptr = RendererColor32*;
using RendererColor32CPtr = const RendererColor32*;

using RendererPalette = std::array<RendererColor32, 256>;

class RendererVertex
{
public:
	Vec3F xyz_;
	RendererColor32 rgba_;
	Vec2F uv_;
}; // RendererVertex


// ==========================================================================
// RendererIndexBuffer
//

struct RendererIndexBufferCreateParam
{
	int index_count_;
}; // RendererIndexBufferCreateParam

struct RendererIndexBufferUpdateParam
{
	int offset_;
	int count_;
	const void* indices_;
}; // RendererIndexBufferUpdateParam

class RendererIndexBuffer
{
protected:
	RendererIndexBuffer() = default;

	virtual ~RendererIndexBuffer() = default;


public:
	virtual void update(
		const RendererIndexBufferUpdateParam& param) = 0;
}; // RendererIndexBuffer

using RendererIndexBufferPtr = RendererIndexBuffer*;

//
// RendererIndexBuffer
// ==========================================================================


// ==========================================================================
// RendererVertexBuffer
//

struct RendererVertexBufferCreateParam
{
	int vertex_count_;
}; // RendererVertexBufferCreateParam

struct RendererVertexBufferUpdateParam
{
	int offset_;
	int count_;
	const RendererVertex* vertices_;
}; // RendererVertexBufferUpdateParam

class RendererVertexBuffer
{
protected:
	RendererVertexBuffer() = default;

	virtual ~RendererVertexBuffer() = default;


public:
	virtual void update(
		const RendererVertexBufferUpdateParam& param) = 0;
}; // RendererVertexBuffer

using RendererVertexBufferPtr = RendererVertexBuffer*;

//
// RendererVertexBuffer
// ==========================================================================


// ==========================================================================
// RendererTexture2d
//

struct RendererTexture2dCreateParam
{
	bool has_rgba_alpha_;
	bool is_generate_mipmaps_;

	int width_;
	int height_;

	bool indexed_is_column_major_;
	const std::uint8_t* indexed_pixels_;
	const RendererPalette* indexed_palette_;
	const bool* indexed_alphas_;

	SpriteCPtr indexed_sprite_;

	const RendererColor32* rgba_pixels_;
}; // RendererTexture2dCreateParam

struct RendererTexture2dUpdateParam
{
	const std::uint8_t* indexed_pixels_;
	const RendererPalette* indexed_palette_;
	const bool* indexed_alphas_;

	SpriteCPtr indexed_sprite_;

	const RendererColor32* rgba_pixels_;
}; // RendererTexture2dUpdateParam

class RendererTexture2d
{
protected:
	RendererTexture2d() = default;

	virtual ~RendererTexture2d() = default;


public:
	virtual void update(
		const RendererTexture2dUpdateParam& param) = 0;
}; // RendererTexture2d

using RendererTexture2dPtr = RendererTexture2d*;

//
// RendererTexture2d
// ==========================================================================


struct RendererDepthStateFlags
{
	using Value = unsigned int;


	Value is_enabled_ : 1;
	Value is_writable_ : 1;
}; // RendererDepthState

struct RendererCommand
{
	struct SetViewport
	{
		int x_;
		int y_;
		int width_;
		int height_;
	}; // SetViewport

	struct Set2d
	{
	}; // Set2d

	struct Set3d
	{
	}; // Set3d

	struct EnableBlending
	{
		bool is_enabled_;
	}; // EnableBlending

	struct SetDepthState
	{
		RendererDepthStateFlags flags_;
	}; // SetDepthState

	struct EnableDepthWriting
	{
		bool is_enabled_;
	}; // EnableDepthWriting

	struct DrawQuads
	{
		int count_;
		int index_offset_;
		RendererTexture2dPtr texture_2d_;
		RendererIndexBufferPtr index_buffer_;
		RendererVertexBufferPtr vertex_buffer_;
	}; // DrawQuads


	RendererCommandId id_;

	union
	{
		SetDepthState set_depth_state_;
		SetViewport set_viewport_;
		Set2d set_2d_;
		Set3d set_3d_;
		EnableBlending enable_blending_;
		EnableDepthWriting enable_depth_writing_;
		DrawQuads draw_quads_;
	}; // union
}; // RendererCommand

struct RendererCommandSet
{
	using Commands = std::vector<RendererCommand>;


	int count_;
	Commands commands_;
}; // RendererCommandSet

using RendererCommandSets = std::vector<RendererCommandSet>;


class Renderer
{
protected:
	Renderer() = default;

	virtual ~Renderer() = default;


public:
	static constexpr int min_vfov_deg = 10;
	static constexpr int max_vfov_deg = 170;


	virtual const std::string& get_error_message() const = 0;


	virtual RendererKind get_kind() const = 0;

	virtual const std::string& get_name() const = 0;

	virtual const std::string& get_description() const = 0;


	virtual bool probe(
		const RendererPath renderer_path) = 0;

	virtual RendererPath get_probe_path() const = 0;


	virtual bool is_initialized() const = 0;

	virtual bool initialize(
		const RendererInitializeParam& param) = 0;

	virtual void uninitialize() = 0;


	virtual RendererPath get_path() const = 0;


	virtual void window_show(
		const bool is_visible) = 0;


	virtual void set_default_viewport(
		const int width,
		const int height) = 0;

	virtual void color_buffer_set_clear_color(
		const RendererColor32& color) = 0;

	virtual void clear_buffers() = 0;

	virtual void present() = 0;


	virtual void palette_update(
		const RendererPalette& palette) = 0;


	virtual void set_2d_projection_matrix(
		const int width,
		const int height) = 0;

	virtual void set_3d_view_matrix(
		const int angle_deg,
		const Vec3F& position) = 0;

	virtual void set_3d_projection_matrix(
		const int width,
		const int height,
		const int vfov_deg,
		const float near_distance,
		const float far_distance) = 0;


	virtual RendererIndexBufferPtr index_buffer_create(
		const RendererIndexBufferCreateParam& param) = 0;

	virtual void index_buffer_destroy(
		RendererIndexBufferPtr index_buffer) = 0;


	virtual RendererVertexBufferPtr vertex_buffer_create(
		const RendererVertexBufferCreateParam& param) = 0;

	virtual void vertex_buffer_destroy(
		RendererVertexBufferPtr vertex_buffer) = 0;


	virtual RendererTexture2dPtr texture_2d_create(
		const RendererTexture2dCreateParam& param) = 0;

	virtual void texture_2d_destroy(
		RendererTexture2dPtr texture_2d) = 0;


	virtual void execute_command_sets(
		const RendererCommandSets& command_sets) = 0;
}; // Renderer

using RendererPtr = Renderer*;


} // bstone


#endif // !BSTONE_RENDERER_INCLUDED
