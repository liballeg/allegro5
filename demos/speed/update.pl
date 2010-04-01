#! /usr/bin/perl -w
#
#  Perl script for updating my progress info. When passed a block of
#  comment text via stdin, it adds this to the end of speed.txt,
#  makes a backup of the current code into a subdirectory named after
#  the current date/time, and also uploads the progress message to
#  http://www.allegro.cc/speedhack/


# read the current time
(undef, $min, $hour, $mday, $mon, $year) = gmtime(time);

$min = '0' . $min if ($min < 10);
$hour = '0' . $hour if ($hour < 10);

$year += 1900;


# read the message text
$msg = join '', <>;
$msg =~ s/^[\r\n]+//;
$msg =~ s/[\r\n]+$//;


# clean the project
system("make clean");


# add message to the readme
open FILE, ">> speed.txt";

$date = "$mon-$mday-$year, at $hour:$min";

print FILE "\n\n$date\n" . ('-' x length($date)) . "\n\n";
print FILE $msg;
print FILE "\n\n";

close FILE;


# make a backup of the code
$dir = "$mon-$mday-$year-at-$hour-$min";

mkdir $dir, 0775;

system("cp -v * $dir");


# upload message to the website
open FILE, "| lwp-request 'http://www.allegro.cc/speedhack/update.taf?_section=update&_UserReference=D50262AC9D32184E383E8C75' -m POST -c application/x-www-form-data -d" or die "Can't open lwp-request\n";

print FILE <<EOF;
-----------------------------7cf2b210f30
Content-Disposition: form-data; name="user_name"

shawn
-----------------------------7cf2b210f30
Content-Disposition: form-data; name="password"

deadbeef
-----------------------------7cf2b210f30
Content-Disposition: form-data; name="comment"


$msg

-----------------------------7cf2b210f30
Content-Disposition: form-data; name="image_name"


-----------------------------7cf2b210f30
Content-Disposition: form-data; name="image"; filename=""
Content-Type: application/octet-stream


-----------------------------7cf2b210f30--
EOF

close FILE;

print "All done!\n";
