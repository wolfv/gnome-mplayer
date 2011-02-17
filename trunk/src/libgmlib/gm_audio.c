/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gm_audio.c
 * Copyright (C) Kevin DeKorte 2006 <kdekorte@gmail.com>
 * 
 * gm_audio.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gm_audio.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with playlist.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include "gm_audio.h"

#ifdef HAVE_ASOUNDLIB
#include <asoundlib.h>
#endif
#ifdef HAVE_PULSEAUDIO
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#endif

GList *audio_devices = NULL;

// private prototypes

#ifdef HAVE_PULSEAUDIO
void gm_audio_context_state_callback(pa_context * context, gpointer data);
#endif


gboolean gm_audio_query_devices()
{

    gint card, err, dev;
    gchar *name = NULL;
    gchar *desc;
    gchar *mplayer_ao;
    AudioDevice *device;

#ifdef HAVE_ASOUNDLIB
    snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
    snd_ctl_t *handle;
    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcminfo;
#endif

    if (audio_devices != NULL) {
        gm_audio_free();
    }

	device = g_new0(AudioDevice, 1);
	device->description = g_strdup(_("Default"));
	device->type = AUDIO_TYPE_SOFTVOL;
	device->mplayer_ao = g_strdup("");
	audio_devices = g_list_append(audio_devices, device);	

	device = g_new0(AudioDevice, 1);
	device->description = g_strdup("ARTS");
	device->type = AUDIO_TYPE_SOFTVOL;
	device->mplayer_ao = g_strdup("arts");
	audio_devices = g_list_append(audio_devices, device);	

	device = g_new0(AudioDevice, 1);
	device->description = g_strdup("ESD");
	device->type = AUDIO_TYPE_SOFTVOL;
	device->mplayer_ao = g_strdup("esd");
	audio_devices = g_list_append(audio_devices, device);	

	device = g_new0(AudioDevice, 1);
	device->description = g_strdup("JACK");
	device->type = AUDIO_TYPE_SOFTVOL;
	device->mplayer_ao = g_strdup("jack");
	audio_devices = g_list_append(audio_devices, device);	

	device = g_new0(AudioDevice, 1);
	device->description = g_strdup("OSS");
	device->type = AUDIO_TYPE_SOFTVOL;
	device->mplayer_ao = g_strdup("oss");
	audio_devices = g_list_append(audio_devices, device);	
	
#ifdef HAVE_ASOUNDLIB
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);

    card = -1;

    while (snd_card_next(&card) >= 0) {
        //printf("card = %i\n", card);
        if (card < 0)
            break;
        if (name != NULL) {
            free(name);
            name = NULL;
        }
        name = malloc(32);
        sprintf(name, "hw:%i", card);
        //printf("name = %s\n",name);

        if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
            continue;
        }

        if ((err = snd_ctl_card_info(handle, info)) < 0) {
            snd_ctl_close(handle);
            continue;
        }

        dev = -1;
        while (1) {
            snd_ctl_pcm_next_device(handle, &dev);
            if (dev < 0)
                break;
            snd_pcm_info_set_device(pcminfo, dev);
            snd_pcm_info_set_subdevice(pcminfo, 0);
            snd_pcm_info_set_stream(pcminfo, stream);
            if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) {
                continue;
            }

            desc = g_strdup_printf("%s (%s) (alsa)", snd_ctl_card_info_get_name(info), snd_pcm_info_get_name(pcminfo));
            mplayer_ao = g_strdup_printf("alsa:device=hw=%i.%i", card, dev);

            device = g_new0(AudioDevice, 1);
            device->description = g_strdup(desc);
            device->type = AUDIO_TYPE_ALSA;
            device->alsa_card = card;
            device->alsa_device = dev;
            device->mplayer_ao = g_strdup(mplayer_ao);
            audio_devices = g_list_append(audio_devices, device);

            g_free(desc);
            g_free(mplayer_ao);

        }

        snd_ctl_close(handle);
        free(name);
        name = NULL;

    }

#else

	device = g_new0(AudioDevice, 1);
	device->description = g_strdup("ALSA");
	device->type = AUDIO_TYPE_SOFTVOL;
	device->mplayer_ao = g_strdup("alsa");
	audio_devices = g_list_append(audio_devices, device);	

#endif

#ifdef HAVE_PULSEAUDIO

    pa_glib_mainloop *loop = pa_glib_mainloop_new(g_main_context_default());
    pa_context *context = pa_context_new(pa_glib_mainloop_get_api(loop), "gmtk context");
    if (context) {
        pa_context_connect(context, NULL, 0, NULL);
        pa_context_set_state_callback(context, gm_audio_context_state_callback, audio_devices);
    }
    // make sure the pulse events are done before we exit this function
    while (gtk_events_pending())
        gtk_main_iteration();

#else
	
	device = g_new0(AudioDevice, 1);
	device->description = g_strdup("PulseAudio");
	device->type = AUDIO_TYPE_SOFTVOL;
	device->mplayer_ao = g_strdup("pulse");
	audio_devices = g_list_append(audio_devices, device);	

#endif

    return TRUE;
}

gboolean gm_audio_update_device(AudioDevice * device)
{
    gboolean ret = FALSE;
    GList *iter;
    AudioDevice *data;

    if (audio_devices == NULL) {
        gm_audio_query_devices();
    }

    //printf("update device, looking for %s\n", device->description);

    iter = audio_devices;
    while (iter != NULL) {
        data = (AudioDevice *) iter->data;
        //printf("Checking %s\n", data->description);
        if (g_ascii_strcasecmp(device->description, data->description) == 0) {
            device->type = data->type;
            device->alsa_card = data->alsa_card;
            device->alsa_device = data->alsa_device;
            device->pulse_index = data->pulse_index;
            if (device->mplayer_ao != NULL) {
                g_free(device->mplayer_ao);
                device->mplayer_ao = NULL;
            }
            device->mplayer_ao = g_strdup(data->mplayer_ao);
        }
        iter = iter->next;
    }


    return ret;
}

void free_list_item(gpointer item, gpointer data)
{
    AudioDevice *device = (AudioDevice *) item;

    g_free(device->description);
    g_free(device->alsa_mixer);
    g_free(device->mplayer_ao);
}


gboolean gm_audio_free()
{
    if (audio_devices) {
        g_list_foreach(audio_devices, free_list_item, NULL);
        g_list_free(audio_devices);
        audio_devices = NULL;
    }
    return TRUE;
}

#ifdef HAVE_PULSEAUDIO

void gm_audio_pa_sink_cb(pa_context * c, const pa_sink_info * i, int eol, gpointer data)
{
    gchar *desc;
    gchar *mplayer_ao;
    AudioDevice *device;

    if (i) {
        desc = g_strdup_printf("%s (PulseAudio)", i->description);
        mplayer_ao = g_strdup_printf("pulse::%i", i->index);

        device = g_new0(AudioDevice, 1);
        device->description = g_strdup(desc);
        device->type = AUDIO_TYPE_PULSE;
        device->pulse_index = i->index;
        device->mplayer_ao = g_strdup(mplayer_ao);
        audio_devices = g_list_append(audio_devices, device);

        g_free(desc);
        g_free(mplayer_ao);

    }
}

void gm_audio_context_state_callback(pa_context * context, gpointer data)
{
    //printf("context state callback\n");
    int i;

    switch (pa_context_get_state(context)) {
    case PA_CONTEXT_READY:{
            for (i = 0; i < 255; i++) {
                pa_context_get_sink_info_by_index(context, i, gm_audio_pa_sink_cb, data);
            }
        }

    default:
        return;
    }

}
#endif
