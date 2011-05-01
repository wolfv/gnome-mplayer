/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gmtk_media_player.c
 * Copyright (C) Kevin DeKorte 2009 <kdekorte@gmail.com>
 * 
 * gmtk_media_player.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * gmtk_media_tracker.c is distributed in the hope that it will be useful,
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

#include "gmtk_media_player.h"
#include "../libgmlib/gmlib.h"

G_DEFINE_TYPE(GmtkMediaPlayer, gmtk_media_player, GTK_TYPE_EVENT_BOX);
static GObjectClass *parent_class = NULL;

static void gmtk_media_player_dispose(GObject * object);
static gboolean gmtk_media_player_expose_event(GtkWidget * widget, GdkEventExpose * event);
static void gmtk_media_player_size_allocate(GtkWidget * widget, GtkAllocation * allocation);
static gboolean player_key_press_event_callback(GtkWidget * widget, GdkEventKey * event, gpointer data);
static gboolean player_button_press_event_callback(GtkWidget * widget, GdkEventButton * event, gpointer data);
static gboolean player_motion_notify_event_callback(GtkWidget * widget, GdkEventMotion * event, gpointer data);
static void gmtk_media_player_restart_complete_callback(GmtkMediaPlayer * player, gpointer data);
static void gmtk_media_player_restart_shutdown_complete_callback(GmtkMediaPlayer * player, gpointer data);

// monitoring functions
gpointer launch_mplayer(gpointer data);
gboolean thread_reader_error(GIOChannel * source, GIOCondition condition, gpointer data);
gboolean thread_reader(GIOChannel * source, GIOCondition condition, gpointer data);
gboolean thread_query(gpointer data);
gboolean thread_complete(GIOChannel * source, GIOCondition condition, gpointer data);

gboolean write_to_mplayer(GmtkMediaPlayer * player, const gchar * cmd);

extern void get_allocation(GtkWidget * widget, GtkAllocation * allocation);
gboolean detect_mplayer_features(GmtkMediaPlayer * player);


gchar *gmtk_media_player_switch_protocol(const gchar * uri, gchar * new_protocol)
{
    gchar *p;

    p = g_strrstr(uri, "://");

    if (p != NULL)
        return g_strdup_printf("%s%s", new_protocol, p);
    else
        return NULL;
}

static void gmtk_media_player_class_init(GmtkMediaPlayerClass * class)
{
    GtkWidgetClass *widget_class;
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS(class);
    widget_class = GTK_WIDGET_CLASS(class);

    parent_class = g_type_class_peek_parent(class);
    G_OBJECT_CLASS(class)->dispose = gmtk_media_player_dispose;
#ifdef GTK3_ENABLED
#else
    widget_class->expose_event = gmtk_media_player_expose_event;
#endif
    widget_class->size_allocate = gmtk_media_player_size_allocate;

    g_signal_new("position-changed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, position_changed),
                 NULL, NULL, g_cclosure_marshal_VOID__DOUBLE, G_TYPE_NONE, 1, G_TYPE_DOUBLE);

    g_signal_new("cache-percent-changed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, cache_percent_changed),
                 NULL, NULL, g_cclosure_marshal_VOID__DOUBLE, G_TYPE_NONE, 1, G_TYPE_DOUBLE);

    g_signal_new("attribute-changed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, attribute_changed),
                 NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

    g_signal_new("player-state-changed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, player_state_changed),
                 NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

    g_signal_new("media-state-changed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, media_state_changed),
                 NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

    g_signal_new("subtitles-changed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, subtitles_changed),
                 NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

    g_signal_new("audio-tracks-changed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, audio_tracks_changed),
                 NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

    g_signal_new("restart-shutdown-complete",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, restart_shutdown_complete),
                 NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    g_signal_new("restart-complete",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, restart_complete),
                 NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

}

static void gmtk_media_player_init(GmtkMediaPlayer * player)
{

    GtkStyle *style;

    gtk_widget_add_events(GTK_WIDGET(player),
                          GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK);


    //gtk_widget_set_size_request(GTK_WIDGET(player), 320, 200);
    //gtk_widget_set_has_window(GTK_WIDGET(player), TRUE);
    //gtk_widget_set_can_focus(GTK_WIDGET(player), TRUE);
    //gtk_widget_set_can_default(GTK_WIDGET(player), TRUE);

    g_signal_connect(player, "key_press_event", G_CALLBACK(player_key_press_event_callback), NULL);
    g_signal_connect(player, "motion_notify_event", G_CALLBACK(player_motion_notify_event_callback), NULL);
    g_signal_connect(player, "button_press_event", G_CALLBACK(player_button_press_event_callback), NULL);
    gtk_widget_push_composite_child();

    player->alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
    player->socket = gtk_socket_new();
    gtk_container_add(GTK_CONTAINER(player), player->alignment);
    gtk_container_add(GTK_CONTAINER(player->alignment), player->socket);
    //gtk_widget_add_events(GTK_WIDGET(player->socket),
    //                      GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
    //                      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
    //                      GDK_POINTER_MOTION_MASK | 
    //                      GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK);
    //gtk_widget_set_size_request(player->socket, 16, 8);
    gtk_widget_set_has_window(GTK_WIDGET(player->socket), TRUE);
    gtk_widget_set_can_focus(GTK_WIDGET(player->socket), TRUE);
    gtk_widget_set_can_default(GTK_WIDGET(player->socket), TRUE);
    gtk_widget_activate(GTK_WIDGET(player->socket));

    g_signal_connect(player->socket, "key_press_event", G_CALLBACK(player_key_press_event_callback), player);

    g_signal_connect(player, "restart-shutdown-complete",
                     G_CALLBACK(gmtk_media_player_restart_shutdown_complete_callback), NULL);
    g_signal_connect(player, "restart-complete", G_CALLBACK(gmtk_media_player_restart_complete_callback), NULL);
    gtk_widget_pop_composite_child();

    gtk_widget_show_all(GTK_WIDGET(player));
    style = gtk_widget_get_style(GTK_WIDGET(player));
    player->default_background = gdk_color_copy(&(style->bg[GTK_STATE_NORMAL]));
    player->player_state = PLAYER_STATE_DEAD;
    player->media_state = MEDIA_STATE_UNKNOWN;
    player->uri = NULL;
    player->message = NULL;
    player->mplayer_thread = NULL;
    player->aspect_ratio = ASPECT_DEFAULT;
    player->mplayer_complete_cond = g_cond_new();
    player->thread_running = g_mutex_new();
    player->video_width = 0;
    player->video_height = 0;
    player->video_present = FALSE;
    player->media_device = NULL;
    player->type = TYPE_FILE;
    player->title_is_menu = FALSE;
    player->start_time = 0.0;
    player->run_time = 0.0;
    player->vo = NULL;
    player->ao = NULL;
    player->cache_size = 0;
    player->subtitles = NULL;
    player->audio_tracks = NULL;
    player->af_export_filename = gm_tempname(NULL, "mplayer-af_exportXXXXXX");
    player->brightness = 0;
    player->contrast = 0;
    player->gamma = 0;
    player->hue = 0;
    player->osdlevel = 0;
    player->saturation = 0;
    player->restart = FALSE;
    player->video_format = NULL;
    player->video_codec = NULL;
    player->audio_format = NULL;
    player->audio_codec = NULL;
    player->disable_upscaling = FALSE;
    player->mplayer_binary = NULL;
    player->media_device = NULL;
    player->extra_opts = NULL;
    player->use_mplayer2 = FALSE;
    player->features_detected = FALSE;
    player->zoom = 1.0;
    player->speed_multiplier = 1.0;
    player->subtitle_scale = 1.0;
    player->subtitle_delay = 0.0;
    player->subtitle_position = 0;
    player->audio_delay = 0.0;
    player->restart = FALSE;
    player->debug = 1;
}

static void gmtk_media_player_dispose(GObject * object)
{

    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(object);

    if (player->uri != NULL) {
        g_free(player->uri);
        player->uri = NULL;
    }
    if (player->media_device != NULL) {
        g_free(player->media_device);
        player->media_device = NULL;
    }
    if (player->vo != NULL) {
        g_free(player->vo);
        player->vo = NULL;
    }
    if (player->ao != NULL) {
        g_free(player->ao);
        player->ao = NULL;
    }

    if (player->af_export_filename != NULL) {
        g_free(player->af_export_filename);
        player->af_export_filename = NULL;
    }

    gdk_color_free(player->default_background);

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static gboolean gmtk_media_player_expose_event(GtkWidget * widget, GdkEventExpose * event)
{
    return FALSE;
}

gboolean gmtk_media_player_send_key_press_event(GmtkMediaPlayer * widget, GdkEventKey * event, gpointer data)
{
    return player_key_press_event_callback(GTK_WIDGET(widget), event, data);
}

static gboolean player_key_press_event_callback(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
    GmtkMediaPlayer *player;
    GtkAllocation alloc;
    gchar *cmd;

    if (data != NULL) {
        player = GMTK_MEDIA_PLAYER(data);
    } else {
        player = GMTK_MEDIA_PLAYER(widget);
    }
    if (event->state == (event->state & (~GDK_CONTROL_MASK))) {
        switch (event->keyval) {
        case GDK_Right:
            if (player->title_is_menu) {
                write_to_mplayer(player, "dvdnav 4\n");
            }
            break;
        case GDK_Left:
            if (player->title_is_menu) {
                write_to_mplayer(player, "dvdnav 3\n");
            }
            break;
        case GDK_Up:
            if (player->title_is_menu) {
                write_to_mplayer(player, "dvdnav 1\n");
            }
            break;
        case GDK_Down:
            if (player->title_is_menu) {
                write_to_mplayer(player, "dvdnav 2\n");
            }
            break;
        case GDK_Return:
            if (player->title_is_menu) {
                write_to_mplayer(player, "dvdnav 6\n");
            }
            return TRUE;
            break;
        case GDK_Home:
            if (!player->title_is_menu) {
                write_to_mplayer(player, "dvdnav menu\n");
            }
            return TRUE;
            break;
        case GDK_space:
        case GDK_p:
            switch (player->media_state) {
            case MEDIA_STATE_PAUSE:
                gmtk_media_player_set_state(player, MEDIA_STATE_PLAY);
                break;
            case MEDIA_STATE_PLAY:
                gmtk_media_player_set_state(player, MEDIA_STATE_PAUSE);
                break;
            }
            break;
        case GDK_1:
            gmtk_media_player_set_attribute_integer_delta(player, ATTRIBUTE_CONTRAST, -5);
            break;
        case GDK_2:
            gmtk_media_player_set_attribute_integer_delta(player, ATTRIBUTE_CONTRAST, 5);
            break;
        case GDK_3:
            gmtk_media_player_set_attribute_integer_delta(player, ATTRIBUTE_BRIGHTNESS, -5);
            break;
        case GDK_4:
            gmtk_media_player_set_attribute_integer_delta(player, ATTRIBUTE_BRIGHTNESS, 5);
            break;
        case GDK_5:
            gmtk_media_player_set_attribute_integer_delta(player, ATTRIBUTE_HUE, -5);
            break;
        case GDK_6:
            gmtk_media_player_set_attribute_integer_delta(player, ATTRIBUTE_HUE, 5);
            break;
        case GDK_7:
            gmtk_media_player_set_attribute_integer_delta(player, ATTRIBUTE_SATURATION, -5);
            break;
        case GDK_8:
            gmtk_media_player_set_attribute_integer_delta(player, ATTRIBUTE_SATURATION, 5);
            break;
        case GDK_plus:
            write_to_mplayer(player, "audio_delay 0.1 0\n");
            break;
        case GDK_minus:
            write_to_mplayer(player, "audio_delay -0.1 0\n");
            break;
        case GDK_numbersign:
            write_to_mplayer(player, "switch_audio -1\n");
            return TRUE;
            break;
        case GDK_period:
            if (player->media_state == MEDIA_STATE_PAUSE)
                write_to_mplayer(player, "frame_step\n");
            break;
        case GDK_KP_Add:
            player->zoom += 0.10;
            player->zoom = CLAMP(player->zoom, 0.1, 10.0);
            get_allocation(GTK_WIDGET(player), &alloc);
            gmtk_media_player_size_allocate(GTK_WIDGET(player), &alloc);
            break;
        case GDK_KP_Subtract:
            player->zoom -= 0.10;
            player->zoom = CLAMP(player->zoom, 0.1, 10.0);
            get_allocation(GTK_WIDGET(player), &alloc);
            gmtk_media_player_size_allocate(GTK_WIDGET(player), &alloc);
            break;
        case GDK_KP_Enter:
            player->zoom = 1.0;
            get_allocation(GTK_WIDGET(player), &alloc);
            gmtk_media_player_size_allocate(GTK_WIDGET(player), &alloc);
            break;
        case GDK_j:
            write_to_mplayer(player, "sub_select\n");
            break;
        case GDK_d:
            write_to_mplayer(player, "frame_drop\n");
            cmd = g_strdup_printf("osd_show_property_text \"%s: ${framedropping}\"\n", _("framedropping"));
            write_to_mplayer(player, cmd);
            g_free(cmd);
            break;
        case GDK_b:
            write_to_mplayer(player, "sub_pos -1 0\n");
            break;
        case GDK_B:
            write_to_mplayer(player, "sub_pos 1 0\n");
            break;
        case GDK_s:
        case GDK_S:
            write_to_mplayer(player, "screenshot 0\n");
            break;
        default:
            if (player->debug)
                printf("ignoring key %i\n", event->keyval);
        }

    }
    return FALSE;
}

static gboolean player_button_press_event_callback(GtkWidget * widget, GdkEventButton * event, gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(widget);
    gchar *cmd;

    if (event->button == 1) {
        if (player->title_is_menu) {
            cmd = g_strdup_printf("dvdnav mouse\n");
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
    }


    return FALSE;
}

static gboolean player_motion_notify_event_callback(GtkWidget * widget, GdkEventMotion * event, gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(widget);
    gchar *cmd;
    gint x, y;

    if (player->title_is_menu) {
        gtk_widget_get_pointer(player->socket, &x, &y);
        cmd = g_strdup_printf("set_mouse_pos %i %i\n", x, y);
        write_to_mplayer(player, cmd);
        g_free(cmd);
    }

    return FALSE;

}

static void gmtk_media_player_size_allocate(GtkWidget * widget, GtkAllocation * allocation)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(widget);
    gdouble video_aspect;
    gdouble widget_aspect;
    gint da_width, da_height;
    gfloat xscale, yscale;

    if (player->video_width == 0 || player->video_height == 0) {
        gtk_alignment_set(GTK_ALIGNMENT(player->alignment), 0.0, 0.0, 1.0, 1.0);
    } else {
        switch (player->aspect_ratio) {
        case ASPECT_4X3:
            video_aspect = 4.0 / 3.0;
            break;
        case ASPECT_16X9:
            video_aspect = 16.0 / 9.0;
            break;
        case ASPECT_16X10:
            video_aspect = 16.0 / 10.0;
            break;
        case ASPECT_WINDOW:
            video_aspect = (gdouble) allocation->width / (gdouble) allocation->height;
            break;
        case ASPECT_DEFAULT:
        default:
            video_aspect = (gdouble) player->video_width / (gdouble) player->video_height;
            break;
        }
        widget_aspect = (gdouble) allocation->width / (gdouble) allocation->height;

        if (player->disable_upscaling && allocation->width > player->video_width
            && allocation->height > player->video_height) {

            gtk_alignment_set(GTK_ALIGNMENT(player->alignment), 0.5, 0.5,
                              (gdouble) player->video_width / (gdouble) allocation->width,
                              (gdouble) player->video_height / (gdouble) allocation->height);

        } else {
            if (video_aspect > widget_aspect) {
                yscale = (allocation->width / video_aspect) / allocation->height;

                gtk_alignment_set(GTK_ALIGNMENT(player->alignment), 0, 0.5, 1, yscale);
            } else {
                xscale = (allocation->height * video_aspect) / allocation->width;

                gtk_alignment_set(GTK_ALIGNMENT(player->alignment), 0.5, 0, xscale, 1);
            }
        }
    }

    GTK_WIDGET_CLASS(parent_class)->size_allocate(widget, allocation);
}

GtkWidget *gmtk_media_player_new()
{
    GtkWidget *player = g_object_new(GMTK_TYPE_MEDIA_PLAYER, NULL);

    return player;
}

static void gmtk_media_player_restart_complete_callback(GmtkMediaPlayer * player, gpointer data)
{
    gmtk_media_player_seek(player, player->restart_position, SEEK_ABSOLUTE);
    // g_signal_emit_by_name(player, "position-changed", player->restart_position);
    if (player->restart_state != gmtk_media_player_get_state(player))
        gmtk_media_player_set_state(GMTK_MEDIA_PLAYER(player), player->restart_state);
    player->restart = FALSE;
    if (player->debug)
        printf("restart complete\n");
}

static void gmtk_media_player_restart_shutdown_complete_callback(GmtkMediaPlayer * player, gpointer data)
{
    if (player->restart)
        gmtk_media_player_set_state(GMTK_MEDIA_PLAYER(player), MEDIA_STATE_PLAY);
}

void gmtk_media_player_restart(GmtkMediaPlayer * player)
{
    if (player->player_state == PLAYER_STATE_RUNNING) {
        player->restart = TRUE;
        player->restart_state = gmtk_media_player_get_state(player);
        gmtk_media_player_set_state(player, MEDIA_STATE_PAUSE);
        player->restart_position = player->position;
        gmtk_media_player_set_state(GMTK_MEDIA_PLAYER(player), MEDIA_STATE_QUIT);
    }
}


void gmtk_media_player_set_uri(GmtkMediaPlayer * player, const gchar * uri)
{
    gchar *cmd;
    gchar *filename = NULL;

    player->uri = g_strdup(uri);
    player->video_width = 0;
    player->video_height = 0;
    player->length = 0;
    player->start_time = 0;

    if (player->player_state == PLAYER_STATE_RUNNING) {
        if (player->uri != NULL) {
            filename = g_filename_from_uri(player->uri, NULL, NULL);
        }
        cmd = g_strdup_printf("loadfile \"%s\" 0\n", filename);
        write_to_mplayer(player, cmd);
        g_free(cmd);
        if (filename != NULL) {
            g_free(filename);
        }
        if (player->media_state == MEDIA_STATE_STOP) {
            gmtk_media_player_set_state(player, MEDIA_STATE_PLAY);
        }
    }

}

const gchar *gmtk_media_player_get_uri(GmtkMediaPlayer * player)
{
    return player->uri;
}

void gmtk_media_player_set_state(GmtkMediaPlayer * player, const GmtkMediaPlayerMediaState new_state)
{
    if (player->player_state == PLAYER_STATE_DEAD) {

        if (new_state == MEDIA_STATE_QUIT) {
            player->media_state = MEDIA_STATE_UNKNOWN;
        }

        if (new_state == MEDIA_STATE_STOP) {
            player->media_state = MEDIA_STATE_UNKNOWN;
        }

        if (new_state == MEDIA_STATE_PAUSE) {
            player->media_state = MEDIA_STATE_UNKNOWN;
        }

        if (new_state == MEDIA_STATE_PLAY) {
            // launch player
            player->mplayer_thread = g_thread_create(launch_mplayer, player, TRUE, NULL);
            if (player->mplayer_thread != NULL) {
                if (player->message != NULL) {
                    g_free(player->message);
                    player->message = NULL;
                }
                player->message = g_strdup_printf(_("Loading..."));
                if (!player->restart)
                    g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_MESSAGE);
                player->player_state = PLAYER_STATE_RUNNING;
                if (!player->restart)
                    g_signal_emit_by_name(player, "player-state-changed", player->player_state);
                player->media_state = MEDIA_STATE_PLAY;
                if (!player->restart)
                    g_signal_emit_by_name(player, "media-state-changed", player->media_state);
            }
        }

    }

    if (player->player_state == PLAYER_STATE_RUNNING) {
        if (new_state == MEDIA_STATE_STOP) {
            write_to_mplayer(player, "seek 0 2\n");
            if (player->media_state == MEDIA_STATE_PLAY) {
                write_to_mplayer(player, "pause\n");
            }
            player->media_state = MEDIA_STATE_STOP;
            g_signal_emit_by_name(player, "position-changed", 0.0);
            if (!player->restart)
                g_signal_emit_by_name(player, "media-state-changed", player->media_state);
        }

        if (new_state == MEDIA_STATE_PLAY) {
            if (player->media_state == MEDIA_STATE_PAUSE || player->media_state == MEDIA_STATE_STOP) {
                write_to_mplayer(player, "pause\n");
                player->media_state = MEDIA_STATE_PLAY;
                if (!player->restart)
                    g_signal_emit_by_name(player, "media-state-changed", player->media_state);
            }
            if (player->media_state == MEDIA_STATE_UNKNOWN) {
                player->media_state = MEDIA_STATE_PLAY;
                if (!player->restart)
                    g_signal_emit_by_name(player, "media-state-changed", player->media_state);
            }
        }

        if (new_state == MEDIA_STATE_PAUSE) {
            if (player->media_state == MEDIA_STATE_PLAY) {
                write_to_mplayer(player, "pause\n");
                player->media_state = MEDIA_STATE_PAUSE;
                if (!player->restart)
                    g_signal_emit_by_name(player, "media-state-changed", player->media_state);
            }
        }

        if (new_state == MEDIA_STATE_QUIT) {
            write_to_mplayer(player, "quit\n");
            //while (player->player_state != PLAYER_STATE_DEAD) {
            //    while (gtk_events_pending())
            //        gtk_main_iteration();
            //}
        }
    }

}

GmtkMediaPlayerMediaState gmtk_media_player_get_state(GmtkMediaPlayer * player)
{
    return player->media_state;
}

void gmtk_media_player_send_command(GmtkMediaPlayer * player, GmtkMediaPlayerCommand command)
{
    if (player->player_state == PLAYER_STATE_RUNNING) {
        switch (command) {
        case COMMAND_SHOW_DVD_MENU:
            write_to_mplayer(player, "dvdnav 5\n");
            break;

        case COMMAND_TAKE_SCREENSHOT:
            write_to_mplayer(player, "screenshot 0\n");
            break;

        case COMMAND_SWITCH_ANGLE:
            write_to_mplayer(player, "switch_angle\n");
            break;

        case COMMAND_SWITCH_AUDIO:
            write_to_mplayer(player, "switch_audio\n");
            break;

        case COMMAND_FRAME_STEP:
            if (player->media_state == MEDIA_STATE_PAUSE)
                write_to_mplayer(player, "frame_step\n");
            break;

        case COMMAND_SUBTITLE_SELECT:
            write_to_mplayer(player, "sub_select\n");
            break;

        case COMMAND_SWITCH_FRAME_DROP:
            write_to_mplayer(player, "frame_drop\n");
            write_to_mplayer(player, "osd_show_property_text \"framedropping: ${framedropping}\"\n");
            break;

        default:
            if (player->debug)
                printf("Unknown command\n");
        }
    }
}

void gmtk_media_player_set_attribute_boolean(GmtkMediaPlayer * player,
                                             GmtkMediaPlayerMediaAttributes attribute, gboolean value)
{
    gchar *cmd = NULL;

    switch (attribute) {
    case ATTRIBUTE_SUB_VISIBLE:
        player->sub_visible = value;
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property sub_visibility %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_ENABLE_FRAME_DROP:
        player->frame_drop = value;
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("frame_drop %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_PLAYLIST:
        player->playlist = value;
        break;

    case ATTRIBUTE_DISABLE_UPSCALING:
        player->disable_upscaling = value;
        break;

    case ATTRIBUTE_SOFTVOL:
        player->softvol = value;
        break;

    case ATTRIBUTE_MUTED:
        player->muted = value;
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("muted %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_ENABLE_ADVANCED_SUBTITLES:
        player->enable_advanced_subtitles = value;
        break;

    case ATTRIBUTE_ENABLE_EMBEDDED_FONTS:
        player->enable_embedded_fonts = value;
        break;

    case ATTRIBUTE_SUBTITLE_OUTLINE:
        player->subtitle_outline = value;
        break;

    case ATTRIBUTE_SUBTITLE_SHADOW:
        player->subtitle_shadow = value;
        break;

    case ATTRIBUTE_DEINTERLACE:
        player->deinterlace = value;
        break;

    case ATTRIBUTE_ENABLE_DEBUG:
        player->debug = value;
        break;

    default:
        if (player->debug)
            printf("Unsupported Attribute\n");
    }

    return;
}

gboolean gmtk_media_player_get_attribute_boolean(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute)
{
    gboolean ret = FALSE;

    switch (attribute) {
    case ATTRIBUTE_SUB_VISIBLE:
        ret = player->sub_visible;
        break;

    case ATTRIBUTE_ENABLE_FRAME_DROP:
        ret = player->frame_drop;
        break;

    case ATTRIBUTE_SUBS_EXIST:
        ret = (player->subtitles != NULL);
        break;

    case ATTRIBUTE_SOFTVOL:
        ret = player->softvol;
        break;

    case ATTRIBUTE_PLAYLIST:
        ret = player->playlist;
        break;

    case ATTRIBUTE_SEEKABLE:
        ret = player->seekable;
        break;

    case ATTRIBUTE_HAS_CHAPTERS:
        ret = player->has_chapters;
        break;

    case ATTRIBUTE_TITLE_IS_MENU:
        ret = player->title_is_menu;
        break;

    case ATTRIBUTE_DISABLE_UPSCALING:
        ret = player->disable_upscaling;
        break;

    case ATTRIBUTE_VIDEO_PRESENT:
        ret = player->video_present;
        break;

    case ATTRIBUTE_MUTED:
        ret = player->muted;
        break;

    case ATTRIBUTE_ENABLE_ADVANCED_SUBTITLES:
        ret = player->enable_advanced_subtitles;
        break;

    case ATTRIBUTE_ENABLE_EMBEDDED_FONTS:
        ret = player->enable_embedded_fonts;
        break;

    case ATTRIBUTE_SUBTITLE_OUTLINE:
        ret = player->subtitle_outline;
        break;

    case ATTRIBUTE_SUBTITLE_SHADOW:
        ret = player->subtitle_shadow;
        break;

    case ATTRIBUTE_DEINTERLACE:
        ret = player->deinterlace;
        break;

    case ATTRIBUTE_ENABLE_DEBUG:
        ret = player->debug;
        break;

    default:
        if (player->debug)
            printf("Unsupported Attribute\n");
    }
    return ret;
}

void gmtk_media_player_set_attribute_double(GmtkMediaPlayer * player,
                                            GmtkMediaPlayerMediaAttributes attribute, gdouble value)
{
    gchar *cmd;

    switch (attribute) {
    case ATTRIBUTE_CACHE_SIZE:
        player->cache_size = value;
        break;

    case ATTRIBUTE_ZOOM:
        player->zoom = CLAMP(value, 0.1, 10.0);
        break;

    case ATTRIBUTE_SPEED_MULTIPLIER:
        player->speed_multiplier = CLAMP(value, 0.1, 10.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("speed_mult %f\n", player->speed_multiplier);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_SUBTITLE_SCALE:
        player->subtitle_scale = CLAMP(value, 0.2, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("sub_scale %f 1\n", player->subtitle_scale);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_SUBTITLE_DELAY:
        player->subtitle_delay = value;
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property sub_delay %f 1\n", player->subtitle_delay);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_AUDIO_DELAY:
        player->audio_delay = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property audio_delay %f 1\n", player->audio_delay);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    default:
        if (player->debug)
            printf("Unsupported Attribute\n");
    }

    return;
}

gdouble gmtk_media_player_get_attribute_double(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute)
{
    gdouble ret = 0.0;

    switch (attribute) {
    case ATTRIBUTE_LENGTH:
        ret = player->length;
        break;

    case ATTRIBUTE_START_TIME:
        ret = player->start_time;
        break;

    case ATTRIBUTE_WIDTH:
        ret = (gdouble) player->video_width;
        break;

    case ATTRIBUTE_HEIGHT:
        ret = (gdouble) player->video_height;
        break;

    case ATTRIBUTE_SUB_COUNT:
        ret = (gdouble) g_list_length(player->subtitles);
        break;

    case ATTRIBUTE_AUDIO_TRACK_COUNT:
        ret = (gdouble) g_list_length(player->audio_tracks);
        break;

    case ATTRIBUTE_ZOOM:
        ret = player->zoom;
        break;

    case ATTRIBUTE_SPEED_MULTIPLIER:
        ret = player->speed_multiplier;
        break;

    case ATTRIBUTE_SUBTITLE_SCALE:
        ret = player->subtitle_scale;
        break;

    case ATTRIBUTE_SUBTITLE_DELAY:
        ret = player->subtitle_delay;
        break;

    case ATTRIBUTE_AUDIO_DELAY:
        ret = player->audio_delay;
        break;

    case ATTRIBUTE_VIDEO_FPS:
        ret = player->video_fps;
        break;

    default:
        if (player->debug)
            printf("Unsupported Attribute\n");
    }
    return ret;
}

void gmtk_media_player_set_attribute_string(GmtkMediaPlayer * player,
                                            GmtkMediaPlayerMediaAttributes attribute, const gchar * value)
{
    gchar *cmd;

    switch (attribute) {
    case ATTRIBUTE_VO:
        if (player->vo != NULL) {
            g_free(player->vo);
        }
        if (value == NULL || strlen(value) == 0) {
            player->vo = NULL;
        } else {
            player->vo = g_strdup(value);
        }
        break;

    case ATTRIBUTE_AO:
        if (player->ao != NULL) {
            g_free(player->ao);
        }
        if (value == NULL || strlen(value) == 0) {
            player->ao = NULL;
        } else {
            player->ao = g_strdup(value);
        }
        break;

    case ATTRIBUTE_MEDIA_DEVICE:
        if (player->media_device != NULL) {
            g_free(player->media_device);
        }
        if (value == NULL || strlen(value) == 0) {
            player->media_device = NULL;
        } else {
            player->media_device = g_strdup(value);
        }
        break;

    case ATTRIBUTE_EXTRA_OPTS:
        if (player->extra_opts != NULL) {
            g_free(player->extra_opts);
        }
        if (value == NULL || strlen(value) == 0) {
            player->extra_opts = NULL;
        } else {
            player->extra_opts = g_strdup(value);
        }
        break;

    case ATTRIBUTE_MPLAYER_BINARY:
        if (player->mplayer_binary != NULL) {
            g_free(player->mplayer_binary);
        }
        if (value == NULL || strlen(value) == 0) {
            player->mplayer_binary = NULL;
        } else {
            player->mplayer_binary = g_strdup(value);
        }
        player->features_detected = FALSE;
        break;

    case ATTRIBUTE_AUDIO_TRACK_FILE:
        if (player->audio_track_file != NULL) {
            g_free(player->audio_track_file);
        }
        if (value == NULL || strlen(value) == 0) {
            player->audio_track_file = NULL;
        } else {
            player->audio_track_file = g_strdup(value);
        }
        break;

    case ATTRIBUTE_SUBTITLE_FILE:
        if (player->subtitle_file != NULL) {
            g_free(player->subtitle_file);
        }
        if (value == NULL || strlen(value) == 0) {
            player->subtitle_file = NULL;
        } else {
            player->subtitle_file = g_strdup(value);
            if (player->player_state == PLAYER_STATE_RUNNING) {
                write_to_mplayer(player, "sub_remove\n");
                cmd = g_strdup_printf("sub_load \"%s\" 1\n", player->subtitle_file);
                write_to_mplayer(player, cmd);
                g_free(cmd);
            }
        }
        break;

    case ATTRIBUTE_SUBTITLE_COLOR:
        if (player->subtitle_color != NULL) {
            g_free(player->subtitle_color);
        }
        if (value == NULL || strlen(value) == 0) {
            player->subtitle_color = NULL;
        } else {
            player->subtitle_color = g_strdup(value);
        }
        break;

    case ATTRIBUTE_SUBTITLE_CODEPAGE:
        if (player->subtitle_codepage != NULL) {
            g_free(player->subtitle_codepage);
        }
        if (value == NULL || strlen(value) == 0) {
            player->subtitle_codepage = NULL;
        } else {
            player->subtitle_codepage = g_strdup(value);
        }
        break;

    case ATTRIBUTE_SUBTITLE_FONT:
        if (player->subtitle_font != NULL) {
            g_free(player->subtitle_font);
        }
        if (value == NULL || strlen(value) == 0) {
            player->subtitle_font = NULL;
        } else {
            player->subtitle_font = g_strdup(value);
        }
        break;

    default:
        if (player->debug)
            printf("Unsupported Attribute\n");
    }
}

const gchar *gmtk_media_player_get_attribute_string(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute)
{
    gchar *value = NULL;
    GList *iter;
    GmtkMediaPlayerAudioTrack *track;
    GmtkMediaPlayerSubtitle *subtitle;

    switch (attribute) {
    case ATTRIBUTE_AF_EXPORT_FILENAME:
        value = player->af_export_filename;
        break;

    case ATTRIBUTE_MEDIA_DEVICE:
        value = player->media_device;
        break;

    case ATTRIBUTE_EXTRA_OPTS:
        value = player->extra_opts;
        break;

    case ATTRIBUTE_MESSAGE:
        value = player->message;
        break;

    case ATTRIBUTE_AUDIO_TRACK:
        iter = player->audio_tracks;
        while (iter) {
            track = (GmtkMediaPlayerAudioTrack *) iter->data;
            if (track->id == player->audio_track_id)
                value = track->label;
            iter = iter->next;
        }
        break;

    case ATTRIBUTE_SUBTITLE:
        iter = player->subtitles;
        while (iter) {
            subtitle = (GmtkMediaPlayerSubtitle *) iter->data;
            if (subtitle->id == player->subtitle_id && subtitle->is_file == player->subtitle_is_file)
                value = subtitle->label;
            iter = iter->next;
        }
        break;

    case ATTRIBUTE_SUBTITLE_COLOR:
        value = player->subtitle_color;
        break;

    case ATTRIBUTE_SUBTITLE_CODEPAGE:
        value = player->subtitle_codepage;
        break;

    case ATTRIBUTE_SUBTITLE_FONT:
        value = player->subtitle_font;
        break;

    case ATTRIBUTE_VIDEO_FORMAT:
        value = player->video_format;
        break;

    case ATTRIBUTE_VIDEO_CODEC:
        value = player->video_codec;
        break;

    case ATTRIBUTE_AUDIO_FORMAT:
        value = player->audio_format;
        break;

    case ATTRIBUTE_AUDIO_CODEC:
        value = player->audio_codec;
        break;

    default:
        if (player->debug)
            printf("Unsupported Attribute\n");
    }

    return value;
}

void gmtk_media_player_set_attribute_integer(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute,
                                             gint value)
{
    gchar *cmd = NULL;

    switch (attribute) {
    case ATTRIBUTE_BRIGHTNESS:
        player->brightness = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property brightness %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_CONTRAST:
        player->contrast = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property contrast %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_GAMMA:
        player->gamma = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property gamma %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_HUE:
        player->hue = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property hue %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_SATURATION:
        player->saturation = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property saturation %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_SUBTITLE_MARGIN:
        player->subtitle_margin = CLAMP(value, 0.0, 200.0);
        break;

    case ATTRIBUTE_SUBTITLE_POSITION:
        player->subtitle_position = CLAMP(value, 0, 100);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property sub_pos %i\n", player->subtitle_position);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    case ATTRIBUTE_OSDLEVEL:
        player->osdlevel = CLAMP(value, 0, 3);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("set_property osdlevel %i\n", player->osdlevel);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
        break;

    default:
        if (player->debug)
            printf("Unsupported Attribute\n");
    }

    return;

}

void gmtk_media_player_set_attribute_integer_delta(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute,
                                                   gint delta)
{
    gint value;

    switch (attribute) {
    case ATTRIBUTE_BRIGHTNESS:
        value = player->brightness + delta;
        break;

    case ATTRIBUTE_CONTRAST:
        value = player->contrast + delta;
        break;

    case ATTRIBUTE_GAMMA:
        value = player->gamma + delta;
        break;

    case ATTRIBUTE_HUE:
        value = player->hue + delta;
        break;

    case ATTRIBUTE_SATURATION:
        value = player->saturation + delta;
        break;

    default:
        return;
    }

    gmtk_media_player_set_attribute_integer(player, attribute, value);

}

gint gmtk_media_player_get_attribute_integer(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute)
{
    gint ret;

    switch (attribute) {
    case ATTRIBUTE_BRIGHTNESS:
        ret = player->brightness;
        break;

    case ATTRIBUTE_CONTRAST:
        ret = player->contrast;
        break;

    case ATTRIBUTE_GAMMA:
        ret = player->gamma;
        break;

    case ATTRIBUTE_HUE:
        ret = player->hue;
        break;

    case ATTRIBUTE_SATURATION:
        ret = player->saturation;
        break;

    case ATTRIBUTE_WIDTH:
        ret = player->video_width;
        break;

    case ATTRIBUTE_HEIGHT:
        ret = player->video_height;
        break;

    case ATTRIBUTE_SUBTITLE_MARGIN:
        ret = player->subtitle_margin;
        break;

    case ATTRIBUTE_SUBTITLE_POSITION:
        ret = player->subtitle_position;
        break;

    case ATTRIBUTE_CHAPTERS:
        ret = player->chapters;
        break;

    case ATTRIBUTE_VIDEO_BITRATE:
        ret = player->video_bitrate;
        break;

    case ATTRIBUTE_AUDIO_BITRATE:
        ret = player->audio_bitrate;
        break;

    case ATTRIBUTE_AUDIO_RATE:
        ret = player->audio_rate;
        break;

    case ATTRIBUTE_AUDIO_NCH:
        ret = player->audio_nch;
        break;

    case ATTRIBUTE_OSDLEVEL:
        ret = player->osdlevel;
        break;

    default:
        return 0;
    }

    return ret;
}


void gmtk_media_player_seek(GmtkMediaPlayer * player, gdouble value, GmtkMediaPlayerSeekType seek_type)
{
    gchar *cmd;

    cmd = g_strdup_printf("seek %lf %i\n", value, seek_type);
    write_to_mplayer(player, cmd);
    g_free(cmd);
}

void gmtk_media_player_seek_chapter(GmtkMediaPlayer * player, gint value, GmtkMediaPlayerSeekType seek_type)
{
    gchar *cmd;
    if (seek_type != SEEK_RELATIVE)
        seek_type = 1;

    cmd = g_strdup_printf("seek_chapter %i %i\n", value, seek_type);
    write_to_mplayer(player, cmd);
    g_free(cmd);
}

void gmtk_media_player_set_volume(GmtkMediaPlayer * player, gdouble value)
{
    gchar *cmd;

    player->volume = value;
    if (player->player_state == PLAYER_STATE_RUNNING) {
        cmd = g_strdup_printf("volume %i 1\n", (gint) (player->volume * 100.0));
        write_to_mplayer(player, cmd);
        g_free(cmd);
    }
}

gdouble gmtk_media_player_get_volume(GmtkMediaPlayer * player)
{
    return player->volume;
}

void gmtk_media_player_set_media_device(GmtkMediaPlayer * player, gchar * media_device)
{
    if (player->media_device != NULL) {
        g_free(player->media_device);
    }
    if (media_device == NULL) {
        player->media_device = NULL;
    } else {
        player->media_device = g_strdup(media_device);
    }
}

void gmtk_media_player_set_aspect(GmtkMediaPlayer * player, GmtkMediaPlayerAspectRatio aspect)
{
    GtkAllocation alloc;

    player->aspect_ratio = aspect;
    get_allocation(GTK_WIDGET(player), &alloc);
    gmtk_media_player_size_allocate(GTK_WIDGET(player), &alloc);
}

GmtkMediaPlayerAspectRatio gmtk_media_player_get_aspect(GmtkMediaPlayer * player)
{
    return player->aspect_ratio;
}

void gmtk_media_player_set_media_type(GmtkMediaPlayer * player, GmtkMediaPlayerMediaType type)
{
    player->type = type;
}

GmtkMediaPlayerMediaType gmtk_media_player_get_media_type(GmtkMediaPlayer * player)
{
    return player->type;
}

void gmtk_media_player_select_subtitle(GmtkMediaPlayer * player, const gchar * label)
{
    GList *list;
    GmtkMediaPlayerSubtitle *subtitle;
    gchar *cmd;

    list = player->subtitles;
    subtitle = NULL;

    while (list != NULL) {
        subtitle = (GmtkMediaPlayerSubtitle *) list->data;
        if (g_ascii_strcasecmp(subtitle->label, label) == 0) {
            break;
        }
        list = list->next;
    }

    if (list != NULL && subtitle != NULL && player->player_state == PLAYER_STATE_RUNNING) {
        if (subtitle->is_file) {
            cmd = g_strdup_printf("sub_file %i \n", subtitle->id);
        } else {
            cmd = g_strdup_printf("sub_demux %i \n", subtitle->id);
        }
        player->subtitle_id = subtitle->id;
        player->subtitle_is_file = subtitle->is_file;
        write_to_mplayer(player, cmd);
        g_free(cmd);
    }
}

void gmtk_media_player_select_audio_track(GmtkMediaPlayer * player, const gchar * label)
{
    GList *list;
    GmtkMediaPlayerAudioTrack *track;
    gchar *cmd;

    list = player->audio_tracks;
    track = NULL;

    while (list != NULL) {
        track = (GmtkMediaPlayerAudioTrack *) list->data;
        if (g_ascii_strcasecmp(track->label, label) == 0) {
            break;
        }
        list = list->next;
    }

    if (list != NULL && track != NULL && player->player_state == PLAYER_STATE_RUNNING) {
        cmd = g_strdup_printf("switch_audio %i \n", track->id);
        player->audio_track_id = track->id;
        write_to_mplayer(player, cmd);
        g_free(cmd);
    }
}

void gmtk_media_player_select_subtitle_by_id(GmtkMediaPlayer * player, gint id)
{
    GList *list;
    GmtkMediaPlayerSubtitle *subtitle;
    gchar *cmd;

    list = player->subtitles;
    subtitle = NULL;

    while (list != NULL) {
        subtitle = (GmtkMediaPlayerSubtitle *) list->data;
        if (subtitle->id == id) {
            break;
        }
        list = list->next;
    }

    if (list != NULL && subtitle != NULL && player->player_state == PLAYER_STATE_RUNNING) {
        if (subtitle->is_file) {
            cmd = g_strdup_printf("sub_file %i \n", subtitle->id);
        } else {
            cmd = g_strdup_printf("sub_demux %i \n", subtitle->id);
        }
        player->subtitle_id = subtitle->id;
        player->subtitle_is_file = subtitle->is_file;
        write_to_mplayer(player, cmd);
        g_free(cmd);
    }
}

void gmtk_media_player_select_audio_track_by_id(GmtkMediaPlayer * player, gint id)
{
    GList *list;
    GmtkMediaPlayerAudioTrack *track;
    gchar *cmd;

    list = player->audio_tracks;
    track = NULL;

    while (list != NULL) {
        track = (GmtkMediaPlayerAudioTrack *) list->data;
        if (track->id == id) {
            break;
        }
        list = list->next;
    }

    if (list != NULL && track != NULL && player->player_state == PLAYER_STATE_RUNNING) {
        cmd = g_strdup_printf("switch_audio %i \n", track->id);
        player->audio_track_id = track->id;
        write_to_mplayer(player, cmd);
        g_free(cmd);
    }
}

gpointer launch_mplayer(gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(data);
    gchar *argv[255];
    gchar *filename = NULL;
    gint argn;
    GPid pid;
    GError *error;
    gint i;
    gint spawn;
    gchar *fontname;
    gchar *size;
    gchar *tmp;
    GList *list;
    GmtkMediaPlayerSubtitle *subtitle;
    GmtkMediaPlayerAudioTrack *track;


    player->seekable = FALSE;
    player->has_chapters = FALSE;
    player->video_present = FALSE;
    player->position = 0.0;
    player->cache_percent = -1.0;
    player->title_is_menu = FALSE;

    gtk_widget_modify_bg(GTK_WIDGET(player), GTK_STATE_NORMAL, player->default_background);
    gtk_widget_modify_bg(GTK_WIDGET(player->socket), GTK_STATE_NORMAL, player->default_background);
    gtk_widget_show(GTK_WIDGET(player->socket));
    //while (gtk_events_pending())
    //    gtk_main_iteration();

    g_mutex_lock(player->thread_running);

    do {
        if (player->debug)
            printf("setting up mplayer\n");

        list = player->subtitles;
        while (list) {
            subtitle = (GmtkMediaPlayerSubtitle *) list->data;
            g_free(subtitle->lang);
            g_free(subtitle->name);
            g_free(subtitle->label);
            list = g_list_remove(list, subtitle);
        }
        player->subtitles = NULL;

        list = player->audio_tracks;
        while (list) {
            track = (GmtkMediaPlayerAudioTrack *) list->data;
            g_free(track->lang);
            g_free(track->name);
            g_free(track->label);
            list = g_list_remove(list, track);
        }
        player->audio_tracks = NULL;

        argn = 0;
        player->playback_error = NO_ERROR;
        if (player->uri != NULL) {
            filename = g_filename_from_uri(player->uri, NULL, NULL);
        }

        player->minimum_mplayer = detect_mplayer_features(player);

        if (player->mplayer_binary == NULL || !g_file_test(player->mplayer_binary, G_FILE_TEST_EXISTS)) {
            argv[argn++] = g_strdup_printf("mplayer");
        } else {
            argv[argn++] = g_strdup_printf("%s", player->mplayer_binary);
        }

        // use the profile to set up some default values
        argv[argn++] = g_strdup_printf("-profile");
        argv[argn++] = g_strdup_printf("gnome-mplayer");

        if (player->vo != NULL) {
            argv[argn++] = g_strdup_printf("-vo");

            if (g_ascii_strncasecmp(player->vo, "vdpau", strlen("vdpau")) == 0) {
                if (player->deinterlace) {
                    argv[argn++] = g_strdup_printf("vdpau:deint=2,", player->vo);
                } else {
                    argv[argn++] = g_strdup_printf("%s,vdpau,", player->vo);
                }

                argv[argn++] = g_strdup_printf("-vc");
                argv[argn++] = g_strdup_printf("ffmpeg12vdpau,ffh264vdpau,ffwmv3vdpau,ffvc1vdpau,ffodivxvdpau,");

            } else if (g_ascii_strncasecmp(player->vo, "vvapi", strlen("vvapi")) == 0) {
                argv[argn++] = g_strdup_printf("%s,", player->vo);

            } else if (g_ascii_strncasecmp(player->vo, "xvmc", strlen("xvmc")) == 0) {
                argv[argn++] = g_strdup_printf("%s,", player->vo);

                argv[argn++] = g_strdup_printf("-vc");
                argv[argn++] = g_strdup_printf("ffmpeg12,");

            } else {
                argv[argn++] = g_strdup_printf("%s", player->vo);
                if (player->deinterlace) {
                    argv[argn++] = g_strdup_printf("-vf-pre");
                    argv[argn++] = g_strdup_printf("yadif,softskip,scale");
                }

                if (player->post_processing_level > 0) {
                    argv[argn++] = g_strdup_printf("-vf-add");
                    argv[argn++] = g_strdup_printf("pp=ac/tn:a");
                    argv[argn++] = g_strdup_printf("-autoq");
                    argv[argn++] = g_strdup_printf("%d", player->post_processing_level);
                }

                argv[argn++] = g_strdup_printf("-vf-add");
                argv[argn++] = g_strdup_printf("screenshot");
            }
        }
        if (player->ao != NULL) {
            argv[argn++] = g_strdup_printf("-ao");
            argv[argn++] = g_strdup_printf("%s", player->ao);

            if (player->hardware_ac3) {
                argv[argn++] = g_strdup_printf("-afm");
                argv[argn++] = g_strdup_printf("hwac3,");
            } else {
                argv[argn++] = g_strdup_printf("-af-add");
                argv[argn++] = g_strdup_printf("export=%s:512", player->af_export_filename);
            }

            if (player->alsa_mixer != NULL) {
                argv[argn++] = g_strdup_printf("-mixer-channel");
                argv[argn++] = g_strdup_printf("%s", player->alsa_mixer);
            }

            argv[argn++] = g_strdup_printf("-channels");
            switch (player->audio_channels) {
            case 1:
                argv[argn++] = g_strdup_printf("4");
                break;
            case 2:
                argv[argn++] = g_strdup_printf("6");
                break;
            case 3:
                argv[argn++] = g_strdup_printf("8");
                break;
            default:
                argv[argn++] = g_strdup_printf("2");
                break;
            }
        }
        argv[argn++] = g_strdup_printf("-quiet");
        argv[argn++] = g_strdup_printf("-slave");
        argv[argn++] = g_strdup_printf("-noidle");
        argv[argn++] = g_strdup_printf("-noconsolecontrols");
        argv[argn++] = g_strdup_printf("-identify");
        if (player->softvol) {
            if ((gint) (player->volume * 100) != 0) {
                argv[argn++] = g_strdup_printf("-volume");
                printf("volume = %f\n", player->volume);
                argv[argn++] = g_strdup_printf("%i", (gint) (player->volume * 100));
            }

            argv[argn++] = g_strdup_printf("-softvol");
        }

        if ((gint) (player->start_time) > 0) {
            argv[argn++] = g_strdup_printf("-ss");
            argv[argn++] = g_strdup_printf("%f", player->start_time);
        }

        if ((gint) (player->run_time) > 0) {
            argv[argn++] = g_strdup_printf("-endpos");
            argv[argn++] = g_strdup_printf("%f", player->run_time);
        }

        if (player->frame_drop)
            argv[argn++] = g_strdup_printf("-framedrop");

        argv[argn++] = g_strdup_printf("-osdlevel");
        argv[argn++] = g_strdup_printf("%i", player->osdlevel);

        argv[argn++] = g_strdup_printf("-delay");
        argv[argn++] = g_strdup_printf("%f", player->audio_delay);

        argv[argn++] = g_strdup_printf("-subdelay");
        argv[argn++] = g_strdup_printf("%f", player->subtitle_delay);

        argv[argn++] = g_strdup_printf("-subpos");
        argv[argn++] = g_strdup_printf("%i", player->subtitle_position);

        argv[argn++] = g_strdup_printf("-wid");
        gtk_widget_realize(player->socket);
        argv[argn++] = g_strdup_printf("0x%x", gtk_socket_get_id(GTK_SOCKET(player->socket)));

        argv[argn++] = g_strdup_printf("-brightness");
        argv[argn++] = g_strdup_printf("%i", player->brightness);
        argv[argn++] = g_strdup_printf("-contrast");
        argv[argn++] = g_strdup_printf("%i", player->contrast);
        //argv[argn++] = g_strdup_printf("-gamma");
        //argv[argn++] = g_strdup_printf("%i", player->gamma);
        argv[argn++] = g_strdup_printf("-hue");
        argv[argn++] = g_strdup_printf("%i", player->hue);
        argv[argn++] = g_strdup_printf("-saturation");
        argv[argn++] = g_strdup_printf("%i", player->saturation);

        /* disable msg stuff to make sure extra console characters don't mess around */
        argv[argn++] = g_strdup_printf("-nomsgcolor");
        argv[argn++] = g_strdup_printf("-nomsgmodule");

        if (player->audio_track_file != NULL && strlen(player->audio_track_file) > 0) {
            argv[argn++] = g_strdup_printf("-audiofile");
            argv[argn++] = g_strdup_printf("%s", player->audio_track_file);
        }

        if (player->subtitle_file != NULL && strlen(player->subtitle_file) > 0) {
            argv[argn++] = g_strdup_printf("-sub");
            argv[argn++] = g_strdup_printf("%s", player->subtitle_file);
        }
        // subtitle stuff
        if (player->enable_advanced_subtitles) {
            argv[argn++] = g_strdup_printf("-ass");

            if (player->subtitle_margin > 0) {
                argv[argn++] = g_strdup_printf("-ass-bottom-margin");
                argv[argn++] = g_strdup_printf("%i", player->subtitle_margin);
                argv[argn++] = g_strdup_printf("-ass-use-margins");
            }

            if (player->enable_embedded_fonts) {
                argv[argn++] = g_strdup_printf("-embeddedfonts");
            } else {
                argv[argn++] = g_strdup_printf("-noembeddedfonts");

                if (player->subtitle_font != NULL && strlen(player->subtitle_font) > 0) {
                    fontname = g_strdup(player->subtitle_font);
                    size = g_strrstr(fontname, " ");
                    size[0] = '\0';
                    size = g_strrstr(fontname, " Bold");
                    if (size)
                        size[0] = '\0';
                    size = g_strrstr(fontname, " Italic");
                    if (size)
                        size[0] = '\0';
                    argv[argn++] = g_strdup_printf("-ass-force-style");
                    argv[argn++] = g_strconcat("FontName=", fontname,
                                               ((g_strrstr(player->subtitle_font, "Italic") !=
                                                 NULL) ? ",Italic=1" : ",Italic=0"),
                                               ((g_strrstr(player->subtitle_font, "Bold") !=
                                                 NULL) ? ",Bold=1" : ",Bold=0"),
                                               (player->subtitle_outline ? ",Outline=1" : ",Outline=0"),
                                               (player->subtitle_shadow ? ",Shadow=2" : ",Shadow=0"), NULL);
                    g_free(fontname);
                }
            }

            argv[argn++] = g_strdup_printf("-ass-font-scale");
            argv[argn++] = g_strdup_printf("%1.2f", player->subtitle_scale);

            if (player->subtitle_color != NULL && strlen(player->subtitle_color) > 0) {
                argv[argn++] = g_strdup_printf("-ass-color");
                argv[argn++] = g_strdup_printf("%s", player->subtitle_color);
            }

        } else {
            if (player->subtitle_scale != 0) {
                argv[argn++] = g_strdup_printf("-subfont-text-scale");
                argv[argn++] = g_strdup_printf("%d", (int) (player->subtitle_scale * 5));       // 5 is the default
            }

            if (player->subtitle_font != NULL && strlen(player->subtitle_font) > 0) {
                fontname = g_strdup(player->subtitle_font);
                size = g_strrstr(fontname, " ");
                size[0] = '\0';
                argv[argn++] = g_strdup_printf("-subfont");
                argv[argn++] = g_strdup_printf("%s", fontname);
                g_free(fontname);
            }
        }

        if (player->subtitle_codepage != NULL && strlen(player->subtitle_codepage) > 0) {
            argv[argn++] = g_strdup_printf("-subcp");
            argv[argn++] = g_strdup_printf("%s", player->subtitle_codepage);
        }

        if (player->extra_opts != NULL) {
            char **opts = g_strsplit(player->extra_opts, " ", -1);
            int i;
            for (i = 0; opts[i] != NULL; i++)
                argv[argn++] = g_strdup(opts[i]);
            g_strfreev(opts);
        }

        switch (player->type) {
        case TYPE_FILE:
            if (filename != NULL) {
                if (g_strrstr(filename, "apple.com")) {
                    argv[argn++] = g_strdup_printf("-user-agent");
                    argv[argn++] = g_strdup_printf("QuickTime/7.6.4");
                }
                if (player->force_cache && player->cache_size >= 32) {
                    argv[argn++] = g_strdup_printf("-cache");
                    argv[argn++] = g_strdup_printf("%i", player->cache_size);
                }
                if (player->playlist) {
                    argv[argn++] = g_strdup_printf("-playlist");
                }
                argv[argn++] = g_strdup_printf("%s", filename);
                break;
            }
        case TYPE_CD:
            argv[argn++] = g_strdup_printf("-nocache");
            argv[argn++] = g_strdup_printf("%s", player->uri);
            if (player->media_device != NULL) {
                argv[argn++] = g_strdup_printf("-dvd-device");
                argv[argn++] = g_strdup_printf("%s", player->media_device);
            }
            break;

        case TYPE_DVD:
            argv[argn++] = g_strdup_printf("-mouse-movements");
            argv[argn++] = g_strdup_printf("-nocache");
            argv[argn++] = g_strdup_printf("dvdnav://");
            if (player->media_device != NULL) {
                argv[argn++] = g_strdup_printf("-dvd-device");
                argv[argn++] = g_strdup_printf("%s", player->media_device);
            }
            break;

        case TYPE_VCD:
            argv[argn++] = g_strdup_printf("-nocache");
            argv[argn++] = g_strdup_printf("vcd://");
            if (player->media_device != NULL) {
                argv[argn++] = g_strdup_printf("-dvd-device");
                argv[argn++] = g_strdup_printf("%s", player->media_device);
            }
            break;

        case TYPE_NETWORK:
            if (player->cache_size >= 32) {
                argv[argn++] = g_strdup_printf("-cache");
                argv[argn++] = g_strdup_printf("%i", (gint) player->cache_size);
            }
            if (player->playlist) {
                argv[argn++] = g_strdup_printf("-playlist");
            }
            argv[argn++] = g_strdup_printf("%s", player->uri);
            break;

        case TYPE_DVB:
        case TYPE_TV:
            if (player->tv_device != NULL) {
                argv[argn++] = g_strdup_printf("-tv:device");
                argv[argn++] = g_strdup_printf("%s", player->tv_device);
            }
            if (player->tv_driver != NULL) {
                argv[argn++] = g_strdup_printf("-tv:driver");
                argv[argn++] = g_strdup_printf("%s", player->tv_driver);
            }
            if (player->tv_input != NULL) {
                argv[argn++] = g_strdup_printf("-tv:input");
                argv[argn++] = g_strdup_printf("%s", player->tv_input);
            }
            if (player->tv_width > 0) {
                argv[argn++] = g_strdup_printf("-tv:width");
                argv[argn++] = g_strdup_printf("%i", player->tv_width);
            }
            if (player->tv_height > 0) {
                argv[argn++] = g_strdup_printf("-tv:height");
                argv[argn++] = g_strdup_printf("%i", player->tv_height);
            }
            if (player->tv_fps > 0) {
                argv[argn++] = g_strdup_printf("-tv:fps");
                argv[argn++] = g_strdup_printf("%i", player->tv_fps);
            }
            argv[argn++] = g_strdup_printf("-nocache");
            argv[argn++] = g_strdup_printf("%s", player->uri);


        default:
            break;
        }
        argv[argn] = NULL;

        if (player->debug) {
            for (i = 0; i < argn; i++) {
                printf("%s ", argv[i]);
            }
            printf("\n");
        }

        error = NULL;
        spawn =
            g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &pid,
                                     &(player->std_in), &(player->std_out), &(player->std_err), &error);
        if (error != NULL) {
            if (player->debug)
                printf("error code = %i - %s\n", error->code, error->message);
            g_error_free(error);
            error = NULL;
        }

        argn = 0;
        while (argv[argn] != NULL) {
            g_free(argv[argn]);
            argv[argn] = NULL;
            argn++;
        }

        if (spawn) {
            player->player_state = PLAYER_STATE_RUNNING;
            player->channel_in = g_io_channel_unix_new(player->std_in);
            player->channel_out = g_io_channel_unix_new(player->std_out);
            player->channel_err = g_io_channel_unix_new(player->std_err);

            g_io_channel_set_close_on_unref(player->channel_in, TRUE);
            g_io_channel_set_close_on_unref(player->channel_out, TRUE);
            g_io_channel_set_close_on_unref(player->channel_err, TRUE);
            player->watch_in_id =
                g_io_add_watch_full(player->channel_out, G_PRIORITY_LOW, G_IO_IN | G_IO_HUP, thread_reader, player,
                                    NULL);
            player->watch_err_id =
                g_io_add_watch_full(player->channel_err, G_PRIORITY_LOW, G_IO_IN | G_IO_ERR | G_IO_HUP,
                                    thread_reader_error, player, NULL);
            player->watch_in_hup_id =
                g_io_add_watch_full(player->channel_out, G_PRIORITY_LOW, G_IO_ERR | G_IO_HUP, thread_complete, player,
                                    NULL);

#ifdef GLIB2_14_ENABLED
            g_timeout_add_seconds(1, thread_query, player);
#else
            g_timeout_add(1000, thread_query, player);
#endif
            g_cond_wait(player->mplayer_complete_cond, player->thread_running);
        }

        if (player->cache_percent < 0.0 && g_str_has_prefix(player->uri, "mms"))
            player->playback_error = ERROR_RETRY_WITH_MMSHTTP;

        if (player->cache_percent < 0.0 && g_str_has_prefix(player->uri, "mmshttp"))
            player->playback_error = ERROR_RETRY_WITH_HTTP;

        switch (player->playback_error) {
        case ERROR_RETRY_WITH_PLAYLIST:
            player->playlist = TRUE;
            break;

        case ERROR_RETRY_WITH_HTTP:
            tmp = gmtk_media_player_switch_protocol(player->uri, "http");
            g_free(player->uri);
            player->uri = tmp;
            break;

        case ERROR_RETRY_WITH_MMSHTTP:
            tmp = gmtk_media_player_switch_protocol(player->uri, "http");
            g_free(player->uri);
            player->uri = tmp;
            break;

        default:
            break;
        }

    } while (player->playback_error != NO_ERROR);

    if (player->debug)
        printf("marking playback complete\n");
    player->player_state = PLAYER_STATE_DEAD;
    player->media_state = MEDIA_STATE_UNKNOWN;
    g_mutex_unlock(player->thread_running);
    player->mplayer_thread = NULL;
    player->start_time = 0.0;
    player->run_time = 0.0;
    if (player->restart) {
        g_signal_emit_by_name(player, "restart-shutdown-complete", NULL);
    } else {
        g_signal_emit_by_name(player, "position-changed", 0.0);
        g_signal_emit_by_name(player, "player-state-changed", player->player_state);
        g_signal_emit_by_name(player, "media-state-changed", player->media_state);
        gtk_widget_modify_bg(GTK_WIDGET(player), GTK_STATE_NORMAL, player->default_background);
        gtk_widget_modify_bg(GTK_WIDGET(player->socket), GTK_STATE_NORMAL, player->default_background);
    }

    return NULL;
}

gboolean thread_complete(GIOChannel * source, GIOCondition condition, gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(data);

    if (player->debug)
        printf("mplayer exiting\n");

    player->player_state = PLAYER_STATE_DEAD;
    player->media_state = MEDIA_STATE_UNKNOWN;
    g_source_remove(player->watch_in_id);
    g_source_remove(player->watch_err_id);
    g_cond_signal(player->mplayer_complete_cond);
    g_unlink(player->af_export_filename);
    gtk_widget_modify_bg(GTK_WIDGET(player), GTK_STATE_NORMAL, player->default_background);
    gtk_widget_modify_bg(GTK_WIDGET(player->socket), GTK_STATE_NORMAL, player->default_background);
    gtk_widget_hide(GTK_WIDGET(player->socket));

    return FALSE;
}

gboolean thread_reader_error(GIOChannel * source, GIOCondition condition, gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(data);
    GString *mplayer_output;
    GIOStatus status;
    GError *error = NULL;
    gchar *error_msg = NULL;
    GtkWidget *dialog;

    if (player == NULL) {
        printf("player is NULL\n");
        return FALSE;
    }

    if (source == NULL) {
        if (player->debug)
            printf("source is null\n");
        g_source_remove(player->watch_in_id);
        g_source_remove(player->watch_in_hup_id);
        return FALSE;
    }

    if (player->player_state == PLAYER_STATE_DEAD) {
        if (player->debug)
            printf("player is dead\n");
        return FALSE;
    }

    mplayer_output = g_string_new("");
    status = g_io_channel_read_line_string(source, mplayer_output, NULL, NULL);

    if (player->debug) {
        if (g_strrstr(mplayer_output->str, "ANS") == NULL)
            printf("ERROR: %s", mplayer_output->str);
    }

    if (strstr(mplayer_output->str, "Couldn't open DVD device") != 0) {
        error_msg = g_strdup(mplayer_output->str);
    }

    if (strstr(mplayer_output->str, "X11 error") != 0) {
        g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_SIZE);
    }

    if (strstr(mplayer_output->str, "signal") != NULL) {
        error_msg = g_strdup(mplayer_output->str);
    }

    if (strstr(mplayer_output->str, "Failed to open") != NULL) {
        if (strstr(mplayer_output->str, "LIRC") == NULL &&
            strstr(mplayer_output->str, "/dev/rtc") == NULL &&
            strstr(mplayer_output->str, "VDPAU") == NULL && strstr(mplayer_output->str, "registry file") == NULL) {
            if (strstr(mplayer_output->str, "<") == NULL && strstr(mplayer_output->str, ">") == NULL
                && player->type != TYPE_NETWORK) {
                error_msg = g_strdup_printf(_("Failed to open %s"), mplayer_output->str + strlen("Failed to open "));
            }

            if (strstr(mplayer_output->str, "mms://") != NULL && player->type == TYPE_NETWORK) {
                player->playback_error = ERROR_RETRY_WITH_MMSHTTP;
            }
        }
    }

    if (strstr(mplayer_output->str, "No stream found to handle url mmshttp://") != NULL) {
        player->playback_error = ERROR_RETRY_WITH_HTTP;
    }

    if (strstr(mplayer_output->str, "Server returned 404:File Not Found") != NULL
        && g_strrstr(player->uri, "mmshttp://") != NULL) {
        player->playback_error = ERROR_RETRY_WITH_HTTP;
    }

    if (strstr(mplayer_output->str, "unknown ASF streaming type") != NULL
        && g_strrstr(player->uri, "mmshttp://") != NULL) {
        player->playback_error = ERROR_RETRY_WITH_HTTP;
    }

    if (strstr(mplayer_output->str, "Error while parsing chunk header") != NULL) {
        player->playback_error = ERROR_RETRY_WITH_HTTP;
    }

    if (strstr(mplayer_output->str, "Failed to initiate \"video/X-ASF-PF\" RTP subsession") != NULL) {
        player->playback_error = ERROR_RETRY_WITH_PLAYLIST;
    }

    if (strstr(mplayer_output->str, "playlist support will not be used") != NULL) {
        player->playback_error = ERROR_RETRY_WITH_PLAYLIST;
    }

    if (strstr(mplayer_output->str, "Compressed SWF format not supported") != NULL) {
        error_msg = g_strdup_printf(_("Compressed SWF format not supported"));
    }

    if (error_msg != NULL) {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE, "%s", error_msg);
        gtk_window_set_title(GTK_WINDOW(dialog), _("GNOME MPlayer Error"));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(error_msg);
    }

    g_string_free(mplayer_output, TRUE);

    return TRUE;
}

gboolean thread_reader(GIOChannel * source, GIOCondition condition, gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(data);
    GString *mplayer_output;
    GIOStatus status;
    GError *error = NULL;
    gchar *buf, *message = NULL, *icy = NULL;
    gint w, h, i;
    gfloat percent, oldposition;
    gchar vm[10];
    gint id;
    GtkAllocation allocation;
    GmtkMediaPlayerSubtitle *subtitle;
    GmtkMediaPlayerAudioTrack *audio_track;
    GList *iter;
    GtkStyle *style;
    GtkWidget *dialog;

    if (player == NULL) {
        printf("player is NULL\n");
        return FALSE;
    }

    if (source == NULL) {
        if (player->debug)
            printf("source is null\n");
        g_source_remove(player->watch_in_id);
        g_source_remove(player->watch_in_hup_id);
        return FALSE;
    }

    if (player->player_state == PLAYER_STATE_DEAD) {
        if (player->debug)
            printf("player is dead\n");
        return FALSE;
    }

    mplayer_output = g_string_new("");

    status = g_io_channel_read_line_string(source, mplayer_output, NULL, &error);
    if (status == G_IO_STATUS_ERROR) {
        if (player->debug)
            printf("GIO IO Error\n");
        return FALSE;
    } else {
        if (player->debug) {
            if (g_strrstr(mplayer_output->str, "ANS") == NULL)
                printf("%s", mplayer_output->str);
        }

        if (strstr(mplayer_output->str, "Cache fill") != 0) {
            buf = strstr(mplayer_output->str, "Cache fill");
            sscanf(buf, "Cache fill: %f%%", &percent);
            buf = g_strdup_printf(_("Cache fill: %2.2f%%"), percent);
            player->cache_percent = percent / 100.0;
            g_signal_emit_by_name(player, "position-changed", player->cache_percent);
        }

        if (strstr(mplayer_output->str, "AO:") != NULL) {
            write_to_mplayer(player, "get_property switch_audio\n");
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_AF_EXPORT_FILENAME);
        }

        if (strstr(mplayer_output->str, "VO:") != NULL) {
            buf = strstr(mplayer_output->str, "VO:");
            sscanf(buf, "VO: [%9[^]]] %ix%i => %ix%i", vm, &w, &h, &(player->video_width), &(player->video_height));
            get_allocation(GTK_WIDGET(player), &allocation);
            if (player->restart) {
                g_signal_emit_by_name(player, "restart-complete", NULL);
            } else {
                g_signal_emit_by_name(player->socket, "size_allocate", &allocation);
            }

            gmtk_media_player_size_allocate(GTK_WIDGET(player), &allocation);

            player->video_present = TRUE;
            style = gtk_widget_get_style(GTK_WIDGET(player));
            gtk_widget_modify_bg(GTK_WIDGET(player), GTK_STATE_NORMAL, &(style->black));
            gtk_widget_modify_bg(GTK_WIDGET(player->socket), GTK_STATE_NORMAL, &(style->black));
            gtk_widget_show(GTK_WIDGET(player->socket));
            write_to_mplayer(player, "get_property sub_source\n");
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_SIZE);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_VIDEO_PRESENT);
            g_signal_emit_by_name(player, "subtitles-changed", g_list_length(player->subtitles));
            g_signal_emit_by_name(player, "audio-tracks-changed", g_list_length(player->audio_tracks));
        }

        if (strstr(mplayer_output->str, "Video: no video") != NULL) {
            player->video_width = 0;
            player->video_height = 0;
            gtk_widget_get_allocation(GTK_WIDGET(player), &allocation);
            if (player->restart) {
                g_signal_emit_by_name(player, "restart-complete", NULL);
            } else {
                g_signal_emit_by_name(player->socket, "size_allocate", &allocation);
            }

            player->video_present = FALSE;
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_SIZE);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_VIDEO_PRESENT);
            g_signal_emit_by_name(player, "subtitles-changed", g_list_length(player->subtitles));
            g_signal_emit_by_name(player, "audio-tracks-changed", g_list_length(player->audio_tracks));
        }

        if (strstr(mplayer_output->str, "ANS_TIME_POSITION") != 0) {
            buf = strstr(mplayer_output->str, "ANS_TIME_POSITION");
            oldposition = player->position;
            sscanf(buf, "ANS_TIME_POSITION=%lf", &player->position);
            if (oldposition != player->position)
                g_signal_emit_by_name(player, "position-changed", player->position);
        }

        if (strstr(mplayer_output->str, "ID_START_TIME") != 0) {
            buf = strstr(mplayer_output->str, "ID_START_TIME");
            sscanf(buf, "ID_START_TIME=%lf", &player->start_time);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_START_TIME);
        }

        if (strstr(mplayer_output->str, "ID_LENGTH") != 0) {
            buf = strstr(mplayer_output->str, "ID_LENGTH");
            sscanf(buf, "ID_LENGTH=%lf", &player->length);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_LENGTH);
        }

        if (strstr(mplayer_output->str, "ANS_LENGTH") != 0) {
            buf = strstr(mplayer_output->str, "ANS_LENGTH");
            sscanf(buf, "ANS_LENGTH=%lf", &player->length);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_LENGTH);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_TRACK") != 0) {
            buf = strstr(mplayer_output->str, "ID_AUDIO_TRACK");
            sscanf(buf, "ID_AUDIO_TRACK=%i", &player->audio_track_id);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_AUDIO_TRACK);
        }

        if (strstr(mplayer_output->str, "ANS_switch_audio") != 0) {
            buf = strstr(mplayer_output->str, "ANS_switch_audio");
            sscanf(buf, "ANS_switch_audio=%i", &player->audio_track_id);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_AUDIO_TRACK);
        }

        if (strstr(mplayer_output->str, "ANS_sub_source") != 0) {
            buf = strstr(mplayer_output->str, "ANS_sub_source");
            sscanf(buf, "ANS_sub_source=%i", &player->subtitle_source);
            switch (player->subtitle_source) {
            case 0:
                player->subtitle_is_file = TRUE;
                write_to_mplayer(player, "get_property sub_file\n");
                break;
            case 1:
                player->subtitle_is_file = FALSE;
                write_to_mplayer(player, "get_property sub_vob\n");
                break;
            case 2:
                player->subtitle_is_file = FALSE;
                write_to_mplayer(player, "get_property sub_demux\n");
                break;
            }
        }

        if (strstr(mplayer_output->str, "ANS_sub_file") != 0) {
            buf = strstr(mplayer_output->str, "ANS_sub_file");
            sscanf(buf, "ANS_sub_file=%i", &player->subtitle_id);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_SUBTITLE);
        }

        if (strstr(mplayer_output->str, "ANS_sub_demux") != 0) {
            buf = strstr(mplayer_output->str, "ANS_sub_demux");
            sscanf(buf, "ANS_sub_demux=%i", &player->subtitle_id);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_SUBTITLE);
        }


        if (strstr(mplayer_output->str, "DVDNAV_TITLE_IS_MENU") != 0) {
            player->title_is_menu = TRUE;
            write_to_mplayer(player, "get_time_length\n");
        }

        if (strstr(mplayer_output->str, "DVDNAV_TITLE_IS_MOVIE") != 0) {
            player->title_is_menu = FALSE;
            write_to_mplayer(player, "get_time_length\n");
        }

        if (strstr(mplayer_output->str, "ID_SUBTITLE_ID=") != 0) {
            buf = strstr(mplayer_output->str, "ID_SUBTITLE_ID");
            sscanf(buf, "ID_SUBTITLE_ID=%i", &id);
            subtitle = g_new0(GmtkMediaPlayerSubtitle, 1);
            subtitle->id = id;
            subtitle->lang = g_strdup_printf(_("Unknown"));
            subtitle->name = g_strdup_printf(_("Unknown"));
            subtitle->label = g_strdup_printf("%s (%s)", subtitle->name, subtitle->lang);
            player->subtitles = g_list_append(player->subtitles, subtitle);

        }

        if (strstr(mplayer_output->str, "ID_SID_") != 0) {
            buf = strstr(mplayer_output->str, "ID_SID_");
            sscanf(buf, "ID_SID_%i_", &id);
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "_LANG=");
            if (buf != NULL) {
                buf += strlen("_LANG=");
                iter = player->subtitles;
                while (iter) {
                    subtitle = ((GmtkMediaPlayerSubtitle *) (iter->data));
                    if (subtitle->id == id && subtitle->is_file == FALSE) {
                        if (subtitle->lang != NULL) {
                            g_free(subtitle->lang);
                            subtitle->lang = NULL;
                        }
                        subtitle->lang = g_strdup(buf);
                    }
                    iter = iter->next;
                }
            }
            buf = strstr(mplayer_output->str, "_NAME=");
            if (buf != NULL) {
                buf += strlen("_NAME=");
                iter = player->subtitles;
                while (iter) {
                    subtitle = ((GmtkMediaPlayerSubtitle *) (iter->data));
                    if (subtitle->id == id && subtitle->is_file == FALSE) {
                        if (subtitle->name != NULL) {
                            g_free(subtitle->name);
                            subtitle->name = NULL;
                        }
                        subtitle->name = g_strdup(buf);
                    }
                    iter = iter->next;
                }
            }
            if (subtitle->label != NULL) {
                g_free(subtitle->label);
                subtitle->label = NULL;
            }
            subtitle->label =
                g_strdup_printf("%s (%s)", (subtitle->name) ? subtitle->name : _("Unknown"), subtitle->lang);
        }

        if (strstr(mplayer_output->str, "ID_FILE_SUB_ID=") != 0) {
            buf = strstr(mplayer_output->str, "ID_FILE_SUB_ID");
            sscanf(buf, "ID_FILE_SUB_ID=%i", &id);
            subtitle = g_new0(GmtkMediaPlayerSubtitle, 1);
            subtitle->id = id;
            subtitle->is_file = TRUE;
            subtitle->label = g_strdup_printf(_("External Subtitle #%i"), id + 1);
            player->subtitles = g_list_append(player->subtitles, subtitle);
            g_signal_emit_by_name(player, "subtitles-changed", g_list_length(player->subtitles));
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_ID=") != 0) {
            buf = strstr(mplayer_output->str, "ID_AUDIO_ID");
            sscanf(buf, "ID_AUDIO_ID=%i", &id);
            iter = player->audio_tracks;
            gboolean found = FALSE;
            while (iter) {
                audio_track = ((GmtkMediaPlayerAudioTrack *) (iter->data));
                if (audio_track->id == id) {
                    found = TRUE;
                }
                iter = iter->next;
            }

            if (!found) {
                audio_track = g_new0(GmtkMediaPlayerAudioTrack, 1);
                audio_track->id = id;
                audio_track->lang = g_strdup_printf(_("Unknown"));
                audio_track->name = g_strdup_printf(_("Unknown"));
                audio_track->label = g_strdup_printf("%s (%s)", audio_track->name, audio_track->lang);
                player->audio_tracks = g_list_append(player->audio_tracks, audio_track);
            }
        }

        if (strstr(mplayer_output->str, "ID_AID_") != 0) {
            buf = strstr(mplayer_output->str, "ID_AID_");
            sscanf(buf, "ID_AID_%i_", &id);
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "_LANG=");
            if (buf != NULL) {
                buf += strlen("_LANG=");
                iter = player->audio_tracks;
                gboolean updated = FALSE;
                while (iter) {
                    audio_track = ((GmtkMediaPlayerAudioTrack *) (iter->data));
                    if (audio_track->id == id) {
                        updated = TRUE;
                        if (audio_track->lang != NULL) {
                            g_free(audio_track->lang);
                            audio_track->lang = NULL;
                        }
                        audio_track->lang = g_strdup(buf);
                    }
                    iter = iter->next;
                }
                if (updated == FALSE) {
                    audio_track = g_new0(GmtkMediaPlayerAudioTrack, 1);
                    audio_track->id = id;
                    audio_track->lang = g_strdup_printf("%s", buf);
                    audio_track->name = g_strdup_printf(_("Unknown"));
                    audio_track->label = g_strdup_printf("%s (%s)", audio_track->name, audio_track->lang);
                    player->audio_tracks = g_list_append(player->audio_tracks, audio_track);
                    g_signal_emit_by_name(player, "audio-tracks-changed", g_list_length(player->audio_tracks));
                }
            }
            buf = strstr(mplayer_output->str, "_NAME=");
            if (buf != NULL) {
                buf += strlen("_NAME=");
                iter = player->audio_tracks;
                while (iter) {
                    audio_track = ((GmtkMediaPlayerAudioTrack *) (iter->data));
                    if (audio_track->id == id) {
                        if (audio_track->name != NULL) {
                            g_free(audio_track->name);
                            audio_track->name = NULL;
                        }
                        audio_track->name = g_strdup(buf);
                    }
                    iter = iter->next;
                }
            }

            if (audio_track->label != NULL) {
                g_free(audio_track->label);
                audio_track->label = NULL;
            }
            audio_track->label =
                g_strdup_printf("%s (%s)", (audio_track->name) ? audio_track->name : _("Unknown"), audio_track->lang);

        }

        if ((strstr(mplayer_output->str, "ID_CHAPTERS=") != NULL)) {
            buf = strstr(mplayer_output->str, "ID_CHAPTERS");
            sscanf(buf, "ID_CHAPTERS=%i", &player->chapters);
            if (player->chapters > 0) {
                player->has_chapters = TRUE;
            } else {
                player->has_chapters = FALSE;
            }
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_HAS_CHAPTERS);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_CHAPTERS);
        }

        if ((strstr(mplayer_output->str, "ID_SEEKABLE=") != NULL)
            && !(strstr(mplayer_output->str, "ID_SEEKABLE=0") != NULL)) {
            player->seekable = TRUE;
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_SEEKABLE);
        }

        if (strstr(mplayer_output->str, "ID_VIDEO_FORMAT") != 0) {
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "ID_VIDEO_FORMAT") + strlen("ID_VIDEO_FORMAT=");
            if (player->video_format != NULL) {
                g_free(player->video_format);
                player->video_format = NULL;
            }
            player->video_format = g_strdup(buf);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_VIDEO_FORMAT);
        }

        if (strstr(mplayer_output->str, "ID_VIDEO_CODEC") != 0) {
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "ID_VIDEO_CODEC") + strlen("ID_VIDEO_CODEC=");
            if (player->video_codec != NULL) {
                g_free(player->video_codec);
                player->video_codec = NULL;
            }
            player->video_codec = g_strdup(buf);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_VIDEO_CODEC);
        }

        if (strstr(mplayer_output->str, "ID_VIDEO_FPS") != 0) {
            buf = strstr(mplayer_output->str, "ID_VIDEO_FPS");
            sscanf(buf, "ID_VIDEO_FPS=%lf", &player->video_fps);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_VIDEO_FPS);
        }

        if (strstr(mplayer_output->str, "ID_VIDEO_BITRATE") != 0) {
            buf = strstr(mplayer_output->str, "ID_VIDEO_BITRATE");
            sscanf(buf, "ID_VIDEO_BITRATE=%i", &player->video_bitrate);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_VIDEO_BITRATE);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_FORMAT") != 0) {
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "ID_AUDIO_FORMAT") + strlen("ID_AUDIO_FORMAT=");
            if (player->audio_format != NULL) {
                g_free(player->audio_format);
                player->audio_format = NULL;
            }
            player->audio_format = g_strdup(buf);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_AUDIO_FORMAT);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_CODEC") != 0) {
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "ID_AUDIO_CODEC") + strlen("ID_AUDIO_CODEC=");
            if (player->audio_codec != NULL) {
                g_free(player->audio_codec);
                player->audio_codec = NULL;
            }
            player->audio_codec = g_strdup(buf);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_AUDIO_CODEC);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_BITRATE") != 0) {
            buf = strstr(mplayer_output->str, "ID_AUDIO_BITRATE");
            sscanf(buf, "ID_AUDIO_BITRATE=%i", &player->audio_bitrate);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_AUDIO_BITRATE);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_RATE") != 0) {
            buf = strstr(mplayer_output->str, "ID_AUDIO_RATE");
            sscanf(buf, "ID_AUDIO_RATE=%i", &player->audio_rate);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_AUDIO_RATE);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_NCH") != 0) {
            buf = strstr(mplayer_output->str, "ID_AUDIO_NCH");
            sscanf(buf, "ID_AUDIO_NCH=%i", &player->audio_nch);
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_AUDIO_NCH);
        }

        if (strstr(mplayer_output->str, "*** screenshot") != 0) {
            buf = strstr(mplayer_output->str, "'") + 1;
            buf[12] = '\0';
            message = g_strdup_printf(_("Screenshot saved to '%s'"), buf);
            dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
                                            GTK_BUTTONS_OK, "%s", message);
            gtk_window_set_title(GTK_WINDOW(dialog), _("GNOME MPlayer Notification"));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            g_free(message);
            message = NULL;
        }

        if (player->minimum_mplayer == FALSE) {
            message = g_strdup_printf(_("MPlayer should be Upgraded to a Newer Version"));
            dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
                                            GTK_BUTTONS_OK, "%s", message);
            gtk_window_set_title(GTK_WINDOW(dialog), _("GNOME MPlayer Notification"));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            g_free(message);
            message = NULL;
            player->minimum_mplayer = TRUE;
        }

        if (strstr(mplayer_output->str, "ICY Info") != NULL) {
            buf = strstr(mplayer_output->str, "'");
            if (message) {
                g_free(message);
                message = NULL;
            }
            if (buf != NULL) {
                for (i = 1; i < (int) strlen(buf) - 1; i++) {
                    if (!strncmp(&buf[i], "\';", 2)) {
                        buf[i] = '\0';
                        break;
                    }
                }
                if (g_ascii_strcasecmp(buf + 1, " - ") != 0) {
                    if (g_utf8_validate(buf + 1, strlen(buf + 1), 0))
                        message = g_markup_printf_escaped("<small>\n\t<big><b>%s</b></big>\n</small>", buf + 1);
                }
            }
            if (message) {
                // reset max values in audio meter
                g_free(message);
                message = g_markup_printf_escaped("\n\t<b>%s</b>\n", buf + 1);
                icy = g_strdup(buf + 1);
                if ((buf = strstr(icy, " - ")) != NULL) {
                    //metadata->title = g_strdup(buf + 3);
                    //buf[0] = '\0';
                    //metadata->artist = g_strdup(icy);
                }
                g_free(icy);
                g_free(message);
                message = NULL;
            }
        }
    }

    g_string_free(mplayer_output, TRUE);
    return TRUE;
}

gboolean thread_query(gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(data);
    gint written;

    //printf("in thread_query, data = %p\n", data);
    if (player == NULL) {
        return FALSE;
    }

    if (player->player_state == PLAYER_STATE_RUNNING) {
        if (player->media_state == MEDIA_STATE_PLAY) {
            //printf("writing\n");
            if (player->use_mplayer2) {
                written = write(player->std_in, "get_time_pos\n", strlen("get_time_pos\n"));
            } else {
                written =
                    write(player->std_in, "pausing_keep_force get_time_pos\n",
                          strlen("pausing_keep_force get_time_pos\n"));
            }
            //printf("written = %i\n", written);
            if (written == -1) {
                //return TRUE;
                return FALSE;
            } else {
                return TRUE;
            }
        } else {
            if (player->media_state == MEDIA_STATE_UNKNOWN || player->media_state == MEDIA_STATE_QUIT) {
                return FALSE;
            } else {
                return TRUE;
            }
        }
    } else {
        return FALSE;
    }
}

gboolean write_to_mplayer(GmtkMediaPlayer * player, const gchar * cmd)
{
    GIOStatus result;
    gsize bytes_written;
    gchar *pkf_cmd;

    //printf("write to mplayer = %s\n", cmd);

    if (player->channel_in) {
        if (player->use_mplayer2) {
            pkf_cmd = g_strdup(cmd);
        } else {
            if (g_strncasecmp(cmd, "pause", strlen("pause")) == 0) {
                pkf_cmd = g_strdup(cmd);
            } else {
                pkf_cmd = g_strdup_printf("pausing_keep_force %s", cmd);
            }
        }
        result = g_io_channel_write_chars(player->channel_in, pkf_cmd, -1, &bytes_written, NULL);
        g_free(pkf_cmd);
        if (result != G_IO_STATUS_ERROR && bytes_written > 0) {
            result = g_io_channel_flush(player->channel_in, NULL);
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }

}

gboolean detect_mplayer_features(GmtkMediaPlayer * player)
{
    gchar *av[255];
    gint ac = 0, i;
    gchar **output;
    GError *error;
    gint exit_status;
    gchar *out = NULL;
    gchar *err = NULL;
    gboolean ret = TRUE;

    if (player->features_detected)
        return ret;

    if (player->mplayer_binary == NULL || !g_file_test(player->mplayer_binary, G_FILE_TEST_EXISTS)) {
        av[ac++] = g_strdup_printf("mplayer");
    } else {
        av[ac++] = g_strdup_printf("%s", player->mplayer_binary);
    }
    av[ac++] = g_strdup_printf("-noidle");
    av[ac++] = g_strdup_printf("-softvol");
    av[ac++] = g_strdup_printf("-volume");
    av[ac++] = g_strdup_printf("100");
    // enable these lines to force newer mplayer
    //av[ac++] = g_strdup_printf("-gamma");
    //av[ac++] = g_strdup_printf("0");

    av[ac] = NULL;

    error = NULL;

    g_spawn_sync(NULL, av, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &err, &exit_status, &error);

    for (i = 0; i < ac; i++) {
        g_free(av[i]);
    }

    if (error != NULL) {
        printf("Error when running: %s\n", error->message);
        g_error_free(error);
        error = NULL;
        if (out != NULL)
            g_free(out);
        if (err != NULL)
            g_free(err);
        return FALSE;
    }
    output = g_strsplit(err, "\n", 0);
    ac = 0;
    while (output[ac] != NULL) {
        if (g_ascii_strncasecmp(output[ac], "Unknown option", strlen("Unknown option")) == 0) {
            ret = FALSE;
        }
        if (g_ascii_strncasecmp(output[ac], "MPlayer2", strlen("MPlayer2")) == 0) {
            player->use_mplayer2 = TRUE;
        }
        ac++;
    }
    g_strfreev(output);
    g_free(out);
    g_free(err);

    player->features_detected = TRUE;
    if (!ret) {
        printf(_("You might want to consider upgrading mplayer to a newer version\n"));
    }
    return ret;
}
