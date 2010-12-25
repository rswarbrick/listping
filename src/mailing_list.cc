#include "mailing_list.h"
#include <stdexcept>
#include <sstream>
#include <utility>
#include <iostream>

using std::ostream;
using std::string;

typedef std::pair<string, string> string_pair;

class _mailing_list
{
public:
  _mailing_list (const std::string &address, const std::string &pass);
  _mailing_list (const _mailing_list& ml);
  ~_mailing_list ();

  void update ();

private:
  friend std::ostream &operator<< (std::ostream &out,
                                   const _mailing_list &ml);

  std::string address, password;

  const std::string get_url () const;
};

_mailing_list::_mailing_list (const string &_address,
                              const string &_password)
: address (_address), password (_password)
{}

_mailing_list::_mailing_list (const _mailing_list& ml)
: address (ml.address), password (ml.password)
{}

_mailing_list::~_mailing_list ()
{}

ostream &operator<< (ostream &out, const _mailing_list &ml)
{
  out << "#[Mailing list: " << ml.address;
  if (ml.password.size() > 0) {
    out << " (password: " << ml.password << ")";
  }
  out << "]";
  return out;
}

static string_pair
split_on_last (const string &str, char split_at)
{
  size_t i = str.find_last_of (split_at);
  if (i == string::npos)
    throw std::runtime_error (
      "Can't split string: char doesn't appear.");

  return string_pair (str.substr (0, i), str.substr (i+1));
}

const string
_mailing_list::get_url () const
{
  string_pair name_domain = split_on_last (address, '@');
  std::ostringstream oss;

  oss << "http://" << name_domain.second
      << "/mailman/admindb/"
      << name_domain.first;

  return oss.str ();
}

void
_mailing_list::update ()
{
  std::cerr << "UPDATE\n";
}

/********************************************************************/
/* mailing_list, the user */
/********************************************************************/
mailing_list::mailing_list (const string &address, const string &pass)
{
  priv = new _mailing_list (address, pass);
}

mailing_list::mailing_list (const mailing_list &ml)
{
  priv = new _mailing_list (*ml.priv);
}

mailing_list::~mailing_list ()
{
  delete priv;
}

ostream &operator<< (ostream &out, const mailing_list &ml)
{
  return out << *ml.priv;
}

void
mailing_list::update ()
{
  priv->update ();
}
