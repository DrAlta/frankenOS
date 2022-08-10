#Doc2Memo.bas
#
#Here's an example program for copying one record from a Doc file to a
#Memo. It requires HotPaw Basic 1.1.5 or later. This program should make
#it easier to move programs onto a Palm handheld or into the POSE
#emulator.
#
#The "r=1" statement selects the first record of a Doc file. Only the
#first record is used so as not to exceed the MemoPad's 4k limit. I'll
#leave it as an "exercize for the student" to copy a Doc file to
#multiple memos without breaking up any lines that cross record
#boundries.
#
# rhn@hotpaw.com
#
input "Doc title ? ",f$
d=db.find(f$) : if d<0 then stop
r = 1 : rem record 1
k = 0 : rem offset
open new "memo",f$ as #4
while not eof
a$=db$(d,r,k)
n=len(a$)
k=k+n
b$=left$(a$,n-1) : rem strip CR
print #4,b$
wend
close #4
end
#
