#ifndef MAILING_LIST_H
#define MAILING_LIST_H

#include <string>
#include <ostream>

class _mailing_list;

class mailing_list {
public:
  mailing_list (const std::string &address, const std::string &pass);
  mailing_list (const mailing_list &ml);
  ~mailing_list ();

  void update ();

private:
  friend std::ostream &operator<< (std::ostream &out,
                                   const mailing_list &ml);

  _mailing_list *priv;
};

std::ostream &operator<< (std::ostream &out, const mailing_list &ml);

#endif
