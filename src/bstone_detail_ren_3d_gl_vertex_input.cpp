/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/


//
// OpenGL vertex input (implementation).
//


#include "bstone_detail_ren_3d_gl_vertex_input.h"

#include <algorithm>

#include "bstone_exception.h"
#include "bstone_ren_3d_tests.h"
#include "bstone_unique_resource.h"

#include "bstone_detail_ren_3d_gl_buffer.h"
#include "bstone_detail_ren_3d_gl_context.h"
#include "bstone_detail_ren_3d_gl_device_features.h"
#include "bstone_detail_ren_3d_gl_error.h"
#include "bstone_detail_ren_3d_gl_utils.h"
#include "bstone_detail_ren_3d_gl_vertex_input_mgr.h"
#include "bstone_detail_ren_3d_utils.h"


namespace bstone
{
namespace detail
{


// =========================================================================
// Ren3dGlVertexInputException
//

class Ren3dGlVertexInputException :
	public Exception
{
public:
	explicit Ren3dGlVertexInputException(
		const char* message) noexcept
		:
		Exception{"REN_3D_GL_VERTEX_INPUT", message}
	{
	}
}; // Ren3dGlVertexInputException

//
// Ren3dGlVertexInputException
// =========================================================================


// =========================================================================
// Ren3dGlVertexInputImpl
//

class Ren3dGlVertexInputImpl final :
	public Ren3dGlVertexInput
{
public:
	Ren3dGlVertexInputImpl(
		const Ren3dGlVertexInputMgrPtr vertex_input_manager,
		const Ren3dCreateVertexInputParam& param);

	~Ren3dGlVertexInputImpl() override;


	void bind_vao() override;

	Ren3dBufferPtr get_index_buffer() const noexcept override;

	void bind() override;


private:
	const Ren3dGlVertexInputMgrPtr manager_;
	const Ren3dDeviceFeatures& device_features_;
	const Ren3dGlDeviceFeatures& gl_device_features_;

	Ren3dGlBufferPtr index_buffer_;
	Ren3dVertexAttribDescrs attrib_descrs_;

	static void vao_deleter(
		GLuint gl_name) noexcept;

	using VaoResource = UniqueResource<GLuint, vao_deleter>;
	VaoResource vao_resource_;


	[[noreturn]]
	static void fail(
		const char* message);

	[[noreturn]]
	static void fail_nested(
		const char* message);


	void initialize_vao();

	void enable_attrib_array(
		const int index,
		const bool is_enable);

	void assign_default_attribute(
		const Ren3dVertexAttribDescr& attribute_description);

	void assign_regular_attribute(
		const Ren3dVertexAttribDescr& attribute_description);

	void assign_attribute(
		const Ren3dVertexAttribDescr& attribute_description);

	void bind_internal();
}; // Ren3dGlVertexInputImpl

//
// Ren3dGlVertexInputImpl
// =========================================================================


// =========================================================================
// Ren3dGlVertexInputImpl
//

Ren3dGlVertexInputImpl::Ren3dGlVertexInputImpl(
	const Ren3dGlVertexInputMgrPtr vertex_input_manager,
	const Ren3dCreateVertexInputParam& param)
try
	:
	manager_{vertex_input_manager},
	device_features_{vertex_input_manager->get_context()->get_device_features()},
	gl_device_features_{vertex_input_manager->get_context()->get_gl_device_features()},
	index_buffer_{},
	attrib_descrs_{},
	vao_resource_{}
{
	const auto max_locations = device_features_.max_vertex_input_locations_;

	Ren3dUtils::vertex_input_validate_param(max_locations, param);

	const auto is_location_out_of_range = std::any_of(
		param.attrib_descrs_.cbegin(),
		param.attrib_descrs_.cend(),
		[max_locations](const auto& item)
		{
			return item.location_ < 0 || item.location_ >= max_locations;
		}
	);

	if (is_location_out_of_range)
	{
		fail("Location out of range.");
	}

	index_buffer_ = static_cast<Ren3dGlBufferPtr>(param.index_buffer_);
	attrib_descrs_ = param.attrib_descrs_;

	initialize_vao();
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dGlVertexInputImpl::~Ren3dGlVertexInputImpl()
{
	manager_->bind_default_vao();
}

void Ren3dGlVertexInputImpl::bind_vao()
try
{
	if (vao_resource_)
	{
		glBindVertexArray(vao_resource_.get());
		Ren3dGlError::ensure_debug();
	}
}
catch (...)
{
	fail_nested(__func__);
}

Ren3dBufferPtr Ren3dGlVertexInputImpl::get_index_buffer() const noexcept
{
	return index_buffer_;
}

void Ren3dGlVertexInputImpl::bind()
try
{
	if (vao_resource_)
	{
		bind_vao();

		for (const auto& attribute_description : attrib_descrs_)
		{
			if (attribute_description.is_default_)
			{
				assign_default_attribute(attribute_description);
			}
		}
	}
	else
	{
		bind_internal();
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGlVertexInputImpl::vao_deleter(
	GLuint gl_name) noexcept
{
	glDeleteVertexArrays(1, &gl_name);
	Ren3dGlError::ensure_assert();
}

[[noreturn]]
void Ren3dGlVertexInputImpl::fail(
	const char* message)
{
	throw Ren3dGlVertexInputException{message};
}

[[noreturn]]
void Ren3dGlVertexInputImpl::fail_nested(
	const char* message)
{
	std::throw_with_nested(Ren3dGlVertexInputException{message});
}

void Ren3dGlVertexInputImpl::initialize_vao()
try
{
	if (gl_device_features_.is_vao_available_)
	{
		auto gl_name = GLuint{};
		glGenVertexArrays(1, &gl_name);
		Ren3dGlError::ensure_debug();

		vao_resource_.reset(gl_name);

		if (!vao_resource_)
		{
			fail("Failed to create VAO.");
		}

		bind_vao();

		if (index_buffer_)
		{
			index_buffer_->set(true);
		}

		for (const auto& attribute_description : attrib_descrs_)
		{
			if (!attribute_description.is_default_)
			{
				assign_regular_attribute(attribute_description);
			}
		}
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGlVertexInputImpl::enable_attrib_array(
	const int index,
	const bool is_enable)
try
{
	const auto gl_func = (is_enable ? glEnableVertexAttribArray : glDisableVertexAttribArray);
	gl_func(index);
	Ren3dGlError::ensure_debug();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGlVertexInputImpl::assign_default_attribute(
	const Ren3dVertexAttribDescr& attribute_description)
try
{
	glVertexAttrib4fv(
		attribute_description.location_,
		attribute_description.default_value_.data()
	);

	Ren3dGlError::ensure_debug();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGlVertexInputImpl::assign_regular_attribute(
	const Ren3dVertexAttribDescr& attribute_description)
try
{
	auto gl_component_count = GLint{};
	auto gl_component_format = GLenum{};
	auto gl_is_normalized = GLboolean{};

	switch (attribute_description.format_)
	{
		case Ren3dVertexAttribFormat::rgba_8_unorm:
			gl_is_normalized = true;
			gl_component_count = 4;
			gl_component_format = GL_UNSIGNED_BYTE;
			break;

		case Ren3dVertexAttribFormat::rg_32_sfloat:
			gl_component_count = 2;
			gl_component_format = GL_FLOAT;
			break;

		case Ren3dVertexAttribFormat::rgb_32_sfloat:
			gl_component_count = 3;
			gl_component_format = GL_FLOAT;
			break;

		default:
			fail("Invalid format.");
	}

	enable_attrib_array(attribute_description.location_, true);

	auto vertex_buffer = static_cast<Ren3dGlBufferPtr>(attribute_description.vertex_buffer_);

	vertex_buffer->set(true);

	const auto vertex_buffer_data = reinterpret_cast<const void*>(
		static_cast<std::intptr_t>(
			attribute_description.offset_));

	glVertexAttribPointer(
		attribute_description.location_,
		gl_component_count,
		gl_component_format,
		gl_is_normalized,
		attribute_description.stride_,
		vertex_buffer_data
	);

	Ren3dGlError::ensure_debug();
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGlVertexInputImpl::assign_attribute(
	const Ren3dVertexAttribDescr& attribute_description)
try
{
	if (attribute_description.is_default_)
	{
		assign_default_attribute(attribute_description);
	}
	else
	{
		assign_regular_attribute(attribute_description);
	}
}
catch (...)
{
	fail_nested(__func__);
}

void Ren3dGlVertexInputImpl::bind_internal()
try
{
	if (index_buffer_)
	{
		index_buffer_->set(true);
	}

	for (const auto& attribute_description : attrib_descrs_)
	{
		assign_attribute(attribute_description);
	}

	for (int i = 0; i < device_features_.max_vertex_input_locations_; ++i)
	{
		const auto it_end = attrib_descrs_.cend();

		const auto it = std::find_if(
			attrib_descrs_.cbegin(),
			it_end,
			[i](const auto& item)
			{
				return !item.is_default_ && item.location_ == i;
			}
		);

		if (it == it_end)
		{
			enable_attrib_array(i, false);
		}
	}
}
catch (...)
{
	fail_nested(__func__);
}

//
// Ren3dGlVertexInputImpl
// =========================================================================


// =========================================================================
// Ren3dGlVertexInputFactory
//

Ren3dGlVertexInputUPtr Ren3dGlVertexInputFactory::create(
	const Ren3dGlVertexInputMgrPtr vertex_input_manager,
	const Ren3dCreateVertexInputParam& param)
{
	return std::make_unique<Ren3dGlVertexInputImpl>(vertex_input_manager, param);
}

//
// Ren3dGlVertexInputFactory
// =========================================================================


} // detail
} // bstone
