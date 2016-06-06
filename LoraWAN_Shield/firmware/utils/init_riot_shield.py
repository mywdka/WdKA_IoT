#!/usr/bin/env python2

import serial
import sys
import time

channels = [ {'ch': 0, 'f':0, 'dcycle': 302, 'drrange':(0,5), 'status': 'on'},
             {'ch': 1, 'f':0, 'dcycle': 302, 'drrange':(0,5), 'status': 'on'},
             {'ch': 2, 'f':0, 'dcycle': 302, 'drrange':(0,5), 'status': 'on'},
             {'ch': 3, 'f':868850000, 'dcycle':   0, 'drrange':(0,5), 'status': 'on'},
             {'ch': 4, 'f':869050000, 'dcycle':   0, 'drrange':(0,5), 'status': 'on'},
             {'ch': 5, 'f':869525000, 'dcycle':   0, 'drrange':(0,5), 'status': 'on'},
             {'ch': 6, 'f':867100000, 'dcycle':   0, 'drrange':(0,5), 'status': 'on'},
             {'ch': 7, 'f':867300000, 'dcycle':   0, 'drrange':(0,5), 'status': 'on'},
             {'ch': 8, 'f':868300000, 'dcycle':   0, 'drrange':(6,6), 'status': 'on'} ]

def set_freq_err(chan, freq):
    print 'error setting frequency %d for chan %d' % (freq, chan)
    sys.exit(1)

def write(s, cmd):
    time.sleep(0.1)
    print 'writing: ' + cmd
    s.write(cmd)

def read(s):
    ret = s.readline().rstrip()
    print 'got: ' + ret + ' ( ' + ':'.join(['%0X' % ord(b) for b in ret]) + ')'
    return ret

if __name__ == '__main__':

    try:
        DEVADDR = sys.argv[2] if (len(sys.argv) >= 3) else ""
        NWKSKEY = sys.argv[3] if (len(sys.argv) >= 4) else ""
        APPSKEY = sys.argv[4] if (len(sys.argv) >= 5) else ""
    except Exception as e:
        print str(e)

    s = serial.Serial(port=sys.argv[1], baudrate=57600)
    s.flushInput()

    write(s, 'sys reset\r\n')
    read(s)

    write(s, 'mac get deveui\r\n')
    DEVEUI = read(s)

    if DEVEUI == 'err' or DEVEUI == 'invalid_param':
        print 'could not read deveui'
        sys.exit(1)
    else:
        print 'DEVEUI: ' + DEVEUI

    for ch in channels:
	num = ch['ch']
	f = ch['f']
	drr = ch['drrange']
	dc = ch['dcycle']
	status = ch['status']

	print 'setting channel: ' + str(ch)
	if f != 0:
            write(s, 'mac set ch freq %d %d\r\n' % (num, f))
            if read(s) != 'ok': set_freq_err(num, f)
        write(s, 'mac set ch dcycle %d %d\r\n' % (num, dc))
        if read(s) != 'ok': set_freq_err(num, f)
        write(s, 'mac set ch drrange %d %d %d\r\n' % (num, drr[0], drr[1]))
        if read(s) != 'ok': set_freq_err(num, f)
        write(s, 'mac set ch status %d %s\r\n' % (num, status))
        if read(s) != 'ok': set_freq_err(num, f)
    
    write(s, 'mac set rx2 0 869525000\r\n')
    if read(s) != 'ok': print 'Error setting rx2'
    
    write(s, 'mac set rxdelay1 1000\r\n')
    if read(s) != 'ok': print 'Error setting rxdelay1'

    write(s, 'mac set retx 3\r\n')
    if read(s) != 'ok': print 'Error setting retx'
	
    #write(s, 'mac set adr on\r\n')
    #if read(s) != 'ok': print 'Error setting adr'

    write(s, 'mac set dr 6\r\n')
    if read(s) != 'ok': print 'Error setting dr'

    if DEVADDR != "":
        write(s, 'mac set devaddr ' + DEVADDR + '\r\n')
        if read(s) != 'ok': print 'Error setting DEVADDR'

    if NWKSKEY != "":
        write(s, 'mac set nwkskey ' + NWKSKEY + '\r\n')
        if read(s) != 'ok': print 'Error setting NWKSKEY'

    if APPSKEY != "":
        write(s, 'mac set appskey ' + APPSKEY + '\r\n')
        if read(s) != 'ok': print 'Error setting APPSKEY'

    #write(s, 'mac set pwridx 1\r\n')
    #if read(s) != 'ok': print 'Error setting pwridx'
	
    write(s, 'mac save\r\n')
    if read(s) != 'ok': print 'Error saving mac data'

    s.close()
