// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * GAction actions.
 */

#include "zrythm-config.h"

#include "actions/actions.h"
#include "actions/arranger_selections.h"
#include "actions/range_action.h"
#include "actions/tracklist_selections.h"
#include "actions/undo_manager.h"
#include "dsp/audio_function.h"
#include "dsp/audio_region.h"
#include "dsp/automation_function.h"
#include "dsp/graph.h"
#include "dsp/graph_export.h"
#include "dsp/instrument_track.h"
#include "dsp/marker.h"
#include "dsp/marker_track.h"
#include "dsp/midi_event.h"
#include "dsp/midi_function.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/track.h"
#include "dsp/transport.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/clipboard.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/file_manager.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/cc_bindings.h"
#include "gui/widgets/cc_bindings_tree.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/dialogs/about_dialog.h"
#include "gui/widgets/dialogs/add_tracks_to_group_dialog.h"
#include "gui/widgets/dialogs/arranger_object_info.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/dialogs/bounce_dialog.h"
#include "gui/widgets/dialogs/export_dialog.h"
#include "gui/widgets/dialogs/export_midi_file_dialog.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "gui/widgets/dialogs/midi_function_dialog.h"
#include "gui/widgets/dialogs/object_color_chooser_dialog.h"
#include "gui/widgets/dialogs/port_info.h"
#include "gui/widgets/dialogs/quantize_dialog.h"
#include "gui/widgets/dialogs/save_chord_preset_dialog.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/greeter.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/log_viewer.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/panel_file_browser.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/port_connections.h"
#include "gui/widgets/port_connections_tree.h"
#include "gui/widgets/preferences.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/tracklist.h"
#include "io/serialization/clipboard.h"
#include "plugins/collection.h"
#include "plugins/collections.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/chord_preset_pack_manager.h"
#include "settings/settings.h"
#include "utils/debug.h"
#include "utils/dialogs.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/localization.h"
#include "utils/log.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/progress_info.h"
#include "utils/resources.h"
#include "utils/stack.h"
#include "utils/string.h"
#include "utils/system.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "project/project_init_flow_manager.h"

#define DEFINE_SIMPLE(x) \
  void x (GSimpleAction * action, GVariant * variant, gpointer user_data)

void
actions_set_app_action_enabled (const char * action_name, const bool enabled)
{
  GAction * action =
    g_action_map_lookup_action (G_ACTION_MAP (zrythm_app), action_name);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

void
activate_news (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  /* this is not used anymore */
#if 0
  gtk_show_uri (
    GTK_WINDOW (MAIN_WINDOW), "https://mastodon.social/@zrythm",
    GDK_CURRENT_TIME);
#endif
}

void
activate_manual (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  LocalizationLanguage lang =
    (LocalizationLanguage) g_settings_get_enum (S_P_UI_GENERAL, "language");
  const char * lang_code = localization_get_string_code (lang);
#ifdef MANUAL_PATH
  char * path =
    g_strdup_printf ("file://%s/%s/index.html", MANUAL_PATH, lang_code);
#else
  char * path =
    g_strdup_printf ("https://manual.zrythm.org/%s/index.html", lang_code);
#endif
  GtkUriLauncher * launcher = gtk_uri_launcher_new (path);
  gtk_uri_launcher_launch (launcher, GTK_WINDOW (MAIN_WINDOW), NULL, NULL, NULL);
  g_object_unref (launcher);
  g_free (path);
}

static void
cycle_focus (const bool backwards)
{
  GtkWidget * widgets[] = {
    z_gtk_get_first_focusable_child (
      GTK_WIDGET (MAIN_WINDOW->start_dock_switcher)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_LEFT_DOCK_EDGE)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_CENTER_DOCK->main_notebook)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_RIGHT_DOCK_EDGE)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_BOT_DOCK_EDGE)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_BOT_BAR)),
  };

  const int num_elements = 6;
  if (backwards)
    {
      for (int i = num_elements - 2; i >= 0; i--)
        {
          if (
            gtk_widget_has_focus (widgets[i + 1])
            || z_gtk_descendant_has_focus (widgets[i + 1]))
            {
              gtk_widget_grab_focus (widgets[i]);
              return;
            }
        }
      gtk_widget_grab_focus (widgets[num_elements - 1]);
    }
  else
    {
      for (int i = 1; i < num_elements; i++)
        {
          if (
            gtk_widget_has_focus (widgets[i - 1])
            || z_gtk_descendant_has_focus (widgets[i - 1]))
            {
              gtk_widget_grab_focus (widgets[i]);
              return;
            }
        }
      gtk_widget_grab_focus (widgets[0]);
    }
}

DEFINE_SIMPLE (activate_cycle_focus)
{
  cycle_focus (false);
}

DEFINE_SIMPLE (activate_cycle_focus_backwards)
{
  cycle_focus (true);
}

DEFINE_SIMPLE (activate_focus_first_widget)
{
  gtk_widget_grab_focus (z_gtk_get_first_focusable_child (
    GTK_WIDGET (MAIN_WINDOW->start_dock_switcher)));
}

DEFINE_SIMPLE (activate_chat)
{
  GtkUriLauncher * launcher =
    gtk_uri_launcher_new ("https://matrix.to/#/#zrythmdaw:matrix.org");
  gtk_uri_launcher_launch (launcher, GTK_WINDOW (MAIN_WINDOW), NULL, NULL, NULL);
  g_object_unref (launcher);
}

DEFINE_SIMPLE (activate_donate)
{
  GtkUriLauncher * launcher =
    gtk_uri_launcher_new ("https://liberapay.com/Zrythm");
  gtk_uri_launcher_launch (launcher, GTK_WINDOW (MAIN_WINDOW), NULL, NULL, NULL);
  g_object_unref (launcher);
}

DEFINE_SIMPLE (activate_bugreport)
{
  GtkUriLauncher * launcher = gtk_uri_launcher_new (NEW_ISSUE_URL);
  gtk_uri_launcher_launch (launcher, GTK_WINDOW (MAIN_WINDOW), NULL, NULL, NULL);
  g_object_unref (launcher);
}

DEFINE_SIMPLE (activate_about)
{
  GtkWindow * window =
    GTK_WINDOW (about_dialog_widget_new (GTK_WINDOW (MAIN_WINDOW)));
  gtk_window_present (window);
}

/**
 * @note This is never called but keep it around
 * for shortcut window.
 */
void
activate_quit (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  g_application_quit (G_APPLICATION (user_data));
}

/**
 * Show preferences window.
 */
void
activate_preferences (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  if (MAIN_WINDOW && MAIN_WINDOW->preferences_opened)
    {
      return;
    }

  AdwDialog * preferences_window = ADW_DIALOG (preferences_widget_new ());
  g_return_if_fail (preferences_window);
  GListModel * toplevels = gtk_window_get_toplevels ();
  GtkWidget * transient_for = GTK_WIDGET (g_list_model_get_item (toplevels, 0));
  adw_dialog_present (preferences_window, transient_for);
  if (MAIN_WINDOW)
    {
      MAIN_WINDOW->preferences_opened = true;
    }
}

/**
 * Show log window.
 */
void
activate_log (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  if (!LOG || !LOG->log_filepath)
    {
      g_message ("No log file found");
      return;
    }

  GFile *           file = g_file_new_for_path (LOG->log_filepath);
  GtkFileLauncher * launcher = gtk_file_launcher_new (file);
  gtk_file_launcher_launch (launcher, NULL, NULL, NULL, NULL);
  g_object_unref (file);
  g_object_unref (launcher);

  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      MAIN_WINDOW->log_has_pending_warnings = false;
      EVENTS_PUSH (ET_LOG_WARNING_STATE_CHANGED, NULL);
    }
}

DEFINE_SIMPLE (activate_audition_mode)
{
  P_TOOL = TOOL_AUDITION;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

DEFINE_SIMPLE (activate_select_mode)
{
  P_TOOL = TOOL_SELECT;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate edit mode.
 */
void
activate_edit_mode (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  P_TOOL = TOOL_EDIT;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate cut mode.
 */
void
activate_cut_mode (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  P_TOOL = TOOL_CUT;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate eraser mode.
 */
void
activate_eraser_mode (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  P_TOOL = TOOL_ERASER;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate ramp mode.
 */
void
activate_ramp_mode (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  P_TOOL = TOOL_RAMP;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

DEFINE_SIMPLE (activate_zoom_in)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

  /* if ends with v, this is a vertical zoom */
  if (g_str_has_suffix (str, "v"))
    {
      if (g_str_has_prefix (str, "editor"))
        {
          ArrangerWidget * arranger =
            clip_editor_inner_widget_get_visible_arranger (MW_CLIP_EDITOR_INNER);

          switch (arranger->type)
            {
            case ARRANGER_WIDGET_TYPE_MIDI:
              midi_arranger_handle_vertical_zoom_action (arranger, true);
              break;
            default:
              g_warning ("unimplemented");
              break;
            }
        }
    }
  else
    {
      RulerWidget * ruler = NULL;
      if (string_is_equal (str, "timeline"))
        {
          ruler = MW_RULER;
        }
      else if (string_is_equal (str, "editor"))
        {
          ruler = EDITOR_RULER;
        }
      else
        {
          switch (PROJECT->last_selection)
            {
            case SELECTION_TYPE_EDITOR:
              ruler = EDITOR_RULER;
              break;
            default:
              ruler = MW_RULER;
              break;
            }
        }

      ruler_widget_set_zoom_level (
        ruler,
        ruler_widget_get_zoom_level (ruler) * RULER_ZOOM_LEVEL_MULTIPLIER);

      EVENTS_PUSH (ET_RULER_VIEWPORT_CHANGED, ruler);
    }
}

DEFINE_SIMPLE (activate_zoom_out)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

  /* if ends with v, this is a vertical zoom */
  if (g_str_has_suffix (str, "v"))
    {
      if (g_str_has_prefix (str, "editor"))
        {
          ArrangerWidget * arranger =
            clip_editor_inner_widget_get_visible_arranger (MW_CLIP_EDITOR_INNER);

          switch (arranger->type)
            {
            case ARRANGER_WIDGET_TYPE_MIDI:
              midi_arranger_handle_vertical_zoom_action (arranger, false);
              break;
            default:
              g_warning ("unimplemented");
              break;
            }
        }
    }
  else
    {
      RulerWidget * ruler = NULL;
      if (string_is_equal (str, "timeline"))
        {
          ruler = MW_RULER;
        }
      else if (string_is_equal (str, "editor"))
        {
          ruler = EDITOR_RULER;
        }
      else
        {
          switch (PROJECT->last_selection)
            {
            case SELECTION_TYPE_EDITOR:
              ruler = EDITOR_RULER;
              break;
            default:
              ruler = MW_RULER;
              break;
            }
        }

      double zoom_level =
        ruler_widget_get_zoom_level (ruler) / RULER_ZOOM_LEVEL_MULTIPLIER;
      ruler_widget_set_zoom_level (ruler, zoom_level);

      EVENTS_PUSH (ET_RULER_VIEWPORT_CHANGED, ruler);
    }
}

DEFINE_SIMPLE (activate_best_fit)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

  RulerWidget * ruler = NULL;
  if (string_is_equal (str, "timeline"))
    {
      ruler = MW_RULER;
    }
  else if (string_is_equal (str, "editor"))
    {
      ruler = EDITOR_RULER;
    }
  else
    {
      switch (PROJECT->last_selection)
        {
        case SELECTION_TYPE_EDITOR:
          ruler = EDITOR_RULER;
          break;
        default:
          ruler = MW_RULER;
          break;
        }
    }

  Position start, end;
  position_init (&start);
  position_init (&end);
  if (ruler == MW_RULER)
    {
      int total_bars = 0;
      tracklist_get_total_bars (TRACKLIST, &total_bars);
      position_set_to_bar (&end, total_bars);
    }
  else if (ruler == EDITOR_RULER)
    {
      ZRegion * r = clip_editor_get_region (CLIP_EDITOR);
      if (!r)
        return;

      ArrangerWidget * arranger = region_get_arranger_for_children (r);
      GPtrArray *      objs_arr = g_ptr_array_new_full (200, NULL);
      arranger_widget_get_all_objects (arranger, objs_arr);
      position_set_to_pos (&start, &r->base.pos);
      position_set_to_pos (&end, &r->base.end_pos);
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          Position global_start, global_end;
          position_set_to_pos (&global_start, &obj->pos);
          position_add_ticks (&global_start, r->base.pos.ticks);
          if (arranger_object_type_has_length (obj->type))
            {
              position_set_to_pos (&global_end, &obj->end_pos);
              position_add_ticks (&global_end, r->base.pos.ticks);
            }
          else
            {
              position_set_to_pos (&global_end, &global_start);
            }

          if (position_is_before (&global_start, &start))
            {
              position_set_to_pos (&start, &global_start);
            }
          if (position_is_after (&global_end, &end))
            {
              position_set_to_pos (&end, &global_end);
            }
        }
      g_ptr_array_unref (objs_arr);
    }

  double total_ticks = position_to_ticks (&end) - position_to_ticks (&start);
  double allocated_px = (double) gtk_widget_get_width (GTK_WIDGET (ruler));
  double buffer_px = allocated_px / 16.0;
  double needed_px_per_tick = (allocated_px - buffer_px) / total_ticks;
  double new_zoom_level =
    ruler_widget_get_zoom_level (ruler)
    * (needed_px_per_tick / ruler->px_per_tick);
  ruler_widget_set_zoom_level (ruler, new_zoom_level);
  int              start_px = ruler_widget_pos_to_px (ruler, &start, false);
  EditorSettings * settings = ruler_widget_get_editor_settings (ruler);
  editor_settings_set_scroll_start_x (settings, start_px, F_VALIDATE);

  EVENTS_PUSH (ET_RULER_VIEWPORT_CHANGED, ruler);
}

DEFINE_SIMPLE (activate_original_size)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

  RulerWidget * ruler = NULL;
  if (string_is_equal (str, "timeline"))
    {
      ruler = MW_RULER;
    }
  else if (string_is_equal (str, "editor"))
    {
      ruler = EDITOR_RULER;
    }
  else
    {
      switch (PROJECT->last_selection)
        {
        case SELECTION_TYPE_EDITOR:
          ruler = EDITOR_RULER;
          break;
        default:
          ruler = MW_RULER;
          break;
        }
    }

  ruler_widget_set_zoom_level (ruler, 1.0);
  EVENTS_PUSH (ET_RULER_VIEWPORT_CHANGED, ruler);
}

DEFINE_SIMPLE (activate_loop_selection)
{
  if (PROJECT->last_selection == SELECTION_TYPE_TIMELINE)
    {
      if (!arranger_selections_has_any ((ArrangerSelections *) TL_SELECTIONS))
        return;

      Position start, end;
      arranger_selections_get_start_pos (
        (ArrangerSelections *) TL_SELECTIONS, &start, F_GLOBAL);
      arranger_selections_get_end_pos (
        (ArrangerSelections *) TL_SELECTIONS, &end, F_GLOBAL);

      position_set_to_pos (&TRANSPORT->loop_start_pos, &start);
      position_set_to_pos (&TRANSPORT->loop_end_pos, &end);

      /* FIXME is this needed? */
      transport_update_positions (TRANSPORT, true);

      EVENTS_PUSH (ET_TIMELINE_LOOP_MARKER_POS_CHANGED, NULL);
    }
}

static void
proceed_to_new_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           data)
{
  if (string_is_equal (response, "discard"))
    {
      GreeterWidget * greeter =
        greeter_widget_new (zrythm_app, GTK_WINDOW (MAIN_WINDOW), false, true);
      gtk_window_present (GTK_WINDOW (greeter));
    }
  else if (string_is_equal (response, "save"))
    {
      g_message ("saving project...");
      GError * err = NULL;
      bool     successful = project_save (
        PROJECT, PROJECT->dir, F_NOT_BACKUP, ZRYTHM_F_NO_NOTIFY, F_NO_ASYNC,
        &err);
      if (!successful)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to save project"));
          return;
        }
      GreeterWidget * greeter =
        greeter_widget_new (zrythm_app, GTK_WINDOW (MAIN_WINDOW), false, true);
      gtk_window_present (GTK_WINDOW (greeter));
    }
  else
    {
      /* do nothing */
    }
}

void
activate_new (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (adw_message_dialog_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Discard Changes?"),
    _ ("Any unsaved changes to the current "
       "project will be lost. Continue?")));
  adw_message_dialog_add_responses (
    dialog, "cancel", _ ("_Cancel"), "discard", _ ("_Discard"), "save",
    _ ("_Save"), NULL);
  adw_message_dialog_set_close_response (dialog, "cancel");
  adw_message_dialog_set_response_appearance (
    dialog, "discard", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_response_appearance (
    dialog, "save", ADW_RESPONSE_SUGGESTED);
  adw_message_dialog_set_default_response (dialog, "save");
  g_signal_connect (
    dialog, "response", G_CALLBACK (proceed_to_new_response_cb), NULL);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
choose_and_open_project (void)
{
  GreeterWidget * greeter =
    greeter_widget_new (zrythm_app, GTK_WINDOW (MAIN_WINDOW), false, false);
  gtk_window_present (GTK_WINDOW (greeter));
}

static void
proceed_to_open_response_cb (
  AdwMessageDialog * dialog,
  gchar *            response,
  gpointer           data)
{
  if (string_is_equal (response, "discard"))
    {
      choose_and_open_project ();
    }
  else if (string_is_equal (response, "save"))
    {
      g_message ("saving project...");
      GError * err = NULL;
      bool     successful = project_save (
        PROJECT, PROJECT->dir, F_NOT_BACKUP, ZRYTHM_F_NO_NOTIFY, F_NO_ASYNC,
        &err);
      if (!successful)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to save project"));
          return;
        }
      choose_and_open_project ();
    }
  else
    {
      /* do nothing */
    }
}

DEFINE_SIMPLE (activate_open)
{
  if (project_has_unsaved_changes (PROJECT))
    {
      AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), _ ("Save Changes?"),
        _ ("The project contains unsaved changes. "
           "If you proceed, any unsaved changes will be "
           "lost.")));
      adw_message_dialog_add_responses (
        dialog, "cancel", _ ("_Cancel"), "discard", _ ("_Discard"), "save",
        _ ("_Save"), NULL);
      adw_message_dialog_set_close_response (dialog, "cancel");
      adw_message_dialog_set_response_appearance (
        dialog, "discard", ADW_RESPONSE_DESTRUCTIVE);
      adw_message_dialog_set_response_appearance (
        dialog, "save", ADW_RESPONSE_SUGGESTED);
      adw_message_dialog_set_default_response (dialog, "save");
      g_signal_connect (
        dialog, "response", G_CALLBACK (proceed_to_open_response_cb), NULL);
      gtk_window_present (GTK_WINDOW (dialog));
    }
  else
    {
      choose_and_open_project ();
    }
}

void
activate_save (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  if (!PROJECT->dir || !PROJECT->title)
    {
      activate_save_as (action, variant, user_data);
      return;
    }
  g_message ("project dir: %s", PROJECT->dir);

  GError * err = NULL;
  bool     success = project_save (
    PROJECT, PROJECT->dir, F_NOT_BACKUP, ZRYTHM_F_NOTIFY, F_NO_ASYNC, &err);
  if (!success)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to save project"));
    }
}

static void
save_project_ready_cb (GObject * source_object, GAsyncResult * res, gpointer data)
{
  GFile * selected_file =
    gtk_file_dialog_save_finish (GTK_FILE_DIALOG (source_object), res, NULL);
  if (selected_file)
    {
      char * filepath = g_file_get_path (selected_file);
      g_object_unref (selected_file);
      g_message ("saving project at: %s", filepath);
      GError * err = NULL;
      bool     success = project_save (
        PROJECT, filepath, F_NOT_BACKUP, ZRYTHM_F_NO_NOTIFY, F_NO_ASYNC, &err);
      if (!success)
        {
          HANDLE_ERROR_LITERAL (err, _ ("Failed to save project"));
        }
      g_free (filepath);
    }
}

void
activate_save_as (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  GtkFileDialog * dialog = gtk_file_dialog_new ();
  char *          project_file_path =
    project_get_path (PROJECT, PROJECT_PATH_PROJECT_FILE, false);
  char * str = io_path_get_parent_dir (project_file_path);
  g_free (project_file_path);
  GFile * file = g_file_new_for_path (str);
  g_free (str);
  gtk_file_dialog_set_initial_file (dialog, file);
  g_object_unref (file);
  gtk_file_dialog_set_initial_name (dialog, PROJECT->title);
  gtk_file_dialog_set_accept_label (dialog, _ ("Save Project"));
  gtk_file_dialog_save (
    dialog, GTK_WINDOW (MAIN_WINDOW), NULL, save_project_ready_cb, NULL);
}

DEFINE_SIMPLE (activate_minimize)
{
  gtk_window_minimize (GTK_WINDOW (MAIN_WINDOW));
}

void
activate_export_as (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  ExportDialogWidget * export = export_dialog_widget_new ();
  gtk_window_set_transient_for (GTK_WINDOW (export), GTK_WINDOW (MAIN_WINDOW));
  gtk_window_present (GTK_WINDOW (export));
}

void
activate_export_graph (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
#ifdef HAVE_CGRAPH
  char * exports_dir = project_get_path (PROJECT, PROJECT_PATH_EXPORTS, false);

  GtkWidget *    dialog, *label, *content_area;
  GtkDialogFlags flags;

  // Create the widgets
  flags = GTK_DIALOG_DESTROY_WITH_PARENT;
  dialog = gtk_dialog_new_with_buttons (
    _ ("Export routing graph"), GTK_WINDOW (MAIN_WINDOW), flags,
    _ ("Image (PNG)"), 1, _ ("Image (SVG)"), 2, _ ("Dot graph"), 3, NULL);
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  char lbl[600];
  sprintf (
    lbl,
    _ ("The graph will be exported to "
       "<a href=\"%s\">%s</a>\n"
       "Please select a format to export as"),
    exports_dir, exports_dir);
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), lbl);
  gtk_widget_set_visible (label, true);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  z_gtk_widget_set_margin (label, 3);

  g_signal_connect (
    G_OBJECT (label), "activate-link",
    G_CALLBACK (z_gtk_activate_dir_link_func), NULL);

  gtk_box_append (GTK_BOX (content_area), label);

  int             result = z_gtk_dialog_run (GTK_DIALOG (dialog), false);
  const char *    filename = NULL;
  GraphExportType export_type;
  switch (result)
    {
    /* png */
    case 1:
      filename = "graph.png";
      export_type = GRAPH_EXPORT_PNG;
      break;
    /* svg */
    case 2:
      filename = "graph.svg";
      export_type = GRAPH_EXPORT_SVG;
      break;
    /* dot */
    case 3:
      filename = "graph.dot";
      export_type = GRAPH_EXPORT_DOT;
      break;
    default:
      gtk_window_destroy (GTK_WINDOW (dialog));
      return;
    }
  gtk_window_destroy (GTK_WINDOW (dialog));

  char * path = g_build_filename (exports_dir, filename, NULL);
  graph_export_as_simple (export_type, path);
  g_free (exports_dir);
  g_free (path);

  ui_show_notification (_ ("Graph exported"));
#endif
}

void
activate_properties (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_message ("ZOOMING IN");
}

DEFINE_SIMPLE (activate_undo)
{
  if (arranger_widget_any_doing_action ())
    {
      g_message ("in the middle of an action, skipping undo");
      return;
    }

  if (undo_stack_is_empty (UNDO_MANAGER->undo_stack))
    return;

  GError * err = NULL;
  int      ret = undo_manager_undo (UNDO_MANAGER, &err);
  if (ret != 0)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to undo"));
    }
}

static void
handle_undo_or_redo_n (int idx, bool redo)
{
  for (int i = 0; i <= idx; i++)
    {
      GError * err = NULL;
      int      ret = 0;
      if (redo)
        {
          ret = undo_manager_redo (UNDO_MANAGER, &err);
        }
      else
        {
          ret = undo_manager_undo (UNDO_MANAGER, &err);
        }

      if (ret != 0)
        {
          if (redo)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to redo action"));
            }
          else
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to undo action"));
            }

        } /* endif ret != 0 */

    } /* endforeach */
}

DEFINE_SIMPLE (activate_undo_n)
{
  if (arranger_widget_any_doing_action ())
    {
      g_message ("in the middle of an action, skipping undo");
      return;
    }

  int idx = (int) g_variant_get_int32 (variant);
  handle_undo_or_redo_n (idx, false);
}

DEFINE_SIMPLE (activate_redo)
{
  if (arranger_widget_any_doing_action ())
    {
      g_message ("in the middle of an action, skipping redo");
      return;
    }

  if (undo_stack_is_empty (UNDO_MANAGER->redo_stack))
    return;

  GError * err = NULL;
  int      ret = undo_manager_redo (UNDO_MANAGER, &err);
  if (ret != 0)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to redo"));
    }

  EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE, NULL);
}

DEFINE_SIMPLE (activate_redo_n)
{
  if (arranger_widget_any_doing_action ())
    {
      g_message ("in the middle of an action, skipping redo");
      return;
    }

  int idx = (int) g_variant_get_int32 (variant);
  handle_undo_or_redo_n (idx, true);
}

void
activate_cut (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  /* activate copy and then delete the selections */
  activate_copy (action, variant, user_data);

  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (PROJECT);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
    case SELECTION_TYPE_EDITOR:
      if (sel && arranger_selections_has_any (sel))
        {
          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_delete (sel, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to delete selections"));
            }
        }
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      if (mixer_selections_has_any (MIXER_SELECTIONS))
        {
          GError * err = NULL;
          bool     ret = mixer_selections_action_perform_delete (
            MIXER_SELECTIONS, PORT_CONNECTIONS_MGR, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to delete selections"));
            }
        }
      break;
    default:
      g_debug ("doing nothing");
      break;
    }
}

void
activate_copy (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (PROJECT);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
    case SELECTION_TYPE_EDITOR:
      if (sel)
        {
          Clipboard * clipboard =
            clipboard_new_for_arranger_selections (sel, F_CLONE);
          g_return_if_fail (clipboard);
          if (clipboard->timeline_sel)
            {
              timeline_selections_set_vis_track_indices (
                clipboard->timeline_sel);
            }
          GError * err = NULL;
          char *   serialized =
            clipboard_serialize_to_json_str (clipboard, true, &err);
          g_return_if_fail (serialized);
          gdk_clipboard_set_text (DEFAULT_CLIPBOARD, serialized);
          clipboard_free (clipboard);
          g_free (serialized);

          ui_show_notification (_ ("Selections copied to clipboard"));
        }
      else
        {
          g_warning ("no selections to copy");
        }
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      if (mixer_selections_has_any (MIXER_SELECTIONS))
        {
          Clipboard * clipboard =
            clipboard_new_for_mixer_selections (MIXER_SELECTIONS, F_CLONE);
          GError * err = NULL;
          char *   serialized =
            clipboard_serialize_to_json_str (clipboard, true, &err);
          g_return_if_fail (serialized);
          gdk_clipboard_set_text (DEFAULT_CLIPBOARD, serialized);
          clipboard_free (clipboard);
          g_free (serialized);

          ui_show_notification (_ ("Plugins copied to clipboard"));
        }
      break;
    case SELECTION_TYPE_TRACKLIST:
      {
        /* TODO fix crashes eg when copy pasting master */
        return;

        Clipboard * clipboard = clipboard_new_for_tracklist_selections (
          TRACKLIST_SELECTIONS, F_CLONE);
        GError * err = NULL;
        char *   serialized =
          clipboard_serialize_to_json_str (clipboard, true, &err);
        g_return_if_fail (serialized);
        gdk_clipboard_set_text (DEFAULT_CLIPBOARD, serialized);
        clipboard_free (clipboard);
        g_free (serialized);

        ui_show_notification (_ ("Tracks copied to clipboard"));
      }
      break;
    default:
      g_warning ("not implemented yet");
      break;
    }
}

static void
paste_cb (GObject * source_object, GAsyncResult * res, gpointer data)
{
  GError * err = NULL;
  char *   txt = gdk_clipboard_read_text_finish (DEFAULT_CLIPBOARD, res, &err);
  if (!txt)
    {
      ui_show_notification_idle_printf (
        _ ("Failed to paste data: %s"), err ? err->message : "no text found");
      if (err)
        {
          g_error_free (err);
        }
      return;
    }

  /*g_debug ("pasted:\n%s", txt);*/

  err = NULL;
  Clipboard * clipboard = clipboard_deserialize_from_json_str (txt, true, &err);
  if (!clipboard)
    {
      ui_show_notification_idle_printf (
        _ ("invalid clipboard data received: %s"), err->message);
      g_error_free (err);
      return;
    }

  ArrangerSelections *  sel = NULL;
  MixerSelections *     mixer_sel = NULL;
  TracklistSelections * tracklist_sel = NULL;
  switch (clipboard->type)
    {
    case CLIPBOARD_TYPE_TIMELINE_SELECTIONS:
    case CLIPBOARD_TYPE_MIDI_SELECTIONS:
    case CLIPBOARD_TYPE_AUTOMATION_SELECTIONS:
    case CLIPBOARD_TYPE_CHORD_SELECTIONS:
    case CLIPBOARD_TYPE_AUDIO_SELECTIONS:
      sel = clipboard_get_selections (clipboard);
      break;
    case CLIPBOARD_TYPE_MIXER_SELECTIONS:
      mixer_sel = clipboard->mixer_sel;
      break;
    case CLIPBOARD_TYPE_TRACKLIST_SELECTIONS:
      tracklist_sel = clipboard->tracklist_sel;
      break;
    default:
      g_warn_if_reached ();
    }

  bool incompatible = false;
  if (sel)
    {
      arranger_selections_post_deserialize (sel);
      if (arranger_selections_can_be_pasted (sel))
        {
          arranger_selections_paste_to_pos (sel, PLAYHEAD, F_UNDOABLE);
        }
      else
        {
          g_message ("can't paste arranger selections");
          incompatible = true;
        }
    }
  else if (mixer_sel)
    {
      ChannelSlotWidget * slot = MW_MIXER->paste_slot;
      mixer_selections_post_deserialize (mixer_sel);
      if (mixer_selections_can_be_pasted (
            mixer_sel, slot->track->channel, slot->type, slot->slot_index))
        {
          mixer_selections_paste_to_slot (
            mixer_sel, slot->track->channel, slot->type, slot->slot_index);
        }
      else
        {
          g_message ("can't paste mixer selections");
          incompatible = true;
        }
    }
  else if (tracklist_sel)
    {
      tracklist_selections_post_deserialize (tracklist_sel);
      tracklist_selections_paste_to_pos (tracklist_sel, TRACKLIST->num_tracks);
    }

  if (incompatible)
    {
      ui_show_notification (_ ("Can't paste incompatible data"));
    }

  clipboard_free (clipboard);
}

void
activate_paste (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  g_message ("paste");

  gdk_clipboard_read_text_async (DEFAULT_CLIPBOARD, NULL, paste_cb, NULL);
}

void
activate_delete (
  GSimpleAction * simple_action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_message ("%s", __func__);

  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (PROJECT);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TRACKLIST:
      g_message ("activating delete selected tracks");
      g_action_group_activate_action (
        G_ACTION_GROUP (MAIN_WINDOW), "delete-selected-tracks", NULL);
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      g_message ("activating delete mixer selections");
      g_action_group_activate_action (
        G_ACTION_GROUP (MAIN_WINDOW), "delete-mixer-selections", NULL);
      break;
    case SELECTION_TYPE_TIMELINE:
    case SELECTION_TYPE_EDITOR:
      if (
        sel && arranger_selections_has_any (sel)
        && !arranger_selections_contains_undeletable_object (sel))
        {
          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_delete (sel, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to delete selections"));
            }
        }
      break;
    default:
      g_warning ("not implemented yet");
      break;
    }
}

void
activate_duplicate (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (PROJECT);

  if (sel && arranger_selections_has_any (sel))
    {
      double length = arranger_selections_get_length_in_ticks (sel);

      GError * err = NULL;
      bool     ret = arranger_selections_action_perform_duplicate (
        sel, length, 0, 0, 0, 0, 0, NULL, F_NOT_ALREADY_MOVED, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to duplicate selections"));
        }
    }
}

void
activate_clear_selection (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (PROJECT);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
    case SELECTION_TYPE_EDITOR:
      if (sel)
        {
          arranger_selections_clear (sel, F_NO_FREE, F_PUBLISH_EVENTS);
        }
      break;
    case SELECTION_TYPE_TRACKLIST:
      tracklist_select_all (TRACKLIST, F_NO_SELECT, F_PUBLISH_EVENTS);
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      {
        Track * track =
          tracklist_selections_get_lowest_track (TRACKLIST_SELECTIONS);
        g_return_if_fail (IS_TRACK_AND_NONNULL (track));
        if (track_type_has_channel (track->type))
          {
            Channel *      ch = track_get_channel (track);
            PluginSlotType slot_type = PLUGIN_SLOT_INSERT;
            if (PROJECT->last_selection == SELECTION_TYPE_MIDI_FX)
              {
                slot_type = PLUGIN_SLOT_MIDI_FX;
              }
            channel_select_all (ch, slot_type, F_NO_SELECT);
          }
      }
      break;
    default:
      g_debug ("%s: doing nothing", __func__);
    }
}

void
activate_select_all (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (PROJECT);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
    case SELECTION_TYPE_EDITOR:
      if (sel)
        {
          arranger_selections_select_all (sel, F_PUBLISH_EVENTS);
        }
      break;
    case SELECTION_TYPE_TRACKLIST:
      tracklist_select_all (TRACKLIST, F_SELECT, F_PUBLISH_EVENTS);
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      {
        Track * track =
          tracklist_selections_get_lowest_track (TRACKLIST_SELECTIONS);
        g_return_if_fail (IS_TRACK_AND_NONNULL (track));
        if (track_type_has_channel (track->type))
          {
            Channel *      ch = track_get_channel (track);
            PluginSlotType slot_type = PLUGIN_SLOT_INSERT;
            if (PROJECT->last_selection == SELECTION_TYPE_MIDI_FX)
              {
                slot_type = PLUGIN_SLOT_MIDI_FX;
              }
            channel_select_all (ch, slot_type, F_SELECT);
          }
      }
      break;
    default:
      g_debug ("%s: doing nothing", __func__);
    }
}

void
activate_toggle_left_panel (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_debug ("left panel toggle");
  g_return_if_fail (MW_CENTER_DOCK);
  panel_dock_set_reveal_start (
    MW_CENTER_DOCK->dock, !panel_dock_get_reveal_start (MW_CENTER_DOCK->dock));
}

void
activate_toggle_right_panel (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_debug ("right panel toggle");
  g_return_if_fail (MW_CENTER_DOCK);
  panel_dock_set_reveal_end (
    MW_CENTER_DOCK->dock, !panel_dock_get_reveal_end (MW_CENTER_DOCK->dock));
}

void
activate_toggle_bot_panel (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_debug ("bot panel toggle");
  g_return_if_fail (MW_CENTER_DOCK);
  panel_dock_set_reveal_bottom (
    MW_CENTER_DOCK->dock, !panel_dock_get_reveal_bottom (MW_CENTER_DOCK->dock));
}

/**
 * Toggle timeline visibility.
 */
void
activate_toggle_top_panel (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_return_if_fail (MW_CENTER_DOCK);
  panel_dock_set_reveal_top (
    MW_CENTER_DOCK->dock, !panel_dock_get_reveal_top (MW_CENTER_DOCK->dock));
}

void
activate_toggle_status_bar (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  gtk_widget_set_visible (
    GTK_WIDGET (MW_BOT_BAR), !gtk_widget_get_visible (GTK_WIDGET (MW_BOT_BAR)));
}

DEFINE_SIMPLE (activate_toggle_drum_mode)
{
  Track * tr = clip_editor_get_track (CLIP_EDITOR);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));
  tr->drum_mode = !tr->drum_mode;

  EVENTS_PUSH (ET_DRUM_MODE_CHANGED, tr);
}

DEFINE_SIMPLE (activate_fullscreen)
{
  if (gtk_window_is_fullscreen (GTK_WINDOW (MAIN_WINDOW)))
    {
      g_debug ("unfullscreening");
      gtk_window_unfullscreen (GTK_WINDOW (MAIN_WINDOW));
    }
  else
    {
      g_debug ("fullscreening");
      gtk_window_fullscreen (GTK_WINDOW (MAIN_WINDOW));
    }
}

/**
 * All purpose menuitem callback for binding MIDI
 * CC to a port.
 *
 * An action will be performed if bound.
 */
void
activate_bind_midi_cc (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  g_return_if_fail (_variant);

  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);
  Port *       port = NULL;
  sscanf (variant, "%p", &port);
  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  BindCcDialogWidget * dialog = bind_cc_dialog_widget_new (port, true);

  z_gtk_dialog_run (GTK_DIALOG (dialog), true);
}

void
activate_delete_midi_cc_bindings (
  GSimpleAction * simple_action,
  GVariant *      _variant,
  gpointer        user_data)
{
  GtkSelectionModel * sel_model =
    gtk_column_view_get_model (MW_CC_BINDINGS->bindings_tree->column_view);
  guint       n_items = g_list_model_get_n_items (G_LIST_MODEL (sel_model));
  GPtrArray * mappings = g_ptr_array_new ();
  for (guint i = 0; i < n_items; i++)
    {
      if (gtk_selection_model_is_selected (sel_model, i))
        {
          WrappedObjectWithChangeSignal * wobj =
            g_list_model_get_item (G_LIST_MODEL (sel_model), i);
          MidiMapping * mm = (MidiMapping *) wobj->obj;
          g_ptr_array_add (mappings, mm);
        }
    }

  for (size_t i = 0; i < mappings->len; i++)
    {
      MidiMapping * mm = g_ptr_array_index (mappings, i);
      int           idx = midi_mapping_get_index (MIDI_MAPPINGS, mm);
      GError *      err = NULL;
      bool          ret = midi_mapping_action_perform_unbind (idx, &err);
      if (!ret)
        {
          HANDLE_ERROR_LITERAL (err, _ ("Failed to unbind"));
          return;
        }
      else
        {
          UndoableAction * ua = undo_manager_get_last_action (UNDO_MANAGER);
          ua->num_actions = i + 1;
        }
    }
}

void
activate_snap_to_grid (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  g_return_if_fail (_variant);

  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);
  if (string_is_equal (variant, "timeline"))
    {
      SNAP_GRID_TIMELINE->snap_to_grid = !SNAP_GRID_TIMELINE->snap_to_grid;
      EVENTS_PUSH (ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_TIMELINE);
    }
  else if (string_is_equal (variant, "editor"))
    {
      SNAP_GRID_EDITOR->snap_to_grid = !SNAP_GRID_EDITOR->snap_to_grid;
      EVENTS_PUSH (ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_EDITOR);
    }
  else if (string_is_equal (variant, "global"))
    {
      if (PROJECT->last_selection == SELECTION_TYPE_TIMELINE)
        {
          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_quantize (
            (ArrangerSelections *) TL_SELECTIONS, QUANTIZE_OPTIONS_TIMELINE,
            &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to quantize"));
            }
        }
    }
  else
    {
      g_return_if_reached ();
    }
}

void
activate_snap_keep_offset (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  g_return_if_fail (_variant);
  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);

  if (string_is_equal (variant, "timeline"))
    {
      SNAP_GRID_TIMELINE->snap_to_grid_keep_offset =
        !SNAP_GRID_TIMELINE->snap_to_grid_keep_offset;
      EVENTS_PUSH (ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_TIMELINE);
    }
  else if (string_is_equal (variant, "editor"))
    {
      SNAP_GRID_EDITOR->snap_to_grid_keep_offset =
        !SNAP_GRID_EDITOR->snap_to_grid_keep_offset;
      EVENTS_PUSH (ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_EDITOR);
    }
  else
    g_return_if_reached ();
}

void
activate_snap_events (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  g_return_if_fail (_variant);
  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);

  if (string_is_equal (variant, "timeline"))
    {
      SNAP_GRID_TIMELINE->snap_to_events = !SNAP_GRID_TIMELINE->snap_to_events;
      EVENTS_PUSH (ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_TIMELINE);
    }
  else if (string_is_equal (variant, "editor"))
    {
      SNAP_GRID_EDITOR->snap_to_events = !SNAP_GRID_EDITOR->snap_to_events;
      EVENTS_PUSH (ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_EDITOR);
    }
  else
    g_return_if_reached ();
}

static void
create_empty_track (TrackType type)
{
  if (zrythm_app_check_and_show_trial_limit_error (zrythm_app))
    return;

  GError * err = NULL;
  bool     ret = track_create_empty_with_action (type, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to create track"));
    }
}

static void
open_files_ready_cb (GtkFileDialog * dialog, GAsyncResult * res, void * user_data)
{
  GError *     err = NULL;
  GListModel * file_list =
    gtk_file_dialog_open_multiple_finish (dialog, res, &err);
  if (!file_list)
    {
      g_message ("Failed to retrieve file list: %s", err->message);
      g_error_free (err);
      return;
    }

  GStrvBuilder * uris_builder = g_strv_builder_new ();
  const guint    num_files = g_list_model_get_n_items (file_list);
  for (guint i = 0; i < num_files; i++)
    {
      GFile * file = G_FILE (g_list_model_get_item (file_list, i));
      char *  path = g_file_get_path (file);
      g_return_if_fail (path);
      g_object_unref (file);
      g_message ("Preparing to import file: %s...", path);
      g_free (path);
      char * uri = g_file_get_uri (file);
      g_strv_builder_add (uris_builder, uri);
    }

  char ** uris = g_strv_builder_end (uris_builder);
  if (!zrythm_app_check_and_show_trial_limit_error (zrythm_app))
    {
      err = NULL;
      bool success = tracklist_import_files (
        TRACKLIST, uris, NULL, NULL, NULL, -1, NULL, NULL, &err);
      if (!success)
        {
          HANDLE_ERROR_LITERAL (err, _ ("Failed to import files"));
        }
    }
  g_strfreev (uris);
}

DEFINE_SIMPLE (activate_import_file)
{
  GtkFileDialog * dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _ ("Select File(s)"));
  gtk_file_dialog_set_modal (dialog, true);
  GtkFileFilter * filter = gtk_file_filter_new ();
  /* note: this handles MIDI as well (audio/midi, audio/x-midi) */
  gtk_file_filter_add_mime_type (filter, "audio/*");
  GListStore * list_store = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_list_store_append (list_store, filter);
  gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (list_store));
  g_object_unref (list_store);
  gtk_file_dialog_open_multiple (
    dialog, GTK_WINDOW (MAIN_WINDOW), NULL,
    (GAsyncReadyCallback) open_files_ready_cb, NULL);
}

DEFINE_SIMPLE (activate_create_audio_track)
{
  create_empty_track (TRACK_TYPE_AUDIO);
}

DEFINE_SIMPLE (activate_create_midi_track)
{
  create_empty_track (TRACK_TYPE_MIDI);
}

DEFINE_SIMPLE (activate_create_audio_bus_track)
{
  create_empty_track (TRACK_TYPE_AUDIO_BUS);
}

DEFINE_SIMPLE (activate_create_midi_bus_track)
{
  create_empty_track (TRACK_TYPE_MIDI_BUS);
}

DEFINE_SIMPLE (activate_create_audio_group_track)
{
  create_empty_track (TRACK_TYPE_AUDIO_GROUP);
}

DEFINE_SIMPLE (activate_create_midi_group_track)
{
  create_empty_track (TRACK_TYPE_MIDI_GROUP);
}

DEFINE_SIMPLE (activate_create_folder_track)
{
  create_empty_track (TRACK_TYPE_FOLDER);
}

DEFINE_SIMPLE (activate_duplicate_selected_tracks)
{
  if (zrythm_app_check_and_show_trial_limit_error (zrythm_app))
    return;

  tracklist_selections_select_foldable_children (TRACKLIST_SELECTIONS);

  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    TRACKLIST_SELECTIONS->tracks[0]->pos + 1, &err);
  if (!ret)
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to duplicate tracks"));
    }
}

DEFINE_SIMPLE (activate_change_track_color)
{
  object_color_chooser_dialog_widget_run (
    GTK_WINDOW (MAIN_WINDOW), NULL, TRACKLIST_SELECTIONS, NULL);
}

DEFINE_SIMPLE (activate_goto_start_marker)
{
  transport_goto_start_marker (TRANSPORT);
}

DEFINE_SIMPLE (activate_goto_end_marker)
{
  transport_goto_end_marker (TRANSPORT);
}

DEFINE_SIMPLE (activate_goto_prev_marker)
{
  transport_goto_prev_marker (TRANSPORT);
}

DEFINE_SIMPLE (activate_goto_next_marker)
{
  transport_goto_next_marker (TRANSPORT);
}

DEFINE_SIMPLE (activate_play_pause)
{
  if (TRANSPORT_IS_ROLLING)
    {
      transport_request_pause (TRANSPORT, true);
      midi_events_panic_all (F_QUEUED);
    }
  else if (TRANSPORT_IS_PAUSED)
    {
      transport_request_roll (TRANSPORT, true);
    }
}

DEFINE_SIMPLE (activate_record_play)
{
  if (TRANSPORT_IS_ROLLING)
    {
      transport_request_pause (TRANSPORT, true);
      midi_events_panic_all (F_QUEUED);
    }
  else if (TRANSPORT_IS_PAUSED)
    {
      transport_set_recording (TRANSPORT, true, true, F_PUBLISH_EVENTS);
      transport_request_roll (TRANSPORT, true);
    }
}

static void
on_delete_tracks_response (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  if (string_is_equal (response, "delete"))
    {
      GError * err = NULL;
      bool     ret = tracklist_selections_action_perform_delete (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, &err);
      if (ret)
        {
          undo_manager_clear_stacks (UNDO_MANAGER, F_FREE);
          EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE, NULL);
        }
      else
        {
          HANDLE_ERROR_LITERAL (err, _ ("Failed to delete tracks"));
        }
    }
}

void
activate_delete_selected_tracks (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_message ("deleting selected tracks");

  tracklist_selections_select_foldable_children (TRACKLIST_SELECTIONS);

  if (tracklist_selections_contains_uninstantiated_plugin (TRACKLIST_SELECTIONS))
    {
      GtkWidget * dialog = adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), _ ("Delete Tracks?"),
        _ ("The selected tracks contain uninstantiated plugins. Deleting them "
           "will not be undoable and the undo history will be cleared. "
           "Continue with deletion?"));
      adw_message_dialog_add_responses (
        ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
        _ ("_Delete"), NULL);
      adw_message_dialog_set_response_appearance (
        ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
      adw_message_dialog_set_default_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      adw_message_dialog_set_close_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      g_signal_connect (
        dialog, "response", G_CALLBACK (on_delete_tracks_response), NULL);
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, &err);
  if (ret)
    {
      EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE, NULL);
    }
  else
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to delete tracks"));
    }
}

DEFINE_SIMPLE (activate_hide_selected_tracks)
{
  g_message ("hiding selected tracks");
  tracklist_selections_toggle_visibility (TRACKLIST_SELECTIONS);
}

DEFINE_SIMPLE (activate_pin_selected_tracks)
{
  g_message ("pin/unpinning selected tracks");

  Track * track = TRACKLIST_SELECTIONS->tracks[0];
  bool    pin = !track_is_pinned (track);

  GError * err = NULL;
  bool     ret;
  if (pin)
    {
      ret = tracklist_selections_action_perform_pin (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, &err);
    }
  else
    {
      ret = tracklist_selections_action_perform_unpin (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, &err);
    }

  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to pin/unpin tracks"));
    }
}

DEFINE_SIMPLE (activate_solo_selected_tracks)
{
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_solo (
    TRACKLIST_SELECTIONS, F_SOLO, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to solo tracks"));
    }
}

DEFINE_SIMPLE (activate_unsolo_selected_tracks)
{
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_solo (
    TRACKLIST_SELECTIONS, F_NO_SOLO, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to unsolo tracks"));
    }
}

DEFINE_SIMPLE (activate_mute_selected_tracks)
{
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_mute (
    TRACKLIST_SELECTIONS, F_MUTE, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to mute tracks"));
    }
}

DEFINE_SIMPLE (activate_unmute_selected_tracks)
{
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_mute (
    TRACKLIST_SELECTIONS, F_NO_MUTE, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to unmute tracks"));
    }
}

DEFINE_SIMPLE (activate_listen_selected_tracks)
{
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_listen (
    TRACKLIST_SELECTIONS, F_LISTEN, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to listen tracks"));
    }
}

DEFINE_SIMPLE (activate_unlisten_selected_tracks)
{
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_listen (
    TRACKLIST_SELECTIONS, F_NO_LISTEN, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to unlisten tracks"));
    }
}

DEFINE_SIMPLE (activate_enable_selected_tracks)
{
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_enable (
    TRACKLIST_SELECTIONS, F_ENABLE, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to enable tracks"));
    }
}

DEFINE_SIMPLE (activate_disable_selected_tracks)
{
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_enable (
    TRACKLIST_SELECTIONS, F_NO_ENABLE, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to disable tracks"));
    }
}

void
change_state_dim_output (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int dim = g_variant_get_boolean (value);

  g_message ("dim is %d", dim);

  g_simple_action_set_state (action, value);
}

void
change_state_loop (GSimpleAction * action, GVariant * value, gpointer user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  transport_set_loop (TRANSPORT, enabled, true);
}

void
change_state_metronome (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  bool enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  transport_set_metronome_enabled (TRANSPORT, enabled);
}

void
change_state_musical_mode (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (S_UI, "musical-mode", enabled);
}

void
change_state_listen_notes (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (S_UI, "listen-notes", enabled);
}

void
change_state_ghost_notes (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (S_UI, "ghost-notes", enabled);
}

static void
do_quantize (const char * variant, bool quick)
{
  g_debug ("quantize opts: %s", variant);

  if (
    string_is_equal (variant, "timeline")
    || (string_is_equal (variant, "global") && PROJECT->last_selection == SELECTION_TYPE_TIMELINE))
    {
      if (quick)
        {
          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_quantize (
            (ArrangerSelections *) TL_SELECTIONS, QUANTIZE_OPTIONS_TIMELINE,
            &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to quantize"));
            }
        }
      else
        {
          QuantizeDialogWidget * quant =
            quantize_dialog_widget_new (QUANTIZE_OPTIONS_TIMELINE);
          gtk_window_set_transient_for (
            GTK_WINDOW (quant), GTK_WINDOW (MAIN_WINDOW));
          z_gtk_dialog_run (GTK_DIALOG (quant), true);
        }
    }
  else if (
    string_is_equal (variant, "editor")
    || (string_is_equal (variant, "global") && PROJECT->last_selection == SELECTION_TYPE_EDITOR))
    {
      if (quick)
        {
          ArrangerSelections * sel =
            clip_editor_get_arranger_selections (CLIP_EDITOR);
          g_return_if_fail (sel);

          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_quantize (
            sel, QUANTIZE_OPTIONS_EDITOR, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to quantize"));
            }
        }
      else
        {
          QuantizeDialogWidget * quant =
            quantize_dialog_widget_new (QUANTIZE_OPTIONS_EDITOR);
          gtk_window_set_transient_for (
            GTK_WINDOW (quant), GTK_WINDOW (MAIN_WINDOW));
          z_gtk_dialog_run (GTK_DIALOG (quant), true);
        }
    }
  else
    {
      ui_show_message_printf (
        _ ("Invalid Selection For Quantize"),
        _ ("Must select either the timeline or the "
           "editor first. The current selection is "
           "%s"),
        selection_type_strings[PROJECT->last_selection].str);
    }
}

void
activate_quick_quantize (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  g_return_if_fail (_variant);
  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);
  do_quantize (variant, true);
}

void
activate_quantize_options (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  g_return_if_fail (_variant);
  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);
  do_quantize (variant, false);
}

void
activate_mute_selection (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  g_return_if_fail (_variant);

  gsize                size;
  const char *         variant = g_variant_get_string (_variant, &size);
  ArrangerSelections * sel = NULL;
  if (string_is_equal (variant, "timeline"))
    {
      sel = (ArrangerSelections *) TL_SELECTIONS;
    }
  else if (string_is_equal (variant, "editor"))
    {
      sel = (ArrangerSelections *) MA_SELECTIONS;
    }
  else if (string_is_equal (variant, "global"))
    {
      sel = project_get_arranger_selections_for_last_selection (PROJECT);
    }
  else
    {
      g_return_if_reached ();
    }

  if (sel)
    {
      GError * err = NULL;
      bool     ret = arranger_selections_action_perform_edit (
        sel, NULL, ARRANGER_SELECTIONS_ACTION_EDIT_MUTE, F_NOT_ALREADY_EDITED,
        &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to mute selections"));
        }
    }

  g_message ("mute/unmute selections");
}

DEFINE_SIMPLE (activate_merge_selection)
{
  g_message ("merge selections activated");

  if (TL_SELECTIONS->num_regions == 0)
    {
      ui_show_error_message ("Error", _ ("No regions selected"));
      return;
    }
  if (
    !arranger_selections_all_on_same_lane ((ArrangerSelections *) TL_SELECTIONS))
    {
      ui_show_error_message ("Error", _ ("Selections must be on the same lane"));
      return;
    }
  if (arranger_selections_contains_looped ((ArrangerSelections *) TL_SELECTIONS))
    {
      ui_show_error_message ("Error", _ ("Cannot merge looped regions"));
      return;
    }
  if (TL_SELECTIONS->num_regions == 1)
    {
      /* nothing to do */
      return;
    }

  GError * err = NULL;
  bool     ret = arranger_selections_action_perform_merge (
    (ArrangerSelections *) TL_SELECTIONS, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to merge selections"));
    }
}

void
activate_set_timebase_master (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_message ("set time base master");
}

void
activate_set_transport_client (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_message ("set transport client");
}

void
activate_unlink_jack_transport (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  g_message ("unlink jack transport");
}

void
activate_toggle_timeline_event_viewer (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  if (!MW_TIMELINE_EVENT_VIEWER)
    return;

  int new_visibility =
    !gtk_widget_get_visible (GTK_WIDGET (MW_TIMELINE_EVENT_VIEWER));

  g_settings_set_boolean (S_UI, "timeline-event-viewer-visible", new_visibility);
  gtk_widget_set_visible (GTK_WIDGET (MW_TIMELINE_EVENT_VIEWER), new_visibility);
}

void
activate_toggle_editor_event_viewer (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  if (!MW_EDITOR_EVENT_VIEWER_STACK)
    return;

  int new_visibility =
    !gtk_widget_get_visible (GTK_WIDGET (MW_EDITOR_EVENT_VIEWER_STACK));

  g_settings_set_boolean (S_UI, "editor-event-viewer-visible", new_visibility);
  gtk_widget_set_visible (
    GTK_WIDGET (MW_EDITOR_EVENT_VIEWER_STACK), new_visibility);
  bot_dock_edge_widget_update_event_viewer_stack_page (MW_BOT_DOCK_EDGE);
}

DEFINE_SIMPLE (activate_insert_silence)
{
  if (!TRANSPORT->has_range)
    return;

  Position start, end;
  transport_get_range_pos (TRANSPORT, true, &start);
  transport_get_range_pos (TRANSPORT, false, &end);

  GError * err = NULL;
  bool     ret = range_action_perform_insert_silence (&start, &end, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to insert silence"));
    }
}

DEFINE_SIMPLE (activate_remove_range)
{
  if (!TRANSPORT->has_range)
    return;

  Position start, end;
  transport_get_range_pos (TRANSPORT, true, &start);
  transport_get_range_pos (TRANSPORT, false, &end);

  GError * err = NULL;
  bool     ret = range_action_perform_remove (&start, &end, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to remove range"));
    }
}

DEFINE_SIMPLE (change_state_timeline_playhead_scroll_edges)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (S_UI, "timeline-playhead-scroll-edges", enabled);

  EVENTS_PUSH (ET_PLAYHEAD_SCROLL_MODE_CHANGED, NULL);
}

DEFINE_SIMPLE (change_state_timeline_playhead_follow)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (S_UI, "timeline-playhead-follow", enabled);

  EVENTS_PUSH (ET_PLAYHEAD_SCROLL_MODE_CHANGED, NULL);
}

DEFINE_SIMPLE (change_state_editor_playhead_scroll_edges)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (S_UI, "editor-playhead-scroll-edges", enabled);

  EVENTS_PUSH (ET_PLAYHEAD_SCROLL_MODE_CHANGED, NULL);
}

DEFINE_SIMPLE (change_state_editor_playhead_follow)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (S_UI, "editor-playhead-follow", enabled);

  EVENTS_PUSH (ET_PLAYHEAD_SCROLL_MODE_CHANGED, NULL);
}

/**
 * Common routine for applying undoable MIDI
 * functions.
 */
static void
do_midi_func (const MidiFunctionType type, const MidiFunctionOpts opts)
{
  ArrangerSelections * sel = (ArrangerSelections *) MA_SELECTIONS;
  if (!arranger_selections_has_any (sel))
    {
      g_message ("no selections, doing nothing");
      return;
    }

  switch (type)
    {
      /* no options needed for these */
    case MIDI_FUNCTION_FLIP_HORIZONTAL:
    case MIDI_FUNCTION_FLIP_VERTICAL:
    case MIDI_FUNCTION_LEGATO:
    case MIDI_FUNCTION_PORTATO:
    case MIDI_FUNCTION_STACCATO:
      {
        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_edit_midi_function (
          sel, type, opts, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to apply MIDI function"));
          }
      }
      break;
    default:
      {
        MidiFunctionDialogWidget * dialog =
          midi_function_dialog_widget_new (GTK_WINDOW (MAIN_WINDOW), type);
        gtk_window_present (GTK_WINDOW (dialog));
      }
    }
}

/**
 * Common routine for applying undoable automation
 * functions.
 */
static void
do_automation_func (AutomationFunctionType type)
{
  ArrangerSelections * sel = (ArrangerSelections *) AUTOMATION_SELECTIONS;
  if (!arranger_selections_has_any (sel))
    {
      g_message ("no selections, doing nothing");
      return;
    }

  GError * err = NULL;
  bool     ret = arranger_selections_action_perform_edit_automation_function (
    sel, type, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to apply automation function"));
    }
}

/**
 * Common routine for applying undoable audio
 * functions.
 *
 * @param uri Plugin URI, if applying plugin.
 */
static void
do_audio_func (
  const AudioFunctionType type,
  const AudioFunctionOpts opts,
  const char *            uri)
{
  g_return_if_fail (region_find (&CLIP_EDITOR->region_id));
  AUDIO_SELECTIONS->region_id = CLIP_EDITOR->region_id;
  ArrangerSelections * sel = (ArrangerSelections *) AUDIO_SELECTIONS;
  if (!arranger_selections_has_any (sel))
    {
      g_message ("no selections, doing nothing");
      return;
    }

  sel = arranger_selections_clone (sel);

  GError * err = NULL;
  bool     ret;
  if (!arranger_selections_validate (sel))
    {
      goto free_audio_sel_and_return;
    }

  zix_sem_wait (&PROJECT->save_sem);

  ret = arranger_selections_action_perform_edit_audio_function (
    sel, type, opts, uri, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to apply audio function"));
    }

  zix_sem_post (&PROJECT->save_sem);

free_audio_sel_and_return:
  arranger_selections_free (sel);
}

static void
set_pitch_ratio (void * object, const char * ratio_str)
{
  double ratio = strtod (ratio_str, NULL);
  if (ratio < 0.0001 || ratio > 100.0)
    {
      ui_show_error_message (
        "Error", _ ("Please enter a ratio between 0.0001 and 100."));
      return;
    }

  AudioFunctionOpts opts;
  opts.amount = ratio;
  do_audio_func (AUDIO_FUNCTION_PITCH_SHIFT, opts, NULL);
}

static const char *
get_pitch_ratio (void * object)
{
  static char * pitch_ratio = NULL;
  if (pitch_ratio == NULL)
    {
      g_free_and_null (pitch_ratio);
    }
  pitch_ratio = g_strdup_printf (
    "%f", g_settings_get_double (S_UI, "audio-function-pitch-shift-ratio"));
  return pitch_ratio;
}

DEFINE_SIMPLE (activate_editor_function)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

  ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    return;

  MidiFunctionOpts mopts = {};

  switch (region->id.type)
    {
    case REGION_TYPE_MIDI:
      {
        if (string_is_equal (str, "crescendo"))
          {
            do_midi_func (MIDI_FUNCTION_CRESCENDO, mopts);
          }
        else if (string_is_equal (str, "current"))
          {
            do_midi_func (g_settings_get_int (S_UI, "midi-function"), mopts);
          }
        else if (string_is_equal (str, "flam"))
          {
            do_midi_func (MIDI_FUNCTION_FLAM, mopts);
          }
        else if (string_is_equal (str, "flip-horizontal"))
          {
            do_midi_func (MIDI_FUNCTION_FLIP_HORIZONTAL, mopts);
          }
        else if (string_is_equal (str, "flip-vertical"))
          {
            do_midi_func (MIDI_FUNCTION_FLIP_VERTICAL, mopts);
          }
        else if (string_is_equal (str, "legato"))
          {
            do_midi_func (MIDI_FUNCTION_LEGATO, mopts);
          }
        else if (string_is_equal (str, "portato"))
          {
            do_midi_func (MIDI_FUNCTION_PORTATO, mopts);
          }
        else if (string_is_equal (str, "staccato"))
          {
            do_midi_func (MIDI_FUNCTION_STACCATO, mopts);
          }
        else if (string_is_equal (str, "strum"))
          {
            do_midi_func (MIDI_FUNCTION_STRUM, mopts);
          }
        else
          {
            g_return_if_reached ();
          }
      }
      break;
    case REGION_TYPE_AUTOMATION:
      {
        if (string_is_equal (str, "current"))
          {
            do_automation_func (
              g_settings_get_int (S_UI, "automation-function"));
          }
        else if (string_is_equal (str, "flip-horizontal"))
          {
            do_automation_func (AUTOMATION_FUNCTION_FLIP_HORIZONTAL);
          }
        else if (string_is_equal (str, "flip-vertical"))
          {
            do_automation_func (AUTOMATION_FUNCTION_FLIP_VERTICAL);
          }
        else if (string_is_equal (str, "flatten"))
          {
            do_automation_func (AUTOMATION_FUNCTION_FLATTEN);
          }
        else
          {
            g_return_if_reached ();
          }
      }
      break;
    case REGION_TYPE_AUDIO:
      {
        bool done = false;
        if (string_is_equal (str, "current"))
          {
            AudioFunctionOpts aopts = { 0 };
            AudioFunctionType type = g_settings_get_int (S_UI, "audio-function");
            switch (type)
              {
              case AUDIO_FUNCTION_PITCH_SHIFT:
                aopts.amount = g_settings_get_double (
                  S_UI, "audio-function-pitch-shift-ratio");
                break;
              default:
                break;
              }
            do_audio_func (type, aopts, NULL);
            done = true;
          }

        for (
          AudioFunctionType i = AUDIO_FUNCTION_INVERT;
          i < AUDIO_FUNCTION_CUSTOM_PLUGIN; i++)
          {
            char * audio_func_target =
              audio_function_get_action_target_for_type (i);
            if (string_is_equal (str, audio_func_target))
              {
                switch (i)
                  {
                  case AUDIO_FUNCTION_PITCH_SHIFT:
                    {
                      StringEntryDialogWidget * dialog =
                        string_entry_dialog_widget_new (
                          _ ("Pitch Ratio"), NULL,
                          (GenericStringGetter) get_pitch_ratio,
                          (GenericStringSetter) set_pitch_ratio);
                      gtk_window_present (GTK_WINDOW (dialog));
                      g_free (audio_func_target);
                    }
                    return;
                  default:
                    {
                      AudioFunctionOpts aopts = {};
                      do_audio_func (i, aopts, NULL);
                      break;
                    }
                  }
              }
            g_free (audio_func_target);
            done = true;
          }

        g_return_if_fail (done);
      }
      break;
    default:
      break;
    }
}

DEFINE_SIMPLE (activate_editor_function_lv2)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

  ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    return;

  PluginDescriptor * descr =
    plugin_manager_find_plugin_from_uri (PLUGIN_MANAGER, str);
  g_return_if_fail (descr);

  switch (region->id.type)
    {
    case REGION_TYPE_MIDI:
      {
      }
      break;
    case REGION_TYPE_AUDIO:
      {
        AudioFunctionOpts aopts = {};
        do_audio_func (AUDIO_FUNCTION_CUSTOM_PLUGIN, aopts, str);
      }
      break;
    default:
      g_return_if_reached ();
      break;
    }
}

DEFINE_SIMPLE (activate_midi_editor_highlighting)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

#define SET_HIGHLIGHT(txt, hl_type) \
  if (string_is_equal (str, txt)) \
    { \
      piano_roll_set_highlighting (PIANO_ROLL, PR_HIGHLIGHT_##hl_type); \
    }

  SET_HIGHLIGHT ("none", NONE);
  SET_HIGHLIGHT ("chord", CHORD);
  SET_HIGHLIGHT ("scale", SCALE);
  SET_HIGHLIGHT ("both", BOTH);

#undef SET_HIGHLIGHT
}

DEFINE_SIMPLE (activate_rename_track)
{
  g_return_if_fail (TRACKLIST_SELECTIONS->num_tracks == 1);
  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Track name"), TRACKLIST_SELECTIONS->tracks[0],
    (GenericStringGetter) track_get_name,
    (GenericStringSetter) track_set_name_with_action);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_rename_arranger_object)
{
  g_debug ("rename arranger object");

  if (PROJECT->last_selection == SELECTION_TYPE_TIMELINE)
    {
      ArrangerSelections * sel = arranger_widget_get_selections (MW_TIMELINE);
      if (arranger_selections_get_num_objects (sel) == 1)
        {
          ArrangerObject * obj = arranger_selections_get_first_object (sel);
          if (arranger_object_type_has_name (obj->type))
            {
              StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
                _ ("Object name"), obj,
                (GenericStringGetter) arranger_object_get_name,
                (GenericStringSetter) arranger_object_set_name_with_action);
              gtk_window_present (GTK_WINDOW (dialog));
            }
        }
    }
  else if (PROJECT->last_selection == SELECTION_TYPE_EDITOR)
    {
      /* nothing can be renamed yet */
    }
}

void
activate_create_arranger_object (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  double       x, y;
  const char * str;
  g_variant_get (value, "(sdd)", &str, &x, &y);
  ArrangerWidget * arranger;
  sscanf (str, "%p", &arranger);
  g_debug ("called %f %f", x, y);
  arranger_widget_create_item (arranger, x, y, false);
  bool action_performed =
    arranger_widget_finish_creating_item_from_action (arranger, x, y);
  if (!action_performed)
    {
      ui_show_notification (
        _ ("An object cannot be created at the selected location."));
    }
}

static void
on_region_color_dialog_response (
  GtkDialog * dialog,
  gint        response_id,
  gpointer    user_data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      /* clone the selections before the change */
      ArrangerSelections * sel_before =
        arranger_selections_clone ((ArrangerSelections *) TL_SELECTIONS);

      GdkRGBA color;
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (dialog), &color);

      for (int i = 0; i < TL_SELECTIONS->num_regions; i++)
        {
          /* change */
          ZRegion * r = TL_SELECTIONS->regions[i];
          r->use_color = true;
          r->color = color;
        }

      /* perform action */
      GError * err = NULL;
      bool     ret = arranger_selections_action_perform_edit (
        sel_before, (ArrangerSelections *) TL_SELECTIONS,
        ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE, true, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to set region color"));
        }

      arranger_selections_free_full (sel_before);
    }

  gtk_window_destroy (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_change_region_color)
{
  if (!timeline_selections_contains_only_regions (TL_SELECTIONS))
    return;

  ZRegion * r = TL_SELECTIONS->regions[0];
  GdkRGBA   color;
  if (r->use_color)
    {
      color = r->color;
    }
  else
    {
      Track * track = arranger_object_get_track ((ArrangerObject *) r);
      color = track->color;
    }

  GtkColorChooserDialog * dialog =
    GTK_COLOR_CHOOSER_DIALOG (gtk_color_chooser_dialog_new (
      _ ("Region Color"), GTK_WINDOW (UI_ACTIVE_WINDOW_OR_NULL)));
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (dialog), &color);

  g_signal_connect_after (
    G_OBJECT (dialog), "response", G_CALLBACK (on_region_color_dialog_response),
    NULL);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_reset_region_color)
{
  if (!timeline_selections_contains_only_regions (TL_SELECTIONS))
    return;

  /* clone the selections before the change */
  ArrangerSelections * sel_before =
    arranger_selections_clone ((ArrangerSelections *) TL_SELECTIONS);

  for (int i = 0; i < TL_SELECTIONS->num_regions; i++)
    {
      /* change */
      ZRegion * r = TL_SELECTIONS->regions[i];
      r->use_color = false;
    }

  /* perform action */
  GError * err = NULL;
  bool     ret = arranger_selections_action_perform_edit (
    sel_before, (ArrangerSelections *) TL_SELECTIONS,
    ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE, true, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to reset region color"));
    }

  arranger_selections_free_full (sel_before);
}

DEFINE_SIMPLE (activate_move_automation_regions)
{
  size_t       size;
  const char * _str = g_variant_get_string (variant, &size);
  Port *       port = NULL;
  sscanf (_str, "%p", &port);
  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  arranger_selections_action_perform_move_timeline (
    TL_SELECTIONS, 0, 0, 0, &port->id, F_NOT_ALREADY_MOVED, NULL);
}

DEFINE_SIMPLE (activate_add_region)
{
  if (TRACKLIST_SELECTIONS->num_tracks == 0)
    return;

  /*Track * track = TRACKLIST_SELECTIONS->tracks[0];*/

  /* TODO add region with default size */
}

DEFINE_SIMPLE (activate_go_to_start)
{
  Position pos;
  position_init (&pos);
  transport_move_playhead (
    TRANSPORT, &pos, F_PANIC, F_SET_CUE_POINT, F_PUBLISH_EVENTS);
}

DEFINE_SIMPLE (activate_input_bpm)
{
  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Please enter a BPM"), P_TEMPO_TRACK, tempo_track_get_current_bpm_as_str,
    tempo_track_set_bpm_from_str);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_tap_bpm)
{
  ui_show_message_literal (
    _ ("Unimplemented"), _ ("Tap BPM not implemented yet"));
}

void
change_state_show_automation_values (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (S_UI, "show-automation-values", enabled);

  EVENTS_PUSH (ET_AUTOMATION_VALUE_VISIBILITY_CHANGED, NULL);
}

DEFINE_SIMPLE (activate_nudge_selection)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

  double ticks = ARRANGER_SELECTIONS_DEFAULT_NUDGE_TICKS;
  bool   left = string_is_equal (str, "left");
  if (left)
    ticks = -ticks;

  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (PROJECT);
  if (!sel || !arranger_selections_has_any (sel))
    return;

  if (sel->type == ARRANGER_SELECTIONS_TYPE_AUDIO)
    {
      AudioFunctionOpts aopts = {};
      do_audio_func (
        left ? AUDIO_FUNCTION_NUDGE_LEFT : AUDIO_FUNCTION_NUDGE_RIGHT, aopts,
        NULL);
      return;
    }

  Position start_pos;
  arranger_selections_get_start_pos (sel, &start_pos, F_GLOBAL);
  if (start_pos.ticks + ticks < 0)
    {
      g_message ("cannot nudge left");
      return;
    }

  GError * err = NULL;
  bool     ret = arranger_selections_action_perform_move (
    sel, ticks, 0, 0, 0, 0, 0, NULL, F_NOT_ALREADY_MOVED, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to move selections"));
    }
}

DEFINE_SIMPLE (activate_detect_bpm)
{
  size_t       size;
  const char * _str = g_variant_get_string (variant, &size);
  ZRegion *    r = NULL;
  sscanf (_str, "%p", &r);
  g_return_if_fail (IS_REGION_AND_NONNULL (r));

  GArray * candidates = g_array_new (false, true, sizeof (float));
  bpm_t    bpm = audio_region_detect_bpm (r, candidates);

  GString * gstr = g_string_new (NULL);
  g_string_append_printf (gstr, _ ("Detected BPM: %.2f"), bpm);
  g_string_append (gstr, "\n\n");
  g_string_append_printf (gstr, _ ("Candidates:"));
  for (size_t i = 0; i < candidates->len; i++)
    {
      float candidate = g_array_index (candidates, float, i);
      g_string_append_printf (gstr, " %.2f", candidate);
    }
  char * str = g_string_free (gstr, false);
  ui_show_message_literal (NULL, str);
  g_free (str);
}

DEFINE_SIMPLE (activate_timeline_function)
{
  AudioFunctionType type = (AudioFunctionType) g_variant_get_int32 (variant);
  AudioFunctionOpts aopts;
  /* FIXME need to show a dialog here too to ask for the
   * pitch shift amount
   * TODO refactor code to avoid duplicating the code here */
  switch (type)
    {
    case AUDIO_FUNCTION_PITCH_SHIFT:
      aopts.amount =
        g_settings_get_double (S_UI, "audio-function-pitch-shift-ratio");
      break;
    default:
      break;
    }

  for (int i = 0; i < TL_SELECTIONS->num_regions; i++)
    {
      ZRegion *         r = TL_SELECTIONS->regions[i];
      AudioSelections * sel = (AudioSelections *) arranger_selections_new (
        ARRANGER_SELECTIONS_TYPE_AUDIO);
      region_identifier_copy (&sel->region_id, &r->id);
      sel->has_selection = true;
      ArrangerObject * r_obj = (ArrangerObject *) r;

      /* timeline start pos */
      position_set_to_pos (&sel->sel_start, &r_obj->clip_start_pos);
      position_add_ticks (&sel->sel_start, r_obj->pos.ticks);

      /* timeline end pos */
      position_set_to_pos (&sel->sel_end, &r_obj->loop_end_pos);
      position_add_ticks (&sel->sel_end, r_obj->pos.ticks);
      if (position_is_after (&sel->sel_end, &r_obj->end_pos))
        {
          position_set_to_pos (&sel->sel_end, &r_obj->end_pos);
        }

      GError * err = NULL;
      bool     ret = arranger_selections_action_perform_edit_audio_function (
        (ArrangerSelections *) sel, type, aopts, NULL, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to apply audio function"));
          break;
        }
      else
        {
          UndoableAction * ua = undo_manager_get_last_action (UNDO_MANAGER);
          ua->num_actions = i + 1;
        }
    }
}

DEFINE_SIMPLE (activate_export_midi_regions)
{
  export_midi_file_dialog_widget_run (GTK_WINDOW (MAIN_WINDOW), TL_SELECTIONS);
}

static void
bounce_progress_close_cb (ExportData * data)
{
  g_thread_join (data->thread);

  exporter_post_export (data->info, data->conns, data->state);

  if (
    progress_info_get_completion_type (data->info->progress_info)
    == PROGRESS_COMPLETED_SUCCESS)
    {
      /* create audio track with bounced material */
      Position first_pos;
      arranger_selections_get_start_pos (
        (ArrangerSelections *) TL_SELECTIONS, &first_pos, F_GLOBAL);
      exporter_create_audio_track_after_bounce (data->info, &first_pos);
    }
}

DEFINE_SIMPLE (activate_quick_bounce_selections)
{
  ArrangerSelections * sel = (ArrangerSelections *) TL_SELECTIONS;
  if (!arranger_selections_has_any (sel) || TL_SELECTIONS->num_regions == 0)
    {
      g_warning ("no selections to bounce");
      return;
    }

  ZRegion * r = TL_SELECTIONS->regions[0];

  ExportData * data = object_new (ExportData);
  data->state = object_new (EngineState);

  data->info = export_settings_new ();
  data->info->mode = EXPORT_MODE_REGIONS;
  export_settings_set_bounce_defaults (
    data->info, EXPORT_FORMAT_WAV, NULL, r->name);
  timeline_selections_mark_for_bounce (
    TL_SELECTIONS, data->info->bounce_with_parents);

  data->conns = exporter_prepare_tracks_for_export (data->info, data->state);

  /* start exporting in a new thread */
  data->thread = g_thread_new (
    "bounce_thread", (GThreadFunc) exporter_generic_export_thread, data->info);

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      data, true, bounce_progress_close_cb, false, F_CANCELABLE);
  adw_dialog_present (ADW_DIALOG (progress_dialog), GTK_WIDGET (MAIN_WINDOW));
}

DEFINE_SIMPLE (activate_bounce_selections)
{
  ArrangerSelections * sel = (ArrangerSelections *) TL_SELECTIONS;
  if (!arranger_selections_has_any (sel) || TL_SELECTIONS->num_regions == 0)
    {
      g_warning ("no selections to bounce");
      return;
    }

  ZRegion * r = TL_SELECTIONS->regions[0];

  BounceDialogWidget * dialog =
    bounce_dialog_widget_new (BOUNCE_DIALOG_REGIONS, r->name);
  z_gtk_dialog_run (GTK_DIALOG (dialog), true);
}

DEFINE_SIMPLE (activate_set_curve_algorithm)
{
  CurveAlgorithm curve_algo = (CurveAlgorithm) g_variant_get_int32 (variant);

  /* clone the selections before the change */
  ArrangerSelections * sel_before =
    arranger_selections_clone ((ArrangerSelections *) AUTOMATION_SELECTIONS);

  /* change */
  for (int i = 0; i < AUTOMATION_SELECTIONS->num_automation_points; i++)
    {
      AutomationPoint * ap = AUTOMATION_SELECTIONS->automation_points[i];
      ap->curve_opts.algo = curve_algo;
    }

  /* perform action */
  GError * err = NULL;
  bool     ret = arranger_selections_action_perform_edit (
    sel_before, (ArrangerSelections *) AUTOMATION_SELECTIONS,
    ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE, true, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to set curve algorithm"));
    }

  arranger_selections_free_full (sel_before);
}

static void
handle_region_fade_algo_preset (const char * pset_id, bool fade_in)
{
  GPtrArray *       arr = curve_get_fade_presets ();
  CurveFadePreset * pset = NULL;
  for (size_t i = 0; i < arr->len; i++)
    {
      CurveFadePreset * cur_pset = g_ptr_array_index (arr, i);

      if (string_is_equal (cur_pset->id, pset_id))
        {
          pset = cur_pset;
          break;
        }
    }
  g_ptr_array_unref (arr);
  g_return_if_fail (pset);

  CurveOptions curve_opts = pset->opts;

  z_return_if_fail_cmp (TL_SELECTIONS->num_regions, ==, 1);

  ArrangerSelections * sel_before =
    arranger_selections_clone ((ArrangerSelections *) TL_SELECTIONS);
  ZRegion *        r = TL_SELECTIONS->regions[0];
  ArrangerObject * r_obj = (ArrangerObject *) r;
  if (fade_in)
    {
      r_obj->fade_in_opts.algo = curve_opts.algo;
      r_obj->fade_in_opts.curviness = curve_opts.curviness;
    }
  else
    {
      r_obj->fade_out_opts.algo = curve_opts.algo;
      r_obj->fade_out_opts.curviness = curve_opts.curviness;
    }

  GError * err = NULL;
  bool     ret = arranger_selections_action_perform_edit (
    sel_before, (ArrangerSelections *) TL_SELECTIONS,
    ARRANGER_SELECTIONS_ACTION_EDIT_FADES, true, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to edit fades"));
    }
}

DEFINE_SIMPLE (activate_set_region_fade_in_algorithm_preset)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

  handle_region_fade_algo_preset (str, true);
}

DEFINE_SIMPLE (activate_set_region_fade_out_algorithm_preset)
{
  size_t       size;
  const char * str = g_variant_get_string (variant, &size);

  handle_region_fade_algo_preset (str, false);
}

DEFINE_SIMPLE (activate_arranger_object_view_info)
{
  size_t           size;
  const char *     str = g_variant_get_string (variant, &size);
  ArrangerObject * obj = NULL;
  sscanf (str, "%p", &obj);
  g_return_if_fail (obj != NULL);
  g_return_if_fail (IS_ARRANGER_OBJECT (obj));

  ArrangerObjectInfoDialogWidget * dialog =
    arranger_object_info_dialog_widget_new (obj);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_save_chord_preset)
{
  g_debug ("save chord preset");

  int num_packs =
    chord_preset_pack_manager_get_num_packs (CHORD_PRESET_PACK_MANAGER);
  bool have_custom = false;
  for (int i = 0; i < num_packs; i++)
    {
      ChordPresetPack * pack =
        chord_preset_pack_manager_get_pack_at (CHORD_PRESET_PACK_MANAGER, i);
      if (!pack->is_standard)
        {
          have_custom = true;
          break;
        }
    }

  if (have_custom)
    {
      SaveChordPresetDialogWidget * dialog =
        save_chord_preset_dialog_widget_new (GTK_WINDOW (MAIN_WINDOW));
      gtk_window_present (GTK_WINDOW (dialog));
    }
  else
    {
      ui_show_error_message (
        "Error",
        _ ("No custom preset packs found. Please "
           "create a preset pack first from the "
           "chord preset browser."));
    }
}

DEFINE_SIMPLE (activate_load_chord_preset)
{
  g_debug ("load chord preset");
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  int          pack_idx, pset_idx;
  sscanf (str, "%d,%d", &pack_idx, &pset_idx);

  int num_packs =
    chord_preset_pack_manager_get_num_packs (CHORD_PRESET_PACK_MANAGER);
  z_return_if_fail_cmp (pack_idx, <, num_packs);
  ChordPresetPack * pack =
    chord_preset_pack_manager_get_pack_at (CHORD_PRESET_PACK_MANAGER, pack_idx);
  z_return_if_fail_cmp (pset_idx, <, pack->num_presets);
  ChordPreset * pset = pack->presets[pset_idx];
  chord_editor_apply_preset (CHORD_EDITOR, pset, true);
}

DEFINE_SIMPLE (activate_load_chord_preset_from_scale)
{
  g_debug ("load chord preset from scale");
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  int          scale, root_note;
  sscanf (str, "%d,%d", &scale, &root_note);

  chord_editor_apply_preset_from_scale (
    CHORD_EDITOR, (MusicalScaleType) scale, (MusicalNote) root_note, true);
}

DEFINE_SIMPLE (activate_transpose_chord_pad)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  g_debug ("transpose chord pad %s", str);

  if (string_is_equal (str, "up"))
    {
      chord_editor_transpose_chords (CHORD_EDITOR, true, true);
    }
  else if (string_is_equal (str, "down"))
    {
      chord_editor_transpose_chords (CHORD_EDITOR, false, true);
    }
  else
    {
      g_critical ("invalid parameter %s", str);
    }
}

static void
on_chord_preset_pack_add_response (
  AdwMessageDialog * dialog,
  char *             response,
  ChordPresetPack *  pack)
{
  if (string_is_equal (response, "ok"))
    {
      if (strlen (pack->name) > 0)
        {
          chord_preset_pack_manager_add_pack (
            CHORD_PRESET_PACK_MANAGER, pack, true);
        }
      else
        {
          ui_show_error_message ("Error", _ ("Failed to create pack"));
        }
    }

  chord_preset_pack_free (pack);
}

DEFINE_SIMPLE (activate_add_chord_preset_pack)
{
  ChordPresetPack * pack = chord_preset_pack_new ("", false);

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Preset Pack Name"), pack,
    (GenericStringGetter) chord_preset_pack_get_name,
    (GenericStringSetter) chord_preset_pack_set_name);
  g_signal_connect_after (
    G_OBJECT (dialog), "response",
    G_CALLBACK (on_chord_preset_pack_add_response), pack);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_delete_preset_chord_pack_response (
  AdwMessageDialog * dialog,
  char *             response,
  ChordPresetPack *  pack)
{
  if (string_is_equal (response, "delete"))
    {
      chord_preset_pack_manager_delete_pack (
        CHORD_PRESET_PACK_MANAGER, pack, true);
    }
}

DEFINE_SIMPLE (activate_delete_chord_preset_pack)
{
  gsize             size;
  const char *      str = g_variant_get_string (variant, &size);
  ChordPresetPack * pack = NULL;
  sscanf (str, "%p", &pack);
  g_return_if_fail (pack);

  GtkWidget * dialog = adw_message_dialog_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Delete?"),
    _ ("Are you sure you want to delete this chord preset pack?"));
  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
    _ ("_Delete"), NULL);
  adw_message_dialog_set_response_appearance (
    ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_default_response (
    ADW_MESSAGE_DIALOG (dialog), "cancel");
  adw_message_dialog_set_close_response (ADW_MESSAGE_DIALOG (dialog), "cancel");
  g_signal_connect (
    dialog, "response", G_CALLBACK (on_delete_preset_chord_pack_response), pack);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_rename_chord_preset_pack)
{
  gsize             size;
  const char *      str = g_variant_get_string (variant, &size);
  ChordPresetPack * pack = NULL;
  sscanf (str, "%p", &pack);
  g_return_if_fail (pack);

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Preset Pack Name"), pack,
    (GenericStringGetter) chord_preset_pack_get_name,
    (GenericStringSetter) chord_preset_pack_set_name);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_delete_chord_preset_response (
  AdwMessageDialog * dialog,
  char *             response,
  ChordPreset *      pset)
{
  if (string_is_equal (response, "delete"))
    {
      chord_preset_pack_manager_delete_preset (
        CHORD_PRESET_PACK_MANAGER, pset, true);
    }
}

DEFINE_SIMPLE (activate_delete_chord_preset)
{
  gsize         size;
  const char *  str = g_variant_get_string (variant, &size);
  ChordPreset * pset = NULL;
  sscanf (str, "%p", &pset);
  g_return_if_fail (pset);

  GtkWidget * dialog = adw_message_dialog_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Delete?"),
    _ ("Are you sure you want to delete this chord preset?"));
  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
    _ ("_Delete"), NULL);
  adw_message_dialog_set_response_appearance (
    ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_default_response (
    ADW_MESSAGE_DIALOG (dialog), "cancel");
  adw_message_dialog_set_close_response (ADW_MESSAGE_DIALOG (dialog), "cancel");
  g_signal_connect (
    dialog, "response", G_CALLBACK (on_delete_chord_preset_response), pset);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_rename_chord_preset)
{
  gsize         size;
  const char *  str = g_variant_get_string (variant, &size);
  ChordPreset * pset = NULL;
  sscanf (str, "%p", &pset);
  g_return_if_fail (pset);

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Preset Name"), pset, (GenericStringGetter) chord_preset_get_name,
    (GenericStringSetter) chord_preset_set_name);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_reset_stereo_balance)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  Port *       port = NULL;
  sscanf (str, "%p", &port);
  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  Track * track = port_get_track (port, true);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));
  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_edit_single_float (
    EDIT_TRACK_ACTION_TYPE_PAN, track, port->control, 0.5f, false, &err);
  if (!ret)
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to change balance"));
    }
}

DEFINE_SIMPLE (activate_plugin_toggle_enabled)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  Plugin *     pl = NULL;
  sscanf (str, "%p", &pl);
  g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));

  GError * err = NULL;
  bool     new_val = !plugin_is_enabled (pl, false);
  if (!plugin_is_selected (pl))
    {
      plugin_select (pl, F_SELECT, F_EXCLUSIVE);
    }
  bool success = mixer_selections_action_perform_change_status (
    MIXER_SELECTIONS, new_val, &err);
  if (!success)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to change plugin states"));
    }
}

DEFINE_SIMPLE (activate_plugin_change_load_behavior)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  Plugin *     pl = NULL;
  char         new_behavior[600];
  sscanf (str, "%p,%s", &pl, new_behavior);
  g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));

  plugin_select (pl, F_SELECT, F_EXCLUSIVE);

  CarlaBridgeMode new_bridge_mode = CARLA_BRIDGE_NONE;
  if (string_is_equal (new_behavior, "normal"))
    {
      new_bridge_mode = CARLA_BRIDGE_NONE;
    }
  else if (string_is_equal (new_behavior, "ui"))
    {
      new_bridge_mode = CARLA_BRIDGE_UI;
    }
  else if (string_is_equal (new_behavior, "full"))
    {
      new_bridge_mode = CARLA_BRIDGE_FULL;
    }

  GError * err = NULL;
  bool     success = mixer_selections_action_perform_change_load_behavior (
    MIXER_SELECTIONS, new_bridge_mode, &err);
  if (!success)
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to change load behavior"));
    }
}

DEFINE_SIMPLE (activate_plugin_inspect)
{
  left_dock_edge_widget_refresh_with_page (
    MW_LEFT_DOCK_EDGE, LEFT_DOCK_EDGE_TAB_PLUGIN);
}

static void
delete_plugins (bool clear_stacks)
{
  GError * err = NULL;
  bool     success = mixer_selections_action_perform_delete (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR, &err);
  if (success && clear_stacks)
    {
      undo_manager_clear_stacks (UNDO_MANAGER, F_FREE);
      EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE, NULL);
    }
  else if (!success)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to delete plugins"));
    }
}

static void
on_delete_plugins_response (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  if (string_is_equal (response, "delete"))
    {
      delete_plugins (true);
    }
}

DEFINE_SIMPLE (activate_mixer_selections_delete)
{

  if (mixer_selections_contains_uninstantiated_plugin (MIXER_SELECTIONS))
    {
      GtkWidget * dialog = adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), _ ("Delete Plugins?"),
        _ ("The selection contains uninstantiated plugins. Deleting them "
           "will not be undoable and the undo history will be cleared."));
      adw_message_dialog_add_responses (
        ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
        _ ("_Delete"), NULL);
      adw_message_dialog_set_response_appearance (
        ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
      adw_message_dialog_set_default_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      adw_message_dialog_set_close_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      g_signal_connect (
        dialog, "response", G_CALLBACK (on_delete_plugins_response), NULL);
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  delete_plugins (false);
}

DEFINE_SIMPLE (activate_reset_fader)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  Fader *      fader = NULL;
  sscanf (str, "%p", &fader);
  g_return_if_fail (fader);

  if (fader->type == FADER_TYPE_AUDIO_CHANNEL)
    {
      Channel * ch = fader_get_channel (fader);
      channel_reset_fader (ch, F_PUBLISH_EVENTS);
    }
  else
    {
      fader_set_amp (fader, 1.0);
    }
}

DEFINE_SIMPLE (activate_reset_control)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  Port *       port = NULL;
  sscanf (str, "%p", &port);
  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  GError * err = NULL;
  bool     ret = port_action_perform_reset_control (&port->id, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, _ ("Failed to reset %s"), port->id.label);
    }
}

DEFINE_SIMPLE (activate_port_view_info)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  Port *       port = NULL;
  sscanf (str, "%p", &port);
  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  PortInfoDialogWidget * dialog = port_info_dialog_widget_new (port);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_port_connection_remove)
{
  GError * err = NULL;
  bool     ret = port_connection_action_perform_disconnect (
    &MW_PORT_CONNECTIONS_TREE->src_port->id,
    &MW_PORT_CONNECTIONS_TREE->dest_port->id, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, _ ("Failed to disconnect %s from %s"),
        MW_PORT_CONNECTIONS_TREE->src_port->id.label,
        MW_PORT_CONNECTIONS_TREE->dest_port->id.label);
    }
}

DEFINE_SIMPLE (activate_panel_file_browser_add_bookmark)
{
  gsize           size;
  const char *    str = g_variant_get_string (variant, &size);
  SupportedFile * sfile = NULL;
  sscanf (str, "%p", &sfile);
  g_return_if_fail (sfile != NULL);

  file_manager_add_location_and_save (FILE_MANAGER, sfile->abs_path);

  EVENTS_PUSH (ET_FILE_BROWSER_BOOKMARK_ADDED, NULL);
}

static void
on_bookmark_delete_response (
  AdwMessageDialog * dialog,
  char *             response,
  const char *       path)
{
  if (string_is_equal (response, "delete"))
    {
      file_manager_remove_location_and_save (FILE_MANAGER, path, true);

      EVENTS_PUSH (ET_FILE_BROWSER_BOOKMARK_DELETED, NULL);
    }
}

DEFINE_SIMPLE (activate_panel_file_browser_delete_bookmark)
{
  FileBrowserLocation * loc =
    panel_file_browser_widget_get_selected_bookmark (MW_PANEL_FILE_BROWSER);
  g_return_if_fail (loc);

  if (loc->special_location > FILE_MANAGER_NONE)
    {
      ui_show_error_message ("Error", _ ("Cannot delete standard bookmark"));
      return;
    }

  GtkWidget * dialog = adw_message_dialog_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Delete?"),
    _ ("Are you sure you want to delete this bookmark?"));
  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
    _ ("_Delete"), NULL);
  adw_message_dialog_set_response_appearance (
    ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_default_response (
    ADW_MESSAGE_DIALOG (dialog), "cancel");
  adw_message_dialog_set_close_response (ADW_MESSAGE_DIALOG (dialog), "cancel");
  g_signal_connect (
    dialog, "response", G_CALLBACK (on_bookmark_delete_response), loc->path);
  gtk_window_present (GTK_WINDOW (dialog));
}

/**
 * Activate \ref setting if given, otherwise create
 * a defeault setting from the descriptor.
 */
static void
activate_plugin_setting (PluginSetting * setting, PluginDescriptor * descr)
{
  if (zrythm_app_check_and_show_trial_limit_error (zrythm_app))
    return;

  bool setting_created = false;
  if (!setting)
    {
      g_return_if_fail (descr);
      setting = plugin_setting_new_default (descr);
      setting_created = true;
    }

  plugin_setting_activate (setting);

  if (setting_created)
    {
      plugin_setting_free (setting);
    }
}

DEFINE_SIMPLE (activate_plugin_browser_add_to_project)
{
  gsize              size;
  const char *       str = g_variant_get_string (variant, &size);
  PluginDescriptor * descr = NULL;
  sscanf (str, "%p", &descr);
  g_return_if_fail (descr != NULL);

  PluginSetting * setting = plugin_setting_new_default (descr);
  /*setting->open_with_carla = true;*/
  /*setting->bridge_mode = CARLA_BRIDGE_NONE;*/
  activate_plugin_setting (setting, NULL);
  plugin_setting_free (setting);
}

DEFINE_SIMPLE (activate_plugin_browser_add_to_project_carla)
{
  gsize              size;
  const char *       str = g_variant_get_string (variant, &size);
  PluginDescriptor * descr = NULL;
  sscanf (str, "%p", &descr);
  g_return_if_fail (descr != NULL);

  PluginSetting * setting = plugin_setting_new_default (descr);
  setting->open_with_carla = true;
  setting->bridge_mode = CARLA_BRIDGE_NONE;
  activate_plugin_setting (setting, NULL);
  plugin_setting_free (setting);
}

DEFINE_SIMPLE (activate_plugin_browser_add_to_project_bridged_ui)
{
  gsize              size;
  const char *       str = g_variant_get_string (variant, &size);
  PluginDescriptor * descr = NULL;
  sscanf (str, "%p", &descr);
  g_return_if_fail (descr != NULL);

  PluginSetting * setting = plugin_setting_new_default (descr);
  setting->open_with_carla = true;
  setting->bridge_mode = CARLA_BRIDGE_UI;
  activate_plugin_setting (setting, NULL);
  plugin_setting_free (setting);
}

DEFINE_SIMPLE (activate_plugin_browser_add_to_project_bridged_full)
{
  gsize              size;
  const char *       str = g_variant_get_string (variant, &size);
  PluginDescriptor * descr = NULL;
  sscanf (str, "%p", &descr);
  g_return_if_fail (descr != NULL);

  PluginSetting * setting = plugin_setting_new_default (descr);
  setting->open_with_carla = true;
  setting->bridge_mode = CARLA_BRIDGE_FULL;
  activate_plugin_setting (setting, NULL);
  plugin_setting_free (setting);
}

DEFINE_SIMPLE (change_state_plugin_browser_toggle_generic_ui) { }

DEFINE_SIMPLE (activate_plugin_browser_add_to_collection)
{
  gsize                    size;
  const char *             str = g_variant_get_string (variant, &size);
  PluginCollection *       collection = NULL;
  const PluginDescriptor * descr = NULL;
  sscanf (str, "%p,%p", &collection, &descr);
  g_return_if_fail (collection != NULL);
  g_return_if_fail (descr != NULL);

  plugin_collection_add_descriptor (collection, descr);
  plugin_collections_serialize_to_file (PLUGIN_MANAGER->collections);

  EVENTS_PUSH (ET_PLUGIN_COLLECTIONS_CHANGED, NULL);
}

DEFINE_SIMPLE (activate_plugin_browser_remove_from_collection)
{
  gsize                    size;
  const char *             str = g_variant_get_string (variant, &size);
  PluginCollection *       collection = NULL;
  const PluginDescriptor * descr = NULL;
  sscanf (str, "%p,%p", &collection, &descr);
  g_return_if_fail (collection != NULL);
  g_return_if_fail (descr != NULL);

  plugin_collection_remove_descriptor (collection, descr);
  plugin_collections_serialize_to_file (PLUGIN_MANAGER->collections);

  EVENTS_PUSH (ET_PLUGIN_COLLECTIONS_CHANGED, NULL);
}

DEFINE_SIMPLE (activate_plugin_browser_reset)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  GtkListView * list_view = NULL;
  if (string_is_equal (str, "category"))
    {
      list_view = MW_PLUGIN_BROWSER->category_list_view;
    }
  else if (string_is_equal (str, "author"))
    {
      list_view = MW_PLUGIN_BROWSER->author_list_view;
    }
  else if (string_is_equal (str, "protocol"))
    {
      list_view = MW_PLUGIN_BROWSER->protocol_list_view;
    }
  else if (string_is_equal (str, "collection"))
    {
      list_view = MW_PLUGIN_BROWSER->collection_list_view;
    }

  if (list_view)
    {
      GtkSelectionModel * model = gtk_list_view_get_model (list_view);
      gtk_selection_model_unselect_all (model);
    }
}

static void
on_plugin_collection_add_response (
  AdwMessageDialog * dialog,
  char *             response,
  PluginCollection * collection)
{
  if (string_is_equal (response, "ok"))
    {
      if (strlen (collection->name) > 0)
        {
          g_debug ("accept collection");
          plugin_collections_add (
            PLUGIN_MANAGER->collections, collection, F_SERIALIZE);
          EVENTS_PUSH (ET_PLUGIN_COLLECTIONS_CHANGED, NULL);
        }
      else
        {
          g_message ("invalid collection name (empty)");
        }
    }

  plugin_collection_free (collection);
}

DEFINE_SIMPLE (activate_plugin_collection_add)
{
  PluginCollection * collection = plugin_collection_new ();

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Collection name"), collection,
    (GenericStringGetter) plugin_collection_get_name,
    (GenericStringSetter) plugin_collection_set_name);
  g_signal_connect_after (
    G_OBJECT (dialog), "response",
    G_CALLBACK (on_plugin_collection_add_response), collection);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_plugin_collection_rename_response (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  plugin_collections_serialize_to_file (PLUGIN_MANAGER->collections);

  EVENTS_PUSH (ET_PLUGIN_COLLECTIONS_CHANGED, NULL);
}

DEFINE_SIMPLE (activate_plugin_collection_rename)
{
  if (MW_PLUGIN_BROWSER->selected_collections->len != 1)
    {
      g_warning ("should not be allowed");
      return;
    }
  PluginCollection * collection = (PluginCollection *) g_ptr_array_index (
    MW_PLUGIN_BROWSER->selected_collections, 0);

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Collection name"), collection,
    (GenericStringGetter) plugin_collection_get_name,
    (GenericStringSetter) plugin_collection_set_name);
  g_signal_connect_after (
    G_OBJECT (dialog), "response",
    G_CALLBACK (on_plugin_collection_rename_response), collection);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_delete_plugin_collection_response (
  AdwMessageDialog * dialog,
  char *             response,
  PluginCollection * collection)
{
  if (string_is_equal (response, "delete"))
    {
      plugin_collections_remove (
        PLUGIN_MANAGER->collections, collection, F_SERIALIZE);

      EVENTS_PUSH (ET_PLUGIN_COLLECTIONS_CHANGED, NULL);
    }
}

DEFINE_SIMPLE (activate_plugin_collection_remove)
{
  if (MW_PLUGIN_BROWSER->selected_collections->len == 0)
    {
      g_warning ("should not be allowed");
      return;
    }
  PluginCollection * collection = (PluginCollection *) g_ptr_array_index (
    MW_PLUGIN_BROWSER->selected_collections, 0);

  int result = GTK_RESPONSE_YES;
  if (collection->num_descriptors > 0)
    {
      GtkWidget * dialog =
        adw_message_dialog_new (GTK_WINDOW (MAIN_WINDOW), _ ("Delete?"), NULL);
      adw_message_dialog_format_body_markup (
        ADW_MESSAGE_DIALOG (dialog),
        _ ("The collection '%s' contains %d plugins. "
           "Are you sure you want to remove it?"),
        collection->name, collection->num_descriptors);
      adw_message_dialog_add_responses (
        ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
        _ ("_Delete"), NULL);
      adw_message_dialog_set_response_appearance (
        ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
      adw_message_dialog_set_default_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      adw_message_dialog_set_close_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      g_signal_connect (
        dialog, "response", G_CALLBACK (on_delete_plugin_collection_response),
        collection);
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  if (result == GTK_RESPONSE_YES)
    {
      plugin_collections_remove (
        PLUGIN_MANAGER->collections, collection, F_SERIALIZE);

      EVENTS_PUSH (ET_PLUGIN_COLLECTIONS_CHANGED, NULL);
    }
}

DEFINE_SIMPLE (activate_track_set_midi_channel)
{
  /* "{track index},{lane index or -1},"
   * "{midi channel 1-16 or 0 for lane to inherit}"
   */
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  int          track_idx, lane_idx, midi_ch;
  sscanf (str, "%d,%d,%d", &track_idx, &lane_idx, &midi_ch);

  Track * track = TRACKLIST->tracks[track_idx];
  if (lane_idx >= 0)
    {
      TrackLane * lane = track->lanes[lane_idx];
      g_message (
        "setting lane '%s' (%d) midi channel to "
        "%d",
        lane->name, lane_idx, midi_ch);
      lane->midi_ch = (midi_byte_t) midi_ch;
    }
  else
    {
      g_message (
        "setting track '%s' (%d) midi channel to "
        "%d",
        track->name, track_idx, midi_ch);
      track->midi_ch = (midi_byte_t) midi_ch;
    }
}

static void
bounce_selected_tracks_progress_close_cb (ExportData * data)
{
  g_thread_join (data->thread);

  exporter_post_export (data->info, data->conns, data->state);

  ProgressInfo * pinfo = data->info->progress_info;

  if (progress_info_get_completion_type (pinfo) == PROGRESS_COMPLETED_SUCCESS)
    {
      /* create audio track with bounced material */
      Marker *         m = marker_track_get_start_marker (P_MARKER_TRACK);
      ArrangerObject * m_obj = (ArrangerObject *) m;
      exporter_create_audio_track_after_bounce (data->info, &m_obj->pos);
    }
}

DEFINE_SIMPLE (activate_quick_bounce_selected_tracks)
{
  Track * track = TRACKLIST_SELECTIONS->tracks[0];

  ExportData * data = object_new (ExportData);
  data->state = object_new (EngineState);

  data->info = export_settings_new ();
  data->info->mode = EXPORT_MODE_TRACKS;
  export_settings_set_bounce_defaults (
    data->info, EXPORT_FORMAT_WAV, NULL, track->name);
  tracklist_selections_mark_for_bounce (
    TRACKLIST_SELECTIONS, data->info->bounce_with_parents, F_NO_MARK_MASTER);

  data->conns = exporter_prepare_tracks_for_export (data->info, data->state);

  /* start exporting in a new thread */
  data->thread = g_thread_new (
    "bounce_thread", (GThreadFunc) exporter_generic_export_thread, data->info);

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      data, true, bounce_selected_tracks_progress_close_cb, false, F_CANCELABLE);
  adw_dialog_present (ADW_DIALOG (progress_dialog), GTK_WIDGET (MAIN_WINDOW));
}

DEFINE_SIMPLE (activate_bounce_selected_tracks)
{
  Track *              track = TRACKLIST_SELECTIONS->tracks[0];
  BounceDialogWidget * dialog =
    bounce_dialog_widget_new (BOUNCE_DIALOG_TRACKS, track->name);
  z_gtk_dialog_run (GTK_DIALOG (dialog), true);
}

static void
handle_direct_out_change (int direct_out_idx, bool new_group)
{
  TracklistSelections * sel_before =
    tracklist_selections_clone (TRACKLIST_SELECTIONS, NULL);

  Track * direct_out = NULL;
  if (new_group)
    {
      direct_out = add_tracks_to_group_dialog_widget_get_track (sel_before);
      if (!direct_out)
        return;
    }
  else
    {
      direct_out = tracklist_get_track (TRACKLIST, direct_out_idx);
    }
  g_return_if_fail (direct_out);

  /* skip if all selected tracks already connected
   * to direct out */
  bool need_change = false;
  for (int i = 0; i < sel_before->num_tracks; i++)
    {
      Track * cur_track = TRACKLIST->tracks[sel_before->tracks[i]->pos];
      if (!track_type_has_channel (cur_track->type))
        return;

      if (cur_track->out_signal_type != direct_out->in_signal_type)
        {
          g_message ("mismatching signal type");
          return;
        }

      Channel * ch = track_get_channel (cur_track);
      g_return_if_fail (IS_CHANNEL_AND_NONNULL (ch));
      if (channel_get_output_track (ch) != direct_out)
        {
          need_change = true;
          break;
        }
    }

  if (!need_change)
    {
      g_message ("no direct out change needed");
      return;
    }

  if (new_group)
    {
      /* reset the selections */
      tracklist_selections_clear (TRACKLIST_SELECTIONS, F_PUBLISH_EVENTS);
      for (int i = 0; i < sel_before->num_tracks; i++)
        {
          Track * cur_track = TRACKLIST->tracks[sel_before->tracks[i]->pos];
          track_select (
            cur_track, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
        }
    }

  UndoableAction * prev_action = undo_manager_get_last_action (UNDO_MANAGER);

  GError * err = NULL;
  bool     ret = tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, direct_out, &err);
  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to change direct output"));
    }
  else
    {
      UndoableAction * ua = undo_manager_get_last_action (UNDO_MANAGER);
      if (new_group)
        {
          /* see add_tracks_to_group_dialog for prev action */
          ua->num_actions = prev_action->num_actions + 1;
        }
    }

  /* free previous selections */
  tracklist_selections_free (sel_before);
}

DEFINE_SIMPLE (activate_selected_tracks_direct_out_to)
{
  int direct_out_pos = g_variant_get_int32 (variant);
  handle_direct_out_change (direct_out_pos, false);
}

DEFINE_SIMPLE (activate_selected_tracks_direct_out_new)
{
  handle_direct_out_change (-1, true);
}

DEFINE_SIMPLE (activate_toggle_track_passthrough_input)
{
  Track * track = TRACKLIST_SELECTIONS->tracks[0];
  g_message (
    "setting track '%s' passthrough MIDI input "
    "to %d",
    track->name, !track->passthrough_midi_input);
  track->passthrough_midi_input = !track->passthrough_midi_input;
}

DEFINE_SIMPLE (activate_show_used_automation_lanes_on_selected_tracks)
{
  for (int i = 0; i < TRACKLIST_SELECTIONS->num_tracks; i++)
    {
      Track *               track = TRACKLIST_SELECTIONS->tracks[i];
      AutomationTracklist * atl = track_get_automation_tracklist (track);
      if (atl == NULL)
        continue;

      bool automation_vis_changed = false;
      for (int j = 0; j < atl->num_ats; j++)
        {
          AutomationTrack * at = atl->ats[j];
          if (!automation_track_contains_automation (at))
            continue;

          if (at->visible)
            continue;

          automation_tracklist_set_at_visible (atl, at, true);
          automation_vis_changed = true;
        }

      if (!track->automation_visible)
        {
          automation_vis_changed = true;
          track->automation_visible = true;

          if (track->type == TRACK_TYPE_TEMPO)
            {
              ui_show_warning_for_tempo_track_experimental_feature ();
            }
        }

      if (automation_vis_changed)
        {
          EVENTS_PUSH (ET_TRACK_AUTOMATION_VISIBILITY_CHANGED, track);
        }
    }
}

DEFINE_SIMPLE (activate_hide_unused_automation_lanes_on_selected_tracks)
{
  for (int i = 0; i < TRACKLIST_SELECTIONS->num_tracks; i++)
    {
      Track *               track = TRACKLIST_SELECTIONS->tracks[i];
      AutomationTracklist * atl = track_get_automation_tracklist (track);
      if (atl == NULL)
        continue;

      bool automation_vis_changed = false;
      for (int j = 0; j < atl->num_ats; j++)
        {
          AutomationTrack * at = atl->ats[j];
          if (automation_track_contains_automation (at))
            continue;

          if (!at->visible)
            continue;

          automation_tracklist_set_at_visible (atl, at, false);
          automation_vis_changed = true;
        }

      if (automation_vis_changed)
        {
          EVENTS_PUSH (ET_TRACK_AUTOMATION_VISIBILITY_CHANGED, track);
        }
    }
}

DEFINE_SIMPLE (activate_append_track_objects_to_selection)
{
  int track_pos = g_variant_get_int32 (variant);

  Track * track = tracklist_get_track (TRACKLIST, track_pos);

  for (int i = 0; i < track->num_lanes; i++)
    {
      TrackLane * lane = track->lanes[i];
      for (int j = 0; j < lane->num_regions; j++)
        {
          ZRegion * r = lane->regions[j];
          arranger_object_select (
            (ArrangerObject *) r, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
        }
    }
}

DEFINE_SIMPLE (activate_append_lane_objects_to_selection)
{
  int track_pos, lane_pos;
  g_variant_get (variant, "(ii)", &track_pos, &lane_pos);

  Track *     track = tracklist_get_track (TRACKLIST, track_pos);
  TrackLane * lane = track->lanes[lane_pos];

  for (int j = 0; j < lane->num_regions; j++)
    {
      ZRegion * r = lane->regions[j];
      arranger_object_select (
        (ArrangerObject *) r, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
    }
}

DEFINE_SIMPLE (activate_append_lane_automation_regions_to_selection)
{
  int track_pos, at_index;
  g_variant_get (variant, "(ii)", &track_pos, &at_index);
  Track *               track = tracklist_get_track (TRACKLIST, track_pos);
  AutomationTracklist * atl = track_get_automation_tracklist (track);
  g_return_if_fail (atl);
  AutomationTrack * at = atl->ats[at_index];
  for (int j = 0; j < at->num_regions; j++)
    {
      ZRegion * r = at->regions[j];
      arranger_object_select (
        (ArrangerObject *) r, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
    }
}

/**
 * Used as a workaround for GTK bug 4422.
 */
DEFINE_SIMPLE (activate_app_action_wrapper)
{
  const char * action_name = g_action_get_name (G_ACTION (action));
  g_action_group_activate_action (
    G_ACTION_GROUP (zrythm_app), action_name, variant);
}
