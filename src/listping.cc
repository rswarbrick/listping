#include "config_file.h"
#include <iostream>
#include <glib-object.h>

int main (int argc, char** argv)
{
  g_type_init ();

  config_file conf ("/home/rupert/.listadmin.ini");
  std::list<mailing_list>::iterator it;

  for (it = conf.get_lists ().begin ();
       it != conf.get_lists ().end ();
       it++) {

    it->update ();

    // Don't spam the servers while I'm testing!
    break;
  }

  std::cout << conf << "\n";
  return 0;
}

