/* registration code for setting up the sample object plugin */
{
   extern DATEDIT_OBJECT_INFO datsample_info;
   extern DATEDIT_GRABBER_INFO datsample_grabber;

   extern DATEDIT_OBJECT_INFO datpatch_info;
   extern DATEDIT_GRABBER_INFO datpatch_grabber;

   datedit_register_object(&datsample_info);
   datedit_register_grabber(&datsample_grabber);

   datedit_register_object(&datpatch_info);
   datedit_register_grabber(&datpatch_grabber);
}

