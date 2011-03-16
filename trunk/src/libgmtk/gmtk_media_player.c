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

G_DEFINE_TYPE(GmtkMediaPlayer, gmtk_media_player, GTK_TYPE_FIXED);
static GObjectClass *parent_class = NULL;

static void gmtk_media_player_dispose(GObject * object);
static gboolean gmtk_media_player_expose_event(GtkWidget * widget, GdkEventExpose * event);
static void gmtk_media_player_size_allocate(GtkWidget * widget, GtkAllocation * allocation);
static gboolean socket_size_allocate_callback(GtkWidget * widget, GtkAllocation * allocation, gpointer data);
static gboolean player_key_press_event_callback(GtkWidget * widget, GdkEventKey * event, gpointer data);
static gboolean player_button_press_event_callback(GtkWidget * widget, GdkEventButton * event, gpointer data);
static gboolean player_motion_notify_event_callback(GtkWidget * widget, GdkEventMotion * event, gpointer data);
static void gmtk_media_player_restart_complete_callback(GmtkMediaPlayer * player, gpointer data);

// monitoring functions
gpointer launch_mplayer(gpointer data);
gboolean thread_reader_error(GIOChannel * source, GIOCondition condition, gpointer data);
gboolean thread_reader(GIOChannel * source, GIOCondition condition, gpointer data);
gboolean thread_query(gpointer data);
gboolean thread_complete(GIOChannel * source, GIOCondition condition, gpointer data);

gboolean write_to_mplayer(GmtkMediaPlayer * player, const gchar * cmd);

static void gmtk_media_player_class_init(GmtkMediaPlayerClass * class)
{
    GtkWidgetClass *widget_class;
    GtkObjectClass *object_class;

    object_class = (GtkObjectClass *) class;
    widget_class = GTK_WIDGET_CLASS(class);

    parent_class = g_type_class_peek_parent(class);
    G_OBJECT_CLASS(class)->dispose = gmtk_media_player_dispose;
    widget_class->expose_event = gmtk_media_player_expose_event;
    widget_class->size_allocate = gmtk_media_player_size_allocate;

    g_signal_new("position-changed",
                 G_OBJECT_CLASS_TYPE(object_class),
                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(GmtkMediaPlayerClass, position_changed),
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


    gtk_widget_set_size_request(GTK_WIDGET(player), 320, 200);
    gtk_widget_set_has_window(GTK_WIDGET(player), TRUE);
    //gtk_widget_set_can_focus(GTK_WIDGET(player), TRUE);
    //gtk_widget_set_can_default(GTK_WIDGET(player), TRUE);

    g_signal_connect(player, "key_press_event", G_CALLBACK(player_key_press_event_callback), NULL);
    g_signal_connect(player, "motion_notify_event", G_CALLBACK(player_motion_notify_event_callback), NULL);
    g_signal_connect(player, "button_press_event", G_CALLBACK(player_button_press_event_callback), NULL);

    style = gtk_widget_get_style(GTK_WIDGET(player));

    gtk_widget_modify_bg(GTK_WIDGET(player), GTK_STATE_NORMAL, &(style->black));
    gtk_widget_push_composite_child();

    player->socket = gtk_socket_new();
    //gtk_widget_add_events(GTK_WIDGET(player->socket),
    //                      GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
    //                      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
    //                      GDK_POINTER_MOTION_MASK | 
    //                      GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK);
    gtk_widget_set_size_request(player->socket, 320, 200);
    gtk_widget_set_has_window(GTK_WIDGET(player->socket), TRUE);
    gtk_widget_set_can_focus(GTK_WIDGET(player->socket), TRUE);
    gtk_widget_set_can_default(GTK_WIDGET(player->socket), TRUE);
    gtk_widget_activate(GTK_WIDGET(player->socket));
    gtk_widget_modify_bg(GTK_WIDGET(player->socket), GTK_STATE_NORMAL, &(style->black));

    gtk_fixed_put(GTK_FIXED(player), player->socket, 0, 0);
    g_signal_connect_swapped(player->socket, "size-allocate", G_CALLBACK(socket_size_allocate_callback), player);
    g_signal_connect(player->socket, "key_press_event", G_CALLBACK(player_key_press_event_callback), player);

    g_signal_connect(player, "restart-complete", G_CALLBACK(gmtk_media_player_restart_complete_callback), NULL);
    gtk_widget_pop_composite_child();

    gtk_widget_show_all(GTK_WIDGET(player));
    player->player_state = PLAYER_STATE_DEAD;
    player->media_state = MEDIA_STATE_UNKNOWN;
    player->uri = NULL;
    player->mplayer_thread = NULL;
    player->aspect_ratio = ASPECT_DEFAULT;
    player->mplayer_complete_cond = g_cond_new();
    player->thread_running = g_mutex_new();
    player->video_width = 0;
    player->video_height = 0;
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
    player->saturation = 0;
    player->restart = FALSE;
    player->video_format = NULL;
    player->video_codec = NULL;
    player->audio_format = NULL;
    player->audio_codec = NULL;
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

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static gboolean gmtk_media_player_expose_event(GtkWidget * widget, GdkEventExpose * event)
{
    return FALSE;
}

static gboolean socket_size_allocate_callback(GtkWidget * widget, GtkAllocation * allocation, gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(widget);

    gtk_fixed_move(GTK_FIXED(player), player->socket, player->top, player->left);
    return FALSE;
}

gboolean gmtk_media_player_send_key_press_event(GmtkMediaPlayer * widget, GdkEventKey * event, gpointer data)
{
    return player_key_press_event_callback(GTK_WIDGET(widget), event, data);
}



static gboolean player_key_press_event_callback(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
    GmtkMediaPlayer *player;

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
        case GDK_KP_Add:
            write_to_mplayer(player, "pausing_keep_force audio_delay 0.1 0\n");
            break;
        case GDK_minus:
        case GDK_KP_Subtract:
            write_to_mplayer(player, "pausing_keep_force audio_delay -0.1 0\n");
            break;
        case GDK_numbersign:
            write_to_mplayer(player, "pausing_keep_force switch_audio -1\n");
            return TRUE;
            break;
        case GDK_period:
            if (player->media_state == MEDIA_STATE_PAUSE)
                write_to_mplayer(player, "frame_step\n");
            break;
        case GDK_j:
            write_to_mplayer(player, "pausing_keep_force sub_select\n");
            break;
        case GDK_d:
            write_to_mplayer(player,
                             "pausing_keep_force frame_drop\npausing_keep_force osd_show_property_text \"framedropping: ${framedropping}\"\n");
            break;
        case GDK_b:
            write_to_mplayer(player, "pausing_keep_force sub_pos -1 0\n");
            break;
        case GDK_B:
            write_to_mplayer(player, "pausing_keep_force sub_pos 1 0\n");
            break;
        case GDK_s:
        case GDK_S:
            write_to_mplayer(player, "pausing_keep_force screenshot 0\n");
            break;
        default:
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

    if (player->video_width == 0 || player->video_height == 0) {
        gtk_widget_set_size_request(player->socket, allocation->width, allocation->height);
        player->top = 0;
        player->left = 0;
    } else {
        video_aspect = (gdouble) player->video_width / (gdouble) player->video_height;
        widget_aspect = (gdouble) allocation->width / (gdouble) allocation->height;

        if (video_aspect > widget_aspect) {
            da_width = allocation->width;
            da_height = floorf((allocation->width / video_aspect) + 0.5);
        } else {
            da_height = allocation->height;
            da_width = floorf((allocation->height * video_aspect) + 0.5);
        }
        gtk_widget_set_size_request(player->socket, da_width, da_height);
        player->top = (allocation->width - da_width) / 2;
        player->left = (allocation->height - da_height) / 2;
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
    g_signal_emit_by_name(player, "position-changed", player->restart_position);
    gmtk_media_player_set_state(GMTK_MEDIA_PLAYER(player), player->restart_state);
    player->restart = FALSE;
    printf("restart complete\n");
}

void gmtk_media_player_restart(GmtkMediaPlayer * player)
{
    player->restart = TRUE;
    player->restart_state = gmtk_media_player_get_state(player);
    gmtk_media_player_set_state(player, MEDIA_STATE_PAUSE);
    player->restart_position = player->position;
    gmtk_media_player_set_state(GMTK_MEDIA_PLAYER(player), MEDIA_STATE_QUIT);
    gmtk_media_player_set_state(GMTK_MEDIA_PLAYER(player), MEDIA_STATE_PLAY);

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

gchar *gmtk_media_player_get_uri(GmtkMediaPlayer * player)
{
    return g_strdup(player->uri);
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
                player->player_state = PLAYER_STATE_RUNNING;
                g_signal_emit_by_name(player, "player-state-changed", player->player_state);
                player->media_state = MEDIA_STATE_PLAY;
                g_signal_emit_by_name(player, "media-state-changed", player->media_state);
            }
        }

    }

    if (player->player_state == PLAYER_STATE_RUNNING) {
        if (new_state == MEDIA_STATE_STOP) {
            write_to_mplayer(player, "pausing_keep_force seek 0 2\n");
            if (player->media_state == MEDIA_STATE_PLAY) {
                write_to_mplayer(player, "pause\n");
            }
            player->media_state = MEDIA_STATE_STOP;
            g_signal_emit_by_name(player, "position-changed", 0.0);
            g_signal_emit_by_name(player, "media-state-changed", player->media_state);
        }

        if (new_state == MEDIA_STATE_PLAY) {
            if (player->media_state == MEDIA_STATE_PAUSE || player->media_state == MEDIA_STATE_STOP) {
                write_to_mplayer(player, "pause\n");
                player->media_state = MEDIA_STATE_PLAY;
                g_signal_emit_by_name(player, "media-state-changed", player->media_state);
            }
            if (player->media_state == MEDIA_STATE_UNKNOWN) {
                player->media_state = MEDIA_STATE_PLAY;
                g_signal_emit_by_name(player, "media-state-changed", player->media_state);
            }
        }

        if (new_state == MEDIA_STATE_PAUSE) {
            if (player->media_state == MEDIA_STATE_PLAY) {
                write_to_mplayer(player, "pause\n");
                player->media_state = MEDIA_STATE_PAUSE;
                g_signal_emit_by_name(player, "media-state-changed", player->media_state);
            }
        }

        if (new_state == MEDIA_STATE_QUIT) {
            write_to_mplayer(player, "quit\n");
            while (player->player_state != PLAYER_STATE_DEAD) {
                while (gtk_events_pending())
                    gtk_main_iteration();
            }
            player->media_state = MEDIA_STATE_QUIT;
            g_signal_emit_by_name(player, "media-state-changed", player->media_state);
        }
    }

}

GmtkMediaPlayerMediaState gmtk_media_player_get_state(GmtkMediaPlayer * player)
{
    return player->media_state;
}

void gmtk_media_player_set_attribute_boolean(GmtkMediaPlayer * player,
                                             GmtkMediaPlayerMediaAttributes attribute, gboolean value)
{
    gchar *cmd = NULL;

    if (attribute == ATTRIBUTE_SUB_VISIBLE) {
        player->sub_visible = value;
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("pausing_keep_force set_property sub_visibility %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
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

    case ATTRIBUTE_SUBS_EXIST:
        ret = (player->subtitles != NULL);
        break;

    case ATTRIBUTE_SOFTVOL:
        ret = player->softvol;
        break;

    case ATTRIBUTE_SEEKABLE:
        ret = player->seekable;
        break;

    case ATTRIBUTE_HAS_CHAPTERS:
        ret = player->has_chapters;
        break;

    default:
        printf("Unsupported Attribute\n");
    }
    return ret;
}

void gmtk_media_player_set_attribute_double(GmtkMediaPlayer * player,
                                            GmtkMediaPlayerMediaAttributes attribute, gdouble value)
{

    if (attribute == ATTRIBUTE_CACHE_SIZE) {
        player->cache_size = value;
    }

    if (attribute == ATTRIBUTE_SOFTVOL) {
        player->softvol = value;
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

    default:
        printf("Unsupported Attribute\n");
    }
    return ret;
}

void gmtk_media_player_set_attribute_string(GmtkMediaPlayer * player,
                                            GmtkMediaPlayerMediaAttributes attribute, const gchar * value)
{
    switch (attribute) {
    case ATTRIBUTE_VO:
        if (value == NULL || strlen(value) == 0) {
            if (player->vo != NULL) {
                g_free(player->vo);
                player->vo = NULL;
            }
        } else {
            player->vo = g_strdup(value);
        }
        break;
    case ATTRIBUTE_AO:
        if (value == NULL || strlen(value) == 0) {
            if (player->ao != NULL) {
                g_free(player->ao);
                player->ao = NULL;
            }
        } else {
            player->ao = g_strdup(value);
        }
        break;
    default:
        printf("Unsupported Attribute\n");
    }
}

const gchar *gmtk_media_player_get_attribute_string(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute)
{
    gchar *value = NULL;
    GList *iter;
    GmtkMediaPlayerAudioTrack *track;
    GmtkMediaPlayerSubtitle *subtitle;

    if (attribute == ATTRIBUTE_AF_EXPORT_FILENAME) {
        value = player->af_export_filename;
    }

    if (attribute == ATTRIBUTE_AUDIO_TRACK) {
        iter = player->audio_tracks;
        while (iter) {
            track = (GmtkMediaPlayerAudioTrack *) iter->data;
            if (track->id == player->audio_track_id)
                value = track->label;
            iter = iter->next;
        }
    }

    if (attribute == ATTRIBUTE_SUBTITLE) {
        iter = player->subtitles;
        while (iter) {
            subtitle = (GmtkMediaPlayerSubtitle *) iter->data;
            if (subtitle->id == player->subtitle_id && subtitle->is_file == player->subtitle_is_file)
                value = subtitle->label;
            iter = iter->next;
        }
    }
    return value;
}

void gmtk_media_player_set_attribute_integer(GmtkMediaPlayer * player, GmtkMediaPlayerMediaAttributes attribute,
                                             gint value)
{
    gchar *cmd = NULL;

    if (attribute == ATTRIBUTE_BRIGHTNESS) {
        player->brightness = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("pausing_keep_force set_property brightness %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
    }

    if (attribute == ATTRIBUTE_CONTRAST) {
        player->contrast = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("pausing_keep_force set_property contrast %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
    }

    if (attribute == ATTRIBUTE_GAMMA) {
        player->gamma = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("pausing_keep_force set_property gamma %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
    }

    if (attribute == ATTRIBUTE_HUE) {
        player->hue = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("pausing_keep_force set_property hue %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
    }

    if (attribute == ATTRIBUTE_SATURATION) {
        player->saturation = CLAMP(value, -100.0, 100.0);
        if (player->player_state == PLAYER_STATE_RUNNING) {
            cmd = g_strdup_printf("pausing_keep_force set_property saturation %i\n", value);
            write_to_mplayer(player, cmd);
            g_free(cmd);
        }
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
    switch (attribute) {
    case ATTRIBUTE_BRIGHTNESS:
        return player->brightness;
        break;
    case ATTRIBUTE_CONTRAST:
        return player->contrast;
        break;
    case ATTRIBUTE_GAMMA:
        return player->gamma;
        break;
    case ATTRIBUTE_HUE:
        return player->hue;
        break;
    case ATTRIBUTE_SATURATION:
        return player->saturation;
        break;
    default:
        return 0;
    }
}


void gmtk_media_player_seek(GmtkMediaPlayer * player, gdouble value, GmtkMediaPlayerSeekType seek_type)
{
    gchar *cmd;

    cmd = g_strdup_printf("seek %lf %i\n", value, seek_type);
    write_to_mplayer(player, cmd);
    g_free(cmd);
}

void gmtk_media_player_set_volume(GmtkMediaPlayer * player, gdouble value)
{
    gchar *cmd;

    player->volume = value;
    if (player->player_state == PLAYER_STATE_RUNNING) {
        cmd = g_strdup_printf("pausing_keep_force volume %i 1\n", (gint) (player->volume * 100.0));
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
            cmd = g_strdup_printf("pausing_keep_force sub_file %i \n", subtitle->id);
        } else {
            cmd = g_strdup_printf("pausing_keep_force sub_demux %i \n", subtitle->id);
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
        cmd = g_strdup_printf("pausing_keep_force switch_audio %i \n", track->id);
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
            cmd = g_strdup_printf("pausing_keep_force sub_file %i \n", subtitle->id);
        } else {
            cmd = g_strdup_printf("pausing_keep_force sub_demux %i \n", subtitle->id);
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
        cmd = g_strdup_printf("pausing_keep_force switch_audio %i \n", track->id);
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
    gint argn = 0;
    GPid pid;
    GError *error;
    gint i;
    gint spawn;

    // TODO: Fix memory leak
    player->subtitles = NULL;
    player->audio_tracks = NULL;

    player->seekable = FALSE;
    player->has_chapters = FALSE;

    g_mutex_lock(player->thread_running);
    if (player->uri != NULL) {
        filename = g_filename_from_uri(player->uri, NULL, NULL);
    }

    argv[argn++] = g_strdup_printf("mplayer");
    if (player->vo != NULL) {
        argv[argn++] = g_strdup_printf("-vo");
        argv[argn++] = g_strdup_printf("%s", player->vo);
    }
    if (player->ao != NULL) {
        argv[argn++] = g_strdup_printf("-ao");
        argv[argn++] = g_strdup_printf("%s", player->ao);
    }
    argv[argn++] = g_strdup_printf("-quiet");
    argv[argn++] = g_strdup_printf("-slave");
    argv[argn++] = g_strdup_printf("-noidle");
    //argv[argn++] = g_strdup_printf("-noconsolecontrols");
    argv[argn++] = g_strdup_printf("-identify");
    if ((gint) (player->volume * 100) != 0) {
        argv[argn++] = g_strdup_printf("-volume");
        printf("volume = %f\n", player->volume);
        argv[argn++] = g_strdup_printf("%i", (gint) (player->volume * 100));
    }

    if (player->softvol == TRUE) {
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

    argv[argn++] = g_strdup_printf("-af-add");
    argv[argn++] = g_strdup_printf("export=%s:512", player->af_export_filename);
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

    switch (player->type) {
    case TYPE_FILE:
        if (g_strrstr(filename, "apple.com")) {
            argv[argn++] = g_strdup_printf("-user-agent");
            argv[argn++] = g_strdup_printf("QuickTime/7.6.4");
        }
        argv[argn++] = g_strdup_printf("%s", filename);
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

    default:
        printf("Unrecognized type\n");
    }
    //argv[argn++] = g_strdup_printf("-v");

    argv[argn] = NULL;

    for (i = 0; i < argn; i++) {
        printf("%s\n", argv[i]);
    }

    error = NULL;
    spawn =
        g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &pid,
                                 &(player->std_in), &(player->std_out), &(player->std_err), &error);
    if (error != NULL) {
        printf("error code = %i - %s\n", error->code, error->message);
        g_error_free(error);
        error = NULL;
    }

    if (spawn) {
        player->channel_in = g_io_channel_unix_new(player->std_in);
        player->channel_out = g_io_channel_unix_new(player->std_out);
        player->channel_err = g_io_channel_unix_new(player->std_err);

        g_io_channel_set_close_on_unref(player->channel_in, TRUE);
        g_io_channel_set_close_on_unref(player->channel_out, TRUE);
        g_io_channel_set_close_on_unref(player->channel_err, TRUE);
        player->watch_in_id =
            g_io_add_watch_full(player->channel_out, G_PRIORITY_LOW, G_IO_IN | G_IO_HUP, thread_reader, player, NULL);
        player->watch_err_id =
            g_io_add_watch_full(player->channel_err, G_PRIORITY_LOW, G_IO_IN | G_IO_ERR | G_IO_HUP,
                                thread_reader_error, player, NULL);
        player->watch_in_hup_id =
            g_io_add_watch_full(player->channel_out, G_PRIORITY_LOW, G_IO_ERR | G_IO_HUP,
                                thread_complete, player, NULL);

        g_timeout_add_seconds(1, thread_query, player);
        g_cond_wait(player->mplayer_complete_cond, player->thread_running);
    }

    player->player_state = PLAYER_STATE_DEAD;
    player->media_state = MEDIA_STATE_UNKNOWN;
    g_mutex_unlock(player->thread_running);
    player->mplayer_thread = NULL;
    player->start_time = 0.0;
    player->run_time = 0.0;
    g_signal_emit_by_name(player, "position-changed", 0.0);
    g_signal_emit_by_name(player, "player-state-changed", player->player_state);
    g_signal_emit_by_name(player, "media-state-changed", player->media_state);

    return NULL;
}

gboolean thread_complete(GIOChannel * source, GIOCondition condition, gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(data);

    player->player_state = PLAYER_STATE_DEAD;
    player->media_state = MEDIA_STATE_UNKNOWN;
    g_source_remove(player->watch_in_id);
    g_source_remove(player->watch_err_id);
    g_cond_signal(player->mplayer_complete_cond);
    g_unlink(player->af_export_filename);

    return FALSE;
}

gboolean thread_reader_error(GIOChannel * source, GIOCondition condition, gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(data);
    GString *mplayer_output;
    GIOStatus status;
    GError *error = NULL;

    if (player == NULL) {
        return FALSE;
    }

    if (source == NULL) {
        g_source_remove(player->watch_in_id);
        g_source_remove(player->watch_in_hup_id);
        return FALSE;
    }

    if (player->player_state == PLAYER_STATE_DEAD) {
        return FALSE;
    }

    mplayer_output = g_string_new("");
    status = g_io_channel_read_line_string(source, mplayer_output, NULL, NULL);

    // printf("ERROR: %s", mplayer_output->str);
    g_string_free(mplayer_output, TRUE);
    return TRUE;
}

gboolean thread_reader(GIOChannel * source, GIOCondition condition, gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(data);
    GString *mplayer_output;
    GIOStatus status;
    GError *error = NULL;
    gchar *buf;
    gint w, h;
    gchar vm[10];
    gint id;
    GtkAllocation allocation;
    GmtkMediaPlayerSubtitle *subtitle;
    GmtkMediaPlayerAudioTrack *audio_track;
    GList *iter;

    if (player == NULL) {
        return FALSE;
    }

    if (source == NULL) {
        g_source_remove(player->watch_in_id);
        g_source_remove(player->watch_in_hup_id);
        return FALSE;
    }

    if (player->player_state == PLAYER_STATE_DEAD) {
        return FALSE;
    }

    mplayer_output = g_string_new("");

    status = g_io_channel_read_line_string(source, mplayer_output, NULL, &error);
    if (status != G_IO_STATUS_NORMAL) {
        return FALSE;
    } else {
        printf("%s", mplayer_output->str);
        if (strstr(mplayer_output->str, "AO:") != NULL) {
            write_to_mplayer(player, "get_property switch_audio\n");
        }

        if (strstr(mplayer_output->str, "VO:") != NULL) {
            buf = strstr(mplayer_output->str, "VO:");
            sscanf(buf, "VO: [%9[^]]] %ix%i => %ix%i", vm, &w, &h, &(player->video_width), &(player->video_height));
            gtk_widget_get_allocation(GTK_WIDGET(player), &allocation);
            if (player->restart) {
                g_signal_emit_by_name(player, "restart-complete", NULL);
            } else {
                g_signal_emit_by_name(player->socket, "size_allocate", &allocation);
            }
            write_to_mplayer(player, "get_property sub_source\n");
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_SIZE);
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
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_SIZE);
            g_signal_emit_by_name(player, "subtitles-changed", g_list_length(player->subtitles));
            g_signal_emit_by_name(player, "audio-tracks-changed", g_list_length(player->audio_tracks));
        }

        if (strstr(mplayer_output->str, "ANS_TIME_POSITION") != 0) {
            buf = strstr(mplayer_output->str, "ANS_TIME_POSITION");
            sscanf(buf, "ANS_TIME_POSITION=%lf", &player->position);
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
            player->audio_track_id--;
            g_signal_emit_by_name(player, "attribute-changed", ATTRIBUTE_AUDIO_TRACK);
        }

        if (strstr(mplayer_output->str, "ANS_switch_audio") != 0) {
            buf = strstr(mplayer_output->str, "ANS_switch_audio");
            sscanf(buf, "ANS_switch_audio=%i", &player->audio_track_id);
            if (player->type == TYPE_DVD) {
                // do nothing  for now
            } else {
                player->audio_track_id--;
            }
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

        if ((strstr(mplayer_output->str, "ID_CHAPTERS=") != NULL)
            && !(strstr(mplayer_output->str, "ID_CHAPTERS=0") != NULL)) {
            player->has_chapters = TRUE;
        }

        if ((strstr(mplayer_output->str, "ID_SEEKABLE=") != NULL)
            && !(strstr(mplayer_output->str, "ID_SEEKABLE=0") != NULL)) {
            player->seekable = TRUE;
        }

        if (strstr(mplayer_output->str, "ID_VIDEO_FORMAT") != 0) {
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "ID_VIDEO_FORMAT") + strlen("ID_VIDEO_FORMAT=");
            if (player->video_format != NULL) {
                g_free(player->video_format);
                player->video_format = NULL;
            }
            player->video_format = g_strdup(buf);
        }

        if (strstr(mplayer_output->str, "ID_VIDEO_CODEC") != 0) {
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "ID_VIDEO_CODEC") + strlen("ID_VIDEO_CODEC=");
            if (player->video_codec != NULL) {
                g_free(player->video_codec);
                player->video_codec = NULL;
            }
            player->video_codec = g_strdup(buf);
        }

        if (strstr(mplayer_output->str, "ID_VIDEO_FPS") != 0) {
            buf = strstr(mplayer_output->str, "ID_VIDEO_FPS");
            sscanf(buf, "ID_VIDEO_FPS=%lf", &player->video_fps);
        }

        if (strstr(mplayer_output->str, "ID_VIDEO_BITRATE") != 0) {
            buf = strstr(mplayer_output->str, "ID_VIDEO_BITRATE");
            sscanf(buf, "ID_VIDEO_BITRATE=%i", &player->video_bitrate);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_FORMAT") != 0) {
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "ID_AUDIO_FORMAT") + strlen("ID_AUDIO_FORMAT=");
            if (player->audio_format != NULL) {
                g_free(player->audio_format);
                player->audio_format = NULL;
            }
            player->audio_format = g_strdup(buf);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_CODEC") != 0) {
            g_string_truncate(mplayer_output, mplayer_output->len - 1);
            buf = strstr(mplayer_output->str, "ID_AUDIO_CODEC") + strlen("ID_AUDIO_CODEC=");
            if (player->audio_codec != NULL) {
                g_free(player->audio_codec);
                player->audio_codec = NULL;
            }
            player->audio_codec = g_strdup(buf);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_BITRATE") != 0) {
            buf = strstr(mplayer_output->str, "ID_AUDIO_BITRATE");
            sscanf(buf, "ID_AUDIO_BITRATE=%i", &player->audio_bitrate);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_RATE") != 0) {
            buf = strstr(mplayer_output->str, "ID_AUDIO_RATE");
            sscanf(buf, "ID_AUDIO_RATE=%i", &player->audio_rate);
        }

        if (strstr(mplayer_output->str, "ID_AUDIO_NCH") != 0) {
            buf = strstr(mplayer_output->str, "ID_AUDIO_RATE");
            sscanf(buf, "ID_AUDIO_RATE=%i", &player->audio_nch);
        }


    }

    g_string_free(mplayer_output, TRUE);
    return TRUE;
}

gboolean thread_query(gpointer data)
{
    GmtkMediaPlayer *player = GMTK_MEDIA_PLAYER(data);
    gint written;

    if (player == NULL) {
        return FALSE;
    }

    if (player->player_state == PLAYER_STATE_RUNNING) {
        if (player->media_state == MEDIA_STATE_PLAY) {
            written =
                write(player->std_in, "pausing_keep_force get_time_pos\n", strlen("pausing_keep_force get_time_pos\n"));
            if (written == -1) {
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

    //printf("send command = %s\n", cmd);

    if (player->channel_in) {
        result = g_io_channel_write_chars(player->channel_in, cmd, -1, &bytes_written, NULL);
        if (result != G_IO_STATUS_ERROR && bytes_written > 0) {
            result = g_io_channel_flush(player->channel_in, NULL);
            return TRUE;
        } else {
            return FALSE;
        }
    }

}
