# loan payment calc.bas
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
 a$=str$(a,6,2)
 s$(10)="Payment"
 s$(11)="$"+a$
wend
end
