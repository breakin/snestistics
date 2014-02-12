import csv

asminclude = open("registers.inc", "wb")

with open('registers.csv', 'rb') as csvfile:
	rdr = csv.reader(csvfile, delimiter=';', quotechar="\"")
	for row in rdr:
		asminclude.write(".define REG_" + row[1] + " $" + row[0][2:] + "\n")