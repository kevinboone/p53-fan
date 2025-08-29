/*=============================================================================

  p53-fan
  main.c
  Copyright (c)2025 Kevin Boone, GPL3.0

=============================================================================*/

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>
#include "config.h"
#include "defs.h"
#include "hwmon_scan.h"
#include "fan.h"
#include "curve.h"
#include "mylog.h"

// dry_run is set by a command-line switch. It has to be global, because it's
// used in the tidy-up signal handler.
BOOL dry_run = FALSE;

// We also have to make the lock FS global, for the same reason. The program
// will hold a lock on this file so long as it is running.
int lock_fd = -1;

/** 
  get_lock

  Create a lock on the lockfile. Return 0 if the file can be locked.  If it
can't be locked, then most likely another instance is already running.
*/
static int get_lock (void)
  {
  if ((lock_fd = open (LOCK_FILE, O_WRONLY|O_CREAT, 0666)) == -1)
    return -1;

  if (flock (lock_fd, LOCK_EX | LOCK_NB) == 0)
    {
    // Write our PID to the lock file
    int pid = getpid();
    char s[50];
    sprintf (s, "%d\n", pid);
    write (lock_fd, s, strlen(s));
    return 0; 
    }
  else
    return -1; 

  return 0;
  }

/** 
  remove_lock

  Close the lock file, releasing the lock, then delete the file. We only do
this at shut-down.
*/
static void remove_lock (void)
  {
  if (lock_fd != -1) close (lock_fd);
  lock_fd = -1;
  unlink (LOCK_FILE);
  }

/**
  signal_quit

  All the quit/stop/terminate signals end up here. We set the fan back to
default, auto mode, and remove the lock.
*/
static void signal_quit (int dummy)
  {
  mylog_info ("Caught signal: cleaning up");
  fan_to_auto (dry_run);
  remove_lock();
  exit (0);
  }

/**
  main_loop 

  This is where all the work gets done: Just keep scanning the temperature and
adjusting the fan, until the program receive a signal.
*/
static void main_loop (int interval, CurveNum curve_num, BOOL nowifi, BOOL nodrivetemp)
  {
  int level = 3; // We have to start somewhere
  HSContext hs_context;
  while (1)
    {
    if (hwmon_scan (&hs_context, nowifi, nodrivetemp) == 0)
      {
      mylog_info ("Max temp %dC, driver '%s' path='%s' label='%s'", hs_context.max_temp,
         hs_context.driver, hs_context.path, hs_context.label);
      int new_level = curve_get_level (curve_num, level, hs_context.max_temp);
      // We need to set the level even if it hasn't changed, because something else
      //   might be fiddling with it
      mylog_info ("Setting fan level %d", new_level);
      fan_set_level (new_level, dry_run);
      level = new_level;
      }
    sleep(interval);
    }
  }

/**
  curve_from_name

  Get the curve number that corresponds to the given name. This function is
only used for parsing the command line, and we just exit if it fails. 
*/
static CurveNum curve_from_name (const char *name)
  {
  if (strcmp (name, "cold") == 0) return CURVE_COLD;
  if (strcmp (name, "cool") == 0) return CURVE_COOL;
  if (strcmp (name, "medium") == 0) return CURVE_MEDIUM;
  if (strcmp (name, "warm") == 0) return CURVE_WARM;
  if (strcmp (name, "hot") == 0) return CURVE_HOT;

  mylog_error ("Unknown curve: %s. "
    "Valid values are 'hot', 'warm', 'medium', 'cool', and 'cold'", name);
  exit (0);
  }

/**
  do_stop

  Try to stop an existing instance of the program.
*/
static void do_stop (void)
  {
  int f = open (LOCK_FILE, O_RDONLY);
  if (f >= 0)
    {
    char line[32];
    int n = read (f, line, sizeof (line));
    if (n > 1 && n < sizeof (line))
      {
      line[n] = 0;
      if (line[n - 1] == 10) line[n - 1] = 0; 
      int pid = atoi (line);
      if (pid > 0)
        {
        if (kill (pid, SIGTERM) != 0)
          mylog_error ("Can't send signal: %s", strerror (errno));
        }
      }
    else
      mylog_error ("Can't read lock file '%s'", LOCK_FILE);
    close (f);
    }
  else
    mylog_error ("Can't open lock file '%s': is program running?", LOCK_FILE);
  }

/**
  main
*/
int main (int argc, char **argv)
  {
  // Command-line switches
  BOOL show_version = FALSE;
  BOOL show_help = FALSE;
  BOOL foreground = FALSE;
  BOOL stop = FALSE;
  BOOL nowifi = FALSE;
  BOOL nodrivetemp = FALSE;
  int interval = -1;
  int log_level = MYLOG_WARN;
  CurveNum curve_num = CURVE_MEDIUM;;

  static struct option long_options[] =
    {
     {"curve", required_argument, NULL, 'c'},
     {"dry-run", no_argument, NULL, 'd'},
     {"foreground", no_argument, NULL, 'f'},
     {"help", no_argument, NULL, 'h'},
     {"interval", required_argument, NULL, 'i'},
     {"log-level", required_argument, NULL, 'l'},
     {"no-drivetemp", no_argument, NULL, 'n'},
     {"stop", no_argument, NULL, 's'},
     {"version", no_argument, NULL, 'v'},
     {"no-wifi", no_argument, NULL, 'w'},
     {0, 0, 0, 0}
    };

  int opt;
  while (1)
    {
    int option_index = 0;
    opt = getopt_long (argc, argv, "ndfhsvwi:l:c:",
      long_options, &option_index);

    if (opt == -1) break;

    switch (opt)
      {
      case 'c': curve_num = curve_from_name (optarg); break;
      case 'd': dry_run = TRUE; break;
      case 'f': foreground = TRUE; break;
      case 'h': show_help = TRUE; break;
      case 'i': interval = atoi (optarg); break;
      case 'l': log_level = atoi (optarg); break;
      case 'n': nodrivetemp = TRUE; break;
      case 's': stop = TRUE; break;
      case 'v': show_version = TRUE; break;
      case 'w': nowifi = TRUE; break;
      }
    }

  if (stop)
    {
    do_stop();
    exit (0);
    }

  if (show_version)
    {
    printf (APPNAME " version " VERSION "\n");
    printf ("Copyright (c)2025 Kevin Boone\n");
    printf ("Distributed under the terms of the GNU Public Licence, v3.0\n");
    exit (0);
    }

  if (show_help)
    {
    printf ("Usage: " APPNAME " [-cdfhilsv]\n");
    printf ("  -c, --curve=name    fan curve name\n");
    printf ("  -d, --dry-run       don't change fan speed at all\n");
    printf ("  -f, --foreground    run in foreground, and log to console\n");
    printf ("  -h, --help          show this message\n");
    printf ("  -i, --interval=N    scan interval seconds (5)\n");
    printf ("  -l, --log-level=N   log verbosity 0-4 (2)\n");
    printf ("      --no-wifi       don't include wifi adapters\n");
    printf ("      --no-drivetemp  don't include information from drivetemp\n");
    printf ("  -s, --stop          stop a running instance\n");
    printf ("  -v, --version       show version\n");
    exit (0);
    }

  mylog_level = log_level;
  // We need to set the logging to syslog quite early, because we test that
  // the fan can be controlled before going into the background. But we don't
  // want to do this at all, if we're staying in the foreground.
  if (!foreground)
    mylog_syslog = TRUE;

  mylog_info ("Starting with fan curve '%s'", curve_get_name (curve_num));

  if (get_lock() == 0)
    {
    if (fan_to_manual (dry_run) == 0)
      {
      signal (SIGINT, signal_quit);
      signal (SIGQUIT, signal_quit);
      signal (SIGTERM, signal_quit);

      if (!foreground)
	{
	mylog_syslog = TRUE;
	remove_lock();
	daemon (0, 0);
	get_lock();
	}
    
      if (interval <= 0) interval = 5;

      main_loop (interval, curve_num, nowifi, nodrivetemp);

      // We don't normally get here
      mylog_info ("Finished");
      fan_to_auto (dry_run);
      }
    remove_lock();
    }
  else
    {
    mylog_error ("Can't lock %s: is another instance running?", LOCK_FILE);
    }

  exit (0);
  } 


