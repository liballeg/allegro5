/* registration code for setting up the MIDI object plugin */
{
   extern DATEDIT_OBJECT_INFO datmidi_info;
   extern DATEDIT_GRABBER_INFO datmidi_grabber;

   datedit_register_object(&datmidi_info);
   datedit_register_grabber(&datmidi_grabber);
}

