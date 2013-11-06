# Arduino emulator ...
import random
import serial
import time

ser = serial.Serial('/dev/ptyp2', 9600, timeout=2)
flag = 1

while (flag == 1) :
	inputBuf = ser.readline()
	if(inputBuf == "") :
		print "."
	else :
		inputBuf = inputBuf[:-1]
		print "Receive >%s< \n" % (inputBuf)	
	if(inputBuf == "?") :
		ser.write("HELP : (D)ump, (L)ist, (V)alue, (Z)ero clinometer, (C)ompensate compass, (A)ltitude set, (G)eo north correction\n")
				
	elif(inputBuf == "Z") :
		ser.write("Clinometer offset setted!\n")
		
	elif(inputBuf == "C") :
		ser.write("Start magnetometer compensation...\n")
		time.sleep(20) 	
		ser.write("... magnetometer compensation DONE !\n")
			
	elif(inputBuf[:1] == "A") :
		alt = inputBuf[1:] 
		ser.write("Set base quota for altimeter at (m) :")
		ser.write(alt)
		ser.write("\n")
			
	elif(inputBuf[:1] == "G") :
		com = inputBuf[1:] 
		ser.write("Set compass correction (o) :")
		ser.write(com)
		ser.write("\n")

	elif(inputBuf == "L") :
		ser.write("-----------------------------------\n")
		ser.write("Clock\tTime stamp\tmsec\n")
		ser.write("Temperature Device ID=1\ttemperature\tCelsius dgr\n")
		ser.write("Temperature Device ID=2\ttemperature\tCelsius dgr\n")
		ser.write("DHT Device\tHumidity\t%\n")
		ser.write("DHT Device\tTemperature\tCelsius dgr\n")
		ser.write("DHT Device\tDew Point\tCelsius dgr\n")
		ser.write("DHT Device\tDew PointFast\tCelsius dgr\n")
		ser.write("Barometer\tPressure\thPas\n")
		ser.write("Barometer\tPressure sl\thPas\n")
		ser.write("Barometer\tAltitude sl\tm\n")
		ser.write("Barometer\tTemperature\tCelsius dgr\n")
		ser.write("Compass\tMagneting heading\tdgr\n")
		ser.write("Compass\tField intensity\tGauss\n")
		ser.write("Compass\tDeclination\tdgr\n")
		ser.write("Compass\tInclination\tdgr\n")
		ser.write("Compass\tGeografic heading\tdgr\n")
		ser.write("Clinometer\tRoll\tdgr\n")
		ser.write("Clinometer\tPitch\tdgr\n")

	elif(inputBuf == "D") :
		ser.write("Not yet implemented\n")

	elif(inputBuf == "V") :
		ser.write('%d' % (int(time.time())) )
		ser.write(",")
		ser.write(  ('%6.4f' % (random.random() * 50.0 - 10.0 )) )
		ser.write(",")
		ser.write(  ('%6.4f' % (random.random() * 50.0 - 10.0 )) )
		ser.write(",")
		ser.write(  ('%6.4f' % (random.random() * 100.0 )) )
		ser.write(",")
		ser.write(  ('%6.4f' % (random.random() * 50.0 - 10.0 )) )
		ser.write(",")
		ser.write(  ('%6.4f' % (random.random() * 100.0 )) )
		ser.write(",")
		ser.write(  ('%6.4f' % (random.random() * 100.0 )) )
		ser.write(",")
		ser.write(  ('%6.1f' % (random.random() * 1200.0 )) )
		ser.write(",")
		ser.write(  ('%6.1f' % (random.random() * 1200.0 )) )
		ser.write(",")
		ser.write(  ('%6.1f' % (random.random() * 2500.0 - 100.0 )) )
		ser.write(",")
		ser.write(  ('%6.4f' % (random.random() * 50.0 - 10.0 )) )
		ser.write(",")

		ser.write(  ('%4.1f' % (random.random() * 360.0 )) )
		ser.write(",")
		ser.write(  ('%6.4f' % (random.random() * 3000.0 )) )
		ser.write(",")
		ser.write(  ('%4.1f' % (random.random() * 360.0 )) )
		ser.write(",")
		ser.write(  ('%4.1f' % (random.random() * 40.0 - 20.0 )) )
		ser.write(",")
		ser.write(  ('%4.1f' % (random.random() * 360.0 )) )
		ser.write(",")
		ser.write(  ('%4.1f' % (random.random() * 90.0 - 45.0 )) )
		ser.write(",")
		ser.write(  ('%4.1f' % (random.random() * 90.0 - 45.0 )) )
		ser.write( ",TAPPO" )
		ser.write("\n")

	elif(inputBuf[:1] == "X") :
		ind = inputBuf[1:] 
		ser.write("Sensor :")
		ser.write(ind)
		ser.write("\n")
	
	elif(inputBuf != "") :
		ser.write("Sorry !\n")
    	
					
	time.sleep(1)
		

ser.close()
	
# --------------


