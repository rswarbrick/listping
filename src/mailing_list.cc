#include "mailing_list.h"
#include <stdexcept>
#include <sstream>
#include <utility>

using std::ostream;
using std::string;

typedef std::pair<string, string> string_pair;

mailing_list::mailing_list (const string &_address,
                            const string &_password)
: address(_address), password(_password)
{}

ostream &operator<< (ostream &out, const mailing_list &ml)
{
  out << "#[Mailing list: "
      << ml.get_address ();
  if (ml.get_password ().size() > 0) {
    out << " (password: " << ml.get_password () << ")";
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
mailing_list::get_url () const
{
  string_pair name_domain = split_on_last (address, '@');
  std::ostringstream oss;

  oss << "http://" << name_domain.second
      << "/mailman/admindb/"
      << name_domain.first;

  return oss.str ();
}
