/* registration code for setting up the image format conversion plugin */
{
   extern DATEDIT_MENU_INFO datitype_type_menu;
   extern DATEDIT_MENU_INFO datitype_depth_menu;

   datedit_register_menu(&datitype_type_menu);
   datedit_register_menu(&datitype_depth_menu);
}

