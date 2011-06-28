#include "mailing_list.h"
#include <stdexcept>
#include <sstream>
#include <utility>
#include <iostream>
#include <libsoup/soup.h>
#include <cstring>

using std::ostream;
using std::string;

typedef std::pair<string, string> string_pair;

class _mailing_list
{
public:
  _mailing_list (const std::string &address, const std::string &pass);
  _mailing_list (const _mailing_list& ml);
  ~_mailing_list ();

  void update (GMutex *mutex);
  ModerationStatus status () const;
  void set_status (ModerationStatus st);

  const std::string& get_address () const;

private:
  friend std::ostream &operator<< (std::ostream &out,
                                   const _mailing_list &ml);

  std::string address, password;
  SoupSession *session;

  ModerationStatus _status;

  std::string get_url () const;
};

_mailing_list::_mailing_list (const string &_address,
                              const string &_password)
: address (_address), password (_password)
{
  session = soup_session_sync_new ();
  _status = MODSTATUS_UNKNOWN;
}

_mailing_list::_mailing_list (const _mailing_list& ml)
: address (ml.address), password (ml.password)
{
  session = soup_session_sync_new ();
  _status = MODSTATUS_UNKNOWN;
}

_mailing_list::~_mailing_list ()
{
  g_object_unref (session);
}

ostream &operator<< (ostream &out, const _mailing_list &ml)
{
  out << "#[Mailing list: " << ml.address;
  if (ml.password.size() > 0) {
    out << " (password: " << ml.password << ")";
  }
  switch (ml._status) {
  case MODSTATUS_UNKNOWN:
    out << " ??";
    break;
  case MODSTATUS_EMPTY:
    out << " --";
    break;
  case MODSTATUS_WAITING:
    out << " ++";
    break;
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

string
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
_mailing_list::update (GMutex *mutex)
{
  SoupMessage *msg;
  GHashTable *request_form;
  gchar *request_body;
  int  http_status;

  request_form = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (request_form,
                       g_strdup ("adminpw"),
                       g_strdup (password.c_str ()));
  request_body = soup_form_encode_urlencoded (request_form);
  g_hash_table_destroy (request_form);

  msg = soup_message_new ("POST", get_url ().c_str ());
  soup_message_set_request (msg, "application/x-www-form-urlencoded",
                            SOUP_MEMORY_TAKE,
                            request_body,
                            strlen (request_body));

  http_status = soup_session_send_message (session, msg);

  if (http_status != 200) {
    std::cerr << "WARNING: Failed to ping mailing list ("
              << *this
              << ").\n  HTTP status = "
              << http_status
              << ".";
    return;
  }

  // We want to "parse" the results to see whether there's any mail
  // waiting. If there isn't, mailman writes "There are no pending
  // requests." so we'll check for that!
  g_mutex_lock (mutex);

  if (g_strstr_len (msg->response_body->data,
                    msg->response_body->length,
                    "There are no pending requests.")) {
    _status = MODSTATUS_EMPTY;
  }
  else _status = MODSTATUS_WAITING;

  g_mutex_unlock (mutex);
}

ModerationStatus
_mailing_list::status () const
{ return _status; }

void
_mailing_list::set_status (ModerationStatus st)
{ _status = st; }

const std::string&
_mailing_list::get_address () const
{ return address; }

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
mailing_list::update (GMutex *mutex)
{
  priv->update (mutex);
}

ModerationStatus
mailing_list::status () const
{
  return priv->status ();
}

const std::string&
mailing_list::get_address () const
{
  return priv->get_address ();
}

void
mailing_list::clear ()
{
  priv->set_status (MODSTATUS_UNKNOWN);
}
