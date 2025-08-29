# p53-fan

Version 0.1a

## What is this?

Yet another fan control utility for Linux...

`p53-fan` is a simple, opinionated fan speed controller for the Lenovo Thinkpad
P53 and similar computers. It requires the Thinkpad-style fan control
interface, located at `/proc/acpi/ibm/fan`, and temperature monitors in the
`hwmon` subsystem. I intended `p53-fan` to be a simpler, smaller alternative
to the `thinkfan` utility, although it works in a similar way.

By `opinionated` I mean that `p53-fan` is essentially a zero-configuration
utility: it doesn't require a configuration file like `thinkfan` does. Instead
it just makes certain assumptions about which temperature sensors are relevant
for fan control. Please see the more detailed description below, concerning the
choice of sensors -- these won't be suitable for everyone. `thinkfan` is highly
configurable, and likely to be more suitable than `p53-fan` for unconventional
configurations. 

## Features

- No configuration needed
- Responds to CPU, GPU, wifi, NVME and SATA temperatures
- Written in C with no dependencies
- Runtime RAM usage less than a megabyte
- Built-in lock mechanism to prevent accidentally running multiple instances
- Background (default) or foreground operation
- Logging to console or system log
- User-selectable fan curves

## Setting up

To enable fan control, you must give the module parameter `fan_control=1` to
the `thinkpad-acpi` kernel module. The way to do this depends on the Linux
flavour.  In my Debian systems, I create a file
`/etc/modprobe.d/thinkpad_acpi.conf` containing the line:

    options thinkpad_acpi fan_control=1

If you want to monitor the temperature of storage devices other than NVME drives,
arrange for Linux to load the `drivetemp` module; this doesn't necessarily have
to happen before starting `p53-fan`.

## Building and installation

The usual:

    $ make
    $ sudo make install

You'll likely want to have `p53-fan` start at boot time. How to do that depends on
which init system you're using, so I can't really give any more information. 
Since `p53-fan` does its own locking, you shouldn't need to have the init system
do anything more than simply running `p53-fan`, perhaps with a `--curve` 
argument of your choice at boot, and stopping it using `p53-fan --stop` at
shutdown. 

## Running

Just run `p53-fan` as `root`. For testing purposes, it has a 'dry run' mode,
which can output what the program would do, if it had permission to do it. This mode
works best with `p53-fan` running in the foreground, with debug logging
enabled:

    $ p53-fan --dry-run --log-level 3 --foreground

Unless you specify otherwise, `p53-fan` will run as a background process,
and output any log information to the system log. Its default logging level
is '2' -- warnings and errors. In normal operation, it should produce no
output at all.

## Fan curves

The fan curve is the relationship between temperature and fan
speed. `p53-fan` has five curves, that can be selected using the 
`--curve` command-line switch: 'cold', 'cool', 'medium', 'warm',
and 'hot'. It should go without saying that, all other things being
equal, running cooler comes at the expense of increased fan noise. 

Unless you specify otherwise, `p53-fan` uses its 'medium' fan curve.
The curve has the following characteristics:

- Fans are off until the reference temperature reaches 45C, and
  when it falls below 43C.
- Fans at maximum speed when temperature reaches 77C
- All other speed increments uniformly distributed between these 
  points. That is, you can expect a reasonably smooth increase
  in fan speed as the temperature increases.

The 'warm' fan curve is similar, except that the fans will come on
at 51C. This curve is quite similar to the default cooling behaviour.

With the 'hot' curve, the fans switch on at 56C. This should lead to
very quiet operation in most applications (except gaming, perhaps),
and the CPU/GPU on the Thinkpad range are rated to run at 60C 
continuously. However, you might find the laptop uncomfortable
to use in this mode, even if it's safe. 

The 'cool' fan curve is scaled to reach maximum fan speed
at 64C, and 'cold' at 57C. In 'cold' mode, you can expect the fan to be
running quite fast at any workload above idle.

## Command-line options

**-c, --curve=name**

Sets the fan curve (see above for curve names).

**-d, --dry-run**

Don't make any changes to fan speed; just report what would be done.

**-f, --foreground**

Don't detach from terminal; log to console.

**-i, --interval**

Set the interval between polls, in seconds. The default value is 5.

**-l, --log-level=N**

Set the logging level from 0 (errors only) to 4 (very verbose).
Level 4 is only available in foreground mode, to avoid overwhelming
the system logger.

**--no-wifi**  
Don't include the temperature of the wifi adapter. Thinkpad wifi adapters tend
to run quite warm, because they aren't easily cooled by the main fans. Including
the wifi adapter temperature (which is the default) will increase the
fan speed quite a lot, for a relatively small cooling effect on the wifi
adapter. Using this option will reduce fan speed without a significant increase
in CPU/GPU temperature, but the wifi adapter will run a little warmer under light load
than it otherwise would.

**--no-drivetemp**
Don't poll hard drive temperature using the `drivetemp` module. See the discussion
of `drivetemp` below for more information.


## Technical notes

### Sensors

`p53-fan` uses the following temperature metrics.

- Wifi adapter
- All cores from the `coretemp` module
- Aggregate CPU and GPU temperatures from `thinkpad_acpi` 
- 'Composite' sensors from all NVME disks
- Temperatures exposed by the `drivetemp` kernel module (usually SATA drives)

It's not a problem if one or more of these sensors is not available, but there
needs to be at least one for the program to start at all. Of coure, it's unlikely
that there isn't even one CPU core. 

So long as at least one sensor is available, the program will look for the other
sensors at the next poll. So it shouldn't be a problem is certain kernel
modules that manage temperature metrics are slow to initialize. It isn't a problem
is some components (like a discrete GPU) aren't fitted at all.

The `drivetemp` module reads SMART statistics from certain drives, and exposes them
as hwmon metrics. Loading this module (which not happen by default on some Linux
flavours) avoids the need for `p53-fan` to use SMART directly.

In practice, at light loads (word processing, web browsing) the fan speed
is often dominated by the Wifi adapter. This component is hard to cool, even
at relatively high fan speeds. With 'medium' or cooler fan curve, it's likely
that the fan will run, at least at low speed, all the time, even though the
CPU/GPU are cool. 

### Start-up checks

To start up at all, `p53-fan` requires:

- A response from at least one temperature sensor
- The ability to write to the fan control API
- Fan control to be enabled in the API
- No existing lock, that is, no other instance running

### Shutting down

`p53-fan` has no specific shut-down procedure. It traps the usual
'shutting down' signals, and cleans up. In particular, it resets
the fan control API to 'auto', which should restore the default
cooling behaviour. 

### 'disengaged' mode

At high temperatures, cooling works most effectively with the fan
'disengaged. That is, with no speed regulation. Turning off speed 
regulation allows the fan to run as much as 25% faster, albeit not
at a controlled speed. 

`p53-fan` will try to set the fan to 'disengaged' when the fan curve
dictates maximum speed. Unfortunately, not all Lenovo laptops support
disengaged mode, and there's no way for `p53-fan` to know whether it
will work or not. 

I could make the use of 'disengaged' mode configurable with a command-line
switch, if there's a demand for this feature. Despite the potential
incompatibility, it makes sense to use 'disengaged' mode where possible,
since it makes such a difference to the cooling performance.

### Hysteresis

`p53-fan` switches the fan at different temperatures, according to whether the
temperature is rising or falling. If it increases the fan level when temperature
increases from (say) 51 to 52 Celsius, it won't decrease it when falling from
52 to 51 Celcius. Instead, it will wait until the temperature falls a couple
of degrees further. It's worth keeping this in mind, when monitoring 
the behaviour of `p53-fan`.

This effect is called `hysteresis`, and it's necessary because, without it,
the fans would switch repeatedly when the temperature is near a boundary value.

### drivetemp

The `drivetemp` kernel module reads SMART statistics from compatible drives,
and creates nodes for them in the hwmon tree. Installing `drivetemp` allows
`p53-fan` to read the temperatures of non-NVME drives. NVME drives
don't need it: they have their own hwmon support.

`drivetemp` doesn't seem well documented; in particular, it's not clear to
me exactly what kinds of drive it monitors. It's not just the presence of
SMART support: NVME drives support SMART, but they don't seem
to be included by `drivetemp` (and they don't need to be). External drives -- I'm not
sure. The ones I've tried don't get included and, again, they shouldn't be:
it doesn't matter how fast a laptop's fans spin, they aren't going
to cool an external drive. 

Linux installations don't usually load `drivetemp` by default. It can be a
little problematic. A particular problem is that some drives get woken from a
sleep state when their temperature is read. This isn't a problem for
`drivetemp` in particular: it happens with other software, too. However,
installing `drivetemp` makes it easier to monitor the drive temperature which
is, of course, exactly its purpose; this in turn makes it easier to keep the
drive awake when there's no need to.

Using the `--no-drivetemp` switch will prevent `p53-fan` using `drivetemp`,
even if the kernel module is loaded. It can be handy to use `drivetemp` if you
have SATA drives, but check they aren't being woken from sleep unnecessarily.  

## Comparison with thinkfan

thinkfan is a well-established utility with a high degree of sophistication. It
requires a rather fiddly configuration although, of course, samples exist for
popular systems. If you have multiple NVME drives, for example, thinkfan
requires you to specify the specific pseudo-files under `/sys` that 
provide their temperatures. 

`p53-fan`, however, is unsophisticated. It makes assumptions about the specific
temperatures that need to be measured, and there's little opportunity for
installation-specific tuning. It's designed to work 'out of the box', on the
relatively small range of devices it will support.

Because `p53-fan` has essentially no configuration, it doesn't have to check the
configuration when it starts. It won't fail to start because a specific sensor
isn't available at the time: it enumerates all the sensors it's interested in
on every temperature poll.

Of course, the lack of configuration means that `p53-fan` can't cope with, for 
example, broken sensors. There's no way to tell it not to 
sample a particular NVME drive, just because its temperature sensor over-reads
by ten Celsius. Problems like this aren't all that uncommon. 

`p53-fan` only reads temperatures from the hwmon subsystem. It can't use SMART
directly on SATA drives (although it can use `drivetemp` to get the same effect)
an it can't use proprietary APIs to read the temperature of a GPU (but it
can read the GPU temperature from `thinkpad_acpi` for some hardware).

I wrote `p53-fan` for my own purposes, and I consider it feature-complete. 
I'm happy to try to fix bugs that are reported to me (via GitHub), but I won't be
adding any new functionality.

## Legal

`p53-fan` is copyright (c)2025 Kevin Boone, released under the terms of the
GNU Public Licence, v3.0. There is no warranty of any kind.

## Release history

0.1a, August 2025  
First release.

