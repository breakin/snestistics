import csv

snestisticsgen = open("../source/registergen.h", "wb")

snestisticsgen.write("void fillHardwareAdresses(HardwareAdressInfo &info) {\n")

with open('registers.csv', 'rb') as csvfile:
	rdr = csv.reader(csvfile, delimiter=';', quotechar="\"")
	for row in rdr:
		snestisticsgen.write("\tinfo[Pointer(0x" + row[0][2:]+")] = HardwareAdressEntry(\""+row[2]+"\", \""+row[1]+"\");\n")
		
snestisticsgen.write("}\n")