#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <string>
#include <list>
#include <ostream>
#include "mailing_list.h"

class config_file {
public:
  config_file (const std::string &path);

  void set_password (const std::string &pass)
    { cur_pass = pass; }

private:
  std::string cur_pass;
  std::list<mailing_list> mailing_lists;

  void parse_line (const std::string &line);

  friend std::ostream &operator<< (std::ostream &out,
                                   const config_file &cf);
};

std::ostream &operator<< (std::ostream &out, const config_file &cf);


#endif
