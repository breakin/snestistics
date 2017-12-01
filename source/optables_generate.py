import json
import os

class CommaList(object):
	def __init__(self, f, newLine = True, comma = True, indent = "", newLineFreq = 1):
		self.hang = False
		self.f = f
		self.indent = indent
		self.emitted = 0
		self.newLineFreq = newLineFreq

		self.comma = comma
		self.newLine = newLine

		if newLine and comma: self.item_break = ",\n"
		elif newLine: self.item_break = "\n"
		elif comma: self.item_break = ", "
		if newLine: self.finalizer = "\n"
	def write(self, line):
		if self.hang:
			self.emitted = self.emitted  + 1
			emitNewLine = False
			if self.newLine and self.emitted == self.newLineFreq:
				emitNewLine = True
				self.emitted = 0

			if self.comma:
				self.f.write(",")
			if emitNewLine:
				self.f.write("\n")
			elif self.comma:
				self.f.write(" ")
		self.f.write(self.indent + line)
		self.hang = True
	def finish(self):
		if self.newLine:
			self.f.write(self.finalizer)

dirname, filename = os.path.split(os.path.abspath(__file__))
absolute_json_file_name = dirname + "/optables.json"
absolute_output_h_file_name = dirname + "/generated/optables.h"

with open(absolute_json_file_name, "rt") as file: 
	ops = json.load(file)

# resave can be used if we want to change optab.json programmatically
# the file is plain json but to make it more readable the json-encoder is "hand"-coded
# TODO: Maybe use commalist instead of first here
def resave_json(b):
	f = open(absolute_json_file_name, "wt")
	f.write("[\n")
	first_line = True
	for l in b:
		if not first_line:
			f.write(",\n")
		first_line = False
		f.write("\t[")
		first = True
		for w in l:
			if not first:
				f.write(", ")
			first = False
			f.write("\"" + w + "\"")
		f.write("]")
	f.write("\n]\n")

#resave_json(ops)

out = open(absolute_output_h_file_name, "wt")

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