/* registration code for setting up the JPEG image plugin */
{
   extern DATEDIT_OBJECT_INFO datjpeg_info;
   extern DATEDIT_GRABBER_INFO datjpeg_grabber;
   extern DATEDIT_MENU_INFO datjpeg_menu;

   datedit_register_object(&datjpeg_info);
   datedit_register_grabber(&datjpeg_grabber);
   datedit_register_menu(&datjpeg_menu);
}

