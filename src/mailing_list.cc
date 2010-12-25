#include "mailing_list.h"

using std::ostream;
using std::string;

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
