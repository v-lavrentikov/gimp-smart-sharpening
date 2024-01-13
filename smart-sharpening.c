#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <gegl.h>

#define PLUG_IN_TITLE   "Smart Sharpening"
#define PLUG_IN_AUTHOR  "Vitali Lavrentikov"
#define PLUG_IN_HELP    "Apply smart sharpening to an image"
#define PLUG_IN_DATE    "2023"
#define PLUG_IN_BINARY  "smart-sharpening"
#define PLUG_IN_PROC    "plug-in-smart-sharpening"
#define PLUG_IN_ROLE    "gimp-smart-sharpening-dialog"

typedef struct {
    gdouble lower;
    gdouble upper;
    gdouble step;
    gdouble *value;
    int digits;
} t_spin_conf;

typedef struct {
    gdouble saturation;
    gdouble edge;
    gdouble blur;
    gdouble strenght;
    gdouble radius;
    gdouble amount;
    gdouble threshold;
    gboolean display_mask;
    gboolean display_unsharp;
} t_conf;

t_conf conf = { 50.0, 2.0, 6.0, 8, 2.0, 0.5, 0.1, FALSE, FALSE };

static void query(void);
static void run(const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals);
static gboolean main_dialog(void);

void process(gint32 drawable_id, gint32 image_id);

GimpPlugInInfo PLUG_IN_INFO = { NULL, NULL, query, run };

MAIN()

static void query(void) {

    static GimpParamDef args[] = {
        { GIMP_PDB_INT32, "run-mode", "Run mode" },
        { GIMP_PDB_IMAGE, "image", "Input image" },
        { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
    };

    gimp_install_procedure(PLUG_IN_PROC, 
        PLUG_IN_HELP, PLUG_IN_HELP, PLUG_IN_AUTHOR, PLUG_IN_AUTHOR, PLUG_IN_DATE, 
        PLUG_IN_TITLE, "RGB*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Tools");
}

static void run(const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals) {

    static GimpParam values[1];
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    GimpRunMode run_mode;

    // Setting mandatory output values
    *nreturn_vals = 1;
    *return_vals = values;

    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = status;

    // Getting run_mode - exit in NONINTERACTIVE mode
    run_mode = param[0].data.d_int32;

    if (run_mode == GIMP_RUN_NONINTERACTIVE) {
        return;
    }

    if (!main_dialog()) {
        return;
    }

    gint32 image_id = param[1].data.d_image;
    gint32 drawable_id = param[2].data.d_drawable;
    
    process(drawable_id, image_id);
}

void gegl_operation(gint32 drawable_id, GeglNode *operation) {

    GeglNode *gegl = gegl_node_new();
    GeglBuffer *src_buffer = gimp_drawable_get_buffer(drawable_id);
    GeglBuffer *dest_buffer = gimp_drawable_get_shadow_buffer(drawable_id);

    GeglNode *input = gegl_node_new_child(gegl,
        "operation", "gegl:buffer-source",
        "buffer", src_buffer, NULL);
    GeglNode *output = gegl_node_new_child(gegl,
        "operation", "gegl:write-buffer",
        "buffer", dest_buffer, NULL);

    gegl_node_add_child(gegl, operation);
    gegl_node_link_many(input, operation, output, NULL);

    gegl_node_process(output);

    g_object_unref(gegl);
    g_object_unref(src_buffer);
    g_object_unref(dest_buffer);

    gimp_drawable_merge_shadow(drawable_id, TRUE);
    gimp_drawable_free_shadow(drawable_id);
}

void gegl_blur(gint32 drawable_id) {

    GeglNode *node = gegl_node_new_child(NULL,
        "operation", "gegl:gaussian-blur",
        "std-dev-x", conf.blur,
        "std-dev-y", conf.blur,
        "filter", 2, // IIR
        "abyss-policy", 1, // Clamp
        NULL);
    gegl_operation(drawable_id, node);
}

void gegl_noise_reduction(gint32 drawable_id) {

    GeglNode *node = gegl_node_new_child(NULL,
        "operation", "gegl:noise-reduction",
        "iterations", (int)conf.strenght,
        NULL);
    gegl_operation(drawable_id, node);
}

void gegl_unsharp_mask(gint32 drawable_id) {

    GeglNode *node = gegl_node_new_child(NULL,
        "operation", "gegl:unsharp-mask",
        "std-dev", conf.radius,
        "scale", conf.amount,
        "threshold", conf.threshold,
        NULL);
    gegl_operation(drawable_id, node);
}

void process(gint32 drawable_id, gint32 image_id) {
    gint nreturn_vals;
    gint num_layers;
    gchar *filename;
    GimpParam *return_vals;

    gegl_init(NULL, NULL);

    if (conf.saturation > 0) {
        gimp_drawable_hue_saturation(drawable_id, GIMP_ALL_HUES, 0, 0, conf.saturation, 0);
    }

    gimp_progress_init("Decompose...");

    return_vals = gimp_run_procedure("plug-in-decompose",
        &nreturn_vals,
        GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
        GIMP_PDB_IMAGE, image_id,
        GIMP_PDB_DRAWABLE, drawable_id,
        GIMP_PDB_STRING, "LAB",
        GIMP_PDB_INT32, 1,
        GIMP_PDB_END);
    gint32 image_mask_id = return_vals[1].data.d_int32;
    gimp_destroy_params(return_vals, nreturn_vals);

    gint *layers_mask = gimp_image_get_layers(image_mask_id, &num_layers);
    gimp_item_set_visible(layers_mask[1], FALSE);
    gimp_item_set_visible(layers_mask[2], FALSE);
    gimp_image_set_active_layer(image_mask_id, layers_mask[0]);

    gint32 image_unsharp_id = gimp_image_duplicate(image_mask_id);
    gint *layers_unsharp = gimp_image_get_layers(image_unsharp_id, &num_layers);

    gimp_progress_init("Edge...");
    
    return_vals = gimp_run_procedure("plug-in-edge",
        &nreturn_vals,
        GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
        GIMP_PDB_IMAGE, image_mask_id,
        GIMP_PDB_DRAWABLE, layers_mask[0],
        GIMP_PDB_FLOAT, conf.edge,
        GIMP_PDB_INT32, 2, // wrapmode = SMEAR
        GIMP_PDB_INT32, 0, // edgemode = SOBEL
        GIMP_PDB_END);
    gimp_destroy_params(return_vals, nreturn_vals);

    gimp_progress_init("Gaussian Blur...");
    gegl_blur(layers_mask[0]);

    gint32 channel_id = gimp_channel_new_from_component(image_unsharp_id, GIMP_ALPHA_CHANNEL, "Sharpening Mask");
    gimp_item_set_visible(channel_id, TRUE);
    gimp_channel_set_opacity(channel_id, 50);
    gimp_image_insert_channel(image_unsharp_id, channel_id, 0, 0);

    gimp_edit_copy(layers_mask[0]);
    gint32 floating_sel_id = gimp_edit_paste(channel_id, TRUE);
    gimp_floating_sel_anchor(floating_sel_id);
    gimp_image_select_item(image_unsharp_id, GIMP_CHANNEL_OP_ADD, channel_id);

    if (conf.strenght > 0) {
        gimp_progress_set_text("Noise Reduction...");
        gimp_selection_invert(image_unsharp_id);
        gegl_noise_reduction(layers_unsharp[0]);
        gimp_selection_invert(image_unsharp_id);
    }
    
    gimp_progress_set_text("Unsharp Mask...");
    gegl_unsharp_mask(layers_unsharp[0]);
    gimp_selection_none(image_unsharp_id);

    gimp_progress_set_text("Compose...");

    return_vals = gimp_run_procedure ("plug-in-drawable-compose",
        &nreturn_vals,
        GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
        GIMP_PDB_IMAGE, image_unsharp_id,
        GIMP_PDB_DRAWABLE, layers_unsharp[0],
        GIMP_PDB_DRAWABLE, layers_unsharp[1],
        GIMP_PDB_DRAWABLE, layers_unsharp[2],
        GIMP_PDB_DRAWABLE, NULL,
        GIMP_PDB_STRING, "LAB",
        GIMP_PDB_END);
    gint32 image_final_id = return_vals[1].data.d_int32;
    gimp_destroy_params (return_vals, nreturn_vals);

    gimp_progress_init("Display...");

    if (conf.display_mask) {
        gimp_display_new(image_mask_id);
    }
    if (conf.display_unsharp) {
        filename = gimp_image_get_filename(image_mask_id);
        gimp_image_set_filename(image_unsharp_id, filename);
        gimp_display_new(image_unsharp_id);
        g_free(filename);
    }

    filename = gimp_image_get_filename(image_id);
    gimp_image_set_filename(image_final_id, filename);
    gimp_display_new(image_final_id);
    g_free(filename);

    gimp_image_delete(image_unsharp_id);
    gimp_image_delete(image_mask_id);
    gimp_image_delete(image_final_id);

    gimp_progress_end();
    gegl_exit();
    gimp_displays_flush();

    g_message("Finished!");
}

static void add_field(const gchar* name, t_spin_conf *conf, GtkWidget *vbox) {
    GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment);
    gtk_container_add(GTK_CONTAINER(vbox), alignment);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 6, 6, 6);

    GtkWidget *main_hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(main_hbox);
    gtk_container_add(GTK_CONTAINER(alignment), main_hbox);

    GtkWidget *radius_label = gtk_label_new_with_mnemonic(name);
    gtk_widget_show(radius_label);
    gtk_box_pack_start(GTK_BOX(main_hbox), radius_label, FALSE, FALSE, 6);
    gtk_label_set_justify(GTK_LABEL(radius_label), GTK_JUSTIFY_RIGHT);

    GtkObject *spinbutton_adj = gtk_adjustment_new(*conf->value, conf->lower, conf->upper, conf->step, 1, 1);
    GtkWidget *spinbutton = gtk_spin_button_new(GTK_ADJUSTMENT(spinbutton_adj), 1, conf->digits);
    gtk_widget_show(spinbutton);
    gtk_box_pack_start(GTK_BOX(main_hbox), spinbutton, FALSE, FALSE, 6);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), TRUE);

    g_signal_connect(spinbutton_adj, "value_changed", G_CALLBACK(gimp_double_adjustment_update), conf->value);
}

static void check_button_toggle(GtkToggleButton *button, gboolean *check) {
  *check = gtk_toggle_button_get_active(button);
}

static void add_checkbox(const gchar* name, gboolean *check, GtkWidget *vbox) {
    GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment);
    gtk_container_add(GTK_CONTAINER(vbox), alignment);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 6, 6, 6);

    GtkWidget *main_hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(main_hbox);
    gtk_container_add(GTK_CONTAINER(alignment), main_hbox);

    GtkWidget *check_box = gtk_check_button_new_with_label(name);
    gtk_widget_show(check_box);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check_box), *check);
    gtk_box_pack_start(GTK_BOX(main_hbox), check_box, FALSE, FALSE, 10);

    g_signal_connect(check_box, "toggled", G_CALLBACK(check_button_toggle), check);
}

static GtkWidget *add_frame(const gchar* name, GtkWidget *vbox) {
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

    GtkWidget *frame_label = gtk_label_new (name);
    gtk_widget_show(frame_label);
    gtk_frame_set_label_widget(GTK_FRAME(frame), frame_label);
    gtk_label_set_use_markup(GTK_LABEL(frame_label), TRUE);

    GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment);
    gtk_container_add(GTK_CONTAINER(frame), alignment);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 6, 6, 6);

    GtkWidget *frame_vbox = gtk_vbox_new(FALSE, 6);
    gtk_widget_show(frame_vbox);
    gtk_container_add(GTK_CONTAINER(alignment), frame_vbox);

    return frame_vbox;
}

static gboolean main_dialog(void) {
    GtkWidget *frame_vbox;

    t_spin_conf saturation_conf = { -100, 100, 1, &conf.saturation, 1 };
    t_spin_conf edge_conf = { 1, 10, 0.1, &conf.edge, 3 };
    t_spin_conf blur_conf = { 0, 1500, 0.1, &conf.blur, 2 };
    t_spin_conf strenght_conf = { 0, 32, 1, &conf.strenght, 0 };
    t_spin_conf radius_conf = { 0, 1500, 0.1, &conf.radius, 3 };
    t_spin_conf amount_conf = { 0, 300, 0.1, &conf.amount, 3 };
    t_spin_conf threshold_conf = { 0, 1, 0.1, &conf.threshold, 3 };

    gimp_ui_init(PLUG_IN_BINARY, FALSE);

    gtk_set_locale();

    GtkWidget *dialog = gimp_dialog_new(PLUG_IN_TITLE, PLUG_IN_ROLE, NULL, 0, NULL, PLUG_IN_PROC,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

    GtkWidget *main_vbox = gtk_vbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), main_vbox);
    gtk_widget_show(main_vbox);

    frame_vbox = add_frame("Mask", main_vbox);
    add_field("Edge Amount", &edge_conf, frame_vbox);
    add_field("Blur", &blur_conf, frame_vbox);

    frame_vbox = add_frame("Noise Reduction", main_vbox);
    add_field("Strenght", &strenght_conf, frame_vbox);
    
    frame_vbox = add_frame("Sharpen", main_vbox);
    add_field("Radius", &radius_conf, frame_vbox);
    add_field("Amount", &amount_conf, frame_vbox);
    add_field("Threshold", &threshold_conf, frame_vbox);

    frame_vbox = add_frame("Adjust Color", main_vbox);
    add_field("Saturation", &saturation_conf, frame_vbox);

    frame_vbox = add_frame("Display", main_vbox);
    add_checkbox("Mask", &conf.display_mask, frame_vbox);
    add_checkbox("Sharpen", &conf.display_unsharp, frame_vbox);
    
    gimp_window_set_transient(GTK_WINDOW(dialog));
    gtk_widget_show(dialog);

    gboolean run = (gimp_dialog_run(GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

    gtk_widget_destroy(dialog);

    return run;
}
