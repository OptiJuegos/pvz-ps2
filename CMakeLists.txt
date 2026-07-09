cmake_minimum_required(VERSION 3.5)

project(re-plants-vs-zombies LANGUAGES C CXX ASM)

file(GLOB SOURCES
	*.cpp
	SexyAppFramework/*.cpp
	SexyAppFramework/graphics/*.cpp
	SexyAppFramework/imagelib/*.cpp
	SexyAppFramework/misc/*.cpp
	SexyAppFramework/paklib/*.cpp
	SexyAppFramework/sound/*.cpp
	SexyAppFramework/widget/*.cpp
	#SexyAppFramework/ogg/*.c
	SexyAppFramework/fcaseopen/*.c
	Sexy.TodLib/*.cpp
	Lawn/*.cpp
	Lawn/Widget/*.cpp
	Lawn/System/*.cpp
	SexyAppFramework/LawnProject.rc
)

# these files are included and must not be built in the project
list(REMOVE_ITEM SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/Sexy.TodLib/TodDrawTriangle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Sexy.TodLib/TodDrawTriangleInc.cpp
)

set(NO_GLEW OFF)
set(LOW_MEMORY OFF)
set(DOWNSCALE_COUNT 1)
if(PS2)
	set(NO_GLEW ON)
	set(LOW_MEMORY ON)
	# Must stay 1: IMG_DOWNSCALE>1 halves *logical* image dims, but the game
	# has no global coordinate scaling — reanim transforms and hardcoded
	# 800x600 positions stay full-res, so everything renders scattered at
	# half size. VRAM/RAM savings come from the texture-level downscale in
	# gles_ps2.cpp instead (PS2_TEX_SHRINK_SHIFT), which is invisible to
	# game-side layout.
	set(DOWNSCALE_COUNT 1)
	list(REMOVE_ITEM SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/sound/SDLSoundManager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/sound/SDLSoundInstance.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/sound/SDLMusicInterface.cpp
	)
	include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ps2_embed_irx.cmake)
	set(PS2_IRX_BLOBS ${CMAKE_CURRENT_BINARY_DIR}/ps2_irx_blobs.c)
	ps2_embed_irx(${PS2_IRX_BLOBS}
		$ENV{PS2SDK}/iop/irx/usbd.irx
		$ENV{PS2SDK}/iop/irx/usbhdfsd.irx
		$ENV{PS2SDK}/iop/irx/audsrv.irx
		# Modern BDM USB stack (real hardware): iomanX+fileXio route newlib IO
		# to iomanX devices; bdm+bdmfs_fatfs+usbmass_bd replace the legacy
		# usbhdfsd, which wedges under sustained load on real consoles.
		$ENV{PS2SDK}/iop/irx/iomanX.irx
		$ENV{PS2SDK}/iop/irx/fileXio.irx
		$ENV{PS2SDK}/iop/irx/bdm.irx
		$ENV{PS2SDK}/iop/irx/bdmfs_fatfs.irx
		$ENV{PS2SDK}/iop/irx/usbmass_bd.irx
		# CD/DVD (or mounted ISO) boot: cdfs exposes the disc as "cdfs:/..."
		# (case-insensitive, forward slashes) so assets read cleanly, unlike the
		# raw cdrom0: paths that mangle subdirs and need uppercase + ";1".
		$ENV{PS2SDK}/iop/irx/cdfs.irx
	)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/main_pvz_ps2.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/gles_ps2.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2TextureConversion.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2VramAllocator.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2PadState.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2IoLock.c
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2Trace.c
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2AssetIndex.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Input.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/Ps2PvzServices.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2/sound/Ps2SoundManager.cpp
		${PS2_IRX_BLOBS}
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/ps2
	)
elseif(NINTENDO_SWITCH)
	set(NO_GLEW ON)
	set(VORBIS_LIBS vorbisfile vorbis)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/switch
	)
elseif(NINTENDO_3DS)
	set(NO_GLEW ON)
	set(LOW_MEMORY ON)
	set(DOWNSCALE_COUNT 2)
	ctr_add_shader_library(colored ${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/colored.v.pica)
	ctr_add_shader_library(textured ${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/textured.v.pica)
	dkp_add_embedded_binary_library(shaders colored textured)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/3ds
	)
else()
	set(VORBIS_LIBS vorbisfile vorbis)
	list(APPEND SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/graphics/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/graphics/GLInterface.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc/Input.cpp
	)
	list(APPEND PLAT_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/platform/pc
	)
endif()

option(CONSOLE "Show console on Windows" ON)

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
if (NOT PS2)
	find_package(SDL2 REQUIRED)
	if (NOT NO_GLEW)
		find_package(GLEW REQUIRED)
	endif()
endif()

if (NOT PS2)
	set(SDL_MIXER_X_STATIC ON CACHE BOOL "Build static library of SDL Mixer X" FORCE)
	set(SDL_MIXER_X_SHARED OFF CACHE BOOL "Build shared library of SDL Mixer X" FORCE)
	set(USE_MIDI OFF CACHE BOOL "SDL Mixer X midi" FORCE)
	set(USE_MODPLUG OFF CACHE BOOL "SDL Mixer X modplug" FORCE)
	set(USE_XMP OFF CACHE BOOL "SDL Mixer X xmp" FORCE)
	set(USE_OPUS OFF CACHE BOOL "SDL Mixer X opus" FORCE)
	set(USE_DRFLAC OFF CACHE BOOL "SDL Mixer X flac" FORCE)
	set(USE_PXTONE OFF CACHE BOOL "SDL Mixer X pxtone" FORCE)
	add_subdirectory(SexyAppFramework/sound/SDL-Mixer-X)
endif()

add_executable(re-plants-vs-zombies ${SOURCES})
target_include_directories(re-plants-vs-zombies PRIVATE
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/SexyAppFramework
	${PROJECT_SOURCE_DIR}/SexyAppFramework/sound/SDL-Mixer-X/include
	${PLAT_INCLUDES}
)
if (NOT PS2)
	target_include_directories(re-plants-vs-zombies PRIVATE ${SDL2_INCLUDE_DIRS})
endif()
target_compile_definitions(re-plants-vs-zombies PRIVATE _DEBUG)
if (PS2)
	target_compile_definitions(re-plants-vs-zombies PRIVATE PS2_PLATFORM)
	target_compile_options(re-plants-vs-zombies PRIVATE -include SDL.h)

	# Every printf on PS2 is a SIF fio RPC to the IOP TTY. On real hardware
	# that traffic can wedge the console, so printf/puts/putchar are wrapped
	# (TodDebug.cpp) and appended to <save prefix>/userdata/log.txt instead,
	# like the original game's log.
	target_link_options(re-plants-vs-zombies PRIVATE
		-Wl,--wrap=printf -Wl,--wrap=puts -Wl,--wrap=putchar)
endif()

if (LOW_MEMORY)
	target_compile_definitions(re-plants-vs-zombies PRIVATE LOW_MEMORY)
endif()
target_compile_definitions(re-plants-vs-zombies PRIVATE IMG_DOWNSCALE=${DOWNSCALE_COUNT})

if (PS2)
	target_link_libraries(re-plants-vs-zombies
		jpeg png z
		gskit gskit_toolkit dmakit
		pad graph dma vux kernel
		audsrv patches mc
		fileXio cdvd
		m
	)
else()
	target_link_libraries(re-plants-vs-zombies jpeg png z SDL2_mixer_ext_Static openmpt mpg123 ${VORBIS_LIBS} ogg ${SDL2_LIBRARIES} ${GLEW_LIBRARIES})
endif()

if (WIN32)
	target_link_options(re-plants-vs-zombies PRIVATE -static -static-libgcc -static-libstdc++)
	target_link_libraries(re-plants-vs-zombies opengl32 ws2_32 ddraw dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 shlwapi)
	target_compile_definitions(re-plants-vs-zombies PRIVATE WINDOWS)

	if(NOT CONSOLE)
		target_link_options(re-plants-vs-zombies PRIVATE -mwindows)
	endif()
elseif (PS2)
	set_target_properties(re-plants-vs-zombies PROPERTIES OUTPUT_NAME "pvz_ps2" SUFFIX ".elf")

	# The whole ELF stays resident in the 32 MB of EE RAM, so binary size is
	# gameplay memory: optimize for size, drop unreferenced code/data at link
	# time, and strip symbols from the final ELF.
	target_compile_options(re-plants-vs-zombies PRIVATE
		-O2
		-ffunction-sections
		-fdata-sections
	)
	target_link_options(re-plants-vs-zombies PRIVATE -Wl,--gc-sections)
	find_program(PS2_STRIP mips64r5900el-ps2-elf-strip)
	if (PS2_STRIP)
		add_custom_command(TARGET re-plants-vs-zombies POST_BUILD
			COMMAND ${PS2_STRIP} --strip-all $<TARGET_FILE:re-plants-vs-zombies>
			COMMENT "Stripping pvz_ps2.elf"
		)
	endif()
elseif (NINTENDO_SWITCH)
	target_link_libraries(re-plants-vs-zombies glad EGL glapi drm_nouveau)

	nx_generate_nacp(re-plants-vs-zombies.nacp
		NAME "Plants vs. Zombies"
		AUTHOR "PopCap Games, Unofficial Switch port by Headshotnoby"
		VERSION "0.1"
	)

	nx_create_nro(re-plants-vs-zombies
		NACP re-plants-vs-zombies.nacp
		ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon-switch.jpg
	)
elseif (NINTENDO_3DS)
	target_link_libraries(re-plants-vs-zombies citro3d shaders)

	ctr_generate_smdh(re-plants-vs-zombies.smdh
		NAME "Plants vs. Zombies"
		DESCRIPTION "GOTY Edition"
		AUTHOR "PopCap Games, Unofficial 3DS port by Headshotnoby"
		VERSION "0.1"
		ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon-3ds.png"
	)

	ctr_create_3dsx(re-plants-vs-zombies
		SMDH re-plants-vs-zombies.smdh
	)
else()
	target_link_libraries(re-plants-vs-zombies GL)
endif()
