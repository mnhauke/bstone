/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/


//
// OpenGL 3D renderer (implementation).
//
// !!! Internal usage only !!!
//



#include "bstone_detail_ren_3d_gl.h"

#include <cassert>

#include "bstone_exception.h"
#include "bstone_ren_3d_cmd_buffer.h"
#include "bstone_ren_3d_limits.h"
#include "bstone_ren_3d_tests.h"
#include "bstone_scope_guard.h"

#include "bstone_detail_ren_3d_gl_error.h"
#include "bstone_detail_ren_3d_gl_utils.h"
#include "bstone_detail_ren_3d_gl_sampler.h"
#include "bstone_detail_ren_3d_gl_sampler_mgr.h"
#include "bstone_detail_ren_3d_gl_texture_2d.h"
#include "bstone_detail_ren_3d_gl_texture_mgr.h"
#include "bstone_detail_ren_3d_gl_vertex_input.h"
#include "bstone_detail_ren_3d_gl_vertex_input_mgr.h"
#include "bstone_detail_ren_3d_gl_shader.h"
#include "bstone_detail_ren_3d_gl_shader_stage.h"
#include "bstone_detail_ren_3d_gl_shader_stage_mgr.h"


namespace bstone
{
namespace detail
{


// ==========================================================================
// Ren3dGlException
//

class Ren3dGlException :
	public Exception
{
public:
	explicit Ren3dGlException(
		const char* message) noexcept
		:
		Exception{"REN_3D_GL", message}
	{
	}
}; // Ren3dGlException

//
// Ren3dGlException
// ==========================================================================


// ==========================================================================
// Ren3dGl
//

Ren3dGl::Ren3dGl(sys::VideoMgr& video_mgr, sys::WindowMgr& window_mgr, const Ren3dCreateParam& param)
try
	:
	video_mgr_{video_mgr},
	window_mgr_{window_mgr},
	kind{},
	name_{},
	description_{},
	device_info_{},
	device_features_{},
	gl_device_features_{},
	screen_width{},
	screen_height{},
	aa_kind_{},
	aa_value_{},
	extension_manager_{},
	context_{},
	msaa_fbo_{},
	msaa_color_rb_{},
	msaa_depth_rb_{}
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

	gl_mgr_ = video_mgr_.make_gl_mgr();
	gl_mgr_->load_default_library();
	// FIXMENOW
	// Unload on exception.

	kind = param.renderer_kind_;

	Ren3dGlUtils::probe_msaa(kind, *gl_mgr_, window_mgr, device_features_, gl_device_features_);

	aa_kind_ = param.aa_kind_;
	aa_value_ = param.aa_value_;

	auto window_param = Ren3dUtilsCreateWindowParam{};
	window_param.renderer_kind_ = kind;
	window_param.window_ = param.window_;
	window_param.aa_kind_ = aa_kind_;
	window_param.aa_value_ = aa_value_;

	if (window_param.window_.rect_2d_.extent_.width_ == 0 ||
		window_param.window_.rect_2d_.extent_.height_ == 0)
	{
		window_param.window_.rect_2d_.extent_.width_ = 1;
		window_param.window_.rect_2d_.extent_.height_ = 1;
	}

	if (window_param.aa_kind_ == Ren3dAaKind::ms)
	{
		if (device_features_.is_msaa_available_)
		{
			if (aa_value_ <= 0)
			{
				aa_value_ = device_features_.max_msaa_degree_;
			}

			if (aa_value_ < Ren3dLimits::min_aa_off)
			{
				aa_value_ = Ren3dLimits::min_aa_off;
			}

			if (aa_value_ > device_features_.max_msaa_degree_)
			{
				aa_value_ = device_features_.max_msaa_degree_;
			}

			if (device_features_.is_msaa_render_to_window_)
			{
				window_param.aa_value_ = aa_value_;
			}
			else
			{
				window_param.aa_kind_ = Ren3dAaKind::none;
				window_param.aa_value_ = 0;
				window_param.is_default_depth_buffer_disabled_ = true;
			}
		}
		else
		{
			window_param.aa_kind_ = Ren3dAaKind::none;
			window_param.aa_value_ = 0;
		}
	}

	Ren3dGlUtils::create_window_and_context(window_param, window_mgr, window_, gl_context_);

	const auto drawable_size = window_->gl_get_drawable_size();
	screen_width = drawable_size.width;
	screen_height = drawable_size.height;

	if (aa_kind_ == Ren3dAaKind::ms && device_features_.is_msaa_render_to_window_)
	{
		aa_value_ = Ren3dGlUtils::get_window_msaa_value(gl_context_->get_attributes());
	}

	extension_manager_ = Ren3dGlExtensionMgrFactory::create(*gl_mgr_);

	if (extension_manager_ == nullptr)
	{
		fail("Failed to create an extension manager.");
	}

	switch (kind)
	{
		case Ren3dKind::gl_2_0:
			extension_manager_->probe(Ren3dGlExtensionId::v2_0);

			if (!extension_manager_->has(Ren3dGlExtensionId::v2_0))
			{
				fail("Failed to load OpenGL 2.0 symbols.");
			}

			break;

		case Ren3dKind::gl_3_2_core:
			extension_manager_->probe(Ren3dGlExtensionId::v3_2_core);

			if (!extension_manager_->has(Ren3dGlExtensionId::v3_2_core))
			{
				fail("Failed to load OpenGL 3.2 core symbols.");
			}

			break;

		case Ren3dKind::gles_2_0:
			extension_manager_->probe(Ren3dGlExtensionId::es_v2_0);

			if (!extension_manager_->has(Ren3dGlExtensionId::es_v2_0))
			{
				fail("Failed to load OpenGL ES 2.0 symbols.");
			}

			break;

		default:
			fail("Unsupported renderer kind.");
	}

	Ren3dGlUtils::set_renderer_features(device_features_);

	gl_device_features_.context_kind_ = Ren3dGlUtils::get_context_kind(gl_context_->get_attributes());

	Ren3dGlUtils::probe_anisotropy(extension_manager_.get(), device_features_);
	Ren3dGlUtils::probe_npot(extension_manager_.get(), device_features_);
	Ren3dGlUtils::probe_mipmap(extension_manager_.get(), device_features_, gl_device_features_);
	Ren3dGlUtils::probe_framebuffer(extension_manager_.get(), gl_device_features_ );
	Ren3dGlUtils::probe_sampler(extension_manager_.get(), device_features_);
	Ren3dGlUtils::probe_max_vertex_arrays(device_features_);
	Ren3dGlUtils::probe_buffer_storage(extension_manager_.get(), gl_device_features_);
	Ren3dGlUtils::probe_dsa(extension_manager_.get(), gl_device_features_);
	Ren3dGlUtils::probe_sso(extension_manager_.get(), gl_device_features_ );

	if (device_features_.max_vertex_input_locations_ <= 0)
	{
		fail("No vertex input locations.");
	}

	Ren3dGlUtils::probe_vsync(*gl_mgr_, device_features_);
	Ren3dGlUtils::probe_vao(extension_manager_.get(), gl_device_features_);

	context_ = Ren3dGlContextFactory::create(kind, device_features_, gl_device_features_ );

	if (device_features_.is_vsync_available_)
	{
		gl_mgr_->set_swap_interval(param.is_vsync_);
	}

	create_framebuffers();

	device_info_ = Ren3dGlUtils::get_device_info();

	set_name_and_description();


	// Present.
	//
	context_->clear(Rgba8{});
	present();
}
BSTONE_STATIC_THROW_NESTED_FUNC

Ren3dGl::~Ren3dGl()
{
	gl_mgr_->unload_library();
}

Ren3dKind Ren3dGl::get_kind() const noexcept
{
	return kind;
}

const std::string& Ren3dGl::get_name() const noexcept
{
	return name_;
}

const std::string& Ren3dGl::get_description() const noexcept
{
	return description_;
}

[[noreturn]]
void Ren3dGl::fail(
	const char* message)
{
	throw Ren3dGlException{message};
}

[[noreturn]]
void Ren3dGl::fail_nested(
	const char* message)
{
	std::throw_with_nested(Ren3dGlException{message});
}

void Ren3dGl::fbo_deleter(
	GLuint gl_name) noexcept
{
	const auto gl_function = (glDeleteFramebuffers ? glDeleteFramebuffers : glDeleteFramebuffersEXT);
	gl_function(1, &gl_name);
	Ren3dGlError::ensure_assert();
}

void Ren3dGl::rbo_deleter(
	GLuint gl_name) noexcept
{
	const auto gl_function = (glDeleteRenderbuffers ? glDeleteRenderbuffers : glDeleteRenderbuffersEXT);
	gl_function(1, &gl_name);
	Ren3dGlError::ensure_assert();
}

void Ren3dGl::set_name_and_description()
try
{
	switch (kind)
	{
		case Ren3dKind::gl_2_0:
			name_ = "GL2";
			description_ = "OpenGL 2.0+";
			break;

		case Ren3dKind::gl_3_2_core:
			name_ = "GL3.2C";
			description_ = "OpenGL 3.2 core";
			break;

		case Ren3dKind::gles_2_0:
			name_ = "GLES2.0";
			description_ = "OpenGL ES 2.0";
			break;

		default:
			fail("Unsupported renderer kind.");
	}
}
catch (...)
{
	fail_nested(__func__);
}

const Ren3dDeviceFeatures& Ren3dGl::get_device_features() const noexcept
{
	return device_features_;
}

const Ren3dDeviceInfo& Ren3dGl::get_device_info() const noexcept
{
	return device_info_;
}

void Ren3dGl::set_window_mode(
	const Ren3dSetWindowModeParam& param)
try
{
	Ren3dUtils::set_window_mode(*window_, param);

	const auto size_changed =
		screen_width != param.rect_2d_.extent_.width_ ||
		screen_height != param.rect_2d_.extent_.height_;

	screen_width = param.rect_2d_.extent_.width_;
	screen_height = param.rect_2d_.extent_.height_;

	if (size_changed && gl_device_features_.is_framebuffer_available_)
	{
		destroy_msaa_framebuffer();
		create_msaa_framebuffer();
	}
}
BSTONE_STATIC_THROW_NESTED_FUNC

void Ren3dGl::set_window_title(const std::string& title_utf8)
try
{
	window_->set_title(title_utf8.c_str());
}
BSTONE_STATIC_THROW_NESTED_FUNC

void Ren3dGl::show_window(bool is_visible)
try
{
	window_->show(is_visible);
}
BSTONE_STATIC_THROW_NESTED_FUNC

bool Ren3dGl::get_vsync() const noexcept
{
	if (!device_features_.is_vsync_available_)
	{
		return false;
	}

	return gl_mgr_->get_swap_interval() == 1;
}

void Ren3dGl::enable_vsync(bool is_enabled)
try
{
	if (!device_features_.is_vsync_available_)
	{
		fail("Not available.");
	}

	if (device_features_.is_vsync_requires_restart_)
	{
		fail("Requires restart.");
	}

	gl_mgr_->set_swap_interval(is_enabled);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::set_anti_aliasing(
	const Ren3dAaKind aa_kind,
	const int aa_value)
try
{
	switch (aa_kind)
	{
		case Ren3dAaKind::none:
		case Ren3dAaKind::ms:
			break;

		default:
			fail("Invalid anti-aliasing kind.");
	}

	auto clamped_aa_value = aa_value;

	if (clamped_aa_value < Ren3dLimits::min_aa_off)
	{
		clamped_aa_value = Ren3dLimits::min_aa_off;
	}

	if (clamped_aa_value > Ren3dLimits::max_aa)
	{
		clamped_aa_value = Ren3dLimits::max_aa;
	}

	switch (aa_kind)
	{
		case Ren3dAaKind::none:
			disable_aa();
			return;

		case Ren3dAaKind::ms:
			set_msaa(clamped_aa_value);
			return;

		default:
			fail("Invalid anti-aliasing kind.");
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::read_pixels_rgb_888(
	void* buffer,
	bool& is_flipped_vertically)
try
{
	is_flipped_vertically = true;

	assert(buffer);

	bind_framebuffers_for_read_pixels();

	glReadBuffer(GL_BACK);
	Ren3dGlError::ensure();
  
	glReadPixels(
		0,
		0,
		screen_width,
		screen_height,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		buffer
	);

	Ren3dGlError::ensure();

	bind_framebuffers();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::present()
try
{
	blit_framebuffers();
	Ren3dGlError::ensure();
	window_->gl_swap_buffers();
	bind_framebuffers();
}
BSTONE_STATIC_THROW_NESTED_FUNC

Ren3dBufferUPtr Ren3dGl::create_buffer(
	const Ren3dCreateBufferParam& param)
try
{
	return context_->create_buffer(param);
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dVertexInputUPtr Ren3dGl::create_vertex_input(
	const Ren3dCreateVertexInputParam& param)
try
{
	return context_->get_vertex_input_manager()->create(param);
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dShaderUPtr Ren3dGl::create_shader(
	const Ren3dCreateShaderParam& param)
try
{
	return context_->create_shader(param);
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dShaderStageUPtr Ren3dGl::create_shader_stage(
	const Ren3dCreateShaderStageParam& param)
try
{
	return context_->get_shader_stage_manager()->create(param);
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dTexture2dUPtr Ren3dGl::create_texture_2d(
	const Ren3dCreateTexture2dParam& param)
try
{
	return context_->get_texture_manager()->create(param);
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dSamplerUPtr Ren3dGl::create_sampler(
	const Ren3dCreateSamplerParam& param)
try
{
	return context_->get_sampler_manager()->create(param);
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dGl::RboResource Ren3dGl::create_renderbuffer()
try
{
	if (!gl_device_features_.is_framebuffer_available_)
	{
		fail("Framebuffer not available.");
	}

	const auto gl_function = (gl_device_features_.is_framebuffer_ext_ ? glGenRenderbuffersEXT : glGenRenderbuffers);

	auto gl_name = GLuint{};
	gl_function(1, &gl_name);
	Ren3dGlError::ensure_debug();

	auto rbo_resource = RboResource{gl_name};

	if (!rbo_resource)
	{
		fail("Failed to create OpenGL renderbuffer object.");
	}

	return rbo_resource;
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::bind_renderbuffer(
	const GLuint gl_renderbuffer_name)
try
{
	const auto gl_func = (gl_device_features_.is_framebuffer_ext_ ? glBindRenderbufferEXT : glBindRenderbuffer);

	gl_func(GL_RENDERBUFFER, gl_renderbuffer_name);
	Ren3dGlError::ensure_debug();
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dGl::FboResource Ren3dGl::create_framebuffer()
try
{
	if (!gl_device_features_.is_framebuffer_available_)
	{
		fail("Framebuffer not available.");
	}

	const auto gl_func = (gl_device_features_.is_framebuffer_ext_ ? glGenFramebuffersEXT : glGenFramebuffers);

	auto gl_name = GLuint{};
	gl_func(1, &gl_name);
	Ren3dGlError::ensure_debug();

	auto fbo_resource = FboResource{gl_name};

	if (!fbo_resource)
	{
		fail("Failed to create OpenGL framebuffer object.");
	}

	return fbo_resource;
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::bind_framebuffer(
	const GLenum gl_target,
	const GLuint gl_name)
try
{
	assert(gl_device_features_.is_framebuffer_available_);

	const auto gl_func = (gl_device_features_.is_framebuffer_ext_ ? glBindFramebufferEXT : glBindFramebuffer);

	gl_func(gl_target, gl_name);
	Ren3dGlError::ensure_debug();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::blit_framebuffer(
	const int src_width,
	const int src_height,
	const int dst_width,
	const int dst_height,
	const bool is_linear_filter)
try
{
	assert(src_width > 0);
	assert(src_height > 0);
	assert(dst_width > 0);
	assert(dst_height > 0);

	assert(gl_device_features_.is_framebuffer_available_);

	const auto gl_func = (
		gl_device_features_.is_framebuffer_ext_ ?
		glBlitFramebufferEXT :
		glBlitFramebuffer
	);

	const auto gl_filter = (is_linear_filter ? GL_LINEAR : GL_NEAREST);

	gl_func(
		0,
		0,
		src_width,
		src_height,
		0,
		0,
		dst_width,
		dst_height,
		GL_COLOR_BUFFER_BIT,
		gl_filter
	);

	Ren3dGlError::ensure_debug();
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dGl::RboResource Ren3dGl::create_renderbuffer(
	const int width,
	const int height,
	const int sample_count,
	const GLenum gl_internal_format)
try
{
	assert(width > 0);
	assert(height > 0);
	assert(sample_count >= 0);
	assert(gl_internal_format > 0);

	auto rbo_resource = create_renderbuffer();
	bind_renderbuffer(rbo_resource.get());

	assert(gl_device_features_.is_framebuffer_available_);

	const auto gl_func = (
		gl_device_features_.is_framebuffer_ext_ ?
		glRenderbufferStorageMultisampleEXT :
		glRenderbufferStorageMultisample
	);

	gl_func(GL_RENDERBUFFER, sample_count, gl_internal_format, width, height);
	Ren3dGlError::ensure_debug();

	bind_renderbuffer(0);

	return rbo_resource;
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::destroy_msaa_color_rb()
try
{
	msaa_color_rb_.reset();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::destroy_msaa_depth_rb()
try
{
	msaa_depth_rb_.reset();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::destroy_msaa_fbo()
try
{
	msaa_fbo_.reset();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::destroy_msaa_framebuffer()
try
{
	destroy_msaa_fbo();
	destroy_msaa_color_rb();
	destroy_msaa_depth_rb();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::create_msaa_color_rb(
	const int width,
	const int height,
	const int sample_count)
try
{
	msaa_color_rb_ = create_renderbuffer(width, height, sample_count, GL_RGBA8);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::create_msaa_depth_rb(
	const int width,
	const int height,
	const int sample_count)
try
{
	msaa_depth_rb_ = create_renderbuffer(width, height, sample_count, GL_DEPTH_COMPONENT);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::create_msaa_framebuffer()
try
{
	auto aa_degree = aa_value_;

	if (aa_kind_ == Ren3dAaKind::none)
	{
		aa_degree = Ren3dLimits::min_aa_off;
	}

	if (aa_degree < Ren3dLimits::min_aa_on)
	{
		aa_degree = Ren3dLimits::min_aa_off;
	}

	if (aa_degree > device_features_.max_msaa_degree_)
	{
		aa_degree = device_features_.max_msaa_degree_;
	}

	create_msaa_color_rb(screen_width, screen_height, aa_degree);
	create_msaa_depth_rb(screen_width, screen_height, aa_degree);

	msaa_fbo_ = create_framebuffer();
	bind_framebuffer(GL_FRAMEBUFFER, msaa_fbo_.get());

	const auto framebuffer_renderbuffer_func = (
		gl_device_features_.is_framebuffer_ext_ ?
		glFramebufferRenderbufferEXT :
		glFramebufferRenderbuffer
	);

	framebuffer_renderbuffer_func(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_RENDERBUFFER,
		msaa_color_rb_.get()
	);

	framebuffer_renderbuffer_func(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER,
		msaa_depth_rb_.get()
	);

	const auto check_framebuffer_status_func = (
		gl_device_features_.is_framebuffer_ext_ ?
		glCheckFramebufferStatusEXT :
		glCheckFramebufferStatus
	);

	const auto framebuffer_status = check_framebuffer_status_func(GL_FRAMEBUFFER);

	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE)
	{
		fail("Incomplete framebuffer object.");
	}

	bind_framebuffer(GL_FRAMEBUFFER, 0);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::destroy_framebuffers()
try
{
	destroy_msaa_framebuffer();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::create_framebuffers()
try
{
	if (!gl_device_features_.is_framebuffer_available_)
	{
		return;
	}

	create_msaa_framebuffer();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::blit_framebuffers()
try
{
	if (!msaa_fbo_)
	{
		return;
	}

	// MSAA FBO -> Default FBO
	//

	// Read: MSAA
	// Draw: Default
	bind_framebuffer(GL_DRAW_FRAMEBUFFER, 0);

	blit_framebuffer(
		screen_width,
		screen_height,
		screen_width,
		screen_height,
		false
	);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::bind_framebuffers()
try
{
	if (!msaa_fbo_)
	{
		return;
	}

	bind_framebuffer(GL_FRAMEBUFFER, msaa_fbo_.get());
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::bind_framebuffers_for_read_pixels()
try
{
	if (!msaa_fbo_)
	{
		return;
	}

	bind_framebuffer(GL_FRAMEBUFFER, 0);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::disable_aa()
try
{
	aa_kind_ = Ren3dAaKind::none;

	if (!msaa_fbo_.get())
	{
		return;
	}

	destroy_msaa_framebuffer();
	create_msaa_framebuffer();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::set_msaa(
	const int aa_value)
try
{
	if (device_features_.is_msaa_requires_restart_)
	{
		fail("Requires restart.");
	}

	if (!gl_device_features_.is_framebuffer_available_)
	{
		fail("Framebuffer not available.");
	}

	if (aa_kind_ == Ren3dAaKind::ms && aa_value_ == aa_value)
	{
		return;
	}

	aa_kind_ = Ren3dAaKind::ms;
	aa_value_ = aa_value;

	destroy_framebuffers();

	create_framebuffers();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_clear(
	const Ren3dClearCmd& command)
try
{
	context_->clear(command.clear_.color_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_culling(
	const Ren3dEnableCullingCmd& command)
try
{
	context_->enable_culling(command.is_enable_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_enable_depth_test(
	const Ren3dEnableDepthTestCmd& command)
try
{
	context_->enable_depth_test(command.is_enable_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_enable_depth_write(
	const Ren3dEnableDepthWriteCmd& command)
try
{
	context_->enable_depth_write(command.is_enable_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_viewport(
	const Ren3dSetViewportCmd& command)
try
{
	context_->set_viewport(command.viewport_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_enable_blending(
	const Ren3dEnableBlendingCmd& command)
try
{
	context_->enable_blending(command.is_enable_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_blending_func(
	const Ren3dSetBlendingFuncCmd& command)
try
{
	context_->set_blending_func(command.blending_func_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_enable_scissor(
	const Ren3dEnableScissorCmd& command)
try
{
	context_->enable_scissor(command.is_enable_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_scissor_box(
	const Ren3dSetScissorBoxCmd& command)
try
{
	context_->set_scissor_box(command.scissor_box_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_texture(
	const Ren3dSetTextureCmd& command)
try
{
	context_->set_texture_2d(static_cast<Ren3dGlTexture2dPtr>(command.texture_2d_));
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_sampler(
	const Ren3dSetSamplerCmd& command)
try
{
	context_->set_sampler(static_cast<Ren3dGlSamplerPtr>(command.sampler_));
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_vertex_input(
	const Ren3dSetVertexInputCmd& command)
try
{
	context_->set_vertex_input(static_cast<Ren3dGlVertexInputPtr>(command.vertex_input_));
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_shader_stage(
	const Ren3dSetShaderStageCmd& command)
try
{
	context_->set_shader_stage(static_cast<Ren3dGlShaderStagePtr>(command.shader_stage_));
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_int32_uniform(
	const Ren3dSetInt32UniformCmd& command)
try
{
	if (!command.var_)
	{
		fail("Null variable.");
	}

	command.var_->set_int32(command.value_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_float32_uniform(
	const Ren3dSetFloat32UniformCmd& command)
try
{
	if (!command.var_)
	{
		fail("Null variable.");
	}

	command.var_->set_float32(command.value_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_vec2_uniform(
	const Ren3dSetVec2UniformCmd& command)
try
{
	if (!command.var_)
	{
		fail("Null variable.");
	}

	command.var_->set_vec2(command.value_.data());
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_vec4_uniform(
	const Ren3dSetVec4UniformCmd& command)
try
{
	if (!command.var_)
	{
		fail("Null variable.");
	}

	command.var_->set_vec4(command.value_.data());
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_mat4_uniform(
	const Ren3dSetMat4UniformCmd& command)
try
{
	if (!command.var_)
	{
		fail("Null variable.");
	}

	command.var_->set_mat4(command.value_.data());
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_set_sampler_2d_uniform(
	const Ren3dSetSampler2dUniformCmd& command)
try
{
	if (!command.var_)
	{
		fail("Null variable.");
	}

	command.var_->set_sampler_2d(command.value_);
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_draw_indexed(
	const Ren3dDrawIndexedCmd& command)
try
{
	const auto& param = command.draw_indexed_;

	auto gl_primitive_topology = GLenum{};

	switch (param.primitive_topology_)
	{
		case Ren3dPrimitiveTopology::point_list:
			gl_primitive_topology = GL_POINTS;
			break;

		case Ren3dPrimitiveTopology::line_list:
			gl_primitive_topology = GL_LINES;
			break;

		case Ren3dPrimitiveTopology::line_strip:
			gl_primitive_topology = GL_LINE_STRIP;
			break;

		case Ren3dPrimitiveTopology::triangle_list:
			gl_primitive_topology = GL_TRIANGLES;
			break;

		case Ren3dPrimitiveTopology::triangle_strip:
			gl_primitive_topology = GL_TRIANGLE_STRIP;
			break;

		default:
			fail("Unsupported primitive topology.");
	}

	if (param.vertex_count_ < 0)
	{
		fail("Vertex count out of range.");
	}

	if (param.vertex_count_ == 0)
	{
		return;
	}

	switch (param.index_byte_depth_)
	{
		case 1:
		case 2:
		case 4:
			break;

		default:
			fail("Unsupported index value byte depth.");
	}

	if (param.index_buffer_offset_ < 0)
	{
		fail("Offset to indices out of range.");
	}

	if (param.index_offset_ < 0)
	{
		fail("Index offset out of range.");
	}

	// Vertex input.
	//
	const auto vertex_input = context_->get_vertex_input();

	if (!vertex_input)
	{
		fail("Null current vertex input.");
	}

	context_->get_vertex_input_manager()->set(vertex_input);

	// Shader stage.
	//
	context_->get_shader_stage_manager()->set(context_->get_shader_stage());

	// Sampler.
	//
	context_->get_sampler_manager()->set(context_->get_sampler());

	// Textures.
	//
	context_->get_texture_manager()->set(context_->get_texture_2d());

	// Index buffer.
	//
	auto index_buffer = static_cast<Ren3dGlBufferPtr>(vertex_input->get_index_buffer());

	if (!index_buffer)
	{
		fail("Null index buffer.");
	}

	const auto index_buffer_offset = param.index_buffer_offset_ + (param.index_offset_ * param.index_byte_depth_);
	const auto index_buffer_indices = reinterpret_cast<const void*>(static_cast<std::intptr_t>(index_buffer_offset));

	const auto gl_element_type = Ren3dGlUtils::index_buffer_get_element_type_by_byte_depth(
		param.index_byte_depth_);

	index_buffer->set(true);

	// Draw it.
	//
	glDrawElements(
		gl_primitive_topology, // mode
		param.vertex_count_, // count
		gl_element_type, // type
		index_buffer_indices // indices
	);

	Ren3dGlError::ensure_debug();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGl::submit_commands(
	Ren3dCmdBufferPtr* const command_buffers,
	const int command_buffer_count)
try
{
	if (command_buffer_count < 0)
	{
		fail("Command buffer count out of range.");
	}

	if (command_buffer_count > 0 && !command_buffers)
	{
		fail("Null command buffers.");
	}

	for (int i = 0; i < command_buffer_count; ++i)
	{
		auto command_buffer = command_buffers[i];

		if (!command_buffer)
		{
			fail("Null command buffer.");
		}

		if (!command_buffer->is_enabled())
		{
			continue;
		}

		const auto command_count = command_buffer->get_count();

		command_buffer->begin_read();

		for (int j = 0; j < command_count; ++j)
		{
			const auto command_id = command_buffer->read_command_id();

			switch (command_id)
			{
			case Ren3dCmdId::clear:
				submit_clear(*command_buffer->read_clear());
				break;

			case Ren3dCmdId::enable_culling:
				submit_culling(*command_buffer->read_enable_culling());
				break;

			case Ren3dCmdId::enable_depth_test:
				submit_enable_depth_test(*command_buffer->read_enable_depth_test());
				break;

			case Ren3dCmdId::enable_depth_write:
				submit_enable_depth_write(*command_buffer->read_enable_depth_write());
				break;

			case Ren3dCmdId::set_viewport:
				submit_set_viewport(*command_buffer->read_set_viewport());
				break;

			case Ren3dCmdId::enable_scissor:
				submit_enable_scissor(*command_buffer->read_enable_scissor());
				break;

			case Ren3dCmdId::set_scissor_box:
				submit_set_scissor_box(*command_buffer->read_set_scissor_box());
				break;

			case Ren3dCmdId::enable_blending:
				submit_enable_blending(*command_buffer->read_enable_blending());
				break;

			case Ren3dCmdId::set_blending_func:
				submit_set_blending_func(*command_buffer->read_set_blending_func());
				break;

			case Ren3dCmdId::set_texture:
				submit_set_texture(*command_buffer->read_set_texture());
				break;

			case Ren3dCmdId::set_sampler:
				submit_set_sampler(*command_buffer->read_set_sampler());
				break;

			case Ren3dCmdId::set_vertex_input:
				submit_set_vertex_input(*command_buffer->read_set_vertex_input());
				break;

			case Ren3dCmdId::set_shader_stage:
				submit_set_shader_stage(*command_buffer->read_set_shader_stage());
				break;

			case Ren3dCmdId::set_int32_uniform:
				submit_set_int32_uniform(*command_buffer->read_set_int32_uniform());
				break;

			case Ren3dCmdId::set_float32_uniform:
				submit_set_float32_uniform(*command_buffer->read_set_float32_uniform());
				break;

			case Ren3dCmdId::set_vec2_uniform:
				submit_set_vec2_uniform(*command_buffer->read_set_vec2_uniform());
				break;

			case Ren3dCmdId::set_vec4_uniform:
				submit_set_vec4_uniform(*command_buffer->read_set_vec4_uniform());
				break;

			case Ren3dCmdId::set_mat4_uniform:
				submit_set_mat4_uniform(*command_buffer->read_set_mat4_uniform());
				break;

			case Ren3dCmdId::set_sampler_2d_uniform:
				submit_set_sampler_2d_uniform(*command_buffer->read_set_sampler_2d_uniform());
				break;

			case Ren3dCmdId::draw_indexed:
				submit_draw_indexed(*command_buffer->read_draw_indexed());
				break;

			default:
				fail("Unsupported command id.");
			}
		}

		command_buffer->end_read();
	}
}
catch (...)
{
	fail_nested(__func__);
}

//
// Ren3dGl
// ==========================================================================


} // detail
} // bstone
