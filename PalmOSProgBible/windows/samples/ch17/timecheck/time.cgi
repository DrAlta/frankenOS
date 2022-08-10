#!/usr/bin/perl
# time.cgi - Sample Web Clipping application for the
#            Palm OS Programming Bible
#
# ©2000 Lonnon R. Foster.  All rights reserved.

print "Content-type:text/html\n\n";

print <<EndOfHTML;
<html>
<head>
<title>Time Checker</title>
<meta name="palmcomputingplatform" content="true">
<meta name="historylisttext" content="Current time - &date &time">
</head>
<body>
<p>The current time at the server is:</p>
EndOfHTML

print "<p>", `date`, "\n</p>";

print <<EndOfHTML;
</body>
</html>
EndOfHTML
