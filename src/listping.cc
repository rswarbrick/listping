#include "config_file.h"
#include <iostream>


int main (int argc, char** argv)
{
  config_file conf ("/home/rupert/.listadmin.ini");

  std::cout << conf << "\n";
  return 0;
}

