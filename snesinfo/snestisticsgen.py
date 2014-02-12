import csv

snestisticsgen = open("../examples/hardware_reg.labels", "wb")

with open('registers.csv', 'rb') as csvfile:
	rdr = csv.reader(csvfile, delimiter=';', quotechar="\"")
	for row in rdr:
		snestisticsgen.write("# " + row[2] + "\n")
		snestisticsgen.write("data 00" + row[0][2:] + " 00" + row[0][2:] + " REG_" + row[1]+ " breakin\n")
	
snestisticsgen.write("}\n")