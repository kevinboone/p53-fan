/*=============================================================================

  p53-fan
  hwmon_scan.c
  Copyright (c)2025 Kevin Boone, GPL3.0

=============================================================================*/

#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "defs.h" 
#include "config.h" 
#include "mylog.h" 
#include "hwmon_scan.h" 

/**
  should_include

  Decide what metrics to include in the calculation of maximum temperature.
  This function takes the driver name and the label -- that is, the
  contents of the file inputNN_label that is alongside inputNN_temp. Not all
  hwmon drivers use labels.

  This method returns TRUE if the metric should be included.
*/
static BOOL should_include (const char *driver, const char *label, 
         const HSContext *context)
  {
  // Include the wifi adapter
  if (!context->nowifi && strncmp (driver, "iwlwifi", 7) == 0) return TRUE; 
  // Include all the CPU cores
  if (strncmp (driver, "coretemp", 8) == 0) return TRUE; 
  // Include the CPU and GPU figures from thinkpad_acpi
  if (strncmp (driver, "thinkpad", 8) == 0)
    {
    if (strncmp (label, "CPU", 3)== 0) return TRUE; 
    if (strncmp (label, "GPU", 3)== 0) return TRUE; 
    }
  // Include NVME drives, but just the 'composite' figure
  if (strncmp (driver, "nvme", 4) == 0)
    {
    if (strncmp (label, "Composite", 9)== 0) return TRUE; 
    }
  // Include drives exposed using the drivetemp kernel module. These are
  //   usually SATA drives.
  if (!context->nodrivetemp && strncmp (driver, "drivetemp", 9) == 0) return TRUE;
  return FALSE;
  }

/**
  read_pseudo_file

  Read a specific pseudo-file into memory. This function is used to read the
metric names from tempNN_label and the temperatures themselves from
tempNN_input.  Both will be short strings, and can be read in a single call to
read().
*/
static int read_pseudo_file (const char *filename, char *result, int len)
  {
  mylog_trace ("Reading pseudofile '%s'", filename);
  int f = open (filename, O_RDONLY);
  if (f >= 0)
    {
    int n = read (f, result, len);
    close (f);
    if (n > 0)
      {
      result[n] = 0;
      int l = strlen (result);
      if (result[l - 1] == 10)
        result[l - 1] = 0;
      return 0;
      }
    else
      {
      mylog_trace ("'%s' could be opened, but error on reading", filename);
      return -1; 
      }
    }
  mylog_trace ("Can' open '%s' for reading", filename);
  return -1; 
  }

/**
  do_file 

  This function is called for every file under /sys/class/hwmon. It ignores
files that don't match 'temp*_input' -- these are the temperature metrics.
For each matching file it reads tempNN_label to get the sensor name, then
calls should_include() to determine whether this is a sensor whose temperature
should be included. If it is, we use it to work out the maximum temperature
for the current poll.

  This function records the driver and sensor name for the maximum
temperature, but this information is used only for logging -- p53-fan doesn't
care where its temperatures come from, as they are all treated the same. 
*/
static void do_file (const char *driver, const char *path, HSContext *context)
  {
  if (strstr (path, "/temp"))
    {
    if (strstr (path, "_input"))
      {
      char label_file[PATH_MAX];
      strcpy (label_file, path);
      char *p = strrchr (label_file, '_');
      if (p)
        {
        strcpy (p, "_label");
        //printf ("file=%s\n", path);
        char label[32];
        label[0] = 0;
        //printf ("label=%s\n", label_file);
        char *current_label;
        if (read_pseudo_file (label_file, label, sizeof (label)) == 0)
          current_label = label; 
        else
          {
          current_label = "?";
          mylog_trace ("Label file '%s' does not exist", label_file);
          }
        // The absence of a label file does not stop us including the
        //temperature
        if (should_include (driver, label, context))
          {
          context->valid = TRUE; // Indicate that we got at least one 
                                 //   matching sensor in this poll
	  char temp_string[30];
	  if (read_pseudo_file (path, temp_string, sizeof (temp_string)) == 0)
	    {
	    // Got a temperature
	    int temp = atoi (temp_string) / 1000;
	    mylog_debug ("Sensor '%s:%s:(%s)' has temperature %d", 
	      driver, current_label, path, temp);
	    if (temp > context->max_temp)
	      {
	      // If this is the maximum temperature so far, record information
	      // about it for logging.
	      strncpy (context->driver, driver, sizeof (context->driver));
	      strncpy (context->label, label, sizeof (context->label));
	      strncpy (context->path, path, sizeof (context->path));
	      context->path[sizeof(context->path) - 1] = 0;
	      context->max_temp = temp;
	      }
	    }
          }
        }
      }
    }
  }

/**
  do_dir
  
  Enumerate a directory under /sys/class/hwmon. Be aware that these
directories are likely to be symlinks, so we can't use the d_type field in
struct dirent to distinguish files from directories: we need to call stat()
explicitly.

  So far as I know, there's never a need to descend more than one level of the
directory tree, to find all the tempNN_input files. In fact, trying to do so
will fail horribly, as there are circular links in the tree.  

  do_dir won't fail if a directory can't be opened, unless it's the very
top-level directory /sys/class/hwmon. It just assumes that the absent
directory corresponds to some driver that hasn't yet initialized fully. But if
/sys/class/hwmon itself is absent, something is very wrong.
*/
static int do_dir (int level, const char *full_path, HSContext *context)
  {
  int ret = 0;
  char name[32];
  char name_file[PATH_MAX];

  const char *current_name;
  strcpy (name_file, full_path);
  strcat (name_file, "/name");
  if (read_pseudo_file (name_file, name, sizeof (name)) == 0)
    current_name = name;
  else
    current_name = "?";

  DIR *d = opendir (full_path);
  if (d)
    {
    struct dirent *de = readdir (d);
    while (de)
      {
      if (de->d_name[0] != '.') 
        {
	char new_path[PATH_MAX];
	strcpy (new_path, full_path);
	strcat (new_path, "/"); 
	strcat (new_path, de->d_name); 
        struct stat sb;
        if (stat (new_path, &sb) == 0)
          {
          if (S_ISDIR (sb.st_mode)) 
            {
	    mylog_trace ("Expanding directory '%s'", new_path);
            if (level < 1) do_dir (level + 1, new_path, context);
            }
          else
            {
            do_file (current_name, new_path, context);
            }
          }
        else
          mylog_warn ("Can't stat directory '%s': %s", full_path, strerror (errno));
        }
      de = readdir (d);
      }
    closedir (d);
    }
  else
    {
    mylog_warn ("Can't open  directory '%s': %s", full_path, strerror (errno));
    // If this is the top-level directory, /sys/class/hwmon, then failing to
    //   open it must be a fatal error
    if (level == 0) ret = -1;
    }
  return ret;
  }

/**
  hwmon_scan 

  Scan the entire hwmon tree, reading temperatures that match our criteria.
The results are stored in context, which also supplies settings that restrict
the search. This method returns zero if it succeeds, which it almost certainly
will. The only reason for it to fail is if /sys/class/hwmon does not exist, or
it can't find even one valid temperature sensor thereunder.
*/
int hwmon_scan (HSContext *context, BOOL nowifi, BOOL nodrivetemp) 
  {
  strcpy (context->driver, "?");
  strcpy (context->label, "?");
  strcpy (context->path, "?");
  context->max_temp = -273; // Absolute zero :)
  context->valid = FALSE;
  context->nowifi = nowifi;
  context->nodrivetemp = nodrivetemp;
  if (do_dir (0, HWMON_ROOT, context) != 0) return -1;
  if (context->valid) return 0;
  mylog_warn ("No valid, matching sensors detected");
  return -1;
  }


