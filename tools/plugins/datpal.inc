/* registration code for setting up the palette object plugin */
{
   extern DATEDIT_OBJECT_INFO datpal_info;
   extern DATEDIT_GRABBER_INFO datpal_grabber;
   extern DATEDIT_MENU_INFO datpal_grabber_menu;

   datedit_register_object(&datpal_info);
   datedit_register_grabber(&datpal_grabber);
   datedit_register_menu(&datpal_grabber_menu);
}

