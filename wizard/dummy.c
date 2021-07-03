#include <connui/connui.h>
#include <connui/connui-log.h>
#include <connui/iapsettings/stage.h>
#include <connui/iapsettings/mapper.h>
#include <connui/iapsettings/widgets.h>
#include <connui/iapsettings/wizard.h>
#include <connui/iapsettings/advanced.h>
#include <icd/osso-ic-gconf.h>

#include <ctype.h>
#include <string.h>
#include <libintl.h>

#include "config.h"

#define _(msgid) dgettext(GETTEXT_PACKAGE, msgid)

#define EAP_GTC			6
#define EAP_TLS			13
#define EAP_TTLS		21
#define EAP_PEAP		25
#define EAP_MS			26
#define EAP_TTLS_PAP		98
#define EAP_TTLS_MS	99

struct dummy_plugin_private_t {
	struct iap_wizard *iw;
	struct iap_wizard_plugin *plugin;
	struct stage stage;
	int combo;
	gboolean initialized;
	osso_context_t *osso;
};

typedef struct dummy_plugin_private_t dummy_plugin_private;

static void iap_wizard_dummy_advanced_show(gpointer user_data, struct stage *s)
{
    // Can set up / fetch defaults here if the mappers are not adequate, focus
    // certain fields, etc
	return;
}

static void iap_wizard_dummy_advanced_done(gpointer user_data)
{
	return;
}

static GtkWidget *dummy_combo_create()
{
	GtkWidget *widget = gtk_combo_box_new_text();
	GtkComboBox *combo_box = GTK_COMBO_BOX(widget);

	gtk_combo_box_append_text(combo_box, "Foo");
	gtk_combo_box_append_text(combo_box, "Bar");
	gtk_combo_box_append_text(combo_box, "Baz");

	return widget;
}

static gint dummy_combo_values[] = { 1, 2, 3, -1 };

static struct stage_widget iap_wizard_dummy_stage_widgets[] = {
	{
	 NULL,			/* export/save-to-gconf or not? (I think) */
	 NULL,			/* validate func */
	 "DUMMY_COMBO",		/* widget name */
	 "dummy_combo",		/* gconf name */
	 NULL,
	 &mapper_combo2int,	/* mapper type */
	 &dummy_combo_values,	/* mapper values if any */
	 },
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

static GtkWidget *dummy_start_create(gpointer user_data)
{
	GdkPixbuf *pixbuf;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *vbox;
	GtkWidget *label;

	hbox = gtk_hbox_new(FALSE, 0);
	pixbuf = connui_pixbuf_load("widgets_wizard", 50);
	image = gtk_image_new_from_pixbuf(pixbuf);
	connui_pixbuf_unref(pixbuf);

	gtk_misc_set_alignment(GTK_MISC(image), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 0);

	label = gtk_label_new(dgettext("osso-connectivity-ui",
				       "conn_set_iap_fi_dummy_details"));

	g_object_set(G_OBJECT(label), "wrap", TRUE, NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);

	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(dgettext("osso-connectivity-ui",
				       "conn_set_iap_fi_tap_next"));

	g_object_set(G_OBJECT(label), "wrap", TRUE, NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_end(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	return GTK_WIDGET(hbox);
}

static struct iap_wizard_page iap_wizard_dummy_pages[] = {
	{
	 "DUMMY_START", /* widget name */
	 "conn_set_iap_ti_dummycreate", /* translation string */
	 dummy_start_create,	/* page creation func */
	 NULL,			/* next clicked func ? */
	 NULL,			/* finish func */
	 NULL,
	 "COMPLETE",		/* next stage ID if hardcoded ? */
	 "Connectivity_Internetsettings_IAPsetupDummyStart",
	 NULL},
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

static struct iap_advanced_widget ti_adv_misc_advanced_widgets[] = {
	{
	 NULL,			/* function to test whether to show */
	 "DUMMY_COMBO",		/* name of widget */
	 NULL,
	 NULL,
	 "conn_set_iap_fi_adv_dummy_type",	/* title in localisation */
	 dummy_combo_create,
	 0},
	{NULL, NULL, NULL, NULL, NULL, NULL, 0}
};

static void
iap_wizard_dummy_combo_note_response_cb(GtkDialog * dialog, gint response_id,
					dummy_plugin_private * priv)
{
	GtkWidget *widget = iap_wizard_get_widget(priv->iw, "DUMMY_COMBO");

	if (response_id == GTK_RESPONSE_CANCEL) {
		iap_wizard_set_import_mode(priv->iw, 1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), priv->combo);
		iap_wizard_set_import_mode(priv->iw, 0);
	} else
		priv->combo = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
dummy_combo_changed_cb(GtkComboBox * widget, dummy_plugin_private * priv)
{
	gint combo;

	if (iap_wizard_get_import_mode(priv->iw))
		return;

	combo = gtk_combo_box_get_active(widget);

	if (combo && combo != priv->combo) {
		GtkWidget *note;
		GtkWidget *toplevel =
		    gtk_widget_get_toplevel(GTK_WIDGET(widget));

		note = hildon_note_new_confirmation(GTK_WINDOW(toplevel),
						    _
						    ("conn_nc_power_saving_warning"));
		iap_common_set_close_response(note, GTK_RESPONSE_CANCEL);
		g_signal_connect(note, "response",
				 G_CALLBACK
				 (iap_wizard_dummy_combo_note_response_cb),
				 priv);
		gtk_widget_show_all(note);
	}
}

static void ti_adv_misc_advanced_activate(gpointer user_data)
{
	dummy_plugin_private *priv = user_data;
	GtkWidget *widget;

	if (!priv->initialized) {
		widget = iap_wizard_get_widget(priv->iw, "DUMMY_COMBO");

		if (widget)
			g_signal_connect(G_OBJECT(widget), "changed",
					 G_CALLBACK(dummy_combo_changed_cb),
					 priv);

		priv->initialized = TRUE;
	}
}

static struct iap_advanced_page iap_wizard_dummy_advanced_pages[] = {
	{
	 0,
	 "conn_set_iap_ti_adv_dummy",
	 ti_adv_misc_advanced_widgets,
	 ti_adv_misc_advanced_activate,
	 "Connectivity_Internetsettings_IAPsetupAdvancedmiscCSD",
	 NULL},
	{0, NULL, NULL, NULL, NULL, NULL}
};

static struct iap_advanced_page *iap_wizard_dummy_get_advanced(gpointer
							       user_data)
{
	dummy_plugin_private *priv = user_data;
	struct iap_advanced_page *rv;

	priv->initialized = FALSE;

	rv = iap_wizard_dummy_advanced_pages;

	return rv;
}


static const char *iap_wizard_dummy_get_page(gpointer user_data, int index,
					     gboolean show_note)
{
	dummy_plugin_private *priv = user_data;
	struct iap_wizard *iw = priv->iw;
	const struct stage *s = iap_wizard_get_active_stage(priv->iw);

	if (!s && index == -1)
		return NULL;

	if (index != -1) {
		GtkWidget *widget = iap_wizard_get_widget(iw, "NAME");
		gchar *text = (gchar *) gtk_entry_get_text(GTK_ENTRY(widget));

		if (text && *text && !iap_settings_is_empty(text)) {
			iap_wizard_set_active_stage(iw, &priv->stage);
			return "DUMMY_START";
		}

		if (show_note) {
			GtkWidget *dialog = iap_wizard_get_dialog(iw);

			hildon_banner_show_information(GTK_WIDGET(dialog), NULL,
						       _("conn_ib_enter_name"));
			gtk_widget_grab_focus(widget);
		}
	} else {
		gchar *type = stage_get_string(s, "type");

		if (type && !strncmp(type, "DUMMY", 5)) {
			iap_wizard_select_plugin_label(iw, "DUMMY", 0);

			if (s != &priv->stage) {
				stage_copy(s, &priv->stage);
				iap_wizard_set_active_stage(iw, &priv->stage);
			}

			g_free(type);
			return "DUMMY_START";
		}

		g_free(type);
	}

	return NULL;
}

static const gchar **iap_wizard_dummy_advanced_get_widgets(gpointer user_data)
{
	dummy_plugin_private *priv = user_data;
	struct stage *s = iap_wizard_get_active_stage(priv->iw);
	gchar *type = NULL;
	static const gchar *dummy_widgets[2] = { NULL, NULL };
	static gboolean first_time2 = TRUE;

	if (s)
		type = stage_get_string(s, "type");

	if (first_time2 || (type && !strncmp(type, "DUMMY", 5)))
		dummy_widgets[0] = _("conn_set_iap_fi_dummy");
	else
		dummy_widgets[0] = NULL;

	first_time2 = FALSE;
	g_free(type);

	return dummy_widgets;
}

static void iap_wizard_dummy_save_state(gpointer user_data, GByteArray * state)
{
	dummy_plugin_private *priv = user_data;
	stage_dump_cache(&priv->stage, state);
}

static void
iap_wizard_dummy_restore_state(gpointer user_data, struct stage_cache *cache)
{
	dummy_plugin_private *priv = user_data;

	stage_restore_cache(&priv->stage, cache);
}

gboolean
iap_wizard_plugin_init(struct iap_wizard *iw, struct iap_wizard_plugin *plugin)
{
	dummy_plugin_private *priv = g_new0(dummy_plugin_private, 1);
	struct stage *s = &priv->stage;

	priv->iw = iw;
	priv->plugin = plugin;

	stage_create_cache(s, NULL);
	priv->stage.name = g_strdup("DUMMY_STAGE");
	stage_set_string(s, "type", "DUMMY");

	plugin->name = "DUMMY";
	plugin->prio = 1000;
	plugin->priv = priv;

	plugin->get_advanced = iap_wizard_dummy_get_advanced;
	plugin->stage_widgets = iap_wizard_dummy_stage_widgets;
	plugin->pages = iap_wizard_dummy_pages;
	plugin->get_widgets = iap_wizard_dummy_advanced_get_widgets;
	plugin->advanced_show = iap_wizard_dummy_advanced_show;
	plugin->advanced_done = iap_wizard_dummy_advanced_done;
	plugin->save_state = iap_wizard_dummy_save_state;
	plugin->restore = iap_wizard_dummy_restore_state;
	plugin->get_page = iap_wizard_dummy_get_page;
	plugin->widgets =
	    g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return TRUE;
}

void
iap_wizard_plugin_destroy(struct iap_wizard *iw,
			  struct iap_wizard_plugin *plugin)
{
	dummy_plugin_private *priv = (dummy_plugin_private *) plugin->priv;

	if (priv && priv->osso) {
		osso_deinitialize(priv->osso);
		priv->osso = NULL;
	}

	iap_scan_close();
	stage_free(&priv->stage);
	g_hash_table_destroy(plugin->widgets);
	g_free(plugin->priv);
}
