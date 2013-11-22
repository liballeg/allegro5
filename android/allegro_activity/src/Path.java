package org.liballeg.android;

import android.text.TextUtils;
import java.util.ArrayList;

final class Path
{
   // AssetManager does not interpret the path(!) so we do it here.
   static String simplifyPath(final String path)
   {
      final String[] pieces = path.split("/");
      ArrayList<String> keep = new ArrayList<String>();
      for (String piece : pieces) {
         if (piece.equals(""))
            continue;
         if (piece.equals("."))
            continue;
         if (piece.equals("..")) {
            if (keep.size() > 0)
               keep.remove(keep.size() - 1);
            continue;
         }
         keep.add(piece);
      }
      return TextUtils.join("/", keep);
   }
}

/* vim: set sts=3 sw=3 et: */
