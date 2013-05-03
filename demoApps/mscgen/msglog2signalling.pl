#!/usr/bin/perl

#
# Script to convert NENA message logs to a format suitable for MSC-Generator:
# http://sourceforge.net/projects/msc-generator/
#

use File::Basename;
use strict;

# do not include messages of the following types (separated by spaces)
my $filter_type = "t_timer";
my $filter_from = 
	"netlet://edu.kit.tm/itm/simpleArch/SimpleRoutingNetlet ".
	"netadapt://boost/udp/50779 ".
	"netadapt://boost/udp/50778 ";
my $filter_to = 
	"netlet://edu.kit.tm/itm/simpleArch/SimpleRoutingNetlet ".
	"netadapt://boost/udp/50779 ".
	"netadapt://boost/udp/50778 ";

my $msgcount = 0;
my $msgcount_skipped = 0;

if (@ARGV == 0 || $ARGV[0] eq '--help' || $ARGV[0] eq '-h') {
    print("\n Usage:\n   $0 <NENA messages log file name>\n");
    exit();
}

-e $ARGV[0] or die("Directory/file $ARGV[0] not found.\n");

my $infilename = $ARGV[0];
my $outfilename = basename($infilename, "log") . "signalling";

open(IN, "<$infilename") or die("Cannot open $infilename for reading: $!\n");
open(OUT, ">$outfilename") or die("Cannot open $outfilename for writing: $!\n");

print(OUT "hscale=auto;\n");

while (<IN>) {
	chop;
	my ($time, $from, $to, $type, $msg) = split(/ /);
	if (!($filter_type =~ /$type/) and 
		!($filter_from =~ /$from/) and
		!($filter_to =~ /$to/))
	{
		print(OUT "\"$from\"->\"$to\":\"$type: $msg\";\n");
		$msgcount++;
	} else {
		$msgcount_skipped++;
	}
}

print("$msgcount messages ($msgcount_skipped skipped)\n");

