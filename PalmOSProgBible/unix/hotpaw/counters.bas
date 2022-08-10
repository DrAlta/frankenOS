#counters.bas
#  example program for HotPaw Basic
#    for version 0.99b60 or later
#  by Ron Nicholson, 2000-Jan-06
#
f$="count-listdb-4"
draw -1
form btn 120,40,32,12,"A",1
form btn 120,60,32,12,"B",1
form btn 120,80,32,12,"C",1
form btn 120,100,32,12,"D",1
form btn 120,120,32,12,"clear",1
dim c(4)
gosub listinit()
for i=1 to 4
  a$=str$(c(i),5)
  draw a$,70,40+(i-1)*20
next i
while 1
n = asc(input$(1))
# handle up/down keys!
if n=11 then n=14
if n=12 then n=17
n=n-13
if n < 5
  c(n)=c(n)+1
  gosub myupdate(n, c(n))
endif
if n = 5
  for i=1 to 4
    c(i)=0
    gosub myupdate(i, c(i))
  next i
endif
wend
end
#load counts from database
sub listinit()
n = db.find(f$)
if (n < 0) then gosub newdb()
for i=1 to 4
  s$(i)=db$(f$,i,1)
  c(i)=val(db$(f$,i,2))
  draw s$(i),10,40+(i-1)*20
next i
return
# update counter
sub myupdate(n, c)
  a$=str$(c,5)
  draw a$,70,40+(n-1)*20
  db$(f$,n,2)=str$(c)
return
#create new database
sub newdb()
  mydb=open f$ as new "LSdb",3
  if mydb < 0 then end
  for i=1 to 4
    s$(20)="counter"+str$(i)
    s$(21)="0" : s$(22)=""
    put mydb,i,103
  next i
return
run

