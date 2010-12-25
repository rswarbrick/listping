#ifndef MAILING_LIST_H
#define MAILING_LIST_H

#include <string>
#include <ostream>

class _mailing_list;

enum ModerationStatus {
  MODSTATUS_UNKNOWN,
  MODSTATUS_EMPTY,
  MODSTATUS_WAITING
};

class mailing_list {
public:
  mailing_list (const std::string &address, const std::string &pass);
  mailing_list (const mailing_list &ml);
  ~mailing_list ();

  void update ();
  ModerationStatus status() const;

private:
  friend std::ostream &operator<< (std::ostream &out,
                                   const mailing_list &ml);

  _mailing_list *priv;
};

std::ostream &operator<< (std::ostream &out, const mailing_list &ml);

#endif
