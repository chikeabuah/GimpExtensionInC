//Chike Abuah's sketchy (no pun intended) implementation of a picture framing plug-in for the GIMP, 
//with help from Dave Neary of the GIMP Development 
//team: http://developer.gimp.org/writing-a-plug-in/3/index.html 

//But did you get the pun?

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


typedef struct
{
  gint offset;
  gint weather;
} MyFrame;

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);  //ID stuff for plugin
static void frame  (GimpDrawable     *drawable, gint image);
static gboolean frame_dialog (GimpDrawable *drawable);

// Set up default values for options 
static MyFrame vals =
  {
    100,  // offset 
    30,   //weather
  };

GimpPlugInInfo PLUG_IN_INFO =
  {
    NULL,   //run on init
    NULL,   //run on quit
    query,  //run on installation
    run     //central plug-in processes
  };

MAIN()

//---------------------------------------------------------------------------------------------------------------//

static void
query (void)
{
  static GimpParamDef args[] =
    {
      {
	GIMP_PDB_INT32,  // param type
	"run-mode",      // param name
	"Run mode"       // description (interactive/noninteractive)
      },
      {
	GIMP_PDB_IMAGE,
	"image",
	"Input image"    
      },
      {
	GIMP_PDB_DRAWABLE,
	"drawable",
	"Input drawable"
      },
      {
	GIMP_PDB_INT32,
	"framewid",
	"Width of the frame"
      }
    };

  gimp_install_procedure 
    (
     "plug-in-frame",          //procedure name
     "Frame",                  //plugin title
     "frames the image",         //description
     "Chike Abuah",             //author
     "Copyright Chike Abuah",   //copyright 
     "2011",                    //year
     "Frame",                   //nameagain?
     "RGB*, GRAY*",             //image types
     GIMP_PLUGIN,
     G_N_ELEMENTS (args), 0,
     args, NULL);

  gimp_plugin_menu_register ("plug-in-frame",
                             "<Image>/Filters/Misc");
}

//----------------------------------------------------------------------------------------------------//

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals) //parameters
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;
  GimpDrawable     *drawable; 
  gint image;

  *nreturn_vals = 1;                  //outputs one thing
  *return_vals  = values;             //and it's parameters

  values[0].type = GIMP_PDB_STATUS;   
  values[0].data.d_status = status;


  //Getting run_mode - we won't display a dialog if
   // we are in NONINTERACTIVE mode
   
  run_mode = param[0].data.d_int32;

  //  Get the specified drawable  
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  image = param[1].data.d_image;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      // Get options last values if needed 
      gimp_get_data ("plug-in-frame", &vals);

      // Display the dialog 
      if (! frame_dialog (drawable))
	return;
      break;
    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 4)
	status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
	vals.offset = param[3].data.d_int32;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      //  Get options last values if needed  
      gimp_get_data ("plug-in-frame", &vals);
      break;

    default:
      break;
    }

  //image = gimp_drawable_get_image(param[2].data.d_drawable); this will also work

  gimp_progress_init ("My Frame...");

  frame (drawable, image);

  gimp_displays_flush ();
  gimp_drawable_detach (drawable);

  //  Finally, set options in the core  
  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_set_data ("plug-in-frame", &vals, sizeof (MyFrame));

  return;

}

//------------------------------------------------------------------------------------------------------------------//



static void
frame (GimpDrawable *drawable, gint image)
{
  gint         i, j, k, channels;
  gint         width, height;
  GimpPixelRgn rgn_in, rgn_out;
  guchar       output[4];
  gint32   selection;


  // Get information on the image
  width = gimp_image_width (image);
  height = gimp_image_height (image);

  // Do the real work.
  gimp_image_undo_group_start (image);
  selection = gimp_selection_save (image); 
  gimp_rect_select (image, 
                    vals.offset, vals.offset, 
                    width - 2*vals.offset, height - 2*vals.offset,  
                    //Sam's more efficient code corrections
                    0, TRUE, vals.weather);
  gimp_selection_invert (image);
  gimp_edit_fill (drawable->drawable_id, 0);
  gimp_selection_none (image);
  gimp_selection_load (selection);
  gimp_drawable_delete (selection);
  gimp_image_undo_group_end (image);

}

//-------------------------------------------------------------------------------------------------------//

    
static gboolean
frame_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *main_hbox;
  GtkWidget *frame;
  GtkWidget *frame_label;
  GtkWidget *alignment;
  GtkWidget *spinbutton;
  GtkWidget  *spinbutton2;
  GtkWidget  *text;
  GtkObject *spinbutton_adj;
  GtkWidget *border_label;
  gboolean   run;

  gimp_ui_init ("frame", FALSE);

  dialog = gimp_dialog_new ("My frame", "myframe",
			    NULL, 0,
			    gimp_standard_help_func, "plug-in-frame",

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_OK,     GTK_RESPONSE_OK,

			    NULL);

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);

  alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_show (alignment);
  gtk_container_add (GTK_CONTAINER (frame), alignment);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 6, 6, 6);

  main_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (main_hbox);
  gtk_container_add (GTK_CONTAINER (alignment), main_hbox);

  frame_label = gtk_label_new_with_mnemonic ("_Offset-Weather");
  gtk_widget_show (frame_label);
  gtk_box_pack_start (GTK_BOX (main_hbox), frame_label, FALSE, FALSE, 6);
  gtk_label_set_justify (GTK_LABEL (frame_label), GTK_JUSTIFY_RIGHT);

  spinbutton_adj = gtk_adjustment_new (0, 1, 100, 1, 5, 5);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_adj), 1, 0);
  gtk_widget_show (spinbutton);
  gtk_box_pack_start (GTK_BOX (main_hbox), spinbutton, FALSE, FALSE, 6);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);


  border_label = gtk_label_new ("Modify frame");
  gtk_widget_show (border_label);
  gtk_frame_set_label_widget (GTK_FRAME (frame), frame_label);
  gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);

  g_signal_connect (spinbutton_adj, "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &vals.offset);

  spinbutton_adj = gtk_adjustment_new (0, 1, 255, 1, 5, 5);
  spinbutton2 = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_adj), 1, 0);
  gtk_widget_show (spinbutton2);
  gtk_box_pack_start (GTK_BOX (main_hbox), spinbutton2, FALSE, FALSE, 6);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton2), TRUE);

  g_signal_connect (spinbutton_adj, "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &vals.weather);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
//end

