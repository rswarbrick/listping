#include "config_file.h"
#include <iostream>
#include <glib-object.h>
#include <glib.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <libnotify/notify.h>

using std::list;
using std::string;

struct update_data
{
  bool          run;
  GMutex       *mutex;
  config_file  *conf;
};

static void
update_lists (list<mailing_list> &lists, GMutex *mutex)
{
  list<mailing_list>::iterator it;
  for (it = lists.begin (); it != lists.end (); it++) {
    it->update (mutex);
  }
}

static gpointer
update_thread_main (update_data *data)
{
  while (1) {
    if (!data->run) break;

    update_lists (data->conf->get_lists (), data->mutex);

    sleep (120);
  }
  return NULL;
}

static void
notify_for_list (const mailing_list &list)
{
  NotifyNotification *n;
  GError *err = NULL;

  string message =
    "Message to moderate in list: " + list.get_address ();

  n = notify_notification_new ("Message awaits moderation",
                               message.c_str (), NULL);
  if (!n) {
      // Probably there's no notification daemon running or
      // something. Output to standard error on the off chance
      // someone's looking.
      std::cerr << "Listping: " << message.c_str () << "\n";
      throw std::runtime_error (
          string("Couldn't create notification: ") +
          err->message);
  }

  notify_notification_set_timeout (n, 5000);

  if (!notify_notification_show (n, &err)) {
    throw std::runtime_error (
      string("Couldn't display notification: ") +
      err->message);
  }

  g_object_unref (G_OBJECT (n));
}

// Call this with mutex held.
static void
maybe_notify_all (list<mailing_list> &lists)
{
  list<mailing_list>::iterator it;
  for (it = lists.begin (); it != lists.end (); it++) {
    if (it->status () == MODSTATUS_WAITING) {
      notify_for_list (*it);
      it->clear ();
    }    
  }
}

int main (int argc, char** argv)
{
  g_type_init ();
  g_thread_init (NULL);
  notify_init("listping");

  config_file conf ("/home/rupert/.listadmin.ini");
  GError *err = NULL;

  update_data data;
  data.run = true;
  data.mutex = g_mutex_new ();
  data.conf = &conf;

  GThread *updater =
    g_thread_create ((GThreadFunc)update_thread_main,
                     &data, FALSE, &err);
  if (!updater) {
    throw std::runtime_error (
      string("Failed to create update thread: ") +
      err->message);
  }

  sleep (10);

  while (1) {
    // Every thirty seconds, check to see whether the updater's
    // noticed something. If so, send a notification.
    g_mutex_lock (data.mutex);
    maybe_notify_all (conf.get_lists ());
    g_mutex_unlock (data.mutex);

    sleep (30);
  }

  return 0;
}

