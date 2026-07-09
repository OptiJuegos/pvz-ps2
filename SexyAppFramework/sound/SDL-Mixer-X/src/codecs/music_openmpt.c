/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifdef MUSIC_MOD_OPENMPT

#include "SDL_loadso.h"

#include "music_openmpt.h"

#ifdef OPENMPT_HEADER
#include OPENMPT_HEADER
#else
#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_ext.h>
#endif

typedef struct
{
    int loaded;
    void *handle;

    openmpt_module_ext* (*openmpt_module_ext_create_from_memory)(const void *filedata, size_t filesize, openmpt_log_func logfunc, void *loguser, openmpt_error_func errfunc, void *erruser, int *error, const char **error_message, const openmpt_module_initial_ctl *ctls);
    void (*openmpt_module_ext_destroy)(openmpt_module_ext *mod_ext);
	openmpt_module* (*openmpt_module_ext_get_module)(openmpt_module_ext *mod_ext);
	int (*openmpt_module_ext_get_interface)(openmpt_module_ext *mod_ext, const char *interface_id, void *interface, size_t interface_size);
	int (*openmpt_module_set_repeat_count)(openmpt_module *mod, int32_t repeat_count);
	int (*openmpt_module_set_render_param)(openmpt_module *mod, int param, int32_t value);
	size_t (*openmpt_module_read_mono)(openmpt_module *mod, int32_t samplerate, size_t count, int16_t *mono);
	size_t (*openmpt_module_read_stereo)(openmpt_module *mod, int32_t samplerate, size_t count, int16_t *left, int16_t *right);
	double (*openmpt_module_set_position_order_row)(openmpt_module *mod, int32_t order, int32_t row);
	int32_t (*openmpt_module_get_current_order)(openmpt_module *mod);
	double (*openmpt_module_set_position_seconds)(openmpt_module *mod, double seconds);
	double (*openmpt_module_get_position_seconds)(openmpt_module *mod);
	double (*openmpt_module_get_duration_seconds)(openmpt_module *mod);
	const char* (*openmpt_module_get_metadata)(openmpt_module *mod, const char *key);
} openmpt_loader;

static openmpt_loader openmpt;

static struct OpenMpt_Settings
{
	int mChannels;
	int mBits;
	int mFrequency;
} settings;

#ifdef OPENMPT_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    openmpt.FUNC = (SIG) SDL_LoadFunction(openmpt.handle, #FUNC); \
    if (openmpt.FUNC == NULL) { SDL_UnloadObject(openmpt.handle); return -1; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    openmpt.FUNC = FUNC; \
    if (openmpt.FUNC == NULL) { Mix_SetError("Missing libopenmpt.framework"); return -1; }
#endif

static int OPENMPT_Load(void)
#ifdef __APPLE__
	/* Need to turn off optimizations so weak framework load check works */
	__attribute__ ((optnone))
#endif
{
	if (openmpt.loaded == 0)
	{
#ifdef OPENMPT_DYNAMIC
		openmpt.handle = SDL_LoadObject(OPENMPT_DYNAMIC);
		if (openmpt.handle == NULL)
			return -1;
#endif

		FUNCTION_LOADER(openmpt_module_ext_create_from_memory, openmpt_module_ext* (*)(const void *filedata, size_t filesize, openmpt_log_func logfunc, void *loguser, openmpt_error_func errfunc, void *erruser, int *error, const char **error_message, const openmpt_module_initial_ctl *ctls))
		FUNCTION_LOADER(openmpt_module_ext_destroy, void (*)(openmpt_module_ext *mod_ext))
		FUNCTION_LOADER(openmpt_module_ext_get_module, openmpt_module* (*)(openmpt_module_ext *mod_ext))
		FUNCTION_LOADER(openmpt_module_ext_get_interface, int (*)(openmpt_module_ext *mod_ext, const char *interface_id, void *interface, size_t interface_size))
		FUNCTION_LOADER(openmpt_module_set_repeat_count, int (*)(openmpt_module *mod, int32_t repeat_count))
		FUNCTION_LOADER(openmpt_module_set_render_param, int (*)(openmpt_module *mod, int param, int32_t value))
		FUNCTION_LOADER(openmpt_module_read_mono, size_t (*)(openmpt_module *mod, int32_t samplerate, size_t count, int16_t *mono))
		FUNCTION_LOADER(openmpt_module_read_stereo, size_t (*)(openmpt_module *mod, int32_t samplerate, size_t count, int16_t *left, int16_t *right))
		FUNCTION_LOADER(openmpt_module_set_position_order_row, double (*)(openmpt_module *mod, int32_t order, int32_t row))
		FUNCTION_LOADER(openmpt_module_get_current_order, int32_t (*)(openmpt_module *mod))
		FUNCTION_LOADER(openmpt_module_set_position_seconds, double (*)(openmpt_module *mod, double seconds))
		FUNCTION_LOADER(openmpt_module_get_position_seconds, double (*)(openmpt_module *mod))
		FUNCTION_LOADER(openmpt_module_get_duration_seconds, double (*)(openmpt_module *mod))
		FUNCTION_LOADER(openmpt_module_get_metadata, const char* (*)(openmpt_module *mod, const char *key))
	}
	++openmpt.loaded;
	return 0;
}

static void OPENMPT_Unload(void)
{
	if (openmpt.loaded == 0)
		return;

	if (openmpt.loaded == 1)
	{
#ifdef OPENMPT_DYNAMIC
		SDL_UnloadObject(openmpt.handle);
#endif
	}
	--openmpt.loaded;
}

typedef struct
{
	int volume;
	int play_count;
	openmpt_module *file;
	openmpt_module_ext *ext;
	openmpt_module_ext_interface_interactive *ext_interactive;
	SDL_AudioStream *stream;
	int16_t* buffer;
	int16_t* internal_buf;
	int buffer_size;
	Mix_MusicMetaTags tags;
} OPENMPT_Music;

static int OPENMPT_Seek(void *context, double position);
static void OPENMPT_Delete(void *context);

static int OPENMPT_Open(const SDL_AudioSpec *spec)
{
	if (spec->channels == 1) {
		settings.mChannels = 1;
	} else {
		settings.mChannels = 2;
	}

	if (SDL_AUDIO_BITSIZE(spec->format) == 8) {
		settings.mBits = 8;
	} else {
		settings.mBits = 16;
	}

	settings.mFrequency = spec->freq;
	return 0;
}

/* Load an openmpt stream from an SDL_RWops object */
void *OPENMPT_CreateFromRW(SDL_RWops *src, int freesrc)
{
    OPENMPT_Music *music;
    void *buffer;
    size_t size;

    music = (OPENMPT_Music *)SDL_calloc(1, sizeof(OPENMPT_Music));
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    music->volume = MIX_MAX_VOLUME;

    music->stream = SDL_NewAudioStream((settings.mBits == 8) ? AUDIO_U8 : AUDIO_S16SYS, (Uint8)settings.mChannels, settings.mFrequency,
                                       music_spec.format, music_spec.channels, music_spec.freq);
    if (!music->stream)
	{
        OPENMPT_Delete(music);
        return NULL;
    }

    music->buffer_size = music_spec.samples * (settings.mBits / 8);
    music->buffer = SDL_malloc(sizeof(int16_t) * (size_t)music->buffer_size * settings.mChannels);
    if (!music->buffer)
	{
        OPENMPT_Delete(music);
        return NULL;
    }
	music->internal_buf = SDL_malloc(sizeof(int16_t) * 1024 * 2);
	if (!music->buffer)
	{
		SDL_free(music->buffer);
        OPENMPT_Delete(music);
        return NULL;
    }
	music->ext_interactive = SDL_malloc(sizeof(openmpt_module_ext_interface_interactive));
	if (!music->ext_interactive)
	{
		SDL_free(music->buffer);
		SDL_free(music->internal_buf);
        OPENMPT_Delete(music);
        return NULL;
    }

    buffer = SDL_LoadFile_RW(src, &size, SDL_FALSE);
    if (buffer) {
        music->ext = openmpt.openmpt_module_ext_create_from_memory(buffer, size, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        if (!music->ext) {
            Mix_SetError("openmpt_module_ext_create_from_memory() failed");
        }
        SDL_free(buffer);
    }

    if (!music->ext) {
        OPENMPT_Delete(music);
        return NULL;
    }
	music->file = openmpt.openmpt_module_ext_get_module(music->ext);
	openmpt.openmpt_module_ext_get_interface(music->ext, "interactive", music->ext_interactive, sizeof(openmpt_module_ext_interface_interactive));

    meta_tags_init(&music->tags);
    meta_tags_set(&music->tags, MIX_META_TITLE, openmpt.openmpt_module_get_metadata(music->file, "title"));
	openmpt.openmpt_module_set_repeat_count(music->file, -1);

    if (freesrc) {
        SDL_RWclose(src);
    }
    return music;
}

/* Set the volume for an openmpt stream */
static void OPENMPT_SetVolume(void *context, int volume)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
    music->volume = volume;
	unsigned int mptVol = (unsigned int)volume * 2;
	openmpt.openmpt_module_set_render_param(music->file, OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL, (int32_t)(2000.0*log10(mptVol/128.0)));
}

/* Get the volume for an openmpt stream */
static int OPENMPT_GetVolume(void *context)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
    return music->volume;
}

/* Start playback of a given openmpt stream */
static int OPENMPT_Play(void *context, int play_count)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
    music->play_count = play_count;
    return OPENMPT_Seek(music, 0.0);
}

static void OPENMPT_Stop(void *context)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
    SDL_AudioStreamClear(music->stream);
}

/* Play some of a stream previously started with openmpt_play() */
static int OPENMPT_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
    int filled, amount;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

	int framesize = settings.mBits/8*settings.mChannels;
	int framecount = music->buffer_size/framesize;
	int rendered, totalrendered = 0;

	uint8_t* buf8 = (uint8_t*)music->buffer;
	int16_t* buf16 = music->buffer;

	int frame, channel;

	while(framecount>0)
	{
		int frames = framecount;
		if(frames>1024){
			frames = 1024;
		}

		if (settings.mChannels == 1)
			rendered = (int)openmpt.openmpt_module_read_mono(music->file, settings.mFrequency, frames, music->internal_buf);
		else if (settings.mChannels == 2)
			rendered = (int)openmpt.openmpt_module_read_stereo(music->file, settings.mFrequency, frames, &music->internal_buf[frames*0], &music->internal_buf[frames*1]);

		int16_t* in = music->internal_buf;
		if (settings.mBits == 8)
		{
			for(frame=0; frame < frames; frame++)
			{
				for(channel=0; channel < settings.mChannels; channel++)
				{
					*buf8 = in[frames*channel+frame]/256+0x80;
					buf8++;
				}
			}
		}
		else if (settings.mBits == 16)
		{
			for(frame=0; frame < frames; frame++)
			{
				for(channel=0; channel < settings.mChannels; channel++)
				{
					*buf16 = in[frames*channel+frame];
					buf16++;
				}
			}
		}

		totalrendered += rendered;
		framecount -= frames;
		if (!rendered) break;
	}
	memset(((char*)music->buffer)+totalrendered*framesize, 0, music->buffer_size - totalrendered*framesize);
	amount = totalrendered*framesize;

    if (amount > 0) {
        if (SDL_AudioStreamPut(music->stream, music->buffer, amount) < 0) {
            return -1;
        }
    } else {
        if (music->play_count == 1) {
            music->play_count = 0;
            SDL_AudioStreamFlush(music->stream);
        } else {
            int play_count = -1;
            if (music->play_count > 0) {
                play_count = (music->play_count - 1);
            }
            if (OPENMPT_Play(music, play_count) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

static int OPENMPT_GetAudio(void *context, void *data, int bytes)
{
    return music_pcm_getaudio(context, data, bytes, MIX_MAX_VOLUME, OPENMPT_GetSome);
}

/* Jump to a given order */
static int OPENMPT_Jump(void *context, int order)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
	openmpt.openmpt_module_set_position_order_row(music->file, order, 0);
    return 0;
}

/* Get current order */
static int OPENMPT_GetOrder(void *context, int *outOrder)
{
	OPENMPT_Music *music = (OPENMPT_Music *)context;
	*outOrder = openmpt.openmpt_module_get_current_order(music->file);
	return 0;
}

/* Toggle muted channel status */
static int OPENMPT_MuteChannel(void *context, int channel, int mute)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
	music->ext_interactive->set_channel_mute_status(music->ext, channel, mute);
    return 0;
}

/* Set a channel's volume */
static int OPENMPT_SetChannelVolume(void *context, int channel, int volume)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
	music->ext_interactive->set_channel_volume(music->ext, channel, volume/128.);
    return 0;
}

/* Jump (seek) to a given position */
static int OPENMPT_Seek(void *context, double position)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
	openmpt.openmpt_module_set_position_seconds(music->file, position);
    return 0;
}

static double OPENMPT_Tell(void *context)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
	return openmpt.openmpt_module_get_position_seconds(music->file);
}

/* Return music duration in seconds */
static double OPENMPT_Duration(void *context)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
	return openmpt.openmpt_module_get_duration_seconds(music->file);
}

static const char* OPENMPT_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Close the given openmpt stream */
static void OPENMPT_Delete(void *context)
{
    OPENMPT_Music *music = (OPENMPT_Music *)context;
    meta_tags_clear(&music->tags);
    if (music->ext) {
        openmpt.openmpt_module_ext_destroy(music->ext);
    }
    if (music->stream) {
        SDL_FreeAudioStream(music->stream);
    }
    if (music->buffer) {
        SDL_free(music->buffer);
    }
	if (music->internal_buf) {
        SDL_free(music->internal_buf);
    }
	if (music->ext_interactive) {
        SDL_free(music->ext_interactive);
    }
    SDL_free(music);
}

Mix_MusicInterface Mix_MusicInterface_OPENMPT =
{
    "OPENMPT",
    MIX_MUSIC_OPENMPT,
    MUS_MOD,
    SDL_FALSE,
    SDL_FALSE,

    OPENMPT_Load,
    OPENMPT_Open,
    OPENMPT_CreateFromRW,
    NULL,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    OPENMPT_SetVolume,
    OPENMPT_GetVolume,
    OPENMPT_Play,
    NULL,   /* IsPlaying */
    OPENMPT_GetAudio,
    OPENMPT_Jump,
    OPENMPT_GetOrder,
    OPENMPT_MuteChannel,
    OPENMPT_SetChannelVolume,
    OPENMPT_Seek,
    OPENMPT_Tell,
    OPENMPT_Duration,
    NULL,   /* SetTempo [MIXER-X] */
    NULL,   /* GetTempo [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    NULL,   /* GetTracksCount [MIXER-X] */
    NULL,   /* SetTrackMute [MIXER-X] */
    NULL,   /* LoopStart */
    NULL,   /* LoopEnd */
    NULL,   /* LoopLength */
    OPENMPT_GetMetaTag,
    NULL,   /* GetNumTracks */
    NULL,   /* StartTrack */
    NULL,   /* Pause */
    NULL,   /* Resume */
    OPENMPT_Stop,
    OPENMPT_Delete,
    NULL,   /* Close */
    OPENMPT_Unload
};

#endif /* MUSIC_MOD_OPENMPT */

/* vi: set ts=4 sw=4 expandtab: */
