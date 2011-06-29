/* gmpc-alarm (GMPC plugin)
 * Copyright (C) 2008-2009 Qball Cow <qball@sarine.nl>
 * Copyright (C) 2006-2008 Gavin Gilmour <gavin@brokentrain.net>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "gmpc-alarm.h"

/* Global, required objects */
static GtkWidget *si_alarm = NULL;
static GtkWidget *alarm_pref_vbox = NULL;
static GtkWidget *alarm_pref_table = NULL;
static GtkWidget *enable_alarm;
static GtkWidget *countdown;
static GtkWidget *time_hours_spinner, *time_minutes_spinner, *time_seconds_spinner;
static GTimer *timer = NULL;
static gboolean timer_on = FALSE, prefs_active = FALSE;

/* Rely on MPD interaction functions for deadline actions */
extern int play_song ();
extern int stop_song ();
extern int random_toggle ();

/* Function to allow clean exits */
extern void preferences_show_pref_window(int plugin_id);
static gboolean on_timeout (gpointer user_data);
extern void main_quit ();
guint timeout = 0;
gmpcPlugin plugin;

int plugin_api_version = PLUGIN_API_VERSION;


static gboolean alarm_get_enabled(void)
{
    return     cfg_get_single_value_as_int_with_default (config, "alarm-plugin", "enable",TRUE);
}

static void alarm_start(void)
{
    timer_on = TRUE;
    g_timer_start (timer);
    if(timeout > 0) g_source_remove(timeout);
    timeout = g_timeout_add_seconds (1, (GSourceFunc)on_timeout, timer);

    if(si_alarm)
        gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(si_alarm)), TRUE);
    if (prefs_active)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_alarm), TRUE);
}

static void alarm_stop(void)
{
    timer_on = FALSE;
    g_timer_stop (timer);
    g_timer_reset (timer);
    if(timeout > 0) g_source_remove(timeout);
    timeout = 0;

    if(si_alarm)
        gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(si_alarm)), FALSE);
    if (prefs_active)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_alarm), FALSE);
}



static gboolean alarm_si_button_press_event(GtkWidget *icon, GdkEventButton *event, gpointer data)
{
    if(event->button == 1)
    {
        if (timer_on == FALSE) {
            alarm_start();
        } else {
            alarm_stop();
        }
        /* Again, check for active page and set button otherwise */
        return TRUE;
    }
    else if (event->button == 3)
    {
        preferences_show_pref_window(plugin.id);
    }
    return FALSE;
}
static void alarm_set_enabled(int enabled)
{
    cfg_set_single_value_as_int (config, "alarm-plugin", "enable",enabled);
    if(enabled)
    {
        if(!si_alarm)
        {
            GtkWidget *image  = gtk_image_new_from_icon_name("gtk-properties", GTK_ICON_SIZE_MENU);
            si_alarm = gtk_event_box_new();
            gtk_container_add(GTK_CONTAINER(si_alarm), image);
            main_window_add_status_icon(si_alarm);
            gtk_widget_show_all(si_alarm);
            g_signal_connect(G_OBJECT(si_alarm), "button-press-event", G_CALLBACK(alarm_si_button_press_event), NULL);
        }
        /* Timer is always stopped */
        alarm_stop();
    }
    else
    {
        alarm_stop();
        if(si_alarm) {
            gtk_widget_destroy(si_alarm);
            si_alarm = NULL;
        }
    }
}

static const gchar *alarm_get_translation_domain(void)
{
    return GETTEXT_PACKAGE;
}
gmpcPrefPlugin alarm_gpp = {
    alarm_construct,
    alarm_destroy
};

gmpcPlugin plugin = {
    /* Name */
    .name               = N_("Alarm Timer"),
    /* Version */
    .version            = {0,0,1},
    /* Plugin type */
    .plugin_type        = GMPC_PLUGIN_NO_GUI,
    /* Initialize function */
    .init               = alarm_init, 
    /* Preferences structure */
    .pref               = &alarm_gpp,
    .get_enabled        = alarm_get_enabled,
    .set_enabled        = alarm_set_enabled,
    /* Translation */
    .get_translation_domain = alarm_get_translation_domain
};

/* Represents a time (for easy passing) */
typedef struct {
    glong hours;
    glong minutes;
    glong seconds;
} Time;

/**
 * Basic functions for returning formatted time
 */
static glong get_hours (glong seconds)
{
    return (glong) ((seconds / 3600) % 24);
}

static glong get_minutes (glong seconds)
{
    return (glong) ((seconds / 60) % 60);
}

static glong get_seconds (glong seconds)
{
    return (glong) (seconds % 60);
}

/** 
 * Update the ticking label in hh:mm:ss format. 
 */
static void update_ticker_label (glong current_diff_seconds) {

    /**
     * Crude check to see if the preferences page is active
     * FIXME: Nicer way to check if the widgets are available rather
     * than a global boolean?
     */

    gchar *str = g_strdup_printf ("%02d:%02d:%02d", 
            (guint) get_hours(current_diff_seconds),
            (guint) get_minutes(current_diff_seconds),
            (guint) get_seconds(current_diff_seconds));
    
    if (prefs_active)
        gtk_label_set_text (GTK_LABEL (countdown), (gchar *) str); 
    if(si_alarm)
        gtk_widget_set_tooltip_text(si_alarm, str);
    g_free(str);
}

/**
 * Compares two times and acts on match (deadline)
 */
static void check_for_deadline(Time* current_time_struct, Time* target_time_struct)
{
    if ((current_time_struct->hours == target_time_struct->hours) 
            && (current_time_struct->minutes == target_time_struct->minutes)
            && (current_time_struct->seconds == target_time_struct->seconds)) {

        debug_printf (DEBUG_INFO, "* Alarm has been activated, decide what action to take!");

        /* Decide what action to take from the selected item in the combo */
        switch (cfg_get_single_value_as_int_with_default (config, "alarm-plugin", "action-id", 0)) {
            case 0:
                debug_printf (DEBUG_INFO, "* Attempting to play/pause");
                play_song ();
                break;
            case 1:
                debug_printf (DEBUG_INFO, "* Attempting to stop");
                stop_song ();
                break;
            case 2:
                debug_printf (DEBUG_INFO, "* Stopping and closing gmpc");
                stop_song ();
                main_quit ();
                break;
            case 3:
                debug_printf (DEBUG_INFO, "* Closing gmpc only");
                /* Friendly way of closing gmpc */
                main_quit ();
                break;
            case 4:
                debug_printf (DEBUG_INFO, "* Shutting down system");
                /* TODO: Nice way of halting a system */
                break;
            case 5:
                debug_printf (DEBUG_INFO, "* Toggling random");
                random_toggle ();
                break;
        }

        /* Disable timer, and thus the ticking timeout */
        alarm_stop();
    }
}

/**
 * Handles a continuous tick
 */
static gboolean on_timeout (gpointer user_data)
{
    time_t tt = time(NULL);
    struct tm *a = localtime (&tt);
    Time *current_time_struct, *target_time_struct;
    glong countdown_time, target_time_seconds, current_time_seconds, current_diff_seconds;

    current_time_struct = g_malloc (sizeof (Time));
    target_time_struct = g_malloc (sizeof (Time));

    /* Sort out hours, minutes and seconds for the current time */
    current_time_struct->hours   = a->tm_hour;
    current_time_struct->minutes = a->tm_min;
    current_time_struct->seconds = a->tm_sec;

    /* Get the current elapsed ticks from timer */
    countdown_time = (glong) g_timer_elapsed ((GTimer *)user_data, NULL);

    /* Sort out target deadlines hours, minutes, seconds */
    target_time_struct->hours = cfg_get_single_value_as_int_with_default (config, "alarm-plugin", "time_hours", 0);
    target_time_struct->minutes = cfg_get_single_value_as_int_with_default (config, "alarm-plugin", "time_minutes", 0);
    target_time_struct->seconds = cfg_get_single_value_as_int_with_default (config, "alarm-plugin", "time_seconds", 0);

    /* Debug message */
    debug_printf (DEBUG_INFO, "tick(%d) [%d:%d:%d] [%d:%d:%d]",
            (guint) countdown_time, (guint) current_time_struct->hours, (guint) current_time_struct->minutes, (guint) current_time_struct->seconds,
            (guint) target_time_struct->hours, (guint) target_time_struct->minutes, (guint) target_time_struct->seconds);

    /* Convert both times into seconds and work out a difference */
    target_time_seconds = (target_time_struct->hours * 60 * 60) + (target_time_struct->minutes * 60) + target_time_struct->seconds;
    current_time_seconds = (current_time_struct->hours * 60 * 60) + (current_time_struct->minutes * 60) + current_time_struct->seconds;
    current_diff_seconds = target_time_seconds - current_time_seconds;
    if (current_diff_seconds < 0)
        current_diff_seconds += 86400; // add number of seconds in one day

    /* Update the ticker label */
    update_ticker_label(current_diff_seconds);

    /* Check for deadline */
    check_for_deadline(current_time_struct, target_time_struct);

    /* Free up unneeded time structures */
    g_free(current_time_struct);
    g_free(target_time_struct);
    return timer_on;
}

/**
 * Called on plugin initialization (start-up)
 */

static void alarm_init_delayed(void)
{
    /* make sure icon is setup */
    alarm_set_enabled(alarm_get_enabled());
}
void alarm_init (void)
{
    debug_printf (DEBUG_INFO, "* Alarm plugin initialized");

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    timer = g_timer_new ();

    /* Timer starts on creation, stop it! */
    timer_on = FALSE;
    g_timer_stop (timer);
    g_timer_reset (timer);

    gtk_init_add((GtkFunction)alarm_init_delayed, NULL);
}

/**
 * Called when the action box is changed
 */
static void on_action_value_changed (GtkWidget *widget)
{
    int id = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
    cfg_set_single_value_as_int (config, "alarm-plugin", "action-id", id);
    debug_printf (DEBUG_INFO, "* Set alarm action: %d",
            cfg_get_single_value_as_int_with_default (config, "alarm-plugin", "action-id", 0));
}

/**
 * Toggles activation of the timer
 */
static void on_enable_toggle (GtkWidget *widget)
{
    int alarm_enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    if(alarm_enabled !=  timer_on) 
    {
        if (alarm_enabled) {
            alarm_start();
        } else {
            alarm_stop();
        }
    }
}

/**
 * Called when the spin value is changed
 */
static void on_spin_value_changed (GtkWidget *widget)
{
    gint value;
    gchar *str;
    guint hour, minute, second;

    value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
    str = g_strdup_printf ("%02d", (gint) value);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), value);
    gtk_entry_set_text (GTK_ENTRY (widget), str);

    hour = (guint) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON (time_hours_spinner));
    minute = (guint) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (time_minutes_spinner));
    second = (guint) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (time_seconds_spinner));

    cfg_set_single_value_as_int (config, "alarm-plugin", "time_hours", hour);
    cfg_set_single_value_as_int (config, "alarm-plugin", "time_minutes", minute);
    cfg_set_single_value_as_int (config, "alarm-plugin", "time_seconds", second);

    g_free (str);
}

#if (GTK_MINOR_VERSION >= 20)
/**
 * Called when the spin value is wrapped (gtk+ 2.10 only)
 */
static void on_spin_value_wrapped (GtkWidget *widget, gpointer data)
{
    gint value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));

    if (widget == time_seconds_spinner)
        gtk_spin_button_spin (GTK_SPIN_BUTTON (time_minutes_spinner),
                (value == 0) ? GTK_SPIN_STEP_FORWARD : GTK_SPIN_STEP_BACKWARD, 1);
    else if (widget == time_minutes_spinner)
        gtk_spin_button_spin (GTK_SPIN_BUTTON (time_hours_spinner),
                (value == 0) ? GTK_SPIN_STEP_FORWARD : GTK_SPIN_STEP_BACKWARD, 1);
}
#endif

/**
 * Called when the preferences page is changed/closed.
 */
void alarm_destroy (GtkWidget *container)
{
    debug_printf (DEBUG_INFO, "* Alarm prefs destroyed");
    gtk_container_remove (GTK_CONTAINER (container), alarm_pref_vbox);

    /* Preferences no longer active */
    prefs_active = FALSE;
}

/**
 * Initialize preferences page
 */
void alarm_construct (GtkWidget *container)
{
    GtkWidget *action_label, *time_label, *action_combo;
    GtkWidget *countdown_label, *separator, *separator2;

    /* Preferences page is now visible and thus active */
    prefs_active = TRUE;

    enable_alarm = gtk_check_button_new_with_mnemonic (_("_Enable alarm"));

    /* Set enabled/disable toggle depending on if timer is already running! */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_alarm), (timer_on) ? TRUE : FALSE);

    /* Set up the alarm time spinners */
    time_label = gtk_label_new (_("Time:"));
    time_hours_spinner = gtk_spin_button_new_with_range (0, 23, 1.0);
    time_minutes_spinner = gtk_spin_button_new_with_range (0, 59, 1.0);
    time_seconds_spinner = gtk_spin_button_new_with_range (0, 59, 1.0);

    guint time_hours_stored = cfg_get_single_value_as_int_with_default
        (config, "alarm-plugin", "time_hours", 0);

    /* If the hours are stored then assume the minutes and seconds are also. */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (time_hours_spinner), time_hours_stored);
    gchar *str = g_strdup_printf ("%02d", (gint) time_hours_stored);
    gtk_entry_set_text (GTK_ENTRY (time_hours_spinner), str);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (time_minutes_spinner),
            cfg_get_single_value_as_int_with_default (config, "alarm-plugin", "time_minutes", 0));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (time_seconds_spinner),
            cfg_get_single_value_as_int_with_default (config, "alarm-plugin", "time_seconds", 0));

    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (time_hours_spinner), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (time_minutes_spinner), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (time_seconds_spinner), TRUE);

    separator = gtk_label_new (":");
    separator2 = gtk_label_new (":");
    action_label = gtk_label_new (_("Action:"));
    action_combo = gtk_combo_box_new_text ();

    alarm_pref_table = gtk_table_new (2, 2, FALSE);
    alarm_pref_vbox = gtk_vbox_new (FALSE,6);

    /*
     * Hard-coded possible actions for once alarm deadline hit
     * TODO: Allow customized actions?
     */
    gtk_combo_box_append_text (GTK_COMBO_BOX (action_combo), _("Play/Pause"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (action_combo), _("Stop"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (action_combo), _("Close  (& Stop)"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (action_combo), _("Close only"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (action_combo), _("Shutdown"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (action_combo), _("Toggle random"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (action_combo),
            cfg_get_single_value_as_int_with_default (config, "alarm-plugin", "action-id", 0));

    /* Label to show remaining time */
    countdown_label = gtk_label_new (_("Time left:"));
    countdown = gtk_label_new ("--");

    /* Attach widgets */
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), time_label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), time_hours_spinner, 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), separator, 2, 3, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), time_minutes_spinner, 3, 4, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), separator2, 4, 5, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), time_seconds_spinner, 5, 6, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), action_label, 0, 1, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), action_combo, 1, 2, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), countdown_label, 0, 1, 2, 3);
    gtk_table_attach_defaults (GTK_TABLE (alarm_pref_table), countdown, 1, 2, 2, 3);

    /* Connect signals */
    g_signal_connect (G_OBJECT (time_hours_spinner), "value-changed", G_CALLBACK (on_spin_value_changed), NULL);
    g_signal_connect (G_OBJECT (time_minutes_spinner), "value-changed", G_CALLBACK (on_spin_value_changed), NULL);
    g_signal_connect (G_OBJECT (time_seconds_spinner), "value-changed", G_CALLBACK (on_spin_value_changed), NULL);

    /* Check that we're running gtk+ for supported gtk_spin_button 'wrapped' functionality */
#if (GTK_MINOR_VERSION >= 20) 
    g_signal_connect (G_OBJECT (time_hours_spinner), "wrapped", G_CALLBACK (on_spin_value_wrapped), NULL);
    g_signal_connect (G_OBJECT (time_minutes_spinner), "wrapped", G_CALLBACK (on_spin_value_wrapped), NULL);
    g_signal_connect (G_OBJECT (time_seconds_spinner), "wrapped", G_CALLBACK (on_spin_value_wrapped), NULL);
#endif

    g_signal_connect (G_OBJECT (action_combo), "changed", G_CALLBACK (on_action_value_changed), NULL);
    g_signal_connect (G_OBJECT (enable_alarm), "toggled", G_CALLBACK (on_enable_toggle), NULL);

    gtk_box_pack_start (GTK_BOX (alarm_pref_vbox), enable_alarm, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (alarm_pref_vbox), alarm_pref_table, FALSE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (container), alarm_pref_vbox);
    gtk_widget_show_all (container);
}
