/* registration code for setting up the bitmapped image plugin */
{
   extern DATEDIT_OBJECT_INFO datbitmap_info;
   extern DATEDIT_GRABBER_INFO datbitmap_grabber;

   extern DATEDIT_OBJECT_INFO datrlesprite_info;
   extern DATEDIT_GRABBER_INFO datrlesprite_grabber;

   extern DATEDIT_OBJECT_INFO datcsprite_info;
   extern DATEDIT_GRABBER_INFO datcsprite_grabber;

   extern DATEDIT_OBJECT_INFO datxcsprite_info;
   extern DATEDIT_GRABBER_INFO datxcsprite_grabber;

   datedit_register_object(&datbitmap_info);
   datedit_register_grabber(&datbitmap_grabber);

   datedit_register_object(&datrlesprite_info);
   datedit_register_grabber(&datrlesprite_grabber);

   datedit_register_object(&datcsprite_info);
   datedit_register_grabber(&datcsprite_grabber);

   datedit_register_object(&datxcsprite_info);
   datedit_register_grabber(&datxcsprite_grabber);
}

