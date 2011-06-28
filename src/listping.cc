#include "config_file.h"
#include <iostream>
#include <glib-object.h>
#include <glib.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
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

    sleep (60);
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
                               message.c_str (), NULL, NULL);

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
maybe_notify_all (const list<mailing_list> &lists)
{
  list<mailing_list>::const_iterator it;
  for (it = lists.begin (); it != lists.end (); it++) {
    if (it->status () == MODSTATUS_WAITING)
      notify_for_list (*it);
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
    // Every five minutes check to see whether the updater's noticed
    // something. If so, send a notification.
    g_mutex_lock (data.mutex);
    maybe_notify_all (conf.get_lists ());
    g_mutex_unlock (data.mutex);

    sleep (300);
  }

  /* To terminate the update thread more politely, we could do
        data.run = false;
        g_thread_join (updater);
     (would need to say TRUE to joinable in g_thread_create too). But
     we don't really need to care here, so just die.
  */

  return 0;
}

