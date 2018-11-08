#include "common.h"
#if XASH_VIDEO == VIDEO_SWITCH
#include "client.h"
#include "gl_local.h"
#include "mod_local.h"
#include "input.h"
#include "gl_vidnt.h"
#include <EGL/egl.h>
#include <switch.h>

static EGLDisplay s_display;
static EGLSurface s_surface;
static EGLContext s_context;

static AppletOperationMode operation_mode;

typedef enum
{
	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown
} rserr_t;

#define GL_CALL( x ) #x, NULL
#define GL_CALL_EX( x, y ) #x, NULL

static dllfunc_t opengl_110funcs[] =
{
{ GL_CALL( glClearColor )	},
{ GL_CALL( glClear ) },
{ GL_CALL( glAlphaFunc ) },
{ GL_CALL( glBlendFunc ) },
{ GL_CALL( glCullFace ) },
{ GL_CALL( glDrawBuffer ) },
{ GL_CALL( glReadBuffer ) },
{ GL_CALL( glEnable ) },
{ GL_CALL( glDisable ) },
{ GL_CALL( glEnableClientState ) },
{ GL_CALL( glDisableClientState ) },
{ GL_CALL( glGetBooleanv ) },
{ GL_CALL( glGetDoublev ) },
{ GL_CALL( glGetFloatv ) },
{ GL_CALL( glGetIntegerv ) },
{ GL_CALL( glGetError ) },
{ GL_CALL( glGetString ) },
{ GL_CALL( glFinish ) },
{ GL_CALL( glFlush ) },
{ GL_CALL( glClearDepth ) },
{ GL_CALL( glDepthFunc ) },
{ GL_CALL( glDepthMask ) },
{ GL_CALL( glDepthRange ) },
{ GL_CALL( glFrontFace ) },
{ GL_CALL( glDrawArrays ) },
{ GL_CALL( glColorMask ) },
{ GL_CALL( glIndexPointer ) },
{ GL_CALL( glVertexPointer ) },
{ GL_CALL( glNormalPointer ) },
{ GL_CALL( glColorPointer ) },
{ GL_CALL( glTexCoordPointer ) },
{ GL_CALL( glArrayElement ) },
{ GL_CALL( glColor3f ) },
{ GL_CALL( glColor3fv ) },
{ GL_CALL( glColor4f ) },
{ GL_CALL( glColor4fv ) },
{ GL_CALL( glColor3ub ) },
{ GL_CALL( glColor4ub ) },
{ GL_CALL( glColor4ubv ) },
{ GL_CALL( glTexCoord1f ) },
{ GL_CALL( glTexCoord2f ) },
{ GL_CALL( glTexCoord3f ) },
{ GL_CALL( glTexCoord4f ) },
{ GL_CALL( glTexGenf ) },
{ GL_CALL( glTexGenfv ) },
{ GL_CALL( glTexGeni ) },
{ GL_CALL( glVertex2f ) },
{ GL_CALL( glVertex3f ) },
{ GL_CALL( glVertex3fv ) },
{ GL_CALL( glNormal3f ) },
{ GL_CALL( glNormal3fv ) },
{ GL_CALL( glBegin ) },
{ GL_CALL( glEnd ) },
{ GL_CALL( glLineWidth ) },
{ GL_CALL( glPointSize ) },
{ GL_CALL( glMatrixMode ) },
{ GL_CALL( glOrtho ) },
{ GL_CALL( glRasterPos2f ) },
{ GL_CALL( glFrustum ) },
{ GL_CALL( glViewport ) },
{ GL_CALL( glPushMatrix ) },
{ GL_CALL( glPopMatrix ) },
{ GL_CALL( glPushAttrib ) },
{ GL_CALL( glPopAttrib ) },
{ GL_CALL( glLoadIdentity ) },
{ GL_CALL( glLoadMatrixd ) },
{ GL_CALL( glLoadMatrixf ) },
{ GL_CALL( glMultMatrixd ) },
{ GL_CALL( glMultMatrixf ) },
{ GL_CALL( glRotated ) },
{ GL_CALL( glRotatef ) },
{ GL_CALL( glScaled ) },
{ GL_CALL( glScalef ) },
{ GL_CALL( glTranslated ) },
{ GL_CALL( glTranslatef ) },
{ GL_CALL( glReadPixels ) },
{ GL_CALL( glDrawPixels ) },
{ GL_CALL( glStencilFunc ) },
{ GL_CALL( glStencilMask ) },
{ GL_CALL( glStencilOp ) },
{ GL_CALL( glClearStencil ) },
{ GL_CALL( glIsEnabled ) },
{ GL_CALL( glIsList ) },
{ GL_CALL( glIsTexture ) },
{ GL_CALL( glTexEnvf ) },
{ GL_CALL( glTexEnvfv ) },
{ GL_CALL( glTexEnvi ) },
{ GL_CALL( glTexParameterf ) },
{ GL_CALL( glTexParameterfv ) },
{ GL_CALL( glTexParameteri ) },
{ GL_CALL( glHint ) },
{ GL_CALL( glPixelStoref ) },
{ GL_CALL( glPixelStorei ) },
{ GL_CALL( glGenTextures ) },
{ GL_CALL( glDeleteTextures ) },
{ GL_CALL( glBindTexture ) },
{ GL_CALL( glTexImage1D ) },
{ GL_CALL( glTexImage2D ) },
{ GL_CALL( glTexSubImage1D ) },
{ GL_CALL( glTexSubImage2D ) },
{ GL_CALL( glCopyTexImage1D ) },
{ GL_CALL( glCopyTexImage2D ) },
{ GL_CALL( glCopyTexSubImage1D ) },
{ GL_CALL( glCopyTexSubImage2D ) },
{ GL_CALL( glScissor ) },
{ GL_CALL( glGetTexEnviv ) },
{ GL_CALL( glPolygonOffset ) },
{ GL_CALL( glPolygonMode ) },
{ GL_CALL( glPolygonStipple ) },
{ GL_CALL( glClipPlane ) },
{ GL_CALL( glGetClipPlane ) },
{ GL_CALL( glShadeModel ) },
{ GL_CALL( glFogfv ) },
{ GL_CALL( glFogf ) },
{ GL_CALL( glFogi ) },
{ NULL, NULL }
};

static dllfunc_t pointparametersfunc[] =
{
{ GL_CALL( glPointParameterfEXT ) },
{ GL_CALL( glPointParameterfvEXT ) },
{ NULL, NULL }
};

static dllfunc_t drawrangeelementsfuncs[] =
{
{ GL_CALL( glDrawRangeElements ) },
{ NULL, NULL }
};

static dllfunc_t drawrangeelementsextfuncs[] =
{
{ GL_CALL( glDrawRangeElementsEXT ) },
{ NULL, NULL }
};

static dllfunc_t debugoutputfuncs[] =
{
{ GL_CALL( glDebugMessageControlARB ) },
{ GL_CALL( glDebugMessageInsertARB ) },
{ GL_CALL( glDebugMessageCallbackARB ) },
{ GL_CALL( glGetDebugMessageLogARB ) },
{ NULL, NULL }
};

static dllfunc_t sgis_multitexturefuncs[] =
{
{ GL_CALL( glSelectTextureSGIS ) },
{ GL_CALL( glMTexCoord2fSGIS ) }, // ,
{ NULL, NULL }
};

static dllfunc_t multitexturefuncs[] =
{
{ GL_CALL_EX( glMultiTexCoord1fARB, glMultiTexCoord1f ) },
{ GL_CALL_EX( glMultiTexCoord2fARB, glMultiTexCoord2f ) },
{ GL_CALL_EX( glMultiTexCoord3fARB, glMultiTexCoord3f ) },
{ GL_CALL_EX( glMultiTexCoord4fARB, glMultiTexCoord4f ) },
{ GL_CALL( glActiveTextureARB ) },
{ GL_CALL_EX( glClientActiveTextureARB, glClientActiveTexture ) },
{ GL_CALL( glClientActiveTextureARB ) },
{ NULL, NULL }
};

static dllfunc_t compiledvertexarrayfuncs[] =
{
{ GL_CALL( glLockArraysEXT ) },
{ GL_CALL( glUnlockArraysEXT ) },
{ NULL, NULL }
};

static dllfunc_t texture3dextfuncs[] =
{
{ GL_CALL_EX( glTexImage3DEXT, glTexImage3D ) },
{ GL_CALL_EX( glTexSubImage3DEXT, glTexSubImage3D ) },
{ GL_CALL_EX( glCopyTexSubImage3DEXT, glCopyTexSubImage3D ) },
{ NULL, NULL }
};

static dllfunc_t atiseparatestencilfuncs[] =
{
{ GL_CALL_EX( glStencilOpSeparateATI, glStencilOpSeparate ) },
{ GL_CALL_EX( glStencilFuncSeparateATI, glStencilFuncSeparate ) },
{ NULL, NULL }
};

static dllfunc_t gl2separatestencilfuncs[] =
{
{ GL_CALL( glStencilOpSeparate ) },
{ GL_CALL( glStencilFuncSeparate ) },
{ NULL, NULL }
};

static dllfunc_t stenciltwosidefuncs[] =
{
{ GL_CALL( glActiveStencilFaceEXT ) },
{ NULL, NULL }
};

static dllfunc_t blendequationfuncs[] =
{
{ GL_CALL( glBlendEquationEXT ) },
{ NULL, NULL }
};

static dllfunc_t shaderobjectsfuncs[] =
{
{ GL_CALL( glDeleteObjectARB ) },
{ GL_CALL( glGetHandleARB ) },
{ GL_CALL( glDetachObjectARB ) },
{ GL_CALL( glCreateShaderObjectARB ) },
{ GL_CALL( glShaderSourceARB ) },
{ GL_CALL( glCompileShaderARB ) },
{ GL_CALL( glCreateProgramObjectARB ) },
{ GL_CALL( glAttachObjectARB ) },
{ GL_CALL( glLinkProgramARB ) },
{ GL_CALL( glUseProgramObjectARB ) },
{ GL_CALL( glValidateProgramARB ) },
{ GL_CALL( glUniform1fARB ) },
{ GL_CALL( glUniform2fARB ) },
{ GL_CALL( glUniform3fARB ) },
{ GL_CALL( glUniform4fARB ) },
{ GL_CALL( glUniform1iARB ) },
{ GL_CALL( glUniform2iARB ) },
{ GL_CALL( glUniform3iARB ) },
{ GL_CALL( glUniform4iARB ) },
{ GL_CALL( glUniform1fvARB ) },
{ GL_CALL( glUniform2fvARB ) },
{ GL_CALL( glUniform3fvARB ) },
{ GL_CALL( glUniform4fvARB ) },
{ GL_CALL( glUniform1ivARB ) },
{ GL_CALL( glUniform2ivARB ) },
{ GL_CALL( glUniform3ivARB ) },
{ GL_CALL( glUniform4ivARB ) },
{ GL_CALL( glUniformMatrix2fvARB ) },
{ GL_CALL( glUniformMatrix3fvARB ) },
{ GL_CALL( glUniformMatrix4fvARB ) },
{ GL_CALL( glGetObjectParameterfvARB ) },
{ GL_CALL( glGetObjectParameterivARB ) },
{ GL_CALL( glGetInfoLogARB ) },
{ GL_CALL( glGetAttachedObjectsARB ) },
{ GL_CALL( glGetUniformLocationARB ) },
{ GL_CALL( glGetActiveUniformARB ) },
{ GL_CALL( glGetUniformfvARB ) },
{ GL_CALL( glGetUniformivARB ) },
{ GL_CALL( glGetShaderSourceARB ) },
{ GL_CALL( glVertexAttribPointerARB ) },
{ GL_CALL( glEnableVertexAttribArrayARB ) },
{ GL_CALL( glDisableVertexAttribArrayARB ) },
{ GL_CALL( glBindAttribLocationARB ) },
{ GL_CALL( glGetActiveAttribARB ) },
{ GL_CALL( glGetAttribLocationARB ) },
{ NULL, NULL }
};

static dllfunc_t vertexshaderfuncs[] =
{
{ GL_CALL( glVertexAttribPointerARB ) },
{ GL_CALL( glEnableVertexAttribArrayARB ) },
{ GL_CALL( glDisableVertexAttribArrayARB ) },
{ GL_CALL( glBindAttribLocationARB ) },
{ GL_CALL( glGetActiveAttribARB ) },
{ GL_CALL( glGetAttribLocationARB ) },
{ NULL, NULL }
};

static dllfunc_t vbofuncs[] =
{
{ GL_CALL( glBindBufferARB ) },
{ GL_CALL( glDeleteBuffersARB ) },
{ GL_CALL( glGenBuffersARB ) },
{ GL_CALL( glIsBufferARB ) },
{ GL_CALL( glMapBufferARB ) },
{ GL_CALL( glUnmapBufferARB ) }, // ,
{ GL_CALL( glBufferDataARB ) },
{ GL_CALL( glBufferSubDataARB ) },
{ NULL, NULL}
};

static dllfunc_t occlusionfunc[] =
{
{ GL_CALL( glGenQueriesARB ) },
{ GL_CALL( glDeleteQueriesARB ) },
{ GL_CALL( glIsQueryARB ) },
{ GL_CALL( glBeginQueryARB ) },
{ GL_CALL( glEndQueryARB ) },
{ GL_CALL( glGetQueryivARB ) },
{ GL_CALL( glGetQueryObjectivARB ) }, //,
{ GL_CALL( glGetQueryObjectuivARB ) },
{ NULL, NULL }
};

static dllfunc_t texturecompressionfuncs[] =
{
{ GL_CALL( glCompressedTexImage3DARB ) },
{ GL_CALL( glCompressedTexImage2DARB ) },
{ GL_CALL( glCompressedTexImage1DARB ) },
{ GL_CALL( glCompressedTexSubImage3DARB ) },
{ GL_CALL( glCompressedTexSubImage2DARB ) },
{ GL_CALL( glCompressedTexSubImage1DARB ) },
{ GL_CALL_EX( glGetCompressedTexImageARB, glGetCompressedTexImage ) },
{ NULL, NULL }
};

void GL_CheckExtension( const char *name, const dllfunc_t *funcs, const char *cvarname, int r_ext )
{
	const dllfunc_t	*func;
	convar_t		*parm;

	MsgDev( D_NOTE, "GL_CheckExtension: %s ", name );

	if( cvarname )
	{
		// system config disable extensions
		parm = Cvar_Get( cvarname, "1", CVAR_GLCONFIG, va( "enable or disable %s", name ));

		if( parm->integer == 0 || ( gl_extensions->integer == 0 && r_ext != GL_OPENGL_110 ))
		{
			MsgDev( D_NOTE, "- disabled\n" );
			GL_SetExtension( r_ext, false );
			return; // nothing to process at
		}
		GL_SetExtension( r_ext, true );
	}

	if(( name[2] == '_' || name[3] == '_' ) && !Q_strstr( glConfig.extensions_string, name ))
	{
		GL_SetExtension( r_ext, false );	// update render info
		MsgDev( D_NOTE, "- ^1failed\n" );
		return;
	}


	GL_SetExtension( r_ext, true ); // predict extension state

	if( GL_Support( r_ext ))
		MsgDev( D_NOTE, "- ^2enabled\n" );
	else MsgDev( D_NOTE, "- ^1failed\n" );
}


void GL_InitExtensions( void )
{
	GL_SetExtension( GL_OPENGL_110, true );

	GL_CheckExtension( "glDrawRangeElements", drawrangeelementsfuncs, "gl_drawrangeelments", GL_DRAW_RANGEELEMENTS_EXT );

	if( !GL_Support( GL_DRAW_RANGEELEMENTS_EXT ))
		GL_CheckExtension( "GL_EXT_draw_range_elements", drawrangeelementsextfuncs, "gl_drawrangeelments", GL_DRAW_RANGEELEMENTS_EXT );

	// multitexture
	glConfig.max_texture_units = glConfig.max_texture_coords = glConfig.max_teximage_units = 1;
	GL_CheckExtension( "GL_ARB_multitexture", multitexturefuncs, "gl_arb_multitexture", GL_ARB_MULTITEXTURE );

	if( GL_Support( GL_ARB_MULTITEXTURE ))
	{
		pglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glConfig.max_texture_units );
		GL_CheckExtension( "GL_ARB_texture_env_combine", NULL, "gl_texture_env_combine", GL_ENV_COMBINE_EXT );

		if( !GL_Support( GL_ENV_COMBINE_EXT ))
			GL_CheckExtension( "GL_EXT_texture_env_combine", NULL, "gl_texture_env_combine", GL_ENV_COMBINE_EXT );

		if( GL_Support( GL_ENV_COMBINE_EXT ))
			GL_CheckExtension( "GL_ARB_texture_env_dot3", NULL, "gl_texture_env_dot3", GL_DOT3_ARB_EXT );
	}

	if( glConfig.max_texture_units == 1 )
		GL_SetExtension( GL_ARB_MULTITEXTURE, false );

	// 3d texture support
	GL_CheckExtension( "GL_EXT_texture3D", texture3dextfuncs, "gl_texture_3d", GL_TEXTURE_3D_EXT );

	if( GL_Support( GL_TEXTURE_3D_EXT ))
	{
		pglGetIntegerv( GL_MAX_3D_TEXTURE_SIZE, &glConfig.max_3d_texture_size );

		if( glConfig.max_3d_texture_size < 32 )
		{
			GL_SetExtension( GL_TEXTURE_3D_EXT, false );
			MsgDev( D_ERROR, "GL_EXT_texture3D reported bogus GL_MAX_3D_TEXTURE_SIZE, disabled\n" );
		}
	}

	GL_CheckExtension( "GL_SGIS_generate_mipmap", NULL, "gl_sgis_generate_mipmaps", GL_SGIS_MIPMAPS_EXT );

	// hardware cubemaps
	GL_CheckExtension( "GL_ARB_texture_cube_map", NULL, "gl_texture_cubemap", GL_TEXTURECUBEMAP_EXT );

	if( GL_Support( GL_TEXTURECUBEMAP_EXT ))
	{
		pglGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.max_cubemap_size );

		// check for seamless cubemaps too
		GL_CheckExtension( "GL_ARB_seamless_cube_map", NULL, "gl_seamless_cubemap", GL_ARB_SEAMLESS_CUBEMAP );
	}

	// point particles extension
	GL_CheckExtension( "GL_EXT_point_parameters", pointparametersfunc, NULL, GL_EXT_POINTPARAMETERS );

	GL_CheckExtension( "GL_ARB_texture_non_power_of_two", NULL, "gl_texture_npot", GL_ARB_TEXTURE_NPOT_EXT );
	GL_CheckExtension( "GL_ARB_texture_compression", texturecompressionfuncs, "gl_dds_hardware_support", GL_TEXTURE_COMPRESSION_EXT );
	GL_CheckExtension( "GL_EXT_compiled_vertex_array", compiledvertexarrayfuncs, "gl_cva_support", GL_CUSTOM_VERTEX_ARRAY_EXT );

	if( !GL_Support( GL_CUSTOM_VERTEX_ARRAY_EXT ))
		GL_CheckExtension( "GL_SGI_compiled_vertex_array", compiledvertexarrayfuncs, "gl_cva_support", GL_CUSTOM_VERTEX_ARRAY_EXT );

	GL_CheckExtension( "GL_EXT_texture_edge_clamp", NULL, "gl_clamp_to_edge", GL_CLAMPTOEDGE_EXT );

	if( !GL_Support( GL_CLAMPTOEDGE_EXT ))
		GL_CheckExtension("GL_SGIS_texture_edge_clamp", NULL, "gl_clamp_to_edge", GL_CLAMPTOEDGE_EXT );

	glConfig.max_texture_anisotropy = 0.0f;
	GL_CheckExtension( "GL_EXT_texture_filter_anisotropic", NULL, "gl_ext_anisotropic_filter", GL_ANISOTROPY_EXT );

	if( GL_Support( GL_ANISOTROPY_EXT ))
		pglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.max_texture_anisotropy );

	GL_CheckExtension( "GL_EXT_texture_lod_bias", NULL, "gl_ext_texture_lodbias", GL_TEXTURE_LODBIAS );
	if( GL_Support( GL_TEXTURE_LODBIAS ))
		pglGetFloatv( GL_MAX_TEXTURE_LOD_BIAS_EXT, &glConfig.max_texture_lodbias );

	GL_CheckExtension( "GL_ARB_texture_border_clamp", NULL, "gl_ext_texborder_clamp", GL_CLAMP_TEXBORDER_EXT );

	GL_CheckExtension( "GL_EXT_blend_minmax", blendequationfuncs, "gl_ext_customblend", GL_BLEND_MINMAX_EXT );
	GL_CheckExtension( "GL_EXT_blend_subtract", blendequationfuncs, "gl_ext_customblend", GL_BLEND_SUBTRACT_EXT );

	GL_CheckExtension( "glStencilOpSeparate", gl2separatestencilfuncs, "gl_separate_stencil", GL_SEPARATESTENCIL_EXT );

	if( !GL_Support( GL_SEPARATESTENCIL_EXT ))
		GL_CheckExtension("GL_ATI_separate_stencil", atiseparatestencilfuncs, "gl_separate_stencil", GL_SEPARATESTENCIL_EXT );

	GL_CheckExtension( "GL_EXT_stencil_two_side", stenciltwosidefuncs, "gl_stenciltwoside", GL_STENCILTWOSIDE_EXT );
	GL_CheckExtension( "GL_ARB_vertex_buffer_object", vbofuncs, "gl_vertex_buffer_object", GL_ARB_VERTEX_BUFFER_OBJECT_EXT );

	GL_CheckExtension( "GL_ARB_texture_env_add", NULL, "gl_texture_env_add", GL_TEXTURE_ENV_ADD_EXT );

	// vp and fp shaders
	GL_CheckExtension( "GL_ARB_shader_objects", shaderobjectsfuncs, "gl_shaderobjects", GL_SHADER_OBJECTS_EXT );
	GL_CheckExtension( "GL_ARB_shading_language_100", NULL, "gl_glslprogram", GL_SHADER_GLSL100_EXT );
	GL_CheckExtension( "GL_ARB_vertex_shader", vertexshaderfuncs, "gl_vertexshader", GL_VERTEX_SHADER_EXT );
	GL_CheckExtension( "GL_ARB_fragment_shader", NULL, "gl_pixelshader", GL_FRAGMENT_SHADER_EXT );

	GL_CheckExtension( "GL_ARB_depth_texture", NULL, "gl_depthtexture", GL_DEPTH_TEXTURE );
	GL_CheckExtension( "GL_ARB_shadow", NULL, "gl_arb_shadow", GL_SHADOW_EXT );

	GL_CheckExtension( "GL_ARB_texture_float", NULL, "gl_arb_texture_float", GL_ARB_TEXTURE_FLOAT_EXT );
	GL_CheckExtension( "GL_ARB_depth_buffer_float", NULL, "gl_arb_depth_float", GL_ARB_DEPTH_FLOAT_EXT );

	// occlusion queries
	GL_CheckExtension( "GL_ARB_occlusion_query", occlusionfunc, "gl_occlusion_queries", GL_OCCLUSION_QUERIES_EXT );

	if( GL_Support( GL_SHADER_GLSL100_EXT ))
	{
		pglGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, &glConfig.max_texture_coords );
		pglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &glConfig.max_teximage_units );
	}
	else
	{
		// just get from multitexturing
		glConfig.max_texture_coords = glConfig.max_teximage_units = glConfig.max_texture_units;
	}

	// rectangle textures support
	if( Q_strstr( glConfig.extensions_string, "GL_NV_texture_rectangle" ))
	{
		glConfig.texRectangle = GL_TEXTURE_RECTANGLE_NV;
		pglGetIntegerv( GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &glConfig.max_2d_rectangle_size );
	}
	else if( Q_strstr( glConfig.extensions_string, "GL_EXT_texture_rectangle" ))
	{
		glConfig.texRectangle = GL_TEXTURE_RECTANGLE_EXT;
		pglGetIntegerv( GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &glConfig.max_2d_rectangle_size );
	}
	else glConfig.texRectangle = glConfig.max_2d_rectangle_size = 0; // no rectangle

	Cvar_Set( "gl_anisotropy", va( "%f", bound( 0, gl_texture_anisotropy->value, glConfig.max_texture_anisotropy )));

	glConfig.max_2d_texture_size = 0;
	pglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.max_2d_texture_size );
	if( glConfig.max_2d_texture_size <= 0 ) glConfig.max_2d_texture_size = 256;

	Cvar_Get( "gl_max_texture_size", "0", CVAR_INIT, "opengl texture max dims" );
	Cvar_Set( "gl_max_texture_size", va( "%i", glConfig.max_2d_texture_size ));

	pglGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig.max_vertex_uniforms );
	pglGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig.max_vertex_attribs );


	Cvar_Set( "gl_anisotropy", va( "%f", bound( 0, gl_texture_anisotropy->value, glConfig.max_texture_anisotropy )));

	// software mipmap generator does wrong result with NPOT textures ...
	if( !GL_Support( GL_SGIS_MIPMAPS_EXT ))
		GL_SetExtension( GL_ARB_TEXTURE_NPOT_EXT, false );

	if( GL_Support( GL_TEXTURE_COMPRESSION_EXT ))
		Image_AddCmdFlags( IL_DDS_HARDWARE );

	glw_state.initialized = true;

    tr.framecount = tr.visframecount = 1;
}

void GL_UpdateSwapInterval( void )
{
	if( gl_swapInterval->modified )
	{
		gl_swapInterval->modified = false;
		MsgDev( D_INFO, "GL_UpdateSwapInterval: %d\n", gl_swapInterval->integer );
        eglSwapInterval(s_display, gl_swapInterval->integer);
	}
}

qboolean InitEGL( void )
{
	s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (!s_display) {
		MsgDev( D_ERROR,  "Could not connect to display! error: %d", eglGetError() );
		goto _fail0;
	}

	// Initialize the EGL display connection
	eglInitialize(s_display, NULL, NULL);

	if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
		MsgDev( D_ERROR,  "Could not set API! error: %d", eglGetError() );
		goto _fail1;
	}

	// Get an appropriate EGL framebuffer configuration
	EGLConfig config;
	EGLint numConfigs;
	static const EGLint attributeList[] = {
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE
	};
	eglChooseConfig(s_display, attributeList, &config, 1, &numConfigs);
	if (numConfigs == 0) {
		MsgDev( D_ERROR,  "No config found! error: %d", eglGetError() );
		goto _fail1;
	}

	// Create an EGL window surface
	s_surface = eglCreateWindowSurface(s_display, config, (char*)"", NULL);
	if (!s_surface) {
		MsgDev( D_ERROR,  "Surface creation failed! error: %d", eglGetError() );
		goto _fail1;
	}

	static const EGLint ctxAttributeList[] =  {
		EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
		EGL_NONE
	};

	// Create an EGL rendering context
	s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, ctxAttributeList);
	if (!s_context) {
		MsgDev( D_ERROR,  "Context creation failed! error: %d", eglGetError() );
		goto _fail2;
	}

	// Connect the context to the surface
	eglMakeCurrent(s_display, s_surface, s_surface, s_context);
	return true;

    _fail2:
        eglDestroySurface(s_display, s_surface);
        s_surface = NULL;
    _fail1:
        eglTerminate(s_display);
        s_display = NULL;
    _fail0:
        return false;
}

void DeinitEGL (void)
{
	if (s_display)
	{
		eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (s_context)
		{
			eglDestroyContext(s_display, s_context);
			s_context = NULL;
		}
		if (s_surface)
		{
			eglDestroySurface(s_display, s_surface);
			s_surface = NULL;
		}
		eglTerminate(s_display);
		s_display = NULL;
	}
}

void Switch_GetScreenRes( AppletOperationMode mode, int *width, int *height ) {
	switch (mode)
	{
		default:
		case AppletOperationMode_Handheld:
			*width = 1280;
			*height = 720;
			break;
		case AppletOperationMode_Docked:
			*width = 1920;
			*height = 1080;
			break;
	}
}

void Switch_UpdateMode( AppletOperationMode mode ) {
	int width, height;

	Switch_GetScreenRes(mode, &width, &height);

	Cvar_SetFloat("width", width);
	Cvar_SetFloat("height", height);

	glState.width = width;
	glState.height = height;
	glState.wideScreen = true;
	glState.fullScreen = true;

	SCR_VidInit();

	gfxConfigureResolution(width, height);
}

// Dynamic resolution!
void Switch_CheckResolution( void ) {
	AppletOperationMode current_mode = appletGetOperationMode();
	if (current_mode != operation_mode)
	{
		operation_mode = current_mode;
		Switch_UpdateMode(current_mode);
	}
}

void Switch_SwapBuffers ( void ) {
    eglSwapBuffers(s_display, s_surface);
}


qboolean R_Init_OpenGL( void )
{
	MsgDev(D_INFO, "Initializing OpenGL subsystem\n");

	setenv("MESA_NO_ERROR", "1", 1);

	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;
	int width, height;

	gfxInitResolution(1920, 1080);

	InitEGL();
	gladLoadGL();

	glConfig.color_bits = colorbits;
	glConfig.depth_bits = depthbits;
	glConfig.stencil_bits = stencilbits;

	operation_mode = appletGetOperationMode();
	Switch_UpdateMode(operation_mode);

    gl_swapInterval->modified = true;

    SCR_VidInit();

	eglSwapInterval(s_display, 1);

    MsgDev(D_INFO, "EGL / OpenGL is up and running!\n");
}

void R_Free_OpenGL( void )
{
	DeinitEGL();

	Q_memset( glConfig.extension, 0, sizeof( glConfig.extension[0] ) * GL_EXTCOUNT );
	glw_state.initialized = false;
}

void R_ChangeDisplaySettingsFast( int width, int height )
{
	Cvar_SetFloat("width", width);
	Cvar_SetFloat("height", height);
	MsgDev( D_NOTE, "R_ChangeDisplaySettingsFast(%d, %d)\n", width, height);

	glState.width = width;
	glState.height = height;
	host.window_center_x = width / 2;
	host.window_center_y = height / 2;

	glState.wideScreen = true; // V_AdjustFov will check for widescreen

	SCR_VidInit();
}

rserr_t R_ChangeDisplaySettings( int width, int height, qboolean fullscreen )
{
	return rserr_ok;
}

void VID_RestoreScreenResolution( void ) {}

qboolean VID_SetMode( void )
{
	return true;
}

#endif