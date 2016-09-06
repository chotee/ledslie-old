import time
import serial
import datetime

ser = serial.Serial('/dev/ttyUSB0', baudrate=2400, rtscts=False, dsrdtr=False, xonxoff=False,
                    stopbits=serial.STOPBITS_ONE)

time.sleep(1)

ser.write(bytes('"12\r', 'utf-8'))

# ser.write(bytes(, 'utf-8'))
def w(ser, text, start_delay, char_delay):
     ser.write(bytes('"',encoding="ascii"))
     time.sleep(start_delay/1000.0)
     data = bytearray(text, 'utf-8')
     #for c in data:
#         print(c)
     ser.write(data)
     #    time.sleep(char_delay/1000.0)
     ser.write(bytes('\r', encoding="ascii"))

#text = "-"*48
#text = "0123456789"*5
#text = text + text[::-1]
while True:
    now = datetime.datetime.now().isoformat()[:24]
    w(ser, now, 20, 0)
    time.sleep(100/1000.0)
time.sleep(1)
ser.close()
