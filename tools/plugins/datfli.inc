/* registration code for setting up the FLI object plugin */
{
   extern DATEDIT_OBJECT_INFO datfli_info;
   extern DATEDIT_GRABBER_INFO datfli_grabber;

   datedit_register_object(&datfli_info);
   datedit_register_grabber(&datfli_grabber);
}

