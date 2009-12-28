#! /usr/bin/perl
###########################################################################
# This is an example script that sends external metadata to streamripper.
# It implements an external program that:
#   1) Fetches a web page
#   2) Searches the web page for the artist and title information
#   3) Sends the information to streamripper
# 
# To invoke the script, do this:
#    streamripper URL -E "perl fetch_external_metadata.pl META_URL"
#
# This assumes that META_URL is the URL with the artist/title information
# 
# You will need perl and LWP::Simple installed to run this script. 
# On unix, you install LWP::Simple as root, like this:
#    perl -MCPAN -e 'install LWP::Simple';
# On windows, LWP::Simple is included in the ActiveState perl distribution.
#
# This script is in the public domain. You are free to use, modify and 
# redistribute without restrictions.
###########################################################################

use LWP::Simple;

if ($#ARGV != 0) {
    die "Usage: fetch_external_metadata.pl URL\n";
}
$url = $ARGV[0];

while (1) {
    my $content = get $url;

    if ($content =~ m/title="(.*)" artist="(.*)"/) {
	$title = "TITLE=$1\n";
	$artist = "ARTIST=$2\n";
	$end_of_record = ".\n";
	$meta_data = $title . $artist . $end_of_record;
	syswrite (STDOUT, $meta_data, length($meta_data));
    }
    sleep (10);
}
