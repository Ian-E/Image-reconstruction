import io
import os
import math

#BM in decimal
CONST_HEADER_VAL = 19778

#edit as needed depending on split size
CONST_READ_VALUE = 50000

f = open(input("Enter file name"), "rb")

i = f.read(CONST_READ_VALUE)

file_num = 0
count = 0
#continue until nothing left to read
while len(i) != 0:
	#see if chunk contains header info for bmps
	check = int.from_bytes(i[:2], byteorder='little')
	start = 0
	if check == CONST_HEADER_VAL and int.from_bytes(i[6:8], byteorder='little') == 0 and int.from_bytes(i[8:10], byteorder='little') == 0:
		#get starting point for writing values later
		start = int.from_bytes(i[10:14], byteorder='little')
		
		#get width and height
		width = int.from_bytes(i[18:22], byteorder='little')
		height = int.from_bytes(i[22:26], byteorder='little')

		#info for binning later
		bits_per_pix = int.from_bytes(i[28:30], byteorder='little')
		
		#two possible places for size, check both
		size = int.from_bytes(i[2:6], byteorder='little')
		if size == 0:
			size = int.from_bytes(i[34:38], byteorder='little')
		
		#calculate padding
		padding = int((size - width*height*3)/height);
		count += 1
		#write metadata
		y = open("output/" + str(file_num) + ".csv", "w")
		y.write("BMP," + str(start) + "," + str(width) + "," + str(height) + "," + str(bits_per_pix) + "," + str(size) + "," + str(padding) +",1")

	else:
		y = open("output/" + str(file_num) + ".csv", "w")
		y.write("BMP,,,,,,,0")
	x = open("output/" + str(file_num) + ".byte", "wb")
	z = open("output/" + str(file_num) + ".bgr", "w")
	
	#write raw bytes
	x.write(i)
	
	#write brg values from start point, ignoring headers
	for j in [i[k:k+1] for k in range(start, len(i))]:
		temp = str(int.from_bytes(j, byteorder='little')) + ","
		z.write(temp.rstrip())
	x.close()
	y.close()
	z.close()
	i = f.read(CONST_READ_VALUE)
	file_num += 1
	print("chunk " + str(file_num) + " read")
print(count)
f.close()
