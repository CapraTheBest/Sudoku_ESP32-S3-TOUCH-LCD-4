#!/usr/bin/env python3
# Cattura bounded del monitor seriale: apre la porta, prova un reset
# (toggle DTR/RTS), legge per N secondi e stampa, poi esce.
import sys, time
import serial

port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 18.0

ser = serial.Serial()
ser.port = port
ser.baudrate = 115200
ser.timeout = 0.2
# Non far asserire DTR/RTS all'apertura (evita reset indesiderati su alcuni bridge)
ser.dtr = False
ser.rts = False
ser.open()

# Tentativo di reset classico (EN via RTS). Su USB nativo puo' non avere effetto.
try:
    ser.setDTR(False); ser.setRTS(True);  time.sleep(0.1)
    ser.setRTS(False); time.sleep(0.05)
except Exception as e:
    print("[sermon] reset toggle non supportato:", e)

end = time.time() + secs
buf = b""
while time.time() < end:
    data = ser.read(256)
    if data:
        buf += data
        try:
            sys.stdout.write(data.decode("utf-8", "replace"))
            sys.stdout.flush()
        except Exception:
            pass
ser.close()
print("\n[sermon] fine cattura (%.0fs), %d byte" % (secs, len(buf)))
