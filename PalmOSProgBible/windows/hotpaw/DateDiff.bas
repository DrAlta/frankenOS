                                                                               # DateDiff .bas
#Sunday
#Monday
#Tuesday
#Wednesday
#Thursday
#Friday
#Saturday
#
#autonum
new
draw -1
s$="                    "
c$="choose"
form btn 100,20,40,12,c$,1
form btn 100,62,40,12,c$,1
draw 4,56,156,56
draw 4,98,156,98
c1 = val(fn date$(0))
c1 = 10000 * int(c1/10000) + 101
c2 = val(fn date$(0))
while 1
x1=fn days(c1)
x2=fn days(c2)
d = abs(x1 - x2)
w = round(d/7,1)
y1$=left$(str$(c1),4)
m1$=mid$(str$(c1),5,2)
d1$=right$(str$(c1),2)
dw1=fn dayow(c1)
dw1$=get$("data",dw1+1)
dn1=x1-fn days(val(y1$+"0101"))
dn1$="day "+str$(dn1+1)+s$
wx1=fn dayow(val(y1$+"0101"))
wn2=int((wx1+dn1-1)/7)
if (wx1 < 4) then wn1 = wn1+1
wn1$="wk "+str$(wn1)+"     "
y2$=left$(str$(c2),4)
m2$=mid$(str$(c2),5,2)
d2$=right$(str$(c2),2)
dw2=fn dayow(c2)
dw2$=get$("data",dw2+1)
dn2=x2-fn days(val(y2$+"0101"))
dn2$="day "+str$(dn2+1)+s$
wx2=fn dayow(val(y2$+"0101"))
wn2=int((wx2+dn2-1)/7)
if (wx2 < 4) then wn2 = wn2+1
wn2$="wk "+str$(wn2)+"     "
#
draw y1$,10,22
draw m1$,36,22
draw d1$,51,22
draw dw1$+s$,10,38
draw dn1$,80,38
draw wn1$,132,38
draw y2$,10,64
draw m2$,36,64
draw d2$,51,64
draw dw2$+s$,10,80
draw dn2$,80,80
draw wn2$,132,80
dd$=str$(d)+"  days between       "
dw$=str$(w)+"  weeks      "
draw dd$,10,106
draw dw$,10,120
x=asc(input$(1))
if x= 14 then
 c1 = val(fn date$(-1))
else
  c2 = val(fn date$(-1))
endif
wend
end
run
