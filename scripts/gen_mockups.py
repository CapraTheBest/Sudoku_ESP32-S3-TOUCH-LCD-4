#!/usr/bin/env python3
# Generate SVG mockups (480x480) of the firmware screens, using the same
# palette/layout as the real UI. Output goes to docs/images/.
#
# Run:  python scripts/gen_mockups.py
# Then the README screenshots (docs/images/*.svg) are refreshed in place.
#
# These are representative vector mockups, NOT device captures. Keep the
# palette below in sync with include/ui_theme.h and the layout in src/ui.cpp.
import os

# --- palette (from include/ui_theme.h) ---
BG="#0f1115"; PANEL="#171a21"; CELL="#222732"; PEER="#2a3142"; SAME="#4a3d6b"
SEL="#3b6fe0"; GRIDC="#384055"; THICK="#5a6f9a"; INK="#e8ebf0"; USER="#6db0ff"
MUTED="#8b93a3"; NOTE="#9aa3b5"; ACCENT="#5b8cff"; ACCENT2="#23304d"
DANGER="#e5484d"; GOOD="#2ecc71"; SUN="#d83a3a"
FONT='font-family="Segoe UI, Helvetica, Arial, sans-serif"'

OUT = os.path.join(os.path.dirname(__file__), "..", "docs", "images")
os.makedirs(OUT, exist_ok=True)

def hdr():
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="480" height="480" '
            'viewBox="0 0 480 480">\n'
            f'<rect width="480" height="480" rx="22" fill="{BG}"/>\n')

def rect(x,y,w,h,fill,r=0,stroke=None,sw=0,opa=None):
    s=f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ''
    o=f' opacity="{opa}"' if opa is not None else ''
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}"{s}{o}/>\n'

def text(x,y,s,fill,size,weight="400",anchor="middle"):
    return (f'<text x="{x}" y="{y}" fill="{fill}" font-size="{size}" '
            f'font-weight="{weight}" text-anchor="{anchor}" '
            f'dominant-baseline="central" {FONT}>{s}</text>\n')

def circle(cx,cy,r,fill):
    return f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}"/>\n'

def save(name, body):
    with open(os.path.join(OUT, name), "w", encoding="utf-8") as f:
        f.write(hdr()+body+"</svg>\n")
    print("wrote", name)

# --- icons (simple vector glyphs standing in for the LVGL symbols) ---
def icon_pause(cx,cy):
    return (rect(cx-6,cy-10,4,20,INK,1)+rect(cx+2,cy-10,4,20,INK,1))

def icon_pencil(cx,cy):
    # small pencil at 45deg: body + tip + eraser end
    return (f'<g transform="translate({cx},{cy}) rotate(45)">'
            f'<rect x="-3" y="-9" width="6" height="14" rx="1" fill="{INK}"/>'
            f'<polygon points="-3,5 3,5 0,10" fill="{INK}"/>'
            f'<rect x="-3" y="-11" width="6" height="3" fill="{ACCENT}"/>'
            f'</g>\n')

def icon_eraser(cx,cy):
    # tilted eraser block (mdi-eraser stand-in)
    return (f'<g transform="translate({cx},{cy}) rotate(-30)">'
            f'<rect x="-11" y="-7" width="22" height="14" rx="3" fill="#d9657a"/>'
            f'<rect x="-11" y="-7" width="8" height="14" rx="3" fill="{INK}" opacity="0.35"/>'
            f'</g>\n')

# --- sample puzzle ---
PUZZLE=[5,3,0,0,7,0,0,0,0, 6,0,0,1,9,5,0,0,0, 0,9,8,0,0,0,0,6,0,
        8,0,0,0,6,0,0,0,3, 4,0,0,8,0,3,0,0,1, 7,0,0,0,2,0,0,0,6,
        0,6,0,0,0,0,2,8,0, 0,0,0,4,1,9,0,0,5, 0,0,0,0,8,0,0,7,9]
USER={2:4, 20:5, 24:7}            # user-entered digits (blue)
NOTES={11:[1,2,4], 60:[3,6,9]}    # pencil-mark candidates per cell
SELECTED=2                        # selected cell (has a user digit -> shows "same")

CELLSZ=38; GAP=4
def cx(c): return c*CELLSZ + (c//3)*GAP
def cy(r): return r*CELLSZ + (r//3)*GAP

def topbar(timer="04:12"):
    b=rect(0,0,480,50,PANEL)
    b+=text(16,25,timer,INK,26,"600","start")
    # pencil (notes) toggle - shown active (accent bg)
    b+=rect(294,7,46,36,ACCENT,8)+icon_pencil(317,25)
    # pause
    b+=rect(346,7,46,36,ACCENT2,8)+icon_pause(369,25)
    # MENU
    b+=rect(398,7,74,36,ACCENT2,8)+text(435,25,"MENU",INK,16,"600")
    return b

def grid(selected=SELECTED, show_user=True, show_notes=True):
    gx,gy=65,56; gw=cx(8)+CELLSZ
    g=rect(gx,gy,gw,gw,THICK,3)
    sr,sc=(selected//9,selected%9) if selected is not None else (-1,-1)
    selval=PUZZLE[selected] or USER.get(selected) if selected is not None else None
    for i in range(81):
        r,c=i//9,i%9
        x=gx+cx(c)+1; y=gy+cy(r)+1
        val=PUZZLE[i] or (USER.get(i) if show_user else 0)
        fill=CELL
        if selected is not None and (r==sr or c==sc or (r//3==sr//3 and c//3==sc//3)):
            fill=PEER
        if selval and val==selval:           # same number as selection
            fill=SAME
        if i==selected: fill=SEL
        g+=rect(x,y,CELLSZ-2,CELLSZ-2,fill,2)
        if val:
            col=INK if PUZZLE[i] else USER; w="600" if PUZZLE[i] else "500"
            g+=text(x+(CELLSZ-2)/2, y+(CELLSZ-2)/2, str(val), col, 22, w)
        elif show_notes and i in NOTES:
            # 3x3 mini candidate grid
            for d in NOTES[i]:
                rr=(d-1)//3; cc=(d-1)%3
                nx=x+6+cc*((CELLSZ-2-12)/2); ny=y+6+rr*((CELLSZ-2-12)/2)
                g+=text(nx, ny, str(d), NOTE, 10, "400")
    return g

def numpad():
    n=10; bw=44; gap=4; total=n*bw+(n-1)*gap; sx=(480-total)//2; y=56+(cx(8)+CELLSZ)+10
    p=""
    for k in range(n):
        x=sx+k*(bw+gap)
        p+=rect(x,y,bw,58,ACCENT2,10)
        if k<9:
            p+=text(x+bw/2, y+29, str(k+1), INK, 24, "600")
        else:
            p+=icon_eraser(x+bw/2, y+29)
    return p

# ---------- GAME ----------
save("game.svg", topbar()+grid()+numpad())

# ---------- MENU ----------
def menu():
    m=text(240,90,"SUDOKU",INK,46,"700")
    items=[("Easy",130),("Medium",196),("Hard",262)]
    for label,y in items:
        m+=rect(90,y,300,56,ACCENT2,10)+text(240,y+28,label,INK,26,"600")
    m+=rect(90,332,300,50,ACCENT,10)+text(240,357,"Resume game",INK,22,"600")
    m+=text(240,452,"Best    E --:--     M 03:21     H --:--",MUTED,16)
    return m
save("menu.svg", menu())

# ---------- SPLASH / LANGUAGE ----------
def splash():
    s=circle(240,124,90,SUN)                               # rising-sun red circle
    s+=text(240,124,"数独",INK,56,"700")                   # kanji (rendered if font present)
    s+=text(240,256,"SUDOKU",INK,46,"700")
    s+=rect(100,300,280,54,ACCENT2,10)+text(240,327,"English",INK,26,"600")
    s+=rect(100,366,280,54,ACCENT2,10)+text(240,393,"Italiano",INK,26,"600")
    return s
save("splash.svg", splash())

# ---------- PAUSE ----------
def pause():
    p=f'<g opacity="0.12">{grid(selected=None, show_user=True, show_notes=True)}</g>'
    p+=rect(0,0,480,480,BG,22,opa=0.80)
    p+=text(240,210,"Paused",INK,46,"700")
    p+=text(240,270,"tap to resume",MUTED,18)
    return p
save("pause.svg", pause())

# ---------- WIN ----------
def win():
    w=text(240,130,"You won!",GOOD,46,"700")
    w+=text(240,235,"Medium  -  03:21",INK,28,"600")
    w+=text(240,300,"New record!",GOOD,24,"600")
    w+=rect(130,392,220,56,ACCENT,10)+text(240,420,"Menu",INK,28,"600")
    return w
save("win.svg", win())

print("done")
