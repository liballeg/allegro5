/* registration code for setting up the font object plugin */
{
   extern DATEDIT_OBJECT_INFO datfont_info;
   extern DATEDIT_GRABBER_INFO datfont_grabber;

   datedit_register_object(&datfont_info);
   datedit_register_grabber(&datfont_grabber);
}

