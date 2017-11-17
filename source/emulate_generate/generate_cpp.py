import json

with open('optab.json') as data_file:    
    ops = json.load(data_file)

out = open("optables.h", "wt")

sizes = set([])
sizes.add("WIDE")
mnemnonics = set([])
source_adressing_modes = set([])
for o in ops:
	sm = o[3]
	source_adressing_modes.add(sm)
	mnemnonics.add(o[2])
	sizes.add(o[4])

intend_0 = ""
intend = "\t"

def write_enum(name, array):
	s = sorted(array)
	count = 0
	out.write(intend_0 + "enum class " + name + " {\n")
	for a in s:
		out.write(intend_0 + intend + a + " = " + str(count) +",\n")
		count+=1
	out.write(intend_0 + "};\n\n")

	out.write(intend_0 + "static const char * const " + name + "_names[" + str(len(s)) + "]={\n")
	for a in s:
		out.write(intend_0 + intend + "\"" + a + "\",\n")
	out.write(intend_0 + "};\n\n")	

write_enum("Operation", mnemnonics)
write_enum("InstructionSize", sizes)
write_enum("Operand", source_adressing_modes)

out.write(intend_0 + "struct OpCode {\n")
out.write(intend_0 + intend + "Operation op;\n")
out.write(intend_0 + intend + "InstructionSize size;\n")
out.write(intend_0 + intend + "Operand mode;\n")
out.write(intend_0 + intend + "bool load_operand;\n")
out.write(intend_0 + "};\n")
out.write("\n")

out.write(intend_0 + "static const char * const mnemonic_names[]={ // as used in an assembler\n")
for a in range(256):
	m = ops[a][1]
	out.write(intend_0 + intend + "\"" + m + "\",\n")
out.write(intend_0 + "};\n\n")

out.write(intend_0 + "static const OpCode op_codes[256]={\n")
for o in ops:
	load_operand = "true"
	b = o[2]
	if b=="JSR" or b=="JSL" or b=="JMP" or b == "JML": load_operand = "false"
	if b=="STA" or b=="STZ" or b == "STX" or b == "STY": load_operand = "false"
	if b[0]=="B": load_operand = "false"
	if b=="BIT": load_operand = "true"
	if o[3]=="ACCUMULATOR": load_operand = "false"
	if o[3].startswith("IMMEDIATE"): load_operand = "false"
	if o[3] == "MANUAL": load_operand = "false"
	a = ("{{Operation::{0:<3} InstructionSize::{2:<7} Operand::{1:<38} {5:<5}}}, // {3} {4}".format(o[2]+",", o[3]+",", o[4]+",", o[0], o[1], load_operand))
	out.write(intend_0 + intend + a + "\n")
out.write(intend_0 + "};\n\n")
out.close()