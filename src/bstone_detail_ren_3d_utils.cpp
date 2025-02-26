/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 1992-2013 Apogee Entertainment, LLC
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: GPL-2.0-or-later
*/


//
// 3D renderer utils.
//
// !!! Internal usage only. !!!
//


#include "bstone_detail_ren_3d_utils.h"

#include <algorithm>
#include <unordered_set>

#include "bstone_exception.h"
#include "bstone_ren_3d_limits.h"
#include "bstone_sprite.h"


namespace bstone
{
namespace detail
{


// ==========================================================================
// Ren3dUtilsException
//

class Ren3dUtilsException :
	public Exception
{
public:
	explicit Ren3dUtilsException(
		const char* message) noexcept
		:
		Exception{"REN_3D_UTILS", message}
	{
	}
}; // Ren3dUtilsException

//
// Ren3dUtilsException
// ==========================================================================


// ==========================================================================
// Ren3dUtils
//

float Ren3dUtils::deg_to_rad(
	const float angle_deg) noexcept
{
	return angle_deg * pi_over_180;
}

bool Ren3dUtils::is_pot_value(
	const int value) noexcept
{
	const auto nearest_value = find_nearest_pot_value(value);

	return nearest_value == value;
}

int Ren3dUtils::find_nearest_pot_value(
	const int value) noexcept
{
	for (int i = 0; i < 32; ++i)
	{
		const auto new_value = 1 << i;

		if (new_value >= value)
		{
			return new_value;
		}
	}

	return 0;
}

int Ren3dUtils::calculate_mipmap_count(
	const int width,
	const int height)
try
{
	//
	// mipmap_count = [log2(max(width, height))] + 1
	//

	if (width <= 0)
	{
		fail("Width out of range.");
	}

	if (height <= 0)
	{
		fail("Height out of range.");
	}

	auto log_2 = 0;
	auto max_size = std::max(width, height);

	while (max_size > 1)
	{
		log_2 += 1;
		max_size /= 2;
	}

	const auto result = log_2 + 1;

	return result;
}
catch (...)
{
	fail_nested(__func__);
}

sys::WindowUPtr Ren3dUtils::create_window(
	const Ren3dUtilsCreateWindowParam& param,
	sys::WindowMgr& window_mgr)
try
{
	create_window_validate_param(param);
	const auto gl_attributes = create_window_make_gl_context_attributes(param);

	auto window_param = sys::WindowInitParam{};

	window_param.x =
		param.window_.is_positioned_ ? param.window_.rect_2d_.offset_.x : sys::window_position_centered;

	window_param.y =
		param.window_.is_positioned_ ? param.window_.rect_2d_.offset_.y : sys::window_position_centered;

	window_param.width = param.window_.rect_2d_.extent_.width_;
	window_param.height = param.window_.rect_2d_.extent_.height_;

	window_param.is_visible = param.window_.is_visible;
	window_param.is_fake_fullscreen = param.window_.is_native_;

	switch (param.renderer_kind_)
	{
		case Ren3dKind::gl_2_0:
		case Ren3dKind::gl_3_2_core:
		case Ren3dKind::gles_2_0:
			window_param.is_opengl = true;
			window_param.gl_attributes = &gl_attributes;
			break;

		default:
			break;
	}

	return window_mgr.make_window(window_param);
}
BSTONE_STATIC_THROW_NESTED_FUNC

void Ren3dUtils::set_window_mode(sys::Window& window, const Ren3dSetWindowModeParam& param)
try
{
	if (param.rect_2d_.extent_.height_ <= 0)
	{
		fail("Height out of range.");
	}

	if (param.rect_2d_.extent_.width_ <= 0)
	{
		fail("Width out of range.");
	}

	//
	const auto is_current_native = window.is_fake_fullscreen();
	const auto is_native_changed = (is_current_native != param.is_native);

	//
	const auto current_size = window.get_size();

	if (current_size.width <= 0 || current_size.height <= 0)
	{
		fail("Failed to get current window size.");
	}

	const auto is_size_changed =
		current_size.width != param.rect_2d_.extent_.width_ ||
		current_size.height != param.rect_2d_.extent_.height_;

	//
	if (!is_native_changed && !is_size_changed)
	{
		return;
	}

	if (is_native_changed && !param.is_native)
	{
		window.set_fake_fullscreen(false);
	}

	if (is_size_changed)
	{
		window.set_size(sys::WindowSize{param.rect_2d_.extent_.width_, param.rect_2d_.extent_.height_});

		if (param.is_positioned_)
		{
			const auto x = std::max(param.rect_2d_.offset_.x, 0);
			const auto y = std::max(param.rect_2d_.offset_.y, 0);
			window.set_position(sys::WindowPosition{x, y});
		}
		else
		{
			window.center();
		}
	}

	if (is_native_changed && param.is_native)
	{
		window.set_fake_fullscreen(true);
	}
}
BSTONE_STATIC_THROW_NESTED_FUNC

void Ren3dUtils::validate_initialize_param(
	const Ren3dCreateParam& param)
try
{
	switch (param.renderer_kind_)
	{
	case Ren3dKind::gl_2_0:
	case Ren3dKind::gl_3_2_core:
	case Ren3dKind::gles_2_0:
		break;

	default:
		fail("Unsupported renderer kind.");
	}

	if (param.window_.rect_2d_.extent_.width_ <= 0)
	{
		fail("Invalid window width.");
	}

	if (param.window_.rect_2d_.extent_.height_ <= 0)
	{
		fail("Invalid window height.");
	}

	switch (param.aa_kind_)
	{
		case Ren3dAaKind::none:
		case Ren3dAaKind::ms:
			break;

		default:
			fail("Invalid antialiasing kind.");
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::validate_buffer_create_param(
	const Ren3dCreateBufferParam& param)
try
{
	switch (param.usage_kind_)
	{
		case Ren3dBufferUsageKind::draw_streaming:
		case Ren3dBufferUsageKind::draw_static:
		case Ren3dBufferUsageKind::draw_dynamic:
			break;

		default:
			fail("Invalid usage kind.");
	}

	if (param.size_ <= 0)
	{
		fail("Invalid size.");
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::validate_buffer_update_param(
	const Ren3dUpdateBufferParam& param)
try
{
	if (param.offset_ < 0)
	{
		fail("Invalid offset.");
	}

	if (param.size_ <= 0)
	{
		fail("Invalid count.");
	}

	if (!param.data_)
	{
		fail("Null indices.");
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::vertex_input_validate_format(
	const Ren3dVertexAttribFormat attribute_format)
try
{
	switch (attribute_format)
	{
		case Ren3dVertexAttribFormat::rgba_8_unorm:
		case Ren3dVertexAttribFormat::rg_32_sfloat:
		case Ren3dVertexAttribFormat::rgb_32_sfloat:
			return;

		default:
			fail("Unsupported vertex input attribute format.");
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::vertex_input_validate_param(
	const int max_locations,
	const Ren3dCreateVertexInputParam& param)
try
{
	if (max_locations < 0)
	{
		fail("Maximum location count out of range.");
	}

	if (param.index_buffer_)
	{
		if (param.index_buffer_->get_kind() != Ren3dBufferKind::index)
		{
			fail("Non-index buffer.");
		}
	}

	const auto& attribute_descriptions = param.attrib_descrs_;

	using UsedLocations = std::unordered_set<int>;
	auto used_locations = UsedLocations{};
	used_locations.reserve(max_locations);

	for (const auto& attribute_description : attribute_descriptions)
	{
		if (attribute_description.is_default_)
		{
			continue;
		}

		// Location.
		//
		const auto location = attribute_description.location_;

		if (location < 0 || location >= max_locations)
		{
			fail("Location out of range.");
		}

		const auto has_location = (used_locations.find(location) != used_locations.cend());

		if (has_location)
		{
			fail("Duplicate location.");
		}

		used_locations.insert(location);


		// Format.
		//
		vertex_input_validate_format(attribute_description.format_);


		// Vertex buffer.
		//
		if (!attribute_description.vertex_buffer_)
		{
			fail("Null vertex buffer.");
		}

		if (attribute_description.vertex_buffer_->get_kind() != Ren3dBufferKind::vertex)
		{
			fail("Non-vertex buffer.");
		}


		// Offset.
		//
		if (attribute_description.offset_ < 0)
		{
			fail("Invalid offset.");
		}


		// Stride.
		//
		if (attribute_description.stride_ < 0)
		{
			fail("Invalid stride.");
		}
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::indexed_to_rgba_8(
	const IndexedToRgba8Param& param)
try
{
	if (param.width_ <= 0)
	{
		fail("Width out of range.");
	}

	if (param.height_ <= 0)
	{
		fail("Height out of range.");
	}

	if (!param.indexed_pixels_)
	{
		fail("Null indexed pixels.");
	}

	if (param.rgba_8_buffer_->size() < static_cast<std::size_t>((param.width_ * param.height_)))
	{
		fail("Bitmap buffer too small.");
	}

	const auto is_column_major = param.indexed_is_column_major_;
	const auto has_alphas = (param.indexed_alphas_ != nullptr);

	if (!is_column_major && !has_alphas)
	{
		indexed_to_rgba_8_rm_na(param);
	}
	else if (!is_column_major && has_alphas)
	{
		indexed_to_rgba_8_rm_ha(param);
	}
	else if (is_column_major && !has_alphas)
	{
		indexed_to_rgba_8_cm_na(param);
	}
	else if (is_column_major && has_alphas)
	{
		indexed_to_rgba_8_cm_ha(param);
	}
	else
	{
		fail("Unsupported values combination.");
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::indexed_npot_to_rgba_8_pot(
	const IndexedToRgba8Param& param)
try
{
	if (param.width_ <= 0)
	{
		fail("Width out of range.");
	}

	if (param.height_ <= 0)
	{
		fail("Height out of range.");
	}

	if (param.actual_width_ <= 0)
	{
		fail("Actual width out of range.");
	}

	if (param.actual_height_ <= 0)
	{
		fail("Actual height out of range.");
	}

	if (!param.indexed_pixels_)
	{
		fail("Null indexed pixels.");
	}

	if (param.rgba_8_buffer_->size() < static_cast<std::size_t>((param.actual_width_ * param.actual_height_)))
	{
		fail("Indexed bitmap buffer too small.");
	}

	const auto is_column_major = param.indexed_is_column_major_;
	const auto has_alphas = (param.indexed_alphas_ != nullptr);

	if (!is_column_major && !has_alphas)
	{
		indexed_npot_to_rgba_8_rm_na(param);
	}
	else if (!is_column_major && has_alphas)
	{
		indexed_npot_to_rgba_8_rm_ha(param);
	}
	else if (is_column_major && !has_alphas)
	{
		indexed_npot_to_rgba_8_cm_na(param);
	}
	else if (is_column_major && has_alphas)
	{
		indexed_npot_to_rgba_8_cm_ha(param);
	}
	else
	{
		fail("Unsupported values combination.");
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::indexed_to_rgba_8_pot(
	const IndexedToRgba8Param& param)
try
{
	const auto is_npot = (
		param.width_ != param.actual_width_ ||
		param.height_ != param.actual_height_
	);

	if (!is_npot)
	{
		indexed_to_rgba_8(param);
	}
	else if (is_npot)
	{
		indexed_npot_to_rgba_8_pot(param);
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::indexed_sprite_to_rgba_8_pot(
	const Sprite& indexed_sprite,
	const Rgba8Palette& indexed_palette,
	Rgba8Buffer& texture_buffer)
try
{
	if (!indexed_sprite.is_initialized())
	{
		fail("Indexed sprite not initialized.");
	}

	const auto left = indexed_sprite.get_left();
	const auto right = indexed_sprite.get_right();
	const auto top = indexed_sprite.get_top();
	const auto bottom = indexed_sprite.get_bottom();

	for (int w = 0; w < Sprite::dimension; ++w)
	{
		const std::int16_t* column = nullptr;

		if (w >= left && w <= right)
		{
			column = indexed_sprite.get_column(w - left);
		}

		auto dst_pixels = &texture_buffer[w];

		for (int h = 0; h < Sprite::dimension; ++h)
		{
			auto& dst_pixel = *dst_pixels;

			if (column && h >= top && h <= bottom)
			{
				const auto src_index = h - top;

				const auto src_pixel = column[src_index];

				if (src_pixel < 0)
				{
					dst_pixel.reset();
				}
				else
				{
					dst_pixel = indexed_palette[src_pixel];
				}
			}
			else
			{
				dst_pixel.reset();
			}

			dst_pixels += Sprite::dimension;
		}
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::rgba_8_npot_to_rgba_8_pot(
	const int width,
	const int height,
	const int actual_width,
	const int actual_height,
	const Rgba8* const rgba_8_pixels,
	Rgba8Buffer& texture_buffer)
try
{
	if (width <= 0)
	{
		fail("Width out of range.");
	}

	if (height <= 0)
	{
		fail("Height out of range.");
	}

	if (actual_width <= 0)
	{
		fail("Actual width out of range.");
	}

	if (actual_height <= 0)
	{
		fail("Actual height out of range.");
	}

	if (!rgba_8_pixels)
	{
		fail("Null RGBA pixels.");
	}

	const auto src_du_f = static_cast<double>(width) / static_cast<double>(actual_width);
	const auto src_dv_f = static_cast<double>(height) / static_cast<double>(actual_height);

	auto src_v_d = 0.0;

	auto dst_index = 0;

	for (int h = 0; h < actual_height; ++h)
	{
		const auto src_v = static_cast<int>(src_v_d);

		auto src_u_d = 0.0;

		for (int w = 0; w < actual_width; ++w)
		{
			const auto src_u = static_cast<int>(src_u_d);

			const auto src_index = (src_v * width) + src_u;

			texture_buffer[dst_index] = rgba_8_pixels[src_index];

			++dst_index;

			src_u_d += src_du_f;
		}

		src_v_d += src_dv_f;
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dUtils::build_mipmap(
	const int previous_width,
	const int previous_height,
	const Rgba8CPtr src_colors,
	const Rgba8Ptr dst_colors)
try
{
	if (previous_width <= 0)
	{
		fail("Previous width out of range.");
	}

	if (previous_height <= 0)
	{
		fail("Previous height out of range.");
	}

	if (previous_width == 1 && previous_height == 1)
	{
		fail("No more mipmap to build.");
	}

	if (!src_colors)
	{
		fail("Null source colors.");
	}

	if (!dst_colors)
	{
		fail("Null target colors.");
	}

	if (previous_width == 1)
	{
		build_mipmap_1(previous_height, src_colors, dst_colors);

		return;
	}

	if (previous_height == 1)
	{
		build_mipmap_1(previous_width, src_colors, dst_colors);

		return;
	}

	const auto width = previous_width / 2;
	const auto height = previous_height / 2;

	auto dst_colors_0 = dst_colors;

	auto src_colors_0 = src_colors;
	auto src_colors_1 = src_colors_0 + previous_width;

	for (int h = 0; h < height; ++h)
	{
		for (int w = 0; w < width; ++w)
		{
			*dst_colors_0++ = average_pa(
				src_colors_0[0],
				src_colors_0[1],
				src_colors_1[0],
				src_colors_1[1]
			);

			src_colors_0 += 2;
			src_colors_1 += 2;
		}

		src_colors_0 += previous_width;
		src_colors_1 += previous_width;
	}
}
catch (...)
{
	fail_nested(__func__);
}

[[noreturn]]
void Ren3dUtils::fail(
	const char* message)
{
	throw Ren3dUtilsException{message};
}

[[noreturn]]
void Ren3dUtils::fail_nested(
	const char* message)
{
	std::throw_with_nested(Ren3dUtilsException{message});
}

void Ren3dUtils::create_window_validate_param(
	const Ren3dUtilsCreateWindowParam& param)
try
{
	if (param.window_.is_positioned_ && (
		param.window_.rect_2d_.offset_.x < 0 ||
			param.window_.rect_2d_.offset_.y < 0))
	{
		fail("Invalid position.");
	}

	if (param.window_.rect_2d_.extent_.width_ <= 0 ||
		param.window_.rect_2d_.extent_.height_ <= 0)
	{
		fail("Invalid dimensions.");
	}
}
catch (...)
{
	fail_nested(__func__);
}

sys::GlContextAttributes Ren3dUtils::create_window_make_gl_context_attributes(
	const Ren3dUtilsCreateWindowParam& param)
try
{
	auto gl_attributes = sys::GlContextAttributes{};

	gl_attributes.is_accelerated = true;

	switch (param.aa_kind_)
	{
		case Ren3dAaKind::ms:
			if (param.aa_value_ >= Ren3dLimits::min_aa_on)
			{
				gl_attributes.multisample_buffer_count = 1;
				gl_attributes.multisample_sample_count = param.aa_value_;
			}

			break;


		case Ren3dAaKind::none:
		default:
			break;
	}

	gl_attributes.red_bit_count = 8;
	gl_attributes.green_bit_count = 8;
	gl_attributes.blue_bit_count = 8;

	if (!param.is_default_depth_buffer_disabled_)
	{
		gl_attributes.depth_bit_count = 16;
	}

	switch (param.renderer_kind_)
	{
		case Ren3dKind::gl_2_0:
			gl_attributes.profile = sys::GlContextProfile::compatibility;
			gl_attributes.major_version = 2;
			gl_attributes.minor_version = 0;
			break;

		case Ren3dKind::gl_3_2_core:
			gl_attributes.profile = sys::GlContextProfile::core;
			gl_attributes.major_version = 3;
			gl_attributes.minor_version = 2;
			break;

		case Ren3dKind::gles_2_0:
			gl_attributes.profile = sys::GlContextProfile::es;
			gl_attributes.major_version = 2;
			gl_attributes.minor_version = 0;
			break;

		default:
			fail("Unsupported 3D-renderer kind.");
	}

	return gl_attributes;
}
BSTONE_STATIC_THROW_NESTED_FUNC

// Indexed (row major, has no alpha) -> RGBA
void Ren3dUtils::indexed_to_rgba_8_rm_na(
	const IndexedToRgba8Param& param) noexcept
{
	auto src_pixels = param.indexed_pixels_;
	const auto& src_palette = *param.indexed_palette_;

	auto dst_pixels = param.rgba_8_buffer_->data();

	const auto area = param.width_ * param.height_;

	for (int i = 0; i < area; ++i)
	{
		*dst_pixels++ = src_palette[*src_pixels++];
	}
}

// Indexed (row major, has alpha) -> RGBA
void Ren3dUtils::indexed_to_rgba_8_rm_ha(
	const IndexedToRgba8Param& param) noexcept
{
	auto src_pixels = param.indexed_pixels_;
	const auto& src_palette = *param.indexed_palette_;

	auto dst_pixels = param.rgba_8_buffer_->data();

	const auto area = param.width_ * param.height_;

	for (int i = 0; i < area; ++i)
	{
		const auto src_pixel = *src_pixels++;
		auto& dst_pixel = *dst_pixels++;

		if (param.indexed_alphas_[i])
		{
			dst_pixel = src_palette[src_pixel];
		}
		else
		{
			dst_pixel.reset();
		}
	}
}

// Indexed (column major, has no alpha) -> RGBA
void Ren3dUtils::indexed_to_rgba_8_cm_na(
	const IndexedToRgba8Param& param) noexcept
{
	const auto& src_palette = *param.indexed_palette_;

	auto dst_pixels = param.rgba_8_buffer_->data();

	for (int src_y = 0; src_y < param.height_; ++src_y)
	{
		auto src_index = src_y;

		for (int src_x = 0; src_x < param.width_; ++src_x)
		{
			*dst_pixels++ = src_palette[param.indexed_pixels_[src_index]];

			src_index += param.height_;
		}
	}
}

// Indexed (column major, has alpha) -> RGBA
void Ren3dUtils::indexed_to_rgba_8_cm_ha(
	const IndexedToRgba8Param& param) noexcept
{
	const auto& src_palette = *param.indexed_palette_;

	auto dst_pixels = param.rgba_8_buffer_->data();

	for (int src_y = 0; src_y < param.height_; ++src_y)
	{
		auto src_index = src_y;

		for (int src_x = 0; src_x < param.width_; ++src_x)
		{
			auto& dst_pixel = *dst_pixels++;

			if (param.indexed_alphas_[src_index])
			{
				dst_pixel = src_palette[param.indexed_pixels_[src_index]];
			}
			else
			{
				dst_pixel.reset();
			}

			src_index += param.height_;
		}
	}
}

// Indexed (row major, has no alpha) -> RGBA POT
void Ren3dUtils::indexed_npot_to_rgba_8_rm_na(
	const IndexedToRgba8Param& param) noexcept
{
	const auto src_du_f = static_cast<double>(param.width_) / static_cast<double>(param.actual_width_);
	const auto src_dv_f = static_cast<double>(param.height_) / static_cast<double>(param.actual_height_);

	const auto& src_palette = *param.indexed_palette_;
	auto dst_colors = param.rgba_8_buffer_->data();

	auto src_v_f = 0.0;
	auto cached_src_index = -1;
	auto cached_color = Rgba8{};

	for (int h = 0; h < param.actual_height_; ++h)
	{
		const auto src_v = static_cast<int>(src_v_f);
		const auto src_base_index = src_v * param.width_;

		auto src_u_f = 0.0;

		for (int w = 0; w < param.actual_width_; ++w)
		{
			const auto src_u = static_cast<int>(src_u_f);
			const auto src_index = src_base_index + src_u;

			if (src_index != cached_src_index)
			{
				cached_src_index = src_index;
				cached_color = src_palette[param.indexed_pixels_[src_index]];
			}

			*dst_colors++ = cached_color;

			src_u_f += src_du_f;
		}

		src_v_f += src_dv_f;
	}
}

// Indexed (row major, has alpha) -> RGBA POT
void Ren3dUtils::indexed_npot_to_rgba_8_rm_ha(
	const IndexedToRgba8Param& param) noexcept
{
	const auto src_du_f = static_cast<double>(param.width_) / static_cast<double>(param.actual_width_);
	const auto src_dv_f = static_cast<double>(param.height_) / static_cast<double>(param.actual_height_);

	const auto& src_palette = *param.indexed_palette_;
	auto dst_colors = param.rgba_8_buffer_->data();

	auto src_v_f = 0.0;
	auto cached_src_index = -1;
	auto cached_color = Rgba8{};

	for (int h = 0; h < param.actual_height_; ++h)
	{
		const auto src_v = static_cast<int>(src_v_f);
		const auto src_base_index = src_v * param.width_;

		auto src_u_f = 0.0;

		for (int w = 0; w < param.actual_width_; ++w)
		{
			const auto src_u = static_cast<int>(src_u_f);
			const auto src_index = src_base_index + src_u;

			auto& dst_pixel = *dst_colors++;

			if (param.indexed_alphas_[src_index])
			{
				if (src_index != cached_src_index)
				{
					cached_src_index = src_index;
					cached_color = src_palette[param.indexed_pixels_[src_index]];
				}

				dst_pixel = cached_color;
			}
			else
			{
				dst_pixel.reset();
			}

			src_u_f += src_du_f;
		}

		src_v_f += src_dv_f;
	}
}

// Indexed (column major, has no alpha) -> RGBA POT
void Ren3dUtils::indexed_npot_to_rgba_8_cm_na(
	const IndexedToRgba8Param& param) noexcept
{
	const auto src_du_f = static_cast<double>(param.width_) / static_cast<double>(param.actual_width_);
	const auto src_dv_f = static_cast<double>(param.height_) / static_cast<double>(param.actual_height_);

	const auto& src_palette = *param.indexed_palette_;
	auto dst_colors = param.rgba_8_buffer_->data();

	auto src_v_f = 0.0;
	auto cached_src_index = -1;
	auto cached_color = Rgba8{};

	for (int h = 0; h < param.actual_height_; ++h)
	{
		const auto src_v = static_cast<int>(src_v_f);

		auto src_u_f = 0.0;

		for (int w = 0; w < param.actual_width_; ++w)
		{
			const auto src_u = static_cast<int>(src_u_f);

			const auto src_index = (src_u * param.height_) + src_v;

			if (src_index != cached_src_index)
			{
				cached_src_index = src_index;
				cached_color = src_palette[param.indexed_pixels_[src_index]];
			}

			*dst_colors++ = cached_color;

			src_u_f += src_du_f;
		}

		src_v_f += src_dv_f;
	}
}

// Indexed (column major, has alpha) -> RGBA POT
void Ren3dUtils::indexed_npot_to_rgba_8_cm_ha(
	const IndexedToRgba8Param& param) noexcept
{
	const auto src_du_f = static_cast<double>(param.width_) / static_cast<double>(param.actual_width_);
	const auto src_dv_f = static_cast<double>(param.height_) / static_cast<double>(param.actual_height_);

	const auto& src_palette = *param.indexed_palette_;
	auto dst_colors = param.rgba_8_buffer_->data();

	auto src_v_f = 0.0;
	auto cached_src_index = -1;
	auto cached_color = Rgba8{};

	for (int h = 0; h < param.actual_height_; ++h)
	{
		const auto src_v = static_cast<int>(src_v_f);

		auto src_u_f = 0.0;

		for (int w = 0; w < param.actual_width_; ++w)
		{
			const auto src_u = static_cast<int>(src_u_f);

			const auto src_index = (src_u * param.height_) + src_v;

			auto& dst_pixel = *dst_colors++;

			if (param.indexed_alphas_[src_index])
			{
				if (src_index != cached_src_index)
				{
					cached_src_index = src_index;
					cached_color = src_palette[param.indexed_pixels_[src_index]];
				}

				dst_pixel = cached_color;
			}
			else
			{
				dst_pixel.reset();
			}

			src_u_f += src_du_f;
		}

		src_v_f += src_dv_f;
	}
}

void Ren3dUtils::build_mipmap_1(
	const int previous_dimension,
	const Rgba8CPtr src_colors,
	const Rgba8Ptr dst_colors) noexcept
{
	const auto dimension = previous_dimension / 2;

	auto src_colors_0 = src_colors;
	auto dst_colors_0 = dst_colors;

	for (int i = 0; i < dimension; ++i)
	{
		*dst_colors_0++ = average_pa(
			src_colors_0[0],
			src_colors_0[1]
		);

		src_colors_0 += 2;
	}
}

Rgba8 Ren3dUtils::average_pa(
	const Rgba8 color_0,
	const Rgba8 color_1) noexcept
{
	constexpr auto color_count = 2;
	constexpr auto denominator = 255 * color_count * color_count;

	const auto a_sum = color_0.a_ + color_1.a_;
	const auto r_sum = color_0.r_ + color_1.r_;
	const auto g_sum = color_0.g_ + color_1.g_;
	const auto b_sum = color_0.b_ + color_1.b_;

	return Rgba8
	{
		static_cast<std::uint8_t>((r_sum * a_sum) / denominator),
		static_cast<std::uint8_t>((g_sum * a_sum) / denominator),
		static_cast<std::uint8_t>((b_sum * a_sum) / denominator),
		static_cast<std::uint8_t>(a_sum / color_count),
	};
}

Rgba8 Ren3dUtils::average_pa(
	const Rgba8 color_0,
	const Rgba8 color_1,
	const Rgba8 color_2,
	const Rgba8 color_3) noexcept
{
	constexpr auto color_count = 4;
	constexpr auto denominator = 255 * color_count * color_count;

	const auto a_sum = color_0.a_ + color_1.a_ + color_2.a_ + color_3.a_;
	const auto r_sum = color_0.r_ + color_1.r_ + color_2.r_ + color_3.r_;
	const auto g_sum = color_0.g_ + color_1.g_ + color_2.g_ + color_3.g_;
	const auto b_sum = color_0.b_ + color_1.b_ + color_2.b_ + color_3.b_;

	return Rgba8
	{
		static_cast<std::uint8_t>((r_sum * a_sum) / denominator),
		static_cast<std::uint8_t>((g_sum * a_sum) / denominator),
		static_cast<std::uint8_t>((b_sum * a_sum) / denominator),
		static_cast<std::uint8_t>(a_sum / color_count),
	};
}

//
// Ren3dUtils
// ==========================================================================


} // detail
} // bstone
