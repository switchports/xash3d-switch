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


void GL_CheckExtension( const char *name, const dllfunc_t *funcs, const char *cvarname, int r_ext )
{
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

	// multitexture
	glConfig.max_texture_units = glConfig.max_texture_coords = glConfig.max_teximage_units = 1;

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

	GL_CheckExtension( "GL_ARB_texture_non_power_of_two", NULL, "gl_texture_npot", GL_ARB_TEXTURE_NPOT_EXT );

	GL_CheckExtension( "GL_EXT_texture_edge_clamp", NULL, "gl_clamp_to_edge", GL_CLAMPTOEDGE_EXT );

	if( !GL_Support( GL_CLAMPTOEDGE_EXT ))
		GL_CheckExtension("GL_SGIS_texture_edge_clamp", NULL, "gl_clamp_to_edge", GL_CLAMPTOEDGE_EXT );

	glConfig.max_texture_anisotropy = 0.0f;
	GL_CheckExtension( "GL_EXT_texture_filter_anisotropic", NULL, "gl_ext_anisotropic_filter", GL_ANISOTROPY_EXT );

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

qboolean InitEGL( NWindow* win )
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
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE,     8,
        EGL_GREEN_SIZE,   8,
        EGL_BLUE_SIZE,    8,
        EGL_ALPHA_SIZE,   8,
        EGL_DEPTH_SIZE,   24,
        EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	eglChooseConfig(s_display, attributeList, &config, 1, &numConfigs);
	if (numConfigs == 0) {
		MsgDev( D_ERROR,  "No config found! error: %d", eglGetError() );
		goto _fail1;
	}

	// Create an EGL window surface
	s_surface = eglCreateWindowSurface(s_display, config, win, NULL);
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

	nwindowSetCrop(nwindowGetDefault(), 0, 0, width, height);
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

    NWindow* win = nwindowGetDefault();
	nwindowSetDimensions(win, 1920, 1080);

	InitEGL(win);
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

	return true;
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