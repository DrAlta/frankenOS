#tip-calc.bas
#example program for HotPaw Basic
draw -1
form btn 120,40,32,12,"Dull",1
form btn 120,60,32,12,"OK",1
form btn 120,80,32,12,"Great",1
form fld 40,20,40,12,"12.00",1
form fld 70,40,40,12,"10",1
form fld 70,60,40,12,"15",1
form fld 70,80,40,12,"20",1
while 1
draw "Bill",10,20
a$ = input$(1)
k=asc(a$)-13
if k=1 then x = s$(1)
if k=2 then x = s$(2)
if k=3 then x = s$(3)
b=val(s$(0))
t1=b*(x/100)
t2=b+t1
tip$="$"+str$(t1,4,2)
tot$="$"+str$(t2,4,2)
draw "Tip",10,100
draw tip$,40,100
draw "Total",10,120
draw tot$,40,120
wend
run
