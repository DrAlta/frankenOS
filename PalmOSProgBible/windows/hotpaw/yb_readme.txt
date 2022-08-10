HotPaw(tm) Basic for the Palm Computing Platform
  (formerly cbasPad Pro Basic)

HotPaw Basic is a full-featured and easy-to-use scripting language
which allows you to create and execute small Basic programs directly
on your Palm handheld organizer/computer.

The BASIC language has been around for 3 decades; it's continued
popularity is due to its simplicity and ease of learning.  HotPaw
brings this language to the palm of your hand and enables you to
better use your Palm handheld as a true personal computer.

HotPaw Basic has many features including:
 - over 75 functions and 30 commands built-in;
 - execution of Basic programs contained in your MemoPad;
 - simple built-in dialogs for displaying results and prompting for
   input parameters;
 - custom form creation with user definable buttons, fields and
   checkboxes;
 - access to several standard database formats (JFile Pro, HanDBase,
   DB and List) instead of a proprietary format;
 - sound and graphics drawing commands (including COLOR support);
 - programmable ToDo, Datebook appointment and alarm creation;
 - serial port and infrared (SIR) communication;
 - support for the majority of the ANSI/ISO Mininal Basic programming
   language standard, as well as many MSBasic(tm) functions and
   statements;
 - support for double precision floating point arithmetic and for the
   MathLib transcendental function library;

HotPaw Basic comes with several example programs, as well as a small
application which allows you to start a Basic program directly from
the Application Launcher (or even a button press).

Important Notes and Limitations regarding this version:

  - Please see the LICENSE included in this document for important
    terms and limitations.  If you do not agree with the license
    you are not authorized to use the HotPaw Basic application.
    
  - After 30 days of TRIAL use, yBasic will enter the DEMO mode.
    DEMO will still allow one to write and run up to 4 programs
    contained in the MemoPad.
    
    Unregistered DEMO copies of HotPaw Basic will list a maximum
    of 4 programs in the Program or ScratchPad selector list.  If
    you have more than 4 Basic programs and wish to use HotPaw Basic
    in the DEMO mode, you may have to delete or rename all but the
    4 programs you wish to use.
    
  - Registration of your copy of HotPaw Basic will remove all DEMO
    and TRIAL mode restrictions.
    
    The registered version of HotPaw Basic is on sale at PalmGearHQ:
  
      <http://www.palmgear.com/software/showsoftware.cfm?prodID=5221>  
      
    Contact HotPaw for site licenses.   Significant discounts for
    accredited educational institutions will be available.  Other
    HotPaw applications are also available at <www.palmgear.com>.
    
  - PalmOS 3.0 or later (Palm III, Palm Vx, etc.) is REQUIRED.
  
  - The MathLib.prc (1.0 or 1.1) library is required.  See:
      <http://www.probe.net/~rhuebner/mathlib.html>  or
      <ftp://ftp.rahul.net/pub/rhn/mathlib11.zip>
  
  - The use of the SwitchHack extension for switching between the
    MemoPad and the HotPaw Basic interpreter is highly recommended.
   
  - This application is still being tested to find and remove bugs
    in the application and errors in the documentation.  Please be
    sure to make backup copies of all your important data prior to
    installing and using this application.
    
  - The documentation is still very very preliminary, is subject to
    change without notice, and may not completely or accurately
    describe the current version of HotPaw Basic.
    
    * New improved documentation is currently under development *

  - The database name of this version of HotPaw Basic, "yBasic",
    may change to "Basic" in future versions.
    
The latest version of this documentation can be found at:

  <http://www.hotpaw.com/rhn/hotpaw/cbpb-readme.txt>  
  
***

Important Notes for previous cbasPad classic (0.xx) users:

 - HotPaw Basic now runs programs from the MemoPad.
 
 - Many commands and functions have changed or been removed!
 
 - Some cbasPad programs may be compatible; one needs to test
   each program for compatibility.

Important Notes for new HotPaw Basic users:

 - The current preliminary documentation assumes you already know
   a little about using the classic BASIC programming language.


*** *** *** ***  	  README	  *** *** *** ***
  
README and Command Summary for HotPaw cbasPad Pro Basic
  
This README contains a description of HotPaw Basic, a BASIC programming
language interpreter and scripting utility for 3Com PalmOS handhelds. 
  
  Please see the Revision History for important changes.
  Basic is NOT compatible with cbasPad (although many programs
  written for latter versions of cbasPad may run).

Contents:
  Introduction
  Installation
  Getting Started
  Running a HotPaw Basic program
  Basic Statement, Command & Function Summary
  Additional PalmOS Commands & Functions
  Some Warnings
  Examples
  Revision History
  FAQ
  Copyright and Disclaimer notice

Introduction

  BASIC is an easy to learn computer programming language, invented at
  Dartmouth college around 1963.  Various Tiny Basic implementations
  were written around 1976 for use in early personal computers with
  very limited memory configurations (8 KBytes, or less).

  The original cbasPad interpreter was written as an experiment in
  writing a small Basic interpreter (in portable C) that could run in
  the very tight memory requirements of a Pilot 1000 (PalmOS 1.0).
  cbasPad has many deficiencies due to these restrictions.

  HotPaw cbasPad Pro Basic is an new interpreter for a full version
  of the Basic language, takes advantage of many of the features of
  PalmOS 3.0 and above, and includes much better support for 
  accurate floating point math and for database access.
  
  
  More History of the Basic language

  The BASIC programming language was developed in 1963 at Dartmouth
  College by Professors John G. Kemeny and Thomas Kurtz.  It was meant
  as an simple, easy to learn method for even non-science students to
  learn to write computer programs.  The acronym "BASIC" originally
  stood for "Beginner's All-purpose Symbolic Instruction Code".

  The year 1964 was in an era when even the smallest computers were
  larger than refrigerators and cost several times more than a single
  family house.  Computers sat inside special rooms and had to be
  shared by many students and researchers.  Students would communicate
  with these minicomputers via teletypes, which were consoles that had
  keyboards, but no pointing device and no video display, instead
  having only a slow printer capable of printing one line at a time.

  Because of the simplicity and small size of the original Basic
  programming language, it was widely adopted in the late '70's and
  early '80's as the most common and prevalent language for almost
  all personal computers.  Microsoft's first product was a Basic
  interpreter for the Altair computer.  Most personal computers of
  this era (the Apple II, TRS-80, Commodore-64, Timex Sinclair, and
  the original IBM PC) came with a Basic interpreter included.  Many
  dozens of book on programming in Basic were also published during
  this era.  Since this was before the internet and CDROMS was widely
  available, most people typed-in Basic programs that were printed
  in books and magazines.

  Most early Basic interpreters had an interactive mode, where one
  could type simple commands and see results after hitting the
  "return" key. For instance, typing the commands:

    LET A = 7
    LET B = A + 2
    PRINT B

  would result in the console printing "9" (the result of 7 + 2).

  These simple Basic interpreters also had a stored program mode,
  where one would write a multi-line program, with each line with a
  line number, be able to save the program and then to run the
  program at a later time.

  Since Basic is a complete programming language it includes
  statements that allow calculation, simple logic, input, output,
  looping and conditional branching.
  
  Currently, the Basic programming language is used as the core of
  Visual Basic, Visual Basic for Application (VBA), and VBScript
  (or basicscript) for web page scripting.


***
Installation:

  Install the usual way, using the 3Com/Palm Install and HotSync
  utilities on a Windows or MacOS based PC, or using the linux
  pilot-xfer utility on a computer running linux or unix.
  
  You can have both HotPaw cbasPad Pro Basic and cbasPad 0.9x
  installed on your PalmPilot at the same time; however cbasPad
  classic should be installed and run first if you want to share
  the scratchpad.

Installing HotPaw Basic programs:

  Basic programs are text files or memos containing statements in the
  Basic language.  The HotPaw Basic application can run Basic programs
  whose text is contained in memos of the built-in Palm MemoPad.
  
  If you have a HotPaw compatible Basic program on your desktop PC,
  you can copy and paste the Basic program text into the Desktop
  MemoPad application and HotSync.  On a linux system, you can use
  the pilot-link install-memo utility to copy a short text file into
  your Palm MemoPad.
    
Getting Started:

  Try entering a short Basic program into your Palm MemoPad.  Start
  a new memo (in any category).

  Any memo starting with the '#' character and ending with the 4
  characters ".bas" in the first line will be recognized as a Basic
  program in the HotPaw Basic Show Programs selection screen.
  Here's an example first line:
  
# My Example 1 .bas

  Here's a short test program that will ask you to guess a number:
  
    #guess_a_number.bas
    x = 0
    # set the variable "secret" to a number between 1 and 5
    secret = 1 + rnd(5)
    while (x <> secret)
      input "Guess a number between 1 and 5", x
    wend
    display x; " is correct!"
    end
    
  Switch to the Basic application.  If there is not a border around
  a list of programs, use the menubar to select "Show Programs".
  If you can't see your new program, check the name to make sure it
  starts with a # and ends with .bas on the first line.
  
  Select your program and hit the "Exec" button to execute it.
  
  If you use the Hackmaster extension utility, then try using the
  SwitchHack extension to switch back and forth between Basic to
  the MemoPad application when writing and testing your Basic
  programs.
  
  The "Show ScratchPad" is for small calculations that do not require
  any loops or subroutines.  Just enter a list of equations and a
  print statement for the result, then the statements you wish
  executed, and hit the "Exec" button.
  
  
***
The HotPaw cbasPad Pro Basic BASIC Programming Language

   You can write a Basic program in the built-in Palm MemoPad.
   Memos are limited to about 4000 characters in length.  To create
   longer programs, one can use the
	#include <#TITLE>
   comment/pragma.
   HotPaw Basic will look for a memo entitled "#TITLE" ; and insert
   lines from that memo as in-line statements.  The include
   pragma may not be nested.
   
   The HotPaw Basic language is mostly compatible with books on
   programming in the BASIC language that were published between 1977
   and 1988.  Since these books are mostly out of print, your best bet
   is to try your local public library.  HotPaw Basic is a subset of
   the full Basic language; so some features present in a full version
   of the Basic language are not present.  One key difference from
   "classic" BASIC programming is that line numbers are optional and
   not required in HotPaw Basic (when running programs written in
   the MemoPad).
   
   There is a fairly complete list of www pointers to information on
   the generic Basic language near the end of my 'Basic' web page:
   <http://www.nicholson.com/rhn/basic/>
   
   ALL keywords must entered in be lower case.
   Multiple statements may be used on one line when separated
   by the colon (":") character.
   All lines have a maximum length limit of 80 characters.
   Comment lines in HotPaw Basic can start with the '#' character.
  
***
Program Execution

  Unnumbered statements in the MemoPad are automatically given
  sequential line numbers incrementing by 1 from the previous line.
  Any numeric line number labels present should be in increasing order
  and differ by at least the number of intervening lines plus one.

  All statements after the first "run" command are ignored.
  A "run" command is automatically assumed at the end of a Basic
  program contained in the MemoPad.

  Program execution terminates after encountering the "end" statement.

  The Basic Scratchpad is for mostly for use of direct execution
  statements, e.g. calculation statements that require no branches,
  loops or subroutines.  Very short Basic program may be written in
  the Scratchpad, but with several limitations: programs must start
  with a "new" statement and end with a "run" statement (without line
  numbers), all other statements must have sequential and
  non-overlapping line numbers.  This mode is only for backward
  compatibility with a few older cbasPad programs.
   
  Statements in the ScratchPad view without line numbers can be
  executed by selecting the statement you wish to have executed,
  and then hitting the "Exec" button.

  To stop a running program, hit the "Done" button or the Calendar
  application button.  To stop a running program when an input dialog
  is present, use the "Stop" menu item.
  
  If you have a Basic program entitled "#startup.bas" (without the
  quote marks, of course) in your MemoPad, then you may use the
  included yLaunch.prc application to launch yBasic and automatically
  run your startup Basic program.

  

***

Some Examples:

  # this is a comment line (because of the leading '#').
  # multiple statements per line allowed.
  
    x = 7 : y = 3 + 4

  # a "?" statement prints 1 line to a dialog box.

    ? "hello ", 23+7

  # Arrays must be dimensioned before use.
  # Here's an example of a loop to initialize an array:
  
    dim a(4) : for i=0 to 4 : a(i) = i : next i

***
Basic Statement, Command & Function Summary:

Operators, Statements, Functions and other reserved Keywords:

  Operators:
  
    +  -  *  /  mod  ^
    =   <>  >  >=  <  <=   and  or  xor  not
    &

  Commands and Statement Keywords:
  
    let 
    if  then  else endif
    for  to  step  next
    while wend exit
    goto  gosub  return
    end  run  new  stop  :  rem
    dim
    data read restore

  Functions:
  
    int() abs() sgn()  rnd() bool()  asin() acos()
    sqr() exp() log() log10()  sin() cos() tan() atn() 
    len()  val()  str$() chr$() hex$() mid$() asc()
    ucase$() lcase$() right$() left$() field$()  instr()
    
  Special functions and commands:
  
    msgbox
    inputbox  input$()
    print  input
    ?  display
    sub()  end sub
    fre   timer
    peek()  poke  call  varptr()
    fn  op   draw   sound   morse
    peek$()  float()  randomize
    on as  using
    open close  eof
    eval()  fn  form
    db db$() get$() put find() kill
    
  Reserved words (for future use):
    option base  
    output append load save  random lof loc get
    files  fseek  usr  bload bsave  exec
    quit cont  renum  clear elseif
    date$ time$ timer  erase  say
    home cls gotoxy htab vtab pos  button field
    graphics sprite pset moveto lineto window scrn
    push pop  isarray  mat  select case  function
    def  type class extends  string integer double
    sinh cosh tanh floor ubound  err erl
    dprint

Variables and Constants:

  Floating point constants may use the "e" notation.
    e.g. 7.0, -3.5e10, 6.022e23, 6.6261e-34

  Floating point numbers and variable are represented by a format
  similar to IEEE double.  This is a binary format, which means
  that many decimal fractions will not be exactly represented.

  Hex constants may be entered by preceding with "0x".
  
  Variable names may be up to 15 characters in length.
  Integer variable names must end with the '%' character.

String Variables:

  String variables must end with the '$' character.
  There is a limit of 63 string variables.

  String arrays must be dimensioned before use (except s$() and
  r$()).  String arrays are limited to one dimension and may
  not be redimensioned.

  There are 2 built-in string array: r$() and s$(), both
  auto-dimensioned to 64 elements: e.g. r$(0) .. r$(63)
  
  The string array s$(0) to s$(63) is for passing parameters and
  for temporary use by many of the built-in database, dialog,
  display and code resource functions and commands.
  
Numeric Arrays:

    dim a(10), b(10,10)
    
  Numeric arrays must be dimensioned before use with the DIM
  statement.  Numeric arrays are limited to two dimensions.
  

Expressions and Operators

  Assignment:
    
    let a = b + 2
    
  Using the "let" keywoard before an assignment is optional.
    
    a = b + 3
	
  Arithmetic expressions are limited in complexity to around
  8 levels of parenthesis so as to not overflow the very small
  PalmOS application stack.
   
    a = (b^2 + (d/(2+2))) * c

  The "and", "or" and "xor" operators do bit-wise operations;
  "not" is a boolean operator and only returns 1 or 0.
    
  Use either the "&" or the "+" operator for string concatenation.
  
    a$ = "hello "
    b$ = a$ & "there!"	:' -> result is the string "hello there!"

Flow-of-Control Statements

    :
    	A colon may be used to seperate multiple statements
    	on one line.
  
    end
  
  	Program execution ends at the first end or stop command.
  
    if then endif
    
    	Standard single line IF THEN
    	
    		if x > 1 then y = 2
    		if y = 3 then goto 300
    		
    	Multistatement IF THEN  (acts as a single block if-endif)
    	
    		if x > 1 then y = 2 : z = 3
    		
    	Multiline block IF ENDIF  (No "then" keyword used.)
    	
    		if (x > 1)
    			y = 2
    			z = 3
    		endif
    
    for step next
    
    	for i=1 to 5 step 2 : print i : next i
  
    while wend
    
    	n = 1
    	while (n < 100) : rem - loop until this condition is false
    	  n = n * 2
    	wend
    	print n
  
    gosub sub return
    
    	Standard BASIC subroutine usage:
    		100 gosub 500
    		200 end
    		...
    		500 print x+1
    		510 return
    
    	Named subroutines usage:
    		gosub foo(5)
    		end
    		...
    		sub foo(x)
    		print x+1
    		end sub
    		
    	(Note that HotPaw Basic user-defined subroutines
    	are static (no true local variables), and may not
    	be used recursively.  The name of a user-defined
    	subroutine may be used as a global return variable.)
  
    goto N
    
    	(No example.  See old Basic programming books.)
  
    on N goto M
    
    	(No example.  See old Basic programming books.)
    	
    labelx :
    goto labelx
    
	(An initial variable name followed by a colon is a
	label that can be used as a target by a GOTO statement;
	thus one can program without using line numbers.)


Unimplemented ISO/ANSI Minimal Basic Standard keywords:

    option base [ 0 | 1 ]
	(default sets array lower bound to 0)
    tab
    	(this command is not implemented.)
    def fn
    	(this statement is not implemented.)


Special PalmOS Commands and Functions:

Output Commands

    msgbox(message$ [ , title$ [ , n ] ] )
  	'- displays a message box with a message string.
  	'-   if n = 2 then display "OK" & "Cancel" buttons
  	'- the text message will word-wrap up to 2 lines.
  	
    msgbox( line1$ + chr$(10) + line2$ [...] )
    	'- displays 2 lines  (up to 3 lines are possible)
    	'-  seperated by 0x0A linefeeds.
  	
    print x,y	'- display x and y in a message box.
    		'- (for compatibility with old programs)
    
    ?		'- same as PRINT command
    		'- (for compatibility with old programs)
  
Input Functions:

    input$( prompt$, [ title$ [ , default$ ] ] )
    input$( prompt$, [ title$ [ , n ] ] )
  		'-  displays input dialog & returns input string
  		'-   if n = 1 then only display "OK" button
  		'-   if n = 2 then display both "OK" & "Cancel"
        	'- use  val(input$(prompt$))  to input a number
        	
    inputbox(...)
    		'- same as input$(...)
        	
    input [ prompt$ , ] stringvar$
    		'- uses input$() dialog to LINE INPUT one string
    		'- (for compatibility with old programs)

    input$(1)	'- waits for 1 graffiti char or button press
          	'-   captures presses on the rightmost 5 button.
  		'-   (11 up, 12 down, 5,6,7 = 3 app buttons)
  	  	'-   leftmost app button halts interpreter
  
    inkey$	'- checks for 1 graffiti char or button press
  		'-   nonblocking, returns "" for no input
  		
  		'- input$(1) blocks , inkey$ polls and continues

    fn pen(0)   '- waits for a pen tap (blocking); returns x
    fn pen(1)   '- returns last pen x coordinate
    fn pen(2)   '- returns last pen y coordinate
    fn pen(3)	'- returns tickcount time of last pen tap 
    fn pen(4)   '- returns non-zero if the pen is still down
    fn pen(5)	'- waits for a display tap, key or graffiti char;
    		'-   return 0 for a tap or ascii value for graffiti
  
    get$("data",n)	'- returns line from current program page
  			'-   with "#" comment character stripped
  			
Dialog Form functions:

    form(9,n,title$)	'- displays n line 2 column form
  			'- from 2 to 9 lines
	s$(0), s$(2) ... are the prompt strings
  	s$(1), s$(3) ... are the default & return values
  	return value is 1 for OK, 2 for A button, 3 for B button
  
    form(10,n,title$)	'- displays n line single column form
    
  	s$(0), s$(2) ... will display text, 1st line bolded
  	if s$(0) is empty, s$(1) will display as an input field
  	
    form(12,0,title$)	'- displays 2 column form (with 8 rows)
    			'- with a calculator keypad
    
  	s$(0) and s$(1) display text at the top & bottom
	s$(2), s$(4) ...  are the prompt strings
  	s$(3), s$(5) ...  are the default & return values
  	return values are 0 for Done, 1 for OK
  	  2 for Calc button, and 3 for Calc2 button
  	
    form(0)	'- returns last msgbox or dialog button status
    		'-   also clears the button status to 0.
    
Math Functions:

    sqr(x) 	'- square root
    exp(x) 	'- e ^ x
    log(x) 	'- natural log of x
    log10(x) 	'- log base 10 of x
    
  Mathlib.prc should be installed for accurate transcendental functions.
    
    sin(a) cos(a) tan(a)  '- trig functions for a in radians
    atn(x) asin(x) acos(x)  '- inverse trig, returns radians
    atn(x,y)	'- 4 quadrant arctan
    		'- (trig fn's available only if MathLib is present).

    rnd(n) 	'- returns a pseudo random integer in the range 0..n-1
    int(x) 	'- truncates toward zero.
    round(x)	'- rounds to nearest integer.
    round(x,d)	'- rounds to d places right of the decimal point.
    round(x,0,n)	'- rounds to about n significant digits.

    fn deg(r)	'- converts radians to degrees
    fn rad(d)	'- converts degrees to radians
    
    fn snorm(x)	'- (stat lib) standard normal integral(-inf to x)
    	'- bcmdsnorm.prc must be installed
    
    len(a$)   	'- returns length of string a$
    val(a$)   	'- returns numeric value of a$ or 0
    str$(x) 	'- returns string representation of number x
    str$(x,n)	'- returns string padded to n characters
    str$(x,n,2)	'- returns n char string with 2 decimal places
    str$(x,1,14)	'- double precision (15 place) E conversion
    chr$(c) 	'- returns string of ascii char c, length 1
    hex$(n) 	'- returns hex string of value n
    ucase$(a$)	'- returns uppercase string
    lcase$(a$)	'- returns lowercase string
    right$(a$,n)  	'- returns the right justified substring of a$
    left$(a$,n)   	'- returns the leftmost n char substring of a$
    mid$(a$, n, m)  	'- returns substring starting at n of length m
    mid$(a$, n, 1)  	'- returns a single character out of a$
    field$(a$,n, "," )	'- returns the Nth field (comma seperated)
    instr(a$, b$)	'- finds b$ in a$ or returns 0 for not found
    eval(q$)  	'- evaluates string q$ as a numeric expression ("1+2")
    
    fn strw(a$,n,0) '- width in screen pixels of 1st n chars of a$
    fn trim$(a$)	'- trims off leading and trailing white space

Date and Time Functions:

    timer   	'- returns running seconds timer

    timer(100)  '- returns running tickcount timer
    		'- (100ths of a second on a Palm III)

    fn date$(0)	'- returns todays date as 8-digit string
  		'-   e.g. "19991225"
    fn date$(0,1)	'- returns todays date as in text form
  			'-   e.g. "Dec 25, 1999"
    fn date$(-1)    '- returns 8-digit string result from date chooser
    fn time$(0)	    '- returns 6-digit time string, e.g. "120000"
    fn time$(-1)    '- results from time chooser dialog
    fn days(m)	'- converts 8-digit date number to days from 1904
    fn days(d$)	'- converts 8-digit date string to days from 1904
    fn dayow(n)	'- returns day-of-week (1=Sunday) from 8-digit date
    fn date$(n)	'- converts days from 1904 to 8-digit string

    fn wait(n)	'- delays n seconds (uses 0.02 second increments).
  		'- returns > -1 if a screen tap occured before timeout.
    fn sleep(n)	'- sleeps for between n & n+60 seconds (n >= 120).
  		'- returns approximate seconds elapsed
  		

Financial Functions:
    
    fn pmt(p, r, n)	'- calculate payment, given
  		'-  principle, interest% and number of periods
    fn pv(a, r, n)	'- calculate present value
    fn fv(a, r, n)	'- calculate future value


Graphics Commands:

    draw -1		'- clears the middle of the display
    draw x1,y1,x2,y2	'- draw line on display [0..159]
    draw x1,y1,x2,y2, 2	'- draw a gray line
    draw x1,y1,x2,y2,-1	'- erases a line
    draw x1,y1, w,h, 4	'- draw a rectangle (topLeft, extent)
    draw x1,y1, w,h, 5	'- draw a gray rect
    draw x1,y1, w,h, 7	'- paint a filled rect
    draw x1,y1, w,h,-7	'- erases a rect
    draw circle x1,y1,r		'- draw circle of radius r
    draw circle x1,y1,r,7	'- paint a filled circle
    
    moveto x1,y1	'- sets current pen location to x1,y1
    lineto x2,y2	'- draws a line from current loc to x2,y2
    
    draw t$, x,y [,f]	'- draw text t$ (optionally with font f)
    
    draw t$, x,y, 32	'- draw text using SingleHander jumbo font.
    	'- this requires installation of the singlehand.prc app
    	
    fn colord()		'- returns the color depth available
    			'-   or 0 for PalmOS versions prior to 3.5
    fn colord(n)	'- where n = 1,2,4 (& PalmOS >= 3.5 only)
    			'-   sets grayscale depth to n if possible
    			'- returns prior depth
    			
    fn scrn(x,y,k)	'- returns index, r, g or b value of screen
    			'-  pixel at (x,y) for k = 0, 1, 2 or 3 
    			'-  respectively under PalmOS 3.5 or later.

    * WARNING * the bitmap commands are in beta test.  Only try
    this if you have backed up all your important or valuable data.

    draw x,y, w,h, 100,a$(0)	'- draws a 1 bit/pix bitmap
    		'- w & h must be less than or equal to 32
    		'- bitmap described row-wise by hex strings which
    		'-   are contained in string array a$(0) .. a$(n)
    		'-   bitmap strings must be multiples of 4 in length
    		'-   and the beginning of each row is byte aligned.

    draw x,y, 0,k, 400,s$	'- draws resource ('Tbmp') bitmap
    		'- uses bitmap with resource ID k contained in 
    		'-   the prc or app named by the variable s$.
    		'- draws nothing if prc or resource is missing.

  If you have the new PalmOS 3.5 color ROMS:
  
    draw color r,g,b,c	'- sets the foreground color for c = 1
    			'- background color for c = 2
    			'- text color for c = 3
    	'- r,g,b for red green blue respectively [0 thru 255]
    	
    draw color i,-1,0,c	'- sets the color by color index [0-255]
    
    fn rgb2i(r,g,b)	'- returns color index of nearest RGB color

    draw circle x1,y1,r,a1,a2
    			'- paint a filled arc from a1 to a2 radians
    			
    fn test(-100)	'- returns PalmOS version or highest
    			'-   version supported (e.g. 3.1, 3.5 ...)
			'-   or negative for old non-color yBasic
  
Sound Commands:

    sound { frequency in Hz }, { duration in mS }, { vol 0..63 }
    
    morse string$, wpm, freq, vol, cwpm [ Farnsworth ]
  

Memo (file)Database commands:

    open "memo",n as #1	'- opens memo number n for r/w
    			'- #1 thru #4 are legal file #'s for read
    			'- only #4 will work for write
    			'- use db.find() to get n from title$
    		
    open new "memo",title$ as #4
    			'- creates and opens new memo
  			'- the title string must not be blank
    db.index		'- returns the new memo index number
    
    input #1, a$	'- inputs one line from open memo
    			'- this statement may modify s$(0)
    
    print #4, x$	'- appends to open memo
    			'- memos are limited to 4k characters
    			'- use db.len to check for truncation
    			
    close #1		'- closes memo
    
    kill "memo",(db.find("MemoDB")), n, -9  '- deletes memo #n

    chain n		'- runs Basic program in memo index #n
    			'-   does not clear variables
    		'-  use db.find("memo", title$) to find n
    		'-     n must be greater than 0
    		'-  Note: the MemoPad "All" list displays (n+1)
    		'-  chain is not legal in ScratchPad execution.
    		'-  use fn launch() to start other applications.
    		
  Only descriptors #1 thru #4 are allowed for MemoPad files.
  
MemoPad (file)Database functions:

    db.find("memo", t$)	'- finds memo with title t$, returns index
    	'- searches in reverse order, so always finds last/latest
    	'- returns -1 if no match found
    	
    db.find("memo", t$, 0)  '- searches in forward order
    db.len("memo", n)	'- returns length of memo #n
    db$("memo", n, 0)	'- returns first line of memo #n
    db$("memo", n)	'- returns the next line of memo #n
    db$("memo", n, k)	'- returns text at byte offset k
    		'- note: parameter n should be a single variable,
    		'-       not an expression
    
    eof			'- returns true if last input or
    			'-   db$ read was past end of memo
  
    fn newcat(c)	'- returns 0
    	'-  sets the category of the next new record or memo to c
    
Advanced Form statements:

  Dynamic objects (buttons, field, checkboxes) should only be added
  near the beginning of a program (after a "draw,-1" statement)
  and before any user interaction (print or input).  Form buttons
  should be created before any other form items.  There is a limit
  of 15 form buttons, 2 pop-up list and 32 total form items.  The
  total amount of list selections among all pop-up lists is 63.
  
  See the "#counter.bas" example program, which uses both form
  commands to create buttons, and database commands to save data
  between program runs.
  
  Use "draw -1" to clear the form before adding any dynamic objects.
  Draw text and graphics only when done creating all form items.

  Dynamic objects will change the behavior of the current running
  main Basic program form.  Form statements may only be used in a
  program contained in a Memo or run from a program select view.
  Form statements will produce an error message when run from the
  ScratchPad Edit view.
    
  ** Note that the use of dymamic forms on PalmOS 3.0 and earlier
  is NOT recommended by Palm. **  These commands have only been
  tested under PalmOS 3.1 and later. **
  
    form btn x,y,w,h, title$, 1		'- creates one button
    		'- x,y location.  w,h width & height.
    		
    		
    fn btnkey()  '- returns ascii keycode of last form item
    
  The first button returns 14 to "form(0)" or ascii key chr$(14)
  to the "input$(1)" function.  Note that if you use the "form
  btn" statement more than once, for instance in a loop, you will
  get multiple buttons that will return different key codes. Use
  "fn btncode()" to get the keycode for that new button.
     		
    form fld x,y, w,h, default$, 1
    		'- creates a text input field
    		'- w is in pixel units, h is normally set to 12
    		
    form cbx x,y, w,h, "", 1
    		'- creates a 14x14 checkbox
    		'- w&h are dummy parameters
    		
    		'- 1st field or checkbox returns a string in s$(0)
    		'-   after a form btn press.
    		'- 2nd field or checkbox returns in s$(1), etc.
    		
    form lst x,y, n,sel, a$, 1
    		'- creates a pop-up list with trigger at x,y
    		'- n is the number of items in a list ( < 31 )
    		'- a$ must be a dimensioned array larger than n
    		'- the 1st list element is taken from a$(1)
    		'- sel is the initially selected item number
    		
  Form fields, checkboxes and lists return their value in array
  strings s$(0) thru s$(19) after any form button is pressed.
  s$(0) gets the value of the first field created, s$(1) the next,
  etc...

    form title 0,0,0,0, newtitle$,1
    		'- changes form title to the string newtitle$
    		
    form redraw 0,0,0,0, "", -1
    		'- erases all graphics except for form objects.

    form reset 0,0,0,0, newtitle$, -1
    		'- clears all dynamic objects from dynamic form
    		'-  and resets initial btnkey count to 14
    		
    form btn 0,160,0,0, qbtn$, 1
    		'- if qbtn$ is less than 6 characters in length
    		'-   changes the title of the "Quit" button to 
    		'-   the contents of qbtn$, and sets that buttons
    		'-   keycode to ctrl-c (ASCII 0x03)
    		
Scripting the built-in applications:

    find("date",   0, 19991225 )
  	'- finds 1st non-repeating DateBook record on Dec 25, 1999
  	'-   use  val(fn date$(0))  for todays date
  	'- return -1 in none found
    find("date", -12, date8num)
  	'- finds subsequent non-repeating DateBook records
  	'-   date8num is an 8 digit number in the form YYYYMMDD
    find("date", -21, date8num)
  	'- finds 1st repeating DateBook record (slow)
  	'-   only if repeating event has not been modified
    find("date", -20, date8num)
  	'- finds subsequent repeating DateBook record (slow)
  	
    get$("date", n, 10)
  	'- reads DateBook record #n into s$(20 thru 29)
  	'-  non-repeated entries only
  	s$(20)	= title
  	s$(21)	= year
  	s$(22)	= month
  	s$(23)	= day
  	s$(24)	= hour	(blank for "UnTimed")
  	s$(25)	= minute
  	s$(26)	= ending hour
  	s$(27)	= ending minute
  	s$(28)	= alarm minutes (0 for no alarm)
  	s$(29)	= first 63 characters of note
  	
    put "date",new,10
  	'- creates new DateBook entry from data in s$(20..29)
  	
    find("todo", title$, n)
  	'- finds ToDo record with given title
  	'-  starting search with record n
  	'- return -1 for none found
    find("todo", -11, d)
  	'- finds 1st ToDo record matching 8-digit date d
    find("todo", -12, d)
  	'- finds subsequent ToDo's matching 8-digit date d
    find("todo", -13, d)
  	'- finds 1st undone ToDo record due on or before d
    find("todo", -14, d)
  	'- finds subsequent undone ToDo's due
    find("todo", -16, n)
  	'- finds ToDo record modified since last HotSync
  	'-  starting search with record n
    get$("todo", n, 10)
  	'- reads ToDo list record #n into s$(20 thru 29)
  	'- s$(20) will contain "error" if the ToDo is not found
  	s$(20)	= description (up to 63 characters only)
  	s$(21)	= year	(or blank for no Due Date)
  	s$(22)	= month
  	s$(23)	= day
  	s$(24)	= priority
  	s$(25)	= done
  	s$(26)	= category number
  	s$(27)	= category name
  	s$(29) 	= first 63 characters of note

    put "todo", new, 10
  	'- creates ToDo entry from data in s$()
    put "todo", n, 10
  	'- modifies ToDo record #n
  	'-   modifying only non-blank fields in s$(20..29)
  	'- a category number will override any category name
  	
  	'- Warning: Modifying a ToDo will go much faster if
  	'-  only the s$ fields you want to change are non-blank.
  	'- * If you don't want to truncate a description or note
  	'-  longer than 63 characters, leave that field blank.
  	
  	'- Warning: The order of records (possibly every
  	'-  record number) is changed in the ToDo database when
  	'-  any ToDo record is changed or modified.   So you
  	'-  can't just loop through the ToDoDB by record number.
  	
    db.find("addr", name$)
    	'- finds Address/Phone List entry starting with name
  	
    get$("addr", n, 20)
  	'- reads Address/Phone List entry n into s$(20..39)
  	'- * Only 63 characters of each entry are returned.
  	
    fn launch(app$, n)
    	'- launch application by app Creator or appname
    	'-   use -1 for no data index
    	'-   e.g. i = fn launch("lnch", -1) to return to Launcher.

Database functions:

    db.open xyz$	'- returns access index for named database
  			'-   or negative number for not found
  		
    db.len(dbname$)	'- returns number of records in database
    db.len(dbname$, r)	'- returns length in bytes of record r
    db.find(dbname$)	'- returns db index# of named database
    			'-  return negative number for not found
    			
    err		'- returns a negative number if any database
    		'-  open or read errors occur.
    err(0)	'- returns error code and then clears it.
    
  The formats of 5 Database applications are directly supported:
  
  List - Freeware 3 column database application, creator = "LSdb",
	  found at <http://www.magma.ca/~roo/list/list.html>  
  DB - free open source database application, creator = "DBOS",
	  found at <http://vger.rutgers.edu/~tdyas/pilot/>  
  JFile  - commercial database application, creator = "JBas",
  JFile Pro - commercial database application, ctype = "JFil",
	  found at <http://www.land-j.com/pilprogs.html>  
  HanDBase - commercial database, creator = "PmDB",
  MobileDB - commercial database, creator = "Mdb1".
  
  The level of support varies for each type of database.
  The freeware List and DB database applications are fully
  supported; HotPaw Basic can create new databases of creators
  "LSdb" and "DBOS".  HanDBase databases are supported by
  calling code in the actual HanDBase application; if the
  HanDBase application is not present, commands to access its
  databases will fail.  Only field reads are supported for
  MobileDB databases.  Note that for MobileDB databases, the
  actual data records start with record number 4.
  
  Creating new records in DB and List databases:
    
    db.open name$ as new "DBOS",n
  		'- creates a new DB database named with name$
  		'-   with n fields per record.
  		
    db.open name$ as new "LSdb",3
  		'- creates a new List database named with name$
  		'-   with 3 fields per record.
  		
    db.index	'- returns last created or opened db index
  		
  Only one new database can be created and accessed at a time.
  Creating a new database will close all other databases.
  Therefore any new database should be created before opening
  any other databases for access.
  
  Database access:
  
  Support for each type of database varies, depending on the
  capabilities of the database application, and the information
  supplied by the database application vendor.
  
  == Reading from a database field:
  
    db$(dbname$, r, f)	'- returns field f of record r
  			'-  first record has r = 0
  	'- only returns first 63 characters of any longer fields.
  	'- only fields 1 thru 3 are valid for List db's.
  	'- this function may modify s$(20) thru s$(29)
  	
    db$(dbname$, r, f, z)
    	'- if field f of record r is longer than z characters
    	'-   then returns up to 63 characters of field f
    	'-   starting with the z-th byte (offset).
    
    db$(d, r, f) 	'- returns field f of database index d
  			'-  database d must be opened first.
  			
    open DATABASE_NAME$	'- open for r/w
    db.index		'- returns last opened db index
  
  Note that for MobileDB databases, the first regular data record
  starts with record number r = 4.

  == Searching a database:
  
  For List, DB or JFile Pro type databases:

    db.find(dbname$, x$)
	'- finds x$ in 1st record of database dbname$
	'- returns -1 for failure to find matching string

  For HanDBase type databases:
  
    db.find(fname$, x$, n)
	'- finds x$ in n-th field of HanDBase database fname$
	'- for the first field use n = 1
	'- returns -1 for failure to find matching string

  == Modifying a database field:
  
  If the database is of creator "JBas", "JFil", "PmDB" or "DBOS",
  then:
  
    let db$(dbname$, r, f) = x$
  	'- modifies 1 field of a database, record r field f
  	'- this statements may modify s$(20 thru 39)
  	'- the LET keyword is optional
  	
    let db$(d, r, f) = x$	'- with database id #d
  	'- r must be in range and d must be open
  	
  If the database is of creator "LSdb" then the above statement
  will work only if all fields are shorter than 63 characters.
  Warning: Any longer fields in the List database record
  modified may be silently truncated.
  
    db.len(dbname$, -16)
    	'- returns count of non-deleted records in database
    	'- Note that deleted records may appear in the
    	'- middle of some databases.
  	
  == Adding database records:
  
  It is up to you to make sure that you only add records of the
  proper format.  Only text fields are currently supported.
  
  For DB, JFile Pro or HanDBase databases then
  
    put d,new,(n)		'- adds a new db record
  	'- with n fields taken from s$(20) thu s$(20+n)
  	'-   (max of 19 fields usable)
  	
    db.index	'- for DB databases returns new index
    
  If the database is of creator "LSdb" then
  
    put d,new,103		'- adds a new db record
  	'- with n fields taken from s$(20) thu s$(23)
  	
  == Creating a new database:
  
  For a database of creator "LSdb" then
  
    db.open dbname$ as new "LSdb",3
  	'- creates a new List database with 3 fields per record
  
  For a database of creator "DBOS" then
  
    db.open dbname$ as new "DBOS",n
  	'- creates a new DB database with n fields per record
  	
    db.open ... else label
  	'- goto statement@label on open error
  	
    kill dbname$,dbindex,-99,-9  '- deletes database
  	'- index must match database name
  	
Doc format database functions:

  For a database of creator "REAd"  (Aportis "Doc" format):

    db$(d, r, k)
    	'- uncompresses compressed Doc format
    	'- reads a string from record r starting at offset k
    	'- a max of 63 characters at a time can be read
    	'- returned string may include a CR or LF at the end.
    	'- sets eof if k is past end of record
    	'- record number r must be greater than 0
    	'- use db.find() to get database index d
  	
  	
Database Advanced Functions:

  The db.peek() function allows reading raw database record data.
  Use the db.find(dbname$) function to find a dbindex.
  
    db.peek(dbindex, r, n, 1)	'- returns byte n of record r
    db.peek(dbindex, r, n, 2)
    	'- returns 16-bit word starting of byte n of record r
    	'-   r must be even (word aligned).
    db.peek(dbindex, r, n, 64)
    	'- returns string (63 chars max) starting at offset n
    
    db.peek(dbindex,-19,0,64) 	'- returns database name
    db.peek(dbindex,-17,0,64) 	'- returns database type string
    db.peek(dbindex,-18,0,64) 	'- returns database creator string
    db.peek(dbindex,-20,0, 4) 	'- returns database attributes code

  To peek at string resources inside prc databases use:
  
    db.peek(dbindex, 0x74535452, r, 64)
    	'- where r is the resource id number
    	
  -1 is returned if the referenced data cannot be found or read.

    
Serial Port commands:

    open "com1:",baud_rate as #5  '- opens serial port
    			'- at baud_rate { 2400,9600,19200,38400 }
    print #5, a$	'- prints to serial port
    put #5,byte		'- serial output one byte
    fn serial(5)	'- returns number of input bytes waiting
    get$(#5, n)		'- returns a string up to n bytes long
    get$(#5, 0)		'- returns 1 ascii byte from serial port,
  			'-   or returns -1 if no input is waiting
    input #5, a$	'- waits for 1 line of text
    put #5,-3		'- start break
    put #5,-2		'- stop break
    fn serial(-5)	'- get CTS (2) and DSR (1) status
    fn serctrl(7,2,0)	'- sets 7 bits, even parity, returns 0
    close #5		'- closes serial port

    open ... #5 else label	'- goto statement@label on open error
    open "com1:",rate,ir as #5	'- open serial port in SIR Ir mode
    
    open "PalmPrint" for output as #5
    		'- enables Stevens Creek Software Print Server

  The serial port only stays open during a program execution, and is
  automatically closed at the end of a program or "exec selection".
  
  Only descriptor #5 is allowed for serial port use.

Misc functions:

    fre		'- returns amount of dynamic heap memory left
    
    varptr(y)	'- returns memory address of variable y (or y$).
    peek(a)	'- returns the 8-bit byte at address a
    peek(a,2)	'- ... 16-bit value at word aligned address a
    peek(a,4)	'- ... 32-bit value at word aligned address a
    peek$(a)	'- returns zero terminated string at address a

    fn battery() '- returns scaled battery voltage (V*100)
    
    fn p(18)   '- returns memory address of the clipboard text
    fn p(19)   '- returns length of the clipboard text
    fn p(36)   '- returns current memo get$ offset
    fn p(66)   '- force warm reset of PalmOS
    fn p(67)   '- relaunches the cbasPad Pro interpreter application
    
    fn ftr(sig$, n)	'- reads Feature Manager long int
  
  Warning: Use of undocumented fn or fn p() numbers may cause your
  Pilot to crash or Corrupt data!

Advanced Features:

  Code Plugin functions.

  HotPaw cbasPad Pro can call 'code' resource plugins modules.
  These can be developed similarly to DA code modules.  The resource
  type must be 'bCmd' or 'BCmd', and the code ResID must be 1000.
  The executable routine must be at the start of the code resource,
  use no globals, and should be prototyped:
  'bCmd'
  	long int main(long int param, long int param2, char *s64);
  'BCmd'
  	double main(double param, double param2, char *s64);
  Call from yBasic by using:
  
    fn bcmd("module_name", param)	'- returns a -1 on errors
    
  The database name in this example should be "bcmdmodule_name".
  The second calling parameter is optional and set to zero
  if not present in the bcmd function call. 
  The s64 character pointer will point to s$(48) which is only
  64 chars in length.
  
  There is a source code example on the HotPaw web site.
  
  DA (Desk accessory) modules may also be called; the parameter
  is ignored.  See  <http://member.nifty.ne.jp/yamakado/da/>  for
  details on using and writing DA's.
  
  Feature Manager Data Storage.
  
  To set a Feature Manager feature (32-bit), use the command
  
    put "ftr", "cBA3", n, d
    
  with the feature number #n in the range 1024..1123 and d is the
  data you wish to store.
  
Advanced Features (registered versions only):

  The registered version of HotPaw Basic can use added commands to
  support laser bar code scanning using Symbol SPT 1500/1700 model
  handhelds.  Registered owners please contact HotPaw for details.

  Doc format support:
  
    chain 1, docfilename$
	'- runs a program contained in a Doc file.
	'- (maximum program length of 8k characters).
  
  pedit32 support:
  
    chain n, "Memo32DB"
	'- runs a program contained in pedit32 memo #n
  
    db.find("Memo32DB", title$)
    	'- finds pedit32 memo by title string.

  The "chain" statements only works when used in programs contained
  in the MemoPad.


Debugging Aids:

  In order to save memory, HotPaw Basic does not store the text
  of program lines or the text location when loading a program.
  Thus, the only way for it to display a program line with a given
  line number is to actually load the program again from the main
  program selector view.  The "fn debugline(n)" function will set
  a listing breakpoint that will stop loading whenever the next
  program line with line number n is read.  After viewing the line,
  you can tap the screen to continue loading, or hit the
  "Applications" icon to exit HotPaw Basic.

    fn debugline(n)	'- sets listing breakpoint, returns 0
    
  Another way is to find linenumbers is to add some statements
  with labels, then print the following function value for the
  desired labels:

    fn linenum(label)	'- returns line number for any label
  

Error Message Details:

  "Out of Memory "
  	Too long a program.
  	Too large an array.
  	Too many variables.
  "Out of string memory "
  	Too many string variables.
  	Too large a string array.
  "Line number too big "
  	Line number greater than 30000
  "Line number duplicated "
  	Duplicate line number.
  	Too many lines between numbered lines.
  	Too many lines before the first line number.
  	Unwanted line break in a program line (creating an
  	  extra line number.)
  "Unsupported subroutine call "
  	Subroutine call inside an expression.
  "Undefined line or subroutine "
  	Subroutine target not found
  "Out of Data "
  	Read statement without enough data statements
  "IF without ENDIF or THEN "
  "Missing THEN error "
  "Missing ENDIF "
  	Missing statements.
  "Missing GOSUB "
  	RETURN found outside of a called subroutine
  "Unsupported index var "
  	Label, string variable or constant as FOR index.
  "Missing FOR "
  	NEXT found without a matching FOR statement.
  "FOR w/o NEXT "
  "WHILE without WEND "
  "WEND without WHILE "
  	Missing loop control statements.
  "Syntax error "
  	Miscellaneous errors in statement syntax.
  "Type mismatch error "
  	string/numeric parameter mismatch
  "Subscript err "
  	Missing subscript.  Subscripting a non-array variable.
  "Nesting error "
  	Expression too complex.  Too many parenthesis.
  "Program line too long  "
  	Program line longer than 80 characters.
  "Include file not found "
  	#include <> pragma with incorrect memo title or spelling.
  "ScratchPad programs require line numbers and a 'run' command. "
  	LOOP or GOSUB was found in direct execution mode.
  "Missing return, next or wend "
  	Program ended inside a loop or subroutine.
  "Form item error "
  	Too many buttons, fields or checkboxes.
  	Attepting to recreate a previously created button.
  	Running a From statement from the ScratchPad Edit view.
  "Only #5 available for serial "
  "Only #5 available for printer "
  	OPEN #5 ... statement used with a number other than #5.
  "Out-of-range error "
  "Database not open "
  	Database not opened for write.
  	Database closed by prior database or memo creation
  	statement. 
  "Feature Expired"
  	Feature only available in the registered version.


Some Cautions:

  Don't forget that the s$() array can be modified by db and form
  commands.
  
  Using serial port I/O will drain the battery much faster than
  normal.

  Divide by 0 errors are not trapped, they return "NaN" (IEEE
  floating point code for "not a number".)

  Peek, Poke and Call are dangerous statements.
  You can lock up your Pilot and corrupt ALL your data when using
  the poke and call statements.  Use at your own risk.  HotSync
  and backup often if you want try these functions.  Memory above
  the dynamic heap is protected and cannot be poked without an OS
  write enable call.

Bugs:

  Many... think Costa Rican rain forest.  HotPaw Basic is too
  complex and too new to have been completely tested.
  
  This documentation is preliminary and subject to change or
  correction without notice.
  
  Some syntax errors are reported without line numbers.

  Please send bug reports to rhn@hotpaw.com
  
  * Bug reports without the HotPaw Basic version number and 
  * PalmOS version will be ignored.


***
FAQ:

  Q: How do I share my Basic programs?
  
  A: Because HotPaw Basic programs are contained in MemoPad
     memo's as plain text, one can HotSync them to the desktop
     and email (or post on the web) the contents of these memo's
     as text files.
  
  Q: Can HotPaw Basic compile stand-alone prc applications?
  
  A: No.  A compiler is a completely different, much bigger,
     and far more complex application. 
     
     There are already a several compilers for Palm handhelds,
     among them Quartus Forth for on-board compilation, and
     Metrowerks Codewarrior for Palm for a complete Mac/Win
     hosted IDE.
     
     HotPaw Basic is designed for people who want to be able
     to see and modify programs and scripts directly on their
     Palm handheld.
     
  Q: What's the difference between HotPaw Basic and cbasPad?
  
  A: cbasPad classic was written as a spare evening hobby in 1996
     for Palm Pilot 1000's and 5000's running PalmOS 1.0 with only
     128k/512k of memory.  Some of the arithmetic and math routines
     built into PalmOS 1.0, which cbasPad classic depends upon, are
     not reliable or accurate.  Also, because of its design for
     the single segment restriction of PalmOS 1.0, cbasPad classic
     is no longer expandable.  In spite of its limitations,
     cbasPad classic will remain free for non-commercial and
     non-mission-critical use on old PalmOS 1.0 and 2.0 devices.
     
     HotPaw Basic was written to take advantage of Palm 3.x and
     has double the feature set and a much more accurate math
     calculation capability.

  Q: Where are the other FAQs?
  
  A:  There are several PalmPilot related FAQ's on the Web. 
      Try these URL's:
	<http://www.nicholson.com/rhn/palmfaq.txt>
  	<http://www.palmpilotfaq.com/>


***********************************************************************
Examples
***********************************************************************

Cut and paste these examples into the desktop MemoPad, and
then perform a HotSync operation to install them onto your
Palm handheld.  You can the use the MemoPad application on
your Palm to double check the transfer.

Make sure the very first character in the Memo is a  #
Make sure the last 4 characters of the first line are  .bas
with no trailing spaces.

These examples should then show up in the HotPaw Basic
"Show Programs" display.

Example 1a:  Hello World
-- --	cut here 	-- --
#first_example.bas
# enter this into the MemoPad and start yBasic
# prints "Hello World"
#
a$ = "Hello"
b$ = "World"
print a$, b$
end
-- --	cut here 	-- --


Example 1b:  LOOP AND PRINT
-- --	cut here 	-- --
# example 1 .bas
# prints a small "TIMES THREE" multiplication table
#
for i=1 to 4
  print 3 * i,
next i
print
end
-- --	cut here 	-- --


Example 2a: INPUT AND DISPLAY
(left blank...)


Example 2b: Writing to the MemoPad
--
#memopad table .bas
open new "memo","trig table 1" as #4
m = db.index  :' memo number
for d = 0 to 180 step 15
  r = fn rad(d)
  print #4, d,sin(r),cos(r)
next d
close #4
end
--

Example 2c: MemoPad read
--
#memopad table .bas
m = db.find("memo","trig table")
if (m < 0) then end :' not found
open "memo",m as #1
input #1,title$	:' skip title
input #1,a$
while not eof :' check for end 
  display a$
  input #1,a$
wend
close #1
end
--


Example 3: FINANCIAL CALCULATION
-- --	cut here 	-- --
# loan payment .bas
# calculates loan payments
n$="<your name here>"
for i=0 to 17: s$(i)="": next i
s$(0)="by "+n$
s$(2)="Principal"
s$(4)="Years"
s$(6)="Interest %"
s$(8)="Periods/Yr": s$(9)=12
while (1)
 j = form(12,0,"Payment Calc")
 if j=0 then end
 i=(val(s$(7))/100)/val(s$(9))
 p=val(s$(3))
 n=12*val(s$(5))
 a=fn pmt(p,i,n)
 a$=str$(a,5,2)
 s$(10)="Payment"
 s$(11)="$"+a$
wend
end
-- --	cut here 	-- --


Example 4: GRAPHICS
-- --	cut here 	-- --
# circle demo .bas
draw -1  :' clear screen
cx = 80 : cy = 80 :' set center
r = 30 :' set radius
moveto cx+r,cy
for a = 0 to 6.28 step 0.1
  x = cos(a)
  y = sin(a)
  lineto cx+r*x, cx-r*y
next a
w = fn pen(0) :' wait for tap
end
-- --	cut here 	-- --


Example 5: DATABASE
-- --	cut here 	-- --
# db new+find .bas
# create a new DB database
# fill it with 5 records
# and search for 1 item
f$ = "test-db-1"
mydb=open f$ as new "DBOS",4
if mydb<0 then end
d=fn days(fn date$(0))
for i=0 to 5
# make up some fields
s$(20)=chr$(asc("A")+i) +"2"
s$(21)=right$(fn date$(d+i),4)
s$(23)="00" + str$( i)
s$(22)=str$(timer)
put mydb,i,104
next i
# now find a record
j=db.find(f$,"C")
if j >= 0 then
  rem # get 2nd field
  a$=get$(mydb,j,2)
endif
# display found item
? j,a$
end
-- --	cut here 	-- --


Example 6: SCRIPTING PALM APPS
Example 6a: find appointments
-- --	cut here 	-- --
# datebook today .bas
# find todays DateBook items
d=val(fn date$(0))
k=find("date",0,d)
while (k >= 0)
if k < 0 then end
a$ = get$("date",k,10)
? k,a$
k=find("date",-12,d)
wend
end
-- --	cut here 	-- --

Example 6b:
-- --	cut here 	-- --
# count done todos .bas
# counts up items in your ToDo list
c=db.len("ToDoDB")
n=0
# count only priority 2
p=2 : p$=str$(p)
for i=0 to c-1
get$("todo",i,10)
draw str$(i),80,80
# count only done todos
if s$(25) = "1" and s$(24) = p$
  n=n+1
endif
next i
? n," done todos with priority ", p
end
-- --	cut here 	-- --

Example 6c:
-- --	cut here 	-- --
# todo today .bas
# find ToDos due today
k=0
k=find("todo", -11, val(fn date$(0)) )
30:
if k<0 then end
a$=get$("todo",k,10)
if s$(25)="0" then ? k, a$
# ? "+",k,a$,s$(25)
k=find("todo", -12, val(fn date$(0)) )
goto 30
end
-- --	cut here 	-- --


Example 6d:
# put todays due ToDos into Datebook
(left blank...)


Example 7: Put Appointments in ToDo list
-- --	cut here 	-- --
#datebook2todo.bas
# HotPaw Basic ( cbasPad Pro ) program
#	to make new ToDo's
#	from todays appointments.
t$="make new ToDo ?"
d=val(fn date$(0)) :' today date
k=find("date",0,d) :' find 1st
while (k >= 0) :' anything?
  a$ = get$("date",k,10) :' get it
  s$(24) = 5 :' priority 5
  s$(25) = 0 :' not complete
  s$(26) = 0 :' unfiled
  s$(27) = ""
  s$(29) = "" :' no note
  y = msgbox(a$,t$,2)
  if (y = 1)
    put "todo",new,10  :' make todo
  endif
  k=find("date",-12,d) :' find next
wend
msgbox( "done")
end
-- --	cut here 	-- --


Example 8: SERIAL PORT I/O
-- --	cut here 	-- --
# serial port I/O
# emulates a 1 line terminal
# at 19200 baud
open "com1:",19200 as #5
a$=""
k$=inkey$
while asc(k$) <> 12
# use the scroll down button to exit
if len(k$) > 0
  rem # send graffiti out serial port
  print #5,k$;
  if asc(k$) = 10
    print #5, chr$(13);
  endif
endif
# check for serial input
s = fn serial(5)
if s > 0 then
  a$ = a$ + get$(#5,s)
  rem # display input
  draw a$+"   ",10,90
  if len(a$) > 32 then a$=""
endif
k$=inkey$
wend
close #5
end
-- --	cut here 	-- --


Example 9: dynamic button
-- --	cut here 	-- --
# mybutton.bas
draw -1
form btn 60,80,40,12, "DoIt",1
form fld 60,120,80,12, "Nobody",1
draw "Enter your name",60,100
x = asc(input$(1))  :' wait for button
? "Your name is ",s$(0);"."
end
-- --	cut here 	-- --


Example 10: BENCHMARK
-- --	cut here 	-- --
# sieve.bas
# prime number sieve benchmark
# from Byte magazine 1984
new
20 t = fn 17
25 s = 8192 : rem full size
30 dim f%(s+2)
40 for i = 0 to s-1 : f%(i) = 1 : next i
50 c=0
60 for i = 0 to s-1
70  if f%(i) = 0 then 110
80  p = i+i+3 : if i+p > s then 100
90  for k = i+p to s step p : f%(k) = 0 : next k
100  c = c+1
110 next i
120 ? c ; " primes found in " ;
130 t =(fn 17)-t : t=t/100
140 ? t ; " seconds"
150 end
run
-- --	cut here 	-- --


***********************************************************************
Latest Revision notes :
***********************************************************************

-- HotPaw cbasPad Pro Basic Version 1.2.0 --	(2000May25)

    Added some HanDBase search capability.
    Added some preliminary html documentation (ybdoc.html).
    Fixed a bug in algebraic precedence.
    Fixed a problem with reading Doc file strings that cross pages.
    
-- HotPaw cbasPad Pro Basic Version 1.1.9 --	(2000May08)
    Added a database functions to check type and creator.
    Fixed a compatibility bug in the val("") function.
-- HotPaw cbasPad Pro Basic Version 1.1.8 --	(2000May02)
    Added double precision numeric input capability.
    Added a double precision number display routine (str$()).
    Added fn trim$().
    Added a method to set the open new memo category.
    Added a standard normal (statistics) bcmd extension.
    Fixed a bug in reading data from "Doc" files.
-- HotPaw cbasPad Pro Basic Version 1.1.7 --	(2000Apr14)
    Added a way to display source code at a given line number.
    Added support for programs contained in pedit32 memos.
    Added support for programs contained Doc files.
    Added the capability to read string resources.
    Fixed a problem with reading Address/Phone List records.
    Fixed a problem creating "no time" datebook appointments.
    Fixed a problem with using fn time$(-1) bug in pre 3.5 OS versions.
    Fixed a bug in using the "exit while" statement.
-- (see Versions history for older revision notes)


**********************************************************************
Credits
**********************************************************************
  
  HotPaw Basic ("the Software") is Copyright (c) 1999,2000 by
  Ronald H Nicholson, Jr.,  All Rights Reserved.

  cbasPad and HotPaw Basic are loosely based on the CalTech version
  of Chipmunk Basic 1.0, written by D Gillespie.
  
  Portions of HotPaw Basic contain code which is copyrighted by
  Palm Computing, Inc.

**********************************************************************
LICENSE
**********************************************************************

  LICENSE

  TITLE

  HotPaw Basic ("the Software") is Copyright (c) 1999,2000 by
  Ronald H Nicholson, Jr.,  All Rights Reserved.
  
  Title, ownership rights, and intellectual property rights in the
  Software shall remain with Ronald H Nicholson, Jr.  The Software
  is protected by the copyright laws and treaties of the United
  States of America.

  TERMS

  Ronald H. Nicholson, Jr. grants you the right to use this copy
  of the Software if you agree to the following license terms:
  
  You may use the Software without a license key and in Demo mode
  for educational or non-commercial purposes.
  
  You may not reverse engineer, decompile, or disassemble the
  Software.  
  
  You may transfer or copy a license key or license key database
  for the Software only if required in the normal use of one PalmOS
  handheld unit.  You may also make one copy of the Software license
  key if required for backup purposes.  You are required to keep
  all HotPaw license keys confidential.  You may not loan, rent,
  transfer or assign the license key to another user except with
  (a) the permission of HotPaw and (b) as a permanent transfer of
  the Software and the license key.
  
  Commercial distribution of the Software is not allowed without
  the express permission of the copyright holder.  Contact HotPaw
  for details about obtaining a license.
  
  Usage of the Software is also subject to the following limitations
  and disclaimers.

  WARRANTY AND DISCLAIMER

  The Software is distributed in the hope that it might be useful,
  but WITHOUT ANY WARRANTY OF ANY KIND; not even the implied
  warranties of MERCHANTABILITY, fitness for ANY particular purpose,
  or for non-infringement of any intellectual property rights.
  
  LIMITATION OF LIABILITY

  UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL THEORY, TORT, CONTRACT,
  OR OTHERWISE, SHALL HOTPAW, RONALD NICHOLSON, OR ITS SUPPLIERS OR
  RESELLERS BE LIABLE TO YOU OR ANY OTHER PERSON FOR ANY INDIRECT,
  SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES OF ANY CHARACTER
  INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF GOODWILL, WORK
  STOPPAGE, COMPUTER FAILURE OR MALFUNCTION, OR ANY AND ALL OTHER
  COMMERCIAL DAMAGES OR LOSSES. IN NO EVENT WILL HOTPAW BE LIABLE
  FOR ANY DAMAGES IN EXCESS OF THE AMOUNT HOTPAW RECEIVED FROM YOU
  FOR A LICENSE TO THIS SOFTWARE, EVEN IF HOTPAW OR RONALD NICHOLSON
  SHALL HAVE BEEN INFORMED OF THE POSSIBILITY OF SUCH DAMAGES, OR
  FOR ANY CLAIM BY ANY OTHER PARTY. THIS LIMITATION OF LIABILITY
  SHALL NOT APPLY TO LIABILITY FOR DEATH OR PERSONAL INJURY TO THE
  EXTENT APPLICABLE LAW PROHIBITS SUCH LIMITATION. FURTHERMORE,
  SOME JURISDICTIONS DO NOT ALLOW THE EXCLUSION OR LIMITATION OF
  INCIDENTAL OR CONSEQUENTIAL DAMAGES, SO THIS LIMITATION AND
  EXCLUSION MAY NOT APPLY TO YOU.
  
  IN THE EVENT THIS SOFTWARE INFRINGES UPON ANY OTHER PARTY'S
  INTELLECTUAL PROPERTY RIGHTS, THE LICENSOR'S ENTIRE LIABILITY
  AND YOUR EXCLUSIVE REMEDY SHALL BE, AT THE LICENSOR'S CHOICE,
  EITHER (A) RETURN OF THE PRICE PAID TO THE LICENSOR AND ITS
  AUTHORISED DISTRIBUTORS OR (B) REPLACEMENT OF THE SOFTWARE WITH
  NON-INFRINGING SOFTWARE.

  LIMITATION OF HIGH RISK ACTIVITIES

  The Software is not fault-tolerant and is not designed,
  manufactured or intended for use or resale as on-line control
  equipment in hazardous environments requiring fail-safe
  performance, such as in the operation of nuclear facilities,
  aircraft navigation or communication systems, air traffic control,
  direct life support machines, or weapons systems, in which the
  failure of the Software could lead directly to death, personal
  injury, or severe physical or environmental damage ("High Risk
  Activities").  Ronald Nicholson, HotPaw, and its suppliers
  SPECIFICALLY disclaim ANY express or implied warranty of fitness
  for High Risk Activities.
  
  This license is governed by the laws of the United States and
  the State of California.  If, for any reason, a court of
  competent jurisdiction finds any provision, or portion thereof,
  of this license to be unenforceable, the remainder of this
  license shall continue in full force and effect.
  
  AGREEMENT

  If you do not agree to the terms of this LICENSE, you are not
  authorized to use the Software.
  
   
***
This documentation is preliminary and subject to change at any
time without notice.
--
Palm, PalmOS and HotSync are trademarks of Palm Computing.
HotPaw and cbasPad are trademarks of the HotPaw company.

Copyright 1999,2000 Ronald H. Nicholson, Jr.   rhn@nicholson.com
***




