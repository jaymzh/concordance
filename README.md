# Concordance

This software allows you to program your Logitech Harmony remote using a
configuration object retrieved from the [harmony
website](https://members.harmonyremote.com/EasyZapper/New/ProcLogin/Start.asp?BrowserIsChecked=True)

The website is required. The website is required in Logitech's software as
well, it's just that their software wraps the website. Their website has
all the logic on what codes work with what remotes and what hardware, etc.
The software just takes the binary blobs that the website makes and writes
it to the remote control. This is true for both Logitech's software and
this software.

However, this software has some extra functionality such as being able to
dump (backup) your existing config, being cross-platform, and giving you
extra visibility into what's actually happening with your remote.

NOTE: the Logitech web server now is setting a header 'X-Frame-Options: DENY'
which prevents the update process from working correctly with Concordance.  In
order to work around this, you will have to use a web browser that ignores
this header.  For Firefox, the 'Ignore X-Frame-Options Header' Add-on has been
used successfully.  For other browsers, there may be similar add-ons available.

## Installing

Concordance is packaged in a wide variety of Linux distributions and other
OS package repositories. The easiest path is to use such a package if it's available for your OS/distro.

[![Packaging status](https://repology.org/badge/vertical-allrepos/concordance.svg)](https://repology.org/project/concordance/versions)

If it's not packaged for your OS/distro, then refer to the appropriate `INSTALL` file in this repository for your OS.

## Usage

*NOTE TO MAC USERS*: You need to use 'sudo' since you don't have udev.

*NOTE TO LINUX USERS*: This assumes you have proper udev support setup
(see the INSTALL.Linux file in the libconcord source). If not, you'll
need to use sudo or be root.

0. I got a file from the website, do something useful with it.

  ```
  concordance <filename>
  ```

  This will attempt to figure out what to do, and do it. Note that the update
  process sends TWO files: a connectivity test and the update. You will need
  to do both.

1. Backup the config

  ```
  concordance --dump-config=/tmp/config.EZHex
  ```

  This will read the config off of your remote and write it to /tmp/config.EZHex.
  It's a good tool for development as well as backing up your config. This can
  however be created from the members.harmonyremote.com/EasyZapper website. The
  equals is needed if you pass in a filename since the filename is optional.
  If you don't specify, concordance will use 'config.EZHex' in the current
  directory.

2. Connectivity test

  Go to members.harmonyremote.com/EasyZapper, and when you're ready, choose
  "Update My Remote." Before Logitech provides an actual config, they will
  first attempt to do a connectivity test. Downloaded the Connectivity.EZHex
  file, and then run the test:

  ```
  concordance Connectivity.EZHex
  ```

  If that doesn't work, you can tell concordance what it is manually:

  ```
  concordance --connectivity-test Connectivity.EZHex
  ```

3. Write a config

  Once the connectivity test is successfully completed, the site will prompt you
  to download the actual config in a file called Update.EZHex. Save it and then
  you can use it with:

  ```
  concordance Update.EZHex
  ```

  Again, concordance should do the right thing here, but in case of problems you
  can explicitly tell concordance what to do with:

  ```
  concordance --write-config Update.EZHex
  ```

4. Backup the firmware

  Sometimes the site will want to update your firmware. Concordance allows you to
  backup your old firmware so you may later revert if you prefer. You can do this
  with:

  ```
  concordance --dump-firmware
  ```

  This will read the firmware off of your remote and write it to fimrware.EZHex.
  See "1. Backup the config" for more information.

5. Write firmware

  NOTE: This feature is only implemented for certain models. Please see
    [supported models](SupportedModels.md)

  However for models we support this on, it works like this:

  ```
  concordance Firmware.EZHex
  ```

  Again, if you have a problem, you can tell concordance what to do explicitly:

  ```
  concordance --write-firmware Firmware.EZHex
  ```

There are other options - check out the --help one!

## Related Software

* [Congruity](https://github.com/congruity/congruity) is a cross-platform graphical front-end for libconcord written in python

## Disclaimer

*THIS SOFTWARE IS NOT SUPPORTED BY OR IN ANY WAY RELATED TO LOGITECH!*
