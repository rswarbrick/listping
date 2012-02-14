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

// Wrapper class around NotifyNotification pointers: deals with the
// object ref'ing and unref'ing and has a (hopefully correct) copy
// constructor so can be placed inside lists.
class notification {
public:
  notification (const string &msg);
  notification (const notification &n);
  ~notification ();

  void set_enabled (bool enable_p = true);

private:
  NotifyNotification *n;
};

notification::notification (const string &msg)
  : n (NULL)
{
  GError *err = NULL;
  n = notify_notification_new ("Message awaits moderation",
                               msg.c_str (), NULL);
  if (!n) {
    // Something has to go dramatically wrong for this to fail, I
    // think. Output to standard error on the off chance someone's
    // looking and die.
    std::cerr << "Listping error: "
              << err->message << "\n";
    throw std::runtime_error ("Failed to create notification.");
  }
  notify_notification_set_timeout (n, 5000);
}

notification::notification (const notification &old)
  : n (old.n)
{
  g_object_ref (G_OBJECT (n));
}

notification::~notification ()
{
  g_object_unref (G_OBJECT (n));
}

void
notification::set_enabled (bool enable_p)
{
  GError *err = NULL;
  g_return_if_fail (n);

  if (enable_p) {
    // If there's no notification daemon running,
    // notify_notification_show fails. Let's not die horribly: maybe one
    // will appear one day...
    if (!notify_notification_show (n, &err)) {
      std::cerr << "Listping error: "
                << err->message << "\n";
    }
  }
  else {
    // If the notification isn't visible, close will give an error
    // (GDBus.Error:org.freedesktop.Notifications.InvalidId) but I
    // can't check whether or not it is, so just ignore the error.
    notify_notification_close (n, NULL);
  }
}

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

// Call this with mutex held.
static void
maybe_notify_all (list<mailing_list> &lists,
                  list<notification> &notifications)
{
  list<mailing_list>::iterator l_it;
  list<notification>::iterator n_it;

  for (l_it = lists.begin (), n_it = notifications.begin ();
       l_it != lists.end ();
       l_it++, n_it++) {
    if (l_it->status () == MODSTATUS_WAITING) {
      n_it->set_enabled (true);
      l_it->clear ();
    }
    else n_it->set_enabled (false);
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

  list<mailing_list> lst = conf.get_lists ();
  list<notification> notifications;
  list<mailing_list>::const_iterator it;
  for (it = lst.begin (); it != lst.end (); it++) {
    notifications.push_back (
      notification ("Message to moderate in list: " +
                    it->get_address ()));
  }

  while (1) {
    // Every thirty seconds, check to see whether the updater's
    // noticed something. If so, send a notification.
    g_mutex_lock (data.mutex);
    maybe_notify_all (conf.get_lists (), notifications);
    g_mutex_unlock (data.mutex);

    sleep (30);
  }

  return 0;
}
