#! /usr/bin/perl 

$old_version = "";
$new_version = "";

sub replace_file {
    ($file) = @_;

    $tmp_fn = "uv.tmp";
    open TMP, ">$tmp_fn";
    open INF, "<$file";
    while (<INF>) {
	s/$old_version/$new_version/g;
	print TMP;
    }
    close INF;
    close TMP;
#    `mv $tmp_fn $file`;
    rename ($tmp_fn, $file);
}

###############################################################
##  main
###############################################################
$old_version = shift;
$new_version = shift;

if (!$new_version) {
    $_ = `grep AC_INIT configure.ac`;
    chomp;
    $_ =~ s/^.*,//;
    $_ =~ s/\).*$//;
    die "Usage: upgrade_version.pl old_version new_version\nold_verion=$_\n";
}

@files = (
	  "configure.ac",
	  "lib/rip_manager.h",
	  "winamp_plugin/sr163.nsi",
	  "makedist_win.bat",
	  "streamripper.1.asc",
	  );

for $file (@files) {
    replace_file ($file);
}
print "Don't forget to check the date in streamripper.1.asc\n";
print "Don't forget to recompile streamripper.1.asc\n";
