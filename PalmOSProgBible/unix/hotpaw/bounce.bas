bounce .bas
new
s = 6
x=50 : y=50
draw x,y, s,s,7
dx=1.141 : dy=1.56
xb=0 : xu=161-s
yb=15 : yu=147-s
i=0
while 1
ox=x : oy=y
x=x+dx : y=y+dy
if x<xb then dx=abs(dx): x=x+2*dx
if x>xu then dx=-abs(dx): x=x+2*dx
if y<yb then dy=abs(dy): y=y+2*dy
if y>yu then dy=-abs(dy): y=y+2*dy
draw ox,oy,s,s,-7
draw x,y, s,s,7
#draw str$(x)+"    ",80,150
i=i+1
if (i>400)
  draw "BOUNCE",30+rnd(100),20+rnd(120),1
  i = 0
endif
wend
end
run
