/* registration code for setting up the grab-from-grid plugin */
{
   extern DATEDIT_MENU_INFO datgrid_griddler_menu;
   extern DATEDIT_MENU_INFO datgrid_autocrop_menu;
   extern void datgrid_init(void);

   datedit_register_menu(&datgrid_griddler_menu);
   datedit_register_menu(&datgrid_autocrop_menu);

   datgrid_init();
}

