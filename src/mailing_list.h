#ifndef MAILING_LIST_H
#define MAILING_LIST_H

#include <string>
#include <ostream>

class mailing_list {
public:
  mailing_list (const std::string &address, const std::string &pass);

  const std::string& get_address () const { return address; }
  const std::string& get_password () const { return password; }

  const std::string get_url () const;

private:
  std::string address, password;
};

std::ostream &operator<< (std::ostream &out, const mailing_list &ml);

#endif
