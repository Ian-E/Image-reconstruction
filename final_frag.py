import io
import os
import math
import random

size = 50000
f = open("image", "wb")

path = "/home/ian/flower/jpg/"
files = os.fsencode(path)
name = []
images = []

for file in os.listdir(files):
	name.append(path + os.fsdecode(file))
for n in name:
	x = open(n, "rb")
	z = x.read(size)
	while len(z) == size:
		images.append(z)
		z = x.read(size)
	if len(z) != 0:
		zero = bytes(size - len(z))
		images.append(z + zero)
	x.close()
#random.shuffle(images)
for i in images:
	f.write(i)
f.close()
print("Images split into " + str(size) + " bytes.")
