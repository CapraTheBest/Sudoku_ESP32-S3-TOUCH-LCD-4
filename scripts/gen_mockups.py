#!/usr/bin/env python3
# Genera mockup SVG (480x480) delle schermate del firmware, con la stessa
# palette/layout della UI reale. Output in docs/images/.
import os

# --- palette (da include/ui_theme.h) ---
BG="#0f1115"; PANEL="#171a21"; CELL="#222732"; PEER="#2a3550"; SAME="#33406a"
SEL="#5b8cff"; GRIDC="#384055"; THICK="#5a6f9a"; INK="#e8ebf0"; USER="#6db0ff"
MUTED="#8b93a3"; ACCENT="#5b8cff"; ACCENT2="#23304d"; DANGER="#e5484d"; GOOD="#2ecc71"
FONT='font-family="Segoe UI, Helvetica, Arial, sans-serif"'

OUT = os.path.join(os.path.dirname(__file__), "..", "docs", "images")
os.makedirs(OUT, exist_ok=True)

def hdr():
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="480" height="480" '
            'viewBox="0 0 480 480">\n'
            f'<rect width="480" height="480" rx="22" fill="{BG}"/>\n')

def rect(x,y,w,h,fill,r=0,stroke=None,sw=0):
    s=f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ''
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}"{s}/>\n'

def text(x,y,s,fill,size,weight="400",anchor="middle"):
    return (f'<text x="{x}" y="{y}" fill="{fill}" font-size="{size}" '
            f'font-weight="{weight}" text-anchor="{anchor}" '
            f'dominant-baseline="central" {FONT}>{s}</text>\n')

def save(name, body):
    with open(os.path.join(OUT, name), "w", encoding="utf-8") as f:
        f.write(hdr()+body+"</svg>\n")
    print("wrote", name)

# --- puzzle di esempio ---
PUZZLE=[5,3,0,0,7,0,0,0,0, 6,0,0,1,9,5,0,0,0, 0,9,8,0,0,0,0,6,0,
        8,0,0,0,6,0,0,0,3, 4,0,0,8,0,3,0,0,1, 7,0,0,0,2,0,0,0,6,
        0,6,0,0,0,0,2,8,0, 0,0,0,4,1,9,0,0,5, 0,0,0,0,8,0,0,7,9]
SOL   =[5,3,4,6,7,8,9,1,2, 6,7,2,1,9,5,3,4,8, 1,9,8,3,4,2,5,6,7,
        8,5,9,7,6,1,4,2,3, 4,2,6,8,5,3,7,9,1, 7,1,3,9,2,4,8,5,6,
        9,6,1,5,3,7,2,8,4, 2,8,7,4,1,9,6,3,5, 3,4,5,2,8,6,1,7,9]
USER={2:4, 6:9, 24:5}      # numeri inseriti dall'utente (blu)
SELECTED=3                  # cella selezionata (vuota)

CELLSZ=38; GAP=4
def cx(c): return c*CELLSZ + (c//3)*GAP
def cy(r): return r*CELLSZ + (r//3)*GAP

def topbar(timer="04:12"):
    b=rect(0,0,480,50,PANEL)
    b+=text(16,26,timer,INK,26,"600","start")
    # bottoni pausa e nuova
    b+=rect(376,7,46,36,ACCENT2,8)
    b+=rect(399-5,15,4,20,INK)+rect(399+3,15,4,20,INK)     # icona pausa
    b+=rect(428,7,46,36,ACCENT2,8)
    b+=text(451,25,"+",INK,28,"600")                        # icona nuova
    return b

def grid(selected=SELECTED, show_user=True, conflicts=None):
    gx,gy=65,56; gw=cx(8)+CELLSZ
    g=rect(gx,gy,gw,gw,THICK,3)
    sr,sc=(selected//9,selected%9) if selected is not None else (-1,-1)
    for i in range(81):
        r,c=i//9,i%9
        x=gx+cx(c)+1; y=gy+cy(r)+1
        fill=CELL
        if selected is not None and (r==sr or c==sc or (r//3==sr//3 and c//3==sc//3)):
            fill=PEER
        if i==selected: fill=SEL
        if conflicts and i in conflicts: fill=DANGER
        g+=rect(x,y,CELLSZ-2,CELLSZ-2,fill,2)
        val=PUZZLE[i]; col=INK; w="600"
        if val==0 and show_user and i in USER:
            val=USER[i]; col=USER; w="500"
        if val:
            g+=text(x+(CELLSZ-2)/2, y+(CELLSZ-2)/2, str(val), col, 22, w)
    return g

def numpad():
    n=10; bw=44; gap=4; total=n*bw+(n-1)*gap; sx=(480-total)//2; y=56+(cx(8)+CELLSZ)+10
    p=""
    labels=["1","2","3","4","5","6","7","8","9","&#9003;"]
    for k in range(n):
        x=sx+k*(bw+gap)
        p+=rect(x,y,bw,58,ACCENT2,10)
        p+=text(x+bw/2, y+29, labels[k], INK if k<9 else MUTED, 24 if k<9 else 20, "600")
    return p

# ---------- GIOCO ----------
save("game.svg", topbar()+grid()+numpad())

# ---------- MENU ----------
def menu():
    m=text(240,90,"SUDOKU",INK,46,"700")
    items=[("Facile",130),("Medio",196),("Difficile",262)]
    for label,y in items:
        m+=rect(90,y,300,56,ACCENT2,10)+text(240,y+28,label,INK,26,"600")
    m+=rect(90,332,300,50,ACCENT,10)+text(240,357,"Riprendi partita",INK,22,"600")
    m+=text(240,452,"Record    F --:--     M 03:21     D --:--",MUTED,16)
    return m
save("menu.svg", menu())

# ---------- PAUSA ----------
def pause():
    # griglia di gioco attenuata dietro + velo scuro + testo
    p=f'<g opacity="0.12">{grid(selected=None, show_user=True)}</g>'
    p+=f'<rect width="480" height="480" rx="22" fill="{BG}" opacity="0.80"/>\n'
    p+=text(240,210,"In pausa",INK,46,"700")
    p+=text(240,270,"tocca per riprendere",MUTED,18)
    return p
save("pause.svg", pause())

# ---------- VITTORIA ----------
def win():
    w=text(240,130,"Hai vinto!",GOOD,46,"700")
    w+=text(240,235,"Medio  -  03:21",INK,28,"600")
    w+=text(240,300,"Nuovo record!",GOOD,24,"600")
    w+=rect(130,392,220,56,ACCENT,10)+text(240,420,"Menu",INK,28,"600")
    return w
save("win.svg", win())

print("done")
