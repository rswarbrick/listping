#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <string>
#include <list>
#include <ostream>

class mailing_list {
public:
  mailing_list (const std::string &address, const std::string &pass);

  const std::string& get_address () const { return address; }
  const std::string& get_password () const { return password; }

private:
  std::string address, password;
};

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
std::ostream &operator<< (std::ostream &out, const mailing_list &ml);

#endif
