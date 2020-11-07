/*
 *  GTetrinet
 *  Copyright (C) 1999, 2000, 2001, 2002, 2003  Ka-shu Wong (kswong@zip.com.au)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

#ifdef ENABLE_SERVERLIST
#include <libsoup/soup.h>
#endif

#include "gtetrinet.h"
#include "gtet_config.h"
#include "client.h"
#include "tetrinet.h"
#include "tetris.h"
#include "fields.h"
#include "misc.h"
#include "sound.h"
#include "partyline.h"

/* Adopted and renamed from libgnomeui */
#define GTET_PAD 8
#define GTET_PAD_SMALL 4

extern GtkWidget *app;

/*****************************************************/
/* connecting dialog - a dialog with a cancel button */
/*****************************************************/
static GtkWidget *connectingdialog = 0, *connectdialog;
static GtkWidget *progressbar;
static gint timeouttag = 0;

GtkWidget *team_dialog;

void connectingdialog_button (GtkWidget *dialog, gint button)
{
    switch (button) {
    case GTK_RESPONSE_CANCEL:
        g_source_remove (timeouttag);
        timeouttag = 0;
        if (connectingdialog == 0) return;
        client_disconnect ();
        gtk_widget_destroy (connectingdialog);
        connectingdialog = 0;
        break;
    }
}

gint connectingdialog_delete (void)
{
    return TRUE; /* dont kill me */
}

gint connectingdialog_timeout (void)
{
    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progressbar));
    return TRUE;
}

void connectingdialog_new (void)
{
    if (connectingdialog != NULL)
    {
      gtk_window_present (GTK_WINDOW (connectingdialog));
      return;
    }
    connectingdialog = gtk_dialog_new_with_buttons ("Connect to server",
                                                    GTK_WINDOW (connectdialog),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                    NULL);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (connectingdialog), TRUE);
    progressbar = gtk_progress_bar_new ();
    gtk_widget_show (progressbar);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(connectingdialog)),
                        progressbar, TRUE, TRUE, 0); 


    timeouttag = g_timeout_add (20, (GSourceFunc)connectingdialog_timeout,
                                NULL);
    g_signal_connect (G_OBJECT(connectingdialog), "response",
                        G_CALLBACK(connectingdialog_button), NULL);
    g_signal_connect (G_OBJECT(connectingdialog), "delete_event",
                        G_CALLBACK(connectingdialog_delete), NULL);
    gtk_widget_show (connectingdialog);
}

void connectingdialog_destroy (void)
{
    if (timeouttag != 0) g_source_remove (timeouttag);
    timeouttag = 0;
    if (connectingdialog == 0) return;
    gtk_widget_destroy (connectingdialog);
    connectingdialog = 0;
}

/*******************/
/* the team dialog */
/*******************/
void teamdialog_destroy (void)
{
    gtk_widget_destroy (team_dialog);
    team_dialog = NULL;
}

void teamdialog_button (GtkWidget *button, gint response, gpointer data)
{
    GtkEntry *entry = (GtkEntry*)data;//GTK_ENTRY (gnome_entry_gtk_entry ( (data)));

    switch (response)
    {
      case GTK_RESPONSE_OK :
      {
        g_settings_set_string (settings, "player-team",
                                 gtk_entry_get_text (entry));
        tetrinet_changeteam (gtk_entry_get_text(entry));
      }; break;
    }

    teamdialog_destroy ();
}

void teamdialog_new (void)
{
    GtkWidget *hbox, *widget, *entry;
    gchar *team_utf8 = team;
  
    if (team_dialog != NULL)
    {
      gtk_window_present (GTK_WINDOW (team_dialog));
      return;
    }

    team_dialog = gtk_dialog_new_with_buttons (_("Change team"),
                                               GTK_WINDOW (app),
                                               0,
                                               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                               GTK_STOCK_OK, GTK_RESPONSE_OK,
                                               NULL);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (team_dialog), TRUE);
    gtk_dialog_set_default_response (GTK_DIALOG (team_dialog), GTK_RESPONSE_OK);
    gtk_window_set_position (GTK_WINDOW (team_dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_resizable (GTK_WINDOW (team_dialog), FALSE);

    /* entry and label */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, GTET_PAD_SMALL);
    widget = gtk_label_new ("Team name:");
    gtk_box_pack_start (GTK_BOX (hbox), widget ,TRUE, TRUE, 0);
    entry = gtk_entry_new_with_buffer (gtk_entry_buffer_new("Team", 4));
    gtk_entry_set_text ((GtkEntry*)entry, team_utf8);
    g_object_set ((GObject*)entry, "activates_default", TRUE, NULL);
    gtk_box_pack_start (GTK_BOX (hbox), entry  ,TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), GTET_PAD_SMALL);
    gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area(team_dialog)), hbox ,TRUE, TRUE, 0);

    /* pass the entry in the data pointer */
    g_signal_connect (G_OBJECT(team_dialog), "response",
                        G_CALLBACK(teamdialog_button), (gpointer)entry);
    g_signal_connect (G_OBJECT(team_dialog), "destroy",
                        G_CALLBACK(teamdialog_destroy), NULL);
    gtk_widget_show_all (team_dialog);
}

/**********************/
/* the connect dialog */
/**********************/
static int connecting;
static GtkWidget *serveraddressentry, *serveraddressbox;
static GtkWidget *nicknameentry, *teamnameentry, *spectatorcheck, *passwordentry;
static GtkWidget *passwordlabel, *teamnamelabel;
static GtkWidget *originalradio, *tetrifastradio;
static GtkListStore *serverlist;

void connectdialog_button (GtkDialog *dialog, gint button)
{
    gchar *nick; /* intermediate buffer for recoding purposes */
    const gchar *server1;
    GtkWidget *dialog_error;

    switch (button) {
    case GTK_RESPONSE_OK:
        /* connect now */
        server1 = gtk_entry_get_text ((GtkEntry*)serveraddressentry);
        if (g_utf8_strlen (server1, -1) <= 0)
        {
          dialog_error = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_OK,
                                                 "You must specify a server name.");
          gtk_dialog_run (GTK_DIALOG (dialog_error));
          gtk_widget_destroy (dialog_error);
          return;
        }

	spectating = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(spectatorcheck));

        if (spectating)
        {
          g_utf8_strncpy (specpassword, gtk_entry_get_text (GTK_ENTRY(passwordentry)),
                          g_utf8_strlen (gtk_entry_get_text (GTK_ENTRY (passwordentry)), -1));
          if (g_utf8_strlen (specpassword, -1) <= 0)
          {
            dialog_error = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Please specify a password to connect as spectator.");
            gtk_dialog_run (GTK_DIALOG (dialog_error));
            gtk_widget_destroy (dialog_error);
            return;
          }
        }
        
        GTET_O_STRCPY (team, gtk_entry_get_text((GtkEntry*)teamnameentry));
        
        nick = g_strdup (gtk_entry_get_text ((GtkEntry*)nicknameentry));
        g_strstrip (nick); /* we remove leading and trailing whitespaces */
        if (g_utf8_strlen (nick, -1) > 0)
        {
          client_init (server1, nick);
        }
        else
        {
            dialog_error = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Please specify a valid nickname.");
            gtk_dialog_run (GTK_DIALOG (dialog_error));
            gtk_widget_destroy (dialog_error);
            return;
        }
        
        g_settings_set_string (settings, "server", server1);
        g_settings_set_string (settings, "player-nickname", nick);
        g_settings_set_string (settings, "player-team",
                                 gtk_entry_get_text ((GtkEntry*)teamnameentry));
        g_settings_set_boolean (settings, "gamemode", gamemode);

        g_free (nick);
        break;
    case GTK_RESPONSE_CANCEL:
        gtk_widget_destroy (connectdialog);
        break;
    }
}

void connectdialog_spectoggle (GtkWidget *widget)
{
    //if (GTK_TOGGLE_BUTTON(widget)->active) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        gtk_widget_set_sensitive (passwordentry, TRUE);
        gtk_widget_set_sensitive (passwordlabel, TRUE);
        gtk_widget_set_sensitive (teamnameentry, FALSE);
        gtk_widget_set_sensitive (teamnamelabel, FALSE);
    }
    else {
        gtk_widget_set_sensitive (passwordentry, FALSE);
        gtk_widget_set_sensitive (passwordlabel, FALSE);
        gtk_widget_set_sensitive (teamnameentry, TRUE);
        gtk_widget_set_sensitive (teamnamelabel, TRUE);
    }
}

void connectdialog_originaltoggle (GtkWidget *widget)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        gamemode = ORIGINAL;
    }
}

void connectdialog_tetrifasttoggle (GtkWidget *widget)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        gamemode = TETRIFAST;
    }
}

#ifdef ENABLE_SERVERLIST
void serverlist_reset ()
{
    GtkTreeIter iter;
    const gchar *current;
    const gchar *defaultserver = "tetrinet.fr";
    const gchar *localhost = "localhost";

    gtk_list_store_clear(serverlist);

    // add current value to the dropdown list
    current = gtk_entry_get_text(GTK_ENTRY (serveraddressentry));
    gtk_list_store_append(serverlist, &iter);
    gtk_list_store_set(serverlist, &iter, 0, current, -1);

    if(strcmp(current, defaultserver)!=0)
    {
        gtk_list_store_append(serverlist, &iter);
        gtk_list_store_set(serverlist, &iter, 0, defaultserver, -1);
    }
    if(strcmp(current, localhost)!=0)
    {
        gtk_list_store_append(serverlist, &iter);
        gtk_list_store_set(serverlist, &iter, 0, localhost, -1);
    }
}
void serverlist_xmlparse_open_tag (GMarkupParseContext *context,
                     const gchar         *element_name,
                     const gchar        **attribute_names,
                     const gchar        **attribute_values,
                     gpointer             user_data,
                     GError             **error)
{
    GtkTreeIter iter;
    if(strcmp(element_name,"server") == 0)
    {
        const gchar *attribute_name;
        gint i;
        gboolean hasItem;
        gboolean found;
        GtkTreeIter iter;

        i = 0;
        for (attribute_name = attribute_names[i]; attribute_name; attribute_name = attribute_names[++i])
        {
            if (strcmp (attribute_name, "name") == 0)
            {
                // Verify if there is already an entry with this name (TODO if there is: update the attributes if/once we show them)
                found = FALSE;
                hasItem = gtk_tree_model_get_iter_first(GTK_TREE_MODEL (serverlist), &iter);
                while(hasItem && !found)
                {
                    GValue value = G_VALUE_INIT;
                    gtk_tree_model_get_value(GTK_TREE_MODEL (serverlist), &iter, 0, &value);
                    if(strcmp(attribute_values[i], g_value_get_string(&value))==0)
                    {
                        found = TRUE;
                    }
                    hasItem = gtk_tree_model_iter_next(GTK_TREE_MODEL (serverlist), &iter);
                    g_value_unset(&value);
                }
                if(!found)
                {
                    gtk_list_store_append(serverlist, &iter);
                    gtk_list_store_set(serverlist, &iter, 0, attribute_values[i], -1);
                }
            }
        }
    }
}
void serverlist_xmlparse_error (GMarkupParseContext *context,
                  GError              *error,
                  gpointer             user_data)
{
    fprintf(stderr, "Error parsing serverlist xml: %s\n", error->message);
}
// we don't care about structure in the xml, we only care about the attributes of the server tags
GMarkupParser serverlist_xmlparser = {
    serverlist_xmlparse_open_tag
    , NULL
    , NULL
    , NULL
    , serverlist_xmlparse_error
};
void connectdialog_receivelist (SoupSession *session, SoupMessage *msg, gpointer user_data)
{
    GError *error = NULL;
    GMarkupParseContext *ctx;

    if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code))
    {
	fprintf(
	    stderr
	    , "Trying to get server list from %s, we got HTTP code %d: %s\n"
	    , soup_uri_to_string(soup_message_get_uri(msg),FALSE)
	    , msg->status_code
	    , msg->reason_phrase
	  );
    }
    else
    {
        //printf("%s", msg->response_body->data);
	serverlist_reset();
        ctx = g_markup_parse_context_new(&serverlist_xmlparser, 0, NULL, NULL);
        g_markup_parse_context_parse(ctx, msg->response_body->data, msg->response_body->length, NULL);
        g_markup_parse_context_free(ctx);
	// even with an error, servers may have been added to the list, errors go also through the parser
        gtk_combo_box_popup(GTK_COMBO_BOX (serveraddressbox)); //produces a warning on gtk 3.24.32: no trigger event for menu popup
    }
}
SoupSession *session;
void connectdialog_getlist (GtkWidget *widget, char *url)
{
    SoupRequestHTTP *request;
    GError *error;
    SoupMessage *msg;

    request = soup_session_request_http(session, "GET", url, &error);
    if(request)
    {
        msg = soup_request_http_get_message(request);
        // async + only return when fully received
        // already frees msg if not requeued
        soup_session_queue_message(session, msg, (SoupSessionCallback)connectdialog_receivelist, NULL);
    }
    else
    {
	fprintf(stderr, "Could not initiate serverlist request to %s: %s", url, error->message);
	g_error_free(error);
    }
}
#endif

void connectdialog_connected (void)
{
    if (connectdialog != NULL) gtk_widget_destroy (connectdialog);
}

void connectdialog_destroy (void)
{
    connecting = FALSE;
}

void connectdialog_new (void)
{
    GtkBuilder *builder;
    GtkWidget *getlist_button;

    /* check if dialog is already displayed */
    if (connecting) 
    {
      gtk_window_present (GTK_WINDOW (connectdialog));
      return;
    }
    connecting = TRUE;

    /* make dialog that asks for address/nickname */
    builder = gtk_builder_new_from_resource("/apps/gtetrinet/connectdialog.ui");
    connectdialog = GTK_WIDGET(gtk_builder_get_object(builder, "connectdialog"));
    gtk_window_set_transient_for(GTK_WINDOW (connectdialog), GTK_WINDOW (app));

    serveraddressentry = GTK_WIDGET (gtk_builder_get_object(builder, "serveraddressentry"));
    serveraddressbox = GTK_WIDGET (gtk_builder_get_object(builder, "serveraddressbox"));
    originalradio = GTK_WIDGET (gtk_builder_get_object(builder, "originalradio"));
    tetrifastradio = GTK_WIDGET (gtk_builder_get_object(builder, "tetrifastradio"));
    spectatorcheck = GTK_WIDGET (gtk_builder_get_object(builder, "spectatorcheck"));
    passwordlabel = GTK_WIDGET (gtk_builder_get_object(builder, "passwordlabel"));
    passwordentry = GTK_WIDGET (gtk_builder_get_object(builder, "passwordentry"));
    nicknameentry = GTK_WIDGET (gtk_builder_get_object(builder, "nicknameentry"));
    teamnamelabel = GTK_WIDGET (gtk_builder_get_object(builder, "teamnamelabel"));
    teamnameentry = GTK_WIDGET (gtk_builder_get_object(builder, "teamnameentry"));
    getlist_button = GTK_WIDGET (gtk_builder_get_object(builder, "getlist_button"));

    gtk_entry_set_text ((GtkEntry*)serveraddressentry, server);
    /* game type radio buttons */
    switch (gamemode) {
    case ORIGINAL:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(originalradio), TRUE);
        break;
    case TETRIFAST:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tetrifastradio), TRUE);
        break;
    }

    gtk_entry_set_text ((GtkEntry*)nicknameentry, nick);
    gtk_entry_set_text ((GtkEntry*)teamnameentry, team);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(spectatorcheck), spectating);
    connectdialog_spectoggle (spectatorcheck);

    g_signal_connect (G_OBJECT(connectdialog), "response",
                        G_CALLBACK(connectdialog_button), NULL);
    g_signal_connect (G_OBJECT(connectdialog), "destroy",
                        G_CALLBACK(connectdialog_destroy), NULL);
    g_signal_connect (G_OBJECT(spectatorcheck), "toggled",
                        G_CALLBACK(connectdialog_spectoggle), NULL);
    g_signal_connect (G_OBJECT(originalradio), "toggled",
                        G_CALLBACK(connectdialog_originaltoggle), NULL);
    g_signal_connect (G_OBJECT(tetrifastradio), "toggled",
                        G_CALLBACK(connectdialog_tetrifasttoggle), NULL);
#ifdef ENABLE_SERVERLIST
    gchar *url;
    SoupURI *urlObject;

    // check if the serverlistserver setting contains a valid url
    url = g_settings_get_string(settings, "serverlistserver");
    urlObject = soup_uri_new(url);
    if(!urlObject)
    {
	fprintf(stderr, "Bad serverlist url setting: %s", url);
        gtk_button_set_label(GTK_BUTTON (getlist_button), "<Bad URL in settings>");
    }
    else
    {
        /* edit the label on the button */
        const gchar *original;
        const gchar *servername;

        // get the original button label
        original = gtk_button_get_label(GTK_BUTTON (getlist_button));

        // add the server name to the button label so the user knows where it will connect to
        servername = soup_uri_get_host(urlObject);
        gchar concatted[strlen(original)+strlen(servername)+1];
        strcpy(concatted, original);
        strcat(concatted, servername);
        gtk_button_set_label(GTK_BUTTON (getlist_button), concatted);
        soup_uri_free(urlObject);

        /* other required stuff for serverlist */
        session = soup_session_new_with_options(
            "timeout", 60 // the default for sync request (async is 0)
            , "user-agent", g_strdup_printf("%s/%s",PACKAGE_NAME,PACKAGE_VERSION)
            , NULL
        );
        serverlist = GTK_LIST_STORE(gtk_builder_get_object(builder, "serverlist"));
        serverlist_reset();
        g_signal_connect (G_OBJECT(getlist_button), "clicked",
                            G_CALLBACK(connectdialog_getlist), url);
    }
#else
    gtk_widget_set_visible(getlist_button, FALSE);
#endif

    gtk_widget_show (connectdialog);
}

GtkWidget *prefdialog;

/*************************/
/* the change key dialog */
/*************************/

void key_dialog_callback (GtkWidget *widget, GdkEventKey *key)
{
    gtk_dialog_response (GTK_DIALOG (widget), gdk_keyval_to_lower(key->keyval));
}

gint key_dialog (char *msg)
{
    GtkWidget *dialog, *label;
    gint keydialog_key;

    dialog = gtk_dialog_new_with_buttons ("Change Key", GTK_WINDOW (prefdialog),
                                          GTK_DIALOG_MODAL | 0,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE,
                                          NULL);
    label = gtk_label_new (msg);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(dialog)),
                        label, TRUE, TRUE, GTET_PAD_SMALL);
    g_signal_connect (G_OBJECT (dialog), "key-press-event",
                        G_CALLBACK (key_dialog_callback), NULL);
    gtk_widget_set_events (dialog, GDK_KEY_PRESS_MASK);
    keydialog_key = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_hide (dialog);
    gtk_widget_destroy (dialog);
    if (keydialog_key != GTK_RESPONSE_CLOSE )
      return keydialog_key;
    else
      return 0;
}

/**************************/
/* the preferences dialog */
/**************************/
GtkWidget *themelist, *keyclist;
GtkWidget *timestampcheck;
GtkWidget *soundcheck;
GtkWidget *namelabel, *authlabel, *desclabel;

gchar *actions[K_NUM];

struct themelistentry {
    char dir[1024];
    char name[1024];
} themes[64];

int themecount;
int theme_select;

void prefdialog_destroy (void)
{
    gtk_widget_destroy (prefdialog);
    prefdialog = NULL;
}

void prefdialog_drawkeys (void)
{
    gchar *gconf_keys[K_NUM];
    int i;
    GtkTreeIter iter;
    GtkListStore *keys_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (keyclist)));

    actions[K_RIGHT]    = ("Move right");
    actions[K_LEFT]     = ("Move left");
    actions[K_DOWN]     = ("Move down");
    actions[K_ROTRIGHT] = ("Rotate right");
    actions[K_ROTLEFT]  = ("Rotate left");
    actions[K_DROP]     = ("Drop piece");
    actions[K_DISCARD]  = ("Discard special");
    actions[K_GAMEMSG]  = ("Send message");
    actions[K_SPECIAL1] = ("Special to field 1");
    actions[K_SPECIAL2] = ("Special to field 2");
    actions[K_SPECIAL3] = ("Special to field 3");
    actions[K_SPECIAL4] = ("Special to field 4");
    actions[K_SPECIAL5] = ("Special to field 5");
    actions[K_SPECIAL6] = ("Special to field 6");
  
    gconf_keys[K_RIGHT]    = g_strdup ("right");
    gconf_keys[K_LEFT]     = g_strdup ("left");
    gconf_keys[K_DOWN]     = g_strdup ("down");
    gconf_keys[K_ROTRIGHT] = g_strdup ("rotate-right");
    gconf_keys[K_ROTLEFT]  = g_strdup ("rotate-left");
    gconf_keys[K_DROP]     = g_strdup ("drop");
    gconf_keys[K_DISCARD]  = g_strdup ("discard");
    gconf_keys[K_GAMEMSG]  = g_strdup ("message");
    gconf_keys[K_SPECIAL1] = g_strdup ("special1");
    gconf_keys[K_SPECIAL2] = g_strdup ("special2");
    gconf_keys[K_SPECIAL3] = g_strdup ("special3");
    gconf_keys[K_SPECIAL4] = g_strdup ("special4");
    gconf_keys[K_SPECIAL5] = g_strdup ("special5");
    gconf_keys[K_SPECIAL6] = g_strdup ("special6");

    for (i = 0; i < K_NUM; i ++) {
        gtk_list_store_append (keys_store, &iter);
        gtk_list_store_set (keys_store, &iter,
                            0, actions[i],
                            1, gdk_keyval_name (keys[i]),
                            2, i,
                            3, gconf_keys[i], -1);
    }
    
    for (i = 0; i < K_NUM; i++) g_free (gconf_keys[i]);
}

void prefdialog_restorekeys (void)
{
    GtkTreeIter iter;
    GtkListStore *keys_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (keyclist)));
    gboolean valid;
    gchar *gconf_key;
    gint i;

    valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (keys_store), &iter);
    while (valid)
    {
        gtk_tree_model_get (GTK_TREE_MODEL (keys_store), &iter, 2, &i, 3, &gconf_key, -1);
        gtk_list_store_set (keys_store, &iter, 1, gdk_keyval_name (defaultkeys[i]), -1);
        g_settings_set_string (settings_keys, gconf_key, gdk_keyval_name (defaultkeys[i]));
        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (keys_store), &iter);
        g_free (gconf_key);
    }
}

void prefdialog_changekey (void)
{
    gchar buf[256], *key, *gconf_key;
    gint k;
    GtkListStore *keys_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (keyclist)));
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (keyclist));
    GtkTreeIter selected_row;

    if (!gtk_tree_selection_get_selected (selection, NULL, &selected_row)) return;

    gtk_tree_model_get (GTK_TREE_MODEL (keys_store), &selected_row,
                        0, &key, 3, &gconf_key, -1);
    g_snprintf (buf, sizeof(buf), ("Press new key for \"%s\""), key);
    k = key_dialog (buf);
    if (k) {
        gtk_list_store_set (keys_store, &selected_row, 1, gdk_keyval_name (k), -1);
        g_settings_set_string (settings_keys, gconf_key, gdk_keyval_name (k));
    }
    
    g_free (gconf_key);
}

void prefdialog_soundtoggle (GtkWidget *check)
{
    gboolean enabled;
    enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));

    g_settings_set_boolean (settings, "sound-enable", enabled);
}

void prefdialog_channeltoggle (GtkWidget *check)
{
    gboolean enabled;
    enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));

    g_settings_set_boolean (settings, "partyline-enable-channel-list", enabled);
}

void prefdialog_timestampstoggle (GtkWidget *check)
{
    gboolean enabled;
    enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));

    g_settings_set_boolean (settings, "partyline-enable-timestamps", enabled);
}

void prefdialog_themelistselect (int n)
{
    char author[1024], desc[1024];

    /* update theme description */
    config_getthemeinfo (themes[n].dir, NULL, author, desc);
    leftlabel_set (namelabel, themes[n].name);
    leftlabel_set (authlabel, author);
    leftlabel_set (desclabel, desc);
  
    g_settings_set_string (settings_themes, "directory", themes[n].dir);
}

void prefdialog_themeselect (GtkTreeSelection *treeselection)
{
    GtkListStore *model;
    GtkTreeIter iter;
    gint row;
  
    if (gtk_tree_selection_get_selected (treeselection, NULL, &iter))
    {
      model = GTK_LIST_STORE (gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (treeselection)));
      gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, 1, &row, -1);
      prefdialog_themelistselect (row);
    }
}

static int themelistcomp (const void *a1, const void *b1)
{
    const struct themelistentry *a = a1, *b = b1;
    return strcmp (a->name, b->name);
}

void prefdialog_themelist ()
{
    DIR *d;
    struct dirent *de;
    char str[1024], buf[1024];
    gchar *dir;
    int i;
    char *basedir[2];
    GtkListStore *theme_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (themelist)));
    GtkTreeSelection *theme_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (themelist));
    GtkTreeIter iter, iter_selected;

    dir = g_build_filename (getenv ("HOME"), ".gtetrinet", "themes", NULL);

    basedir[0] = dir; /* load users themes first ... in case we run out */
    basedir[1] = GTETRINET_THEMES;

    themecount = 0;

    for (i = 0; i < 2; i ++) {
        d = opendir (basedir[i]);
        if (d) {
            while ((de = readdir(d))) {
                GTET_O_STRCPY (buf, basedir[i]);
                GTET_O_STRCAT (buf, "/");
                GTET_O_STRCAT (buf, de->d_name);
                GTET_O_STRCAT (buf, "/");

                if (config_getthemeinfo(buf, str, NULL, NULL) == 0) {
                    GTET_O_STRCPY (themes[themecount].dir, buf);
                    GTET_O_STRCPY (themes[themecount].name, str);
                    themecount ++;
                    if (themecount == (sizeof(themes) / sizeof(themes[0])))
                    { /* FIXME: should be dynamic */
                      g_warning("Too many theme files.\n");
                      closedir (d);
                      goto too_many_themes;
                    }
                }
            }
            closedir (d);
        }
    }
    g_free (dir);
 too_many_themes:
    qsort (themes, themecount, sizeof(struct themelistentry), themelistcomp);

    theme_select = 0;
    gtk_list_store_clear (theme_store);
    for (i = 0; i < themecount; i ++) {
        gtk_list_store_append (theme_store, &iter);
        gtk_list_store_set (theme_store, &iter, 0, themes[i].name, 1, i, -1);
        if (strcmp (themes[i].dir, currenttheme->str) == 0)
        {
            iter_selected = iter;
            theme_select = i;
        }
    }
    if (theme_select != 0) 
    {
      gtk_tree_selection_select_iter (theme_selection, &iter_selected);
      prefdialog_themelistselect (theme_select);
    }
}

void prefdialog_response (GtkDialog *dialog,
                          gint arg1)
{
  switch (arg1)
  {
    case GTK_RESPONSE_CLOSE: prefdialog_destroy (); break;
    case GTK_RESPONSE_HELP:  /* here we should open yelp */ break;
  }
}

void prefdialog_new (void)
{
    GtkWidget *label, *table, *frame, *button, *button1, *widget, *table1, *divider, *notebook;
    GtkWidget *themelist_scroll, *key_scroll, *url;
    GtkWidget *channel_list_check;
    GtkListStore *theme_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
    GtkListStore *keys_store = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    GtkTreeSelection *theme_selection;
  
    if (prefdialog != NULL)
    {
      gtk_window_present (GTK_WINDOW (prefdialog));
      return;
    }

    prefdialog = gtk_dialog_new_with_buttons (("GTetrinet Preferences"),
                                              GTK_WINDOW (app),
                                              0 | GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                              GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                              NULL);
    notebook = gtk_notebook_new ();
    gtk_window_set_resizable (GTK_WINDOW (prefdialog), FALSE);

    /* themes */
    themelist = gtk_tree_view_new_with_model (GTK_TREE_MODEL (theme_store));
    themelist_scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (themelist_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER(themelist_scroll), themelist);
    theme_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (themelist));
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (themelist), FALSE);
    gtk_widget_set_size_request (themelist, 160, 200);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (themelist), -1, "theme", renderer,
                                                 "text", 0, NULL);

    label = leftlabel_new (("Select a theme from the list.\n"
                             "Install new themes in ~/.gtetrinet/themes/"));

    table1 = gtk_table_new (3, 2, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER(table1), GTET_PAD_SMALL);
    gtk_table_set_row_spacings (GTK_TABLE(table1), 0);
    gtk_table_set_col_spacings (GTK_TABLE(table1), GTET_PAD_SMALL);
    widget = leftlabel_new (("Name:"));
    gtk_table_attach (GTK_TABLE(table1), widget, 0, 1, 0, 1,
                      GTK_EXPAND | GTK_FILL, 0, 0, 0);
    widget = leftlabel_new (("Author:"));
    gtk_table_attach (GTK_TABLE(table1), widget, 0, 1, 1, 2,
                      GTK_EXPAND | GTK_FILL, 0, 0, 0);
    widget = leftlabel_new (("Description:"));
    gtk_table_attach (GTK_TABLE(table1), widget, 0, 1, 2, 3,
                      GTK_EXPAND | GTK_FILL, 0, 0, 0);
    namelabel = leftlabel_new ("");
    gtk_table_attach (GTK_TABLE(table1), namelabel, 1, 2, 0, 1,
                      GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
    authlabel = leftlabel_new ("");
    gtk_table_attach (GTK_TABLE(table1), authlabel, 1, 2, 1, 2,
                      GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
    desclabel = leftlabel_new ("");
    gtk_table_attach (GTK_TABLE(table1), desclabel, 1, 2, 2, 3,
                      GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

    frame = gtk_frame_new (("Selected Theme"));
    gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_container_set_border_width (GTK_CONTAINER(frame), GTET_PAD_SMALL);
    gtk_widget_set_size_request (frame, 240, 100);
    gtk_container_add (GTK_CONTAINER(frame), table1);
    
    table = gtk_table_new (3, 2, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER(table), GTET_PAD);
    gtk_table_set_row_spacings (GTK_TABLE(table), GTET_PAD_SMALL);
    gtk_table_set_col_spacings (GTK_TABLE(table), GTET_PAD_SMALL);
    gtk_table_attach (GTK_TABLE(table), themelist_scroll, 0, 1, 0, 3,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE(table), label, 1, 2, 0, 1,
                      GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE(table), frame, 1, 2, 1, 2,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    url = gtk_link_button_new_with_label ("http://gtetrinet.sourceforge.net/themes.html", ("Download new themes"));
    gtk_table_attach (GTK_TABLE(table), url, 1, 2, 2, 3,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_SHRINK, 0, 0);
    gtk_widget_show_all (table);

    label = gtk_label_new (("Themes"));
    gtk_widget_show (label);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

    /* partyline */
    timestampcheck = gtk_check_button_new_with_mnemonic (_("Enable _Timestamps"));
    gtk_widget_show(timestampcheck);
    channel_list_check = gtk_check_button_new_with_mnemonic (("Enable Channel _List"));
    gtk_widget_show (channel_list_check);

    frame = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX(frame), timestampcheck, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(frame), channel_list_check, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(timestampcheck),
                                  timestampsenable);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (channel_list_check),
				  list_enabled);

    g_signal_connect (G_OBJECT(timestampcheck), "toggled",
                      G_CALLBACK(prefdialog_timestampstoggle), NULL);
    g_signal_connect (G_OBJECT (channel_list_check), "toggled",
		      G_CALLBACK (prefdialog_channeltoggle), NULL);

    table = gtk_table_new (3, 1, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER(table), GTET_PAD);
    gtk_table_set_row_spacings (GTK_TABLE(table), GTET_PAD_SMALL);
    gtk_table_set_col_spacings (GTK_TABLE(table), GTET_PAD_SMALL);
    gtk_table_attach (GTK_TABLE(table), frame, 0, 1, 0, 1,
                      GTK_EXPAND | GTK_FILL, 0, 0, 0);
    gtk_widget_show (table);

    label = gtk_label_new (("Partyline"));
    gtk_widget_show (label);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

    /* keyboard */
    keyclist = GTK_WIDGET (gtk_tree_view_new_with_model (GTK_TREE_MODEL(keys_store)));
    key_scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (key_scroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER(key_scroll), keyclist);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (keyclist), -1, ("Action"), renderer,
                                                 "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (keyclist), -1, ("Key"), renderer,
                                                 "text", 1, NULL);

    gtk_widget_set_size_request (key_scroll, 240, 200);
    gtk_widget_show (key_scroll);

    label = gtk_label_new (("Select an action from the list and press Change "
                             "Key to change the key associated with the action."));
    gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
    gtk_widget_show (label);
    gtk_widget_set_size_request (label, 180, 100);

    button = gtk_button_new_with_mnemonic (("Change _key..."));
    g_signal_connect (G_OBJECT(button), "clicked",
                      G_CALLBACK (prefdialog_changekey), NULL);
    gtk_widget_show (button);

    button1 = gtk_button_new_with_mnemonic (("_Restore defaults"));
    g_signal_connect (G_OBJECT(button1), "clicked",
                      G_CALLBACK (prefdialog_restorekeys), NULL);
    gtk_widget_show (button1);

    table = gtk_table_new (2, 2, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER(table), GTET_PAD);
    gtk_table_set_row_spacings (GTK_TABLE(table), GTET_PAD_SMALL);
    gtk_table_set_col_spacings (GTK_TABLE(table), GTET_PAD_SMALL);
    gtk_table_attach (GTK_TABLE(table), key_scroll, 0, 1, 0, 2,
                      GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach (GTK_TABLE(table), label, 1, 2, 0, 1,
                      GTK_FILL, 0, 0, 0);
    frame = gtk_box_new (GTK_ORIENTATION_VERTICAL, GTET_PAD_SMALL);
    gtk_box_pack_end (GTK_BOX(frame), button1, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX(frame), button, FALSE, FALSE, 0);
    gtk_widget_show (frame);
    gtk_table_attach (GTK_TABLE(table), frame, 1, 2, 1, 2,
                      GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_widget_show (table);

    label = gtk_label_new (("Keyboard"));
    gtk_widget_show (label);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

    /* sound */
    soundcheck = gtk_check_button_new_with_mnemonic (("Enable _Sound"));
    gtk_widget_show (soundcheck);

    frame = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX(frame), soundcheck, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    divider = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show (divider);

    table = gtk_table_new (3, 1, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER(table), GTET_PAD);
    gtk_table_set_row_spacings (GTK_TABLE(table), GTET_PAD_SMALL);
    gtk_table_set_col_spacings (GTK_TABLE(table), GTET_PAD_SMALL);
    gtk_table_attach (GTK_TABLE(table), frame, 0, 1, 0, 1,
                      GTK_EXPAND | GTK_FILL, 0, 0, 0);
    gtk_table_attach (GTK_TABLE(table), divider, 0, 1, 1, 2,
                      GTK_EXPAND | GTK_FILL, 0, 0, GTET_PAD_SMALL);
    gtk_widget_show (table);

    label = gtk_label_new (("Sound"));
    gtk_widget_show (label);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

    /* init stuff */
    prefdialog_themelist ();

    prefdialog_drawkeys ();

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(soundcheck), soundenable);

#ifndef HAVE_CANBERRAGTK
    gtk_widget_set_sensitive (soundcheck, FALSE);
#endif
    
//    gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (prefdialog)->action_area), 6);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(prefdialog)), notebook, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT(soundcheck), "toggled",
                      G_CALLBACK(prefdialog_soundtoggle), NULL);
    g_signal_connect (G_OBJECT(theme_selection), "changed",
                      G_CALLBACK (prefdialog_themeselect), NULL);
    g_signal_connect (G_OBJECT(prefdialog), "destroy",
                      G_CALLBACK(prefdialog_destroy), NULL);
    g_signal_connect (G_OBJECT(prefdialog), "response",
                      G_CALLBACK(prefdialog_response), NULL);
    gtk_widget_show_all (prefdialog);
}
