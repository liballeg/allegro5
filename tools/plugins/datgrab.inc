/* registration code for setting up the interactive region grab plugin */
{
   extern DATEDIT_MENU_INFO datgrab_grabber_menu;
   extern DATEDIT_MENU_INFO datgrab_boxgrab_menu;
   extern DATEDIT_MENU_INFO datgrab_ungrab_menu;

   datedit_register_menu(&datgrab_grabber_menu);
   datedit_register_menu(&datgrab_boxgrab_menu);
   datedit_register_menu(&datgrab_ungrab_menu);
}

