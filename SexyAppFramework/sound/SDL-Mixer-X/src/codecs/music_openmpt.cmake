option(USE_OPENMPT         "Build with OpenMPT library" ON)
if(USE_OPENMPT)
    option(USE_OPENMPT_DYNAMIC "Use dynamical loading of OpenMPT" OFF)
    option(USE_OPENMPT_STATIC "Use linking with a static OpenMPT" ON)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(OpenMPT)
        message("OpenMPT: [${OpenMPT_FOUND}] ${OpenMPT_INCLUDE_DIRS} ${OpenMPT_LIBRARIES}")
        if(USE_OPENMPT_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DOPENMPT_DYNAMIC=\"${OpenMPT_DYNAMIC_LIBRARY}\")
            message("Dynamic OpenMPT: ${OpenMPT_DYNAMIC_LIBRARY}")
        endif()

    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(OpenMPT_LIBRARIES openmpt${MIX_DEBUG_SUFFIX})
        else()
            find_library(OpenMPT_LIBRARIES NAMES openmpt
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        set(OpenMPT_FOUND 1)
        set(STDCPP_NEEDED 1) # Statically linking openmpt which is C++ library
        set(OpenMPT_INCLUDE_DIRS "${AUDIO_CODECS_PATH}/libopenmpt/include")
    endif()

    if(OpenMPT_FOUND)
        message("== using libopenmpt ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MOD_OPENMPT -DOPENMPT_STATIC)
        if(USE_OPENMPT_STATIC)
            list(APPEND SDL_MIXER_DEFINITIONS)
        endif()
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_OPENMPT_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${OpenMPT_LIBRARIES})
        endif()
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${OpenMPT_INCLUDE_DIRS})
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_openmpt.c
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_openmpt.h
        )
        appendTrackerFormats("667;669;AMF;AMS;C67;CBA;DBM;DIGI;DMF;DSM;DSYM;DTM;ETX;
		                      FAR;FMT;FTM;GDM;GMC;GT2;ICE;IMF;IMS;IT;ITP;J2B;KRIS;MDL;
							  MED;MID;MO3;MOD;MT2;MTM;OKT;PLM;PSM;PT36;PTM;PUMA;RTM;
							  S3M;SFX;STK;STM;STP;SYMMOD;UAX;ULT;WAV;XM;XMF;UMX;XPK;PP20")
    else()
        message("-- skipping libopenmpt --")
    endif()
endif()
