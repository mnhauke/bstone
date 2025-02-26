/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/


//
// OpenGL vertex input (implementation interface).
//


#ifndef BSTONE_DETAIL_REN_3D_GL_VERTEX_INPUT_INCLUDED
#define BSTONE_DETAIL_REN_3D_GL_VERTEX_INPUT_INCLUDED


#include "bstone_ren_3d_vertex_input.h"

#include "bstone_detail_ren_3d_gl_device_features.h"


namespace bstone
{
namespace detail
{


class Ren3dGlVertexInputMgr;
using Ren3dGlVertexInputMgrPtr = Ren3dGlVertexInputMgr*;


// =========================================================================
// Ren3dGlVertexInput
//

class Ren3dGlVertexInput :
	public Ren3dVertexInput
{
protected:
	Ren3dGlVertexInput() = default;


public:
	~Ren3dGlVertexInput() override = default;


	virtual void bind() = 0;

	virtual void bind_vao() = 0;

	virtual Ren3dBufferPtr get_index_buffer() const noexcept = 0;
}; // VertexInput

using Ren3dGlVertexInputPtr = Ren3dGlVertexInput*;
using Ren3dGlVertexInputUPtr = std::unique_ptr<Ren3dGlVertexInput>;

//
// Ren3dGlVertexInput
// =========================================================================


// =========================================================================
// Ren3dGlVertexInputFactory
//

struct Ren3dGlVertexInputFactory
{
	static Ren3dGlVertexInputUPtr create(
		const Ren3dGlVertexInputMgrPtr vertex_input_manager,
		const Ren3dCreateVertexInputParam& param);
}; // Ren3dGlVertexInputFactory

//
// Ren3dGlVertexInputFactory
// =========================================================================


} // detail
} // bstone


#endif // !BSTONE_DETAIL_REN_3D_GL_VERTEX_INPUT_INCLUDED
