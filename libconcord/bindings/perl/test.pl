#!/usr/bin/perl
#
# vim:tw=80:ai:tabstop=4:softtabstop=4:shiftwidth=4:expandtab
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# (C) Copyright Phil Dibowitz 2008
#

#
# WARNING WARNING WARNING! READ THIS!!
# NOTE: This is NOT meant for general use. It is a test script for
#    the perl bindings and NOT for production use. It makes lots
#    of assumptions (a model 880, no bin mode, etc.), does almost
#    minimal error checking, and has sloppy output. This is for
#    TESTING ONLY!
#

use strict;
use warnings;

use Getopt::Long qw(:config bundling);
use concord;

use constant FW_FILE => '/tmp/perl_fw';
use constant CONFIG_FILE => '/tmp/perl_cf';
use constant IR_FILE => '/tmp/LearnIr.EZTut';

sub cb
{
    my ($stage_id, $count, $curr, $total, $type, $data, $stages) = @_;

    print '*';
    #print "CALLBACK: count: $count, curr: $curr, total $total, data:"
    #    . " $data\n";
    return;
}

sub dump_config
{
    my ($blob, $size);
    
    my $err;
    print "Reading config from remote ";
    ($err, $blob, $size) = concord::read_config_from_remote(\&cb, 1);
    if ($err != 0) {
        print "failed to read config from remote\n";
        exit(1);
    }
    print "done\n";
    print "Writing config to file ";
    $err = concord::write_config_to_file($blob, $size, CONFIG_FILE, 0);
    if ($err != 0) {
        print "failed to write config to file\n";
        exit(1);
    }
    print "done\n";
    concord::delete_blob($blob);
}

sub dump_firmware
{
    my ($blob, $size);

    my $err;
    print "Reading fw ";
    ($err, $blob, $size) = concord::read_firmware_from_remote(\&cb, 1);
    if ($err != 0) {
        print "Failed to read fw from remote\n";
        exit(1);
    }
    print "done\n";
    print "Writing fw to file ";
    $err = concord::write_firmware_to_file($blob, $size, FW_FILE, 0);
    if ($err != 0) {
        print "failed to write fw to file\n";
        exit(1);
    }
    print "done\n";
    concord::delete_blob($blob);
}

sub upload_config
{
    my ($err, $type);

    print "Reading config ";
    ($err, $type) = concord::read_and_parse_file(CONFIG_FILE);
    if ($err) {
        print "Failed to read config from file\n";
        exit(1);
    }
    print "done\n";
    print "Writing config ";
    concord::update_configuration(\&cb, 1, 0);
    if ($err) {
        print "Failed to write config to remote";
        exit(1);
    }
    print "done\n";
}

sub upload_firmware
{
    my ($err, $type);

    $err = concord::is_fw_update_supported(0);
    if ($err) {
        print "Sorry, firmware is not supported on your device\n";
        exit(1);
    }
    print "Reading config ";
    ($err, $type) = concord::read_and_parse_file(FW_FILE);
    if ($err) {
        print "Failed to read config from file\n";
        exit(1);
    }
    print "done\n";
    print "writing fw ";
    $err = concord::update_firmware(\&cb, 0, 0, 0);
    if ($err) {
        print "Failed invalidate flash\n";
        exit(1);
    }
    print "done\n";
}

sub learn_ir_commands
{
    my ($err, $type);

    print "Reading IR file ";
    my $type;
    ($err, $type) = concord::read_and_parse_file(IR_FILE);
    print "done\n";

    print "Getting key names ";
    my $key_names;
    ($err, $key_names) = concord::get_key_names();
    print "done\n";

    my ($carrier_clock, $ir_signal, $ir_length); 

    for (my $i = 0; $i < scalar(@$key_names); $i++) {
        print "KEY: $key_names->[$i]\n";
        print "Press the right key within 5 seconds... ";
        ($err, $carrier_clock, $ir_signal, $ir_length) =
            concord::learn_from_remote(\&cb, 0);
        if ($err) {
            print "Failed to learn\n";
            exit(1);
        }

        my $str;
        ($err, $str) = concord::encode_for_posting($carrier_clock,
            $ir_signal, $ir_length);
        
        if ($err) {
            print "Failed to encode\n";
            exit(1);
        }

        concord::delete_ir_signal($ir_signal);

        $err = concord::post_new_code($key_names->[$i], $str, \&cb, 0);

        if ($err) {
            print "Failed to post\n";
            exit(1);
        }
        print "done\n";
    }
}
    

#
# main
#


select STDOUT;
$| = 1;

my $opts = {};
GetOptions($opts,
    'get-identity|i',
    'dump-config|c',
    'upload-config|C',
    'dump-firmware|f',
    'upload-firmware|F',
    'reset-remote|r',
    'learn-ir|l',
    ) || die();

if (keys(%$opts) != 1) {
    print "Only one mode is allowed\n";
    exit(1);
}

my $ret = concord::init_concord();
if ($ret != 0) {
    print "Failed to init concord\n";
    exit;
}
print "Get identity ";
concord::get_identity(\&cb, 0);
print " done\n";

print 'mfg: ' . concord::get_mfg() . "\n";
print 'mfg: ' . concord::get_model() . "\n";


if (exists($opts->{'get-identity'})) {
    exit(0);
} elsif (exists($opts->{'dump-config'})) {
    dump_config();
} elsif (exists($opts->{'upload-config'})) {
    upload_config();
} elsif (exists($opts->{'dump-firmware'})) {
    dump_firmware();
} elsif (exists($opts->{'upload-firmware'})) {
    upload_firmware();
} elsif (exists($opts->{'learn-ir'})) {
    learn_ir_commands();
} elsif (exists($opts->{'reset-remote'})) {
    concord::reset_remote(\&cb, 0);
}
