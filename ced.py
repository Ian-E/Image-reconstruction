import io
import os
import math
import multiprocessing

def concat(x):
	temp = ""
	for i in x:
		temp = temp + i
	return temp

def f(path, outputPath, filing, frag):
		print(filing)
		f = open(path + "/" + filing)
		name = []
		width = int((os.fsdecode(filing).split("_"))[1])
		output = open(outputPath[:-1] + "/" + filing +  ".csv", "w")
		name = []
		lis = [line.split(",") for line in f]
		for i in lis:
			name.append(frag + str(concat(list(filter(str.isdigit, i[0]))))+ ".bgr")
		width_bgr = width * 3
		#loops through every permutation
		for k in range(0, len(name)):
			for j in range(0, len(name)):
				if k == j:
					continue
				file1 = open(name[k], "r")
				file2 = open(name[j], "r")
				rgb1 = file1.readline()
				rgb2 = file2.readline()
				rgblist1 = rgb1.split(",")
				rgblist2 = rgb2.split(",")
				file1.close()
				file2.close()

				if(rgblist1[-1] == ""):
					del rgblist1[-1]
				if(rgblist2[-1] == ""):
					del rgblist2[-1]

				# Taken from f1
				last_row = rgblist1[len(rgblist1)-width_bgr: ]
				second_last_row = rgblist1[len(rgblist1)-(width_bgr*2) : len(rgblist1)-width_bgr]

				#Taken from f2
				first_row = rgblist2[:width_bgr]

				# Calculating ED-nearby
				sum = 0
				for i in range(0, width_bgr):
					sum = sum + (float(last_row[i]) - float(second_last_row[i]))**2
				ed_nearby = math.sqrt(sum) / width

				# Calculating ED-boundary
				sum = 0
				for i in range(0, width_bgr):
					sum = sum + (float(last_row[i]) - float(first_row[i]))**2
				ed_boundary = math.sqrt(sum) / width

				ced = abs(ed_boundary - ed_nearby)
				name1 = name[k].split("/")
				name2 = name[j].split("/")
				output.write(name1[-1] + "," + name2[-1]+ "," + str(ced) + "\n")
		output.close()

def ced(path, outputPath, frag):
	pool = multiprocessing.Pool()
	a =  os.listdir(path)
	a.sort(reverse=True)
	for filing in a:
		pool.apply_async(f, args = (path, outputPath, filing, frag))
	pool.close()
	pool.join()
#main
binPath = input("Enter bins path")
outputPath = input("Enter where u want them to go")
frag = input("enter fragment path")
ced(binPath, outputPath, frag)
print("done")
