#!/usr/bin/perl
# hello.cgi - Sample Web Clipping application for the
#             Palm OS Programming Bible
#
# ©2000 Lonnon R. Foster.  All rights reserved.

use strict;

use CGI qw(:standard);


# Set up the titles array.
my @titles = ("None",
              "Mr.",
              "Mrs.",
              "Ms.",
              "Dr.",
              "General",
              "King",
              "Queen");

# Retrieve parameters
my $titleIndex = param("title");
my $firstName  = param("fname");
my $lastName   = param("lname");
my $time       = param("time");

# Assemble an appropriate formal name if there is enough name to provide a
# proper greeting.
my $titleName = "";
if ($titleIndex == 0) {
    if ($firstName && $lastName) {
	$titleName = $firstName . " " . $lastName;
    }
} elsif ($titleIndex >= 1 && $titleIndex <= 5) {
    if ($lastName) {
	$titleName = $titles[$titleIndex] . " " . $lastName;
    }
} elsif ($titleIndex >= 6 && $titleIndex <= 7) {
    if ($firstName) {
	$titleName = $titles[$titleIndex] . " " . $firstName;
    }
}
	
# Make an appropriate string for the time of day.
my $greetTime;
$time =~ s/:.+//;
if ($time < 4 || $time >= 22) {
    $greetTime = "night";
} elsif ($time >= 4 && $time < 12) {
    $greetTime = "morning";
} elsif ($time >= 12 && $time < 17) {
    $greetTime = "afternoon";
} else {
    $greetTime = "evening";
}

# Output the Web Clipping.
print header();
print start_html(-title => 'Greeting',
		 -meta  => {'palmcomputingplatform' => 'true',
			    'historylisttext' => 'Greeting - &date &time'});

# Check to see if there is a title name.  If title name has not been assembled,
# there is not enough data to form a proper greeting.  Present an appropriate
# message to the user.
if (! $titleName) {
    my $errTitle;
    my $errName;
    if ($titleIndex == 0) {
	$errTitle = ", but without a title,";
	$errName  = "first and last names";
    } elsif ($titleIndex == 1) {
	$errTitle = " sir, but";
	$errName  = "last name";
    } elsif ($titleIndex == 2) {
	$errTitle = " ma'am, but";
	$errName  = "last name";
    } elsif ($titleIndex == 3) {
	$errTitle = " miss, but";
	$errName  = "last name";
    } elsif ($titleIndex == 4) {
	$errTitle = " doctor, but";
	$errName  = "last name";
    } elsif ($titleIndex == 5) {
	$errTitle = " General, but";
	$errName  = "last name";
    } elsif ($titleIndex == 6) {
	$errTitle = " your Majesty, but";
	$errName  = "first name";
    } elsif ($titleIndex == 7) {
	$errTitle = " your Majesty, but";
	$errName  = "first name";
    } 
    print p("I'm sorry$errTitle I need your $errName to give you a proper greeting.");
    print p("Tap the back arrow above and try again.");
} else {
    print p("Good $greetTime, $titleName.");
}

print pre("  ");
print a({href => 'file:Greeting.pqa/index.html',
	 button => undef},
	"Get Another Greeting");
print end_html();
