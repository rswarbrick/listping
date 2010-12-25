#include <fstream>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <utility>

#include "config_file.h"

using std::vector;
using std::list;
using std::string;
using std::ostream;

// Types for a registry of directive -> handler.
typedef void (*directive_handler)(const string &, config_file &);

typedef std::pair<std::string, directive_handler> handler_pair;

#define DECLARE_HANDLER(name)                                           \
  static void (name)(const string &argument, config_file &conf);

DECLARE_HANDLER(password_handler)

static handler_pair handlers[] = {
  handler_pair("password", password_handler),
  handler_pair("username", NULL),
  handler_pair("adminurl", NULL),
  handler_pair("default", NULL),
  handler_pair("action", NULL),
  handler_pair("spamlevel", NULL),
  handler_pair("spamheader", NULL),
  handler_pair("not_spam_if_from", NULL),
  handler_pair("not_spam_if_subject", NULL),
  handler_pair("discard_if_from", NULL),
  handler_pair("discard_if_subject", NULL),
  handler_pair("discard_if_reason", NULL),
  handler_pair("subscription_default", NULL),
  handler_pair("subscription_action", NULL),
  handler_pair("confirm", NULL),
  handler_pair("log", NULL),
  handler_pair("meta_member_support", NULL),
  handler_pair("", NULL)
};

static void
discard_line (std::istream &stream)
{
  char next;
  while (stream.good ()) {
    stream.get (next);
    if (next == '\n') break;
  }
}

static void
chug_stream (std::istream &stream)
{
  char next;
  while (stream.good ()) {
    next = static_cast<char> (stream.peek ());
    if ((next == ' ') || (next == '\t')) {
      stream.get (next);
    }
    else break;
  }
}

static vector<string>
read_config_line (std::istream &stream)
{
  char next;
  bool quoted = false;
  string acc;
  vector<string> ret;

  while (stream.good ()) {
    stream.get (next);
    // Check for eof
    if (!stream.good ()) break;

    if (quoted) {
      // When we're in a "quoted" bit, the only special characters are
      // " and \.
      if (next == '"') {
        quoted = false;
        continue;
      }
      else if (next == '\\') {
        stream.get (next);
        if (!stream.good ()) {
          std::cerr << "WARNING: Backslash at end of file.\n";
          break;
        }
        if (!((next == '\\') || (next == '"'))) {
          std::cerr << "WARNING: Escape in string before '"
                    << std::string(1, next)
                    << "' not needed\n";
        }
      }
      // Not a special character.
      acc.append (1, next);
    }
    else {
      // (Not quoted) Special characters \n, # and \.
      if (next == '#') {
        discard_line (stream);
        break;
      }
      if (next == '\n') {
        // A newline after a \ gets caught by the special backslash
        // handling code, so this must be a real end of line.
        break;
      }
      if (next == '\\') {
        stream.get (next);
        if (!stream.good ()) {
          std::cerr << "WARNING: Backslash at end of file.\n";
          break;
        }
        if (next == '\n') {
          chug_stream (stream);
          continue;
        }
        else {
          // Put the character back so it gets read again.
          stream.unget ();
        }
        acc.append (1, '\\');
        continue;
      }
      if ((next == ' ') || (next == '\t')) {
        if (acc.size () > 0) {
          ret.push_back (acc);
          acc = "";
        }
        continue;
      }
      if (next == '"') {
        quoted = true;
        continue;
      }

      acc.append (1, next);
    }
  }

  if (acc.size () > 0) ret.push_back (acc);
  return ret;
}

static void
handle_directive (const string &directive,
                  const string &arg,
                  config_file &conf)
{
  for (unsigned int i=0; handlers[i].first.size () > 0; i++) {
    if (directive == handlers[i].first) {
      if (handlers[i].second) {
        handlers[i].second (arg, conf);
      }
      return;
    }
  }

  std::cerr << "WARNING: Unhandled directive. '"
            << directive << "' = '"
            << arg << "'\n";
}

config_file::config_file (const string &path)
{
  std::ifstream stream (path.c_str ());
  if (!stream.good ()) {
    throw std::runtime_error ("Can't open config file: "+path);
  }

  vector<string> tokens;
  while (stream.good ()) {
    tokens = read_config_line (stream);

    switch (tokens.size ()) {
    case 0:
      break;

    case 1:
      mailing_lists.push_back (mailing_list (tokens[0], cur_pass));
      break;

    case 2:
      handle_directive (tokens[0], tokens[1], *this);
      break;

    default:
      throw std::runtime_error (
        "Can't parse config file: too many tokens");
    }
  }
}

static void
password_handler (const string &arg, config_file &conf)
{
  conf.set_password (arg);
}

ostream &operator<< (ostream &out, const config_file &cf)
{
  out << "#[Config file:\n";
  for (list<mailing_list>::const_iterator it = cf.mailing_lists.begin ();
       it != cf.mailing_lists.end ();
       it++) {
    out << "  " << *it << "\n";
  }
  out << "]";
  return out;
}
