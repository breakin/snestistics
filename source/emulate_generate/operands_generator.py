from commalist import CommaList

# http://westerndesigncenter.com/wdc/documentation/w65c816s.pdf
# http://wiki.superfamicom.org/snes/show/65816+Reference

# NOTE:
#   Some have aliases (in asm). See http://wiki.superfamicom.org/snes/show/65816+Reference
#   Sometimes the correct opcode can't be derived when there is both 16-bit and 24-bit version.
#     Assembler can add say LDA.W LDA.L to differentiate.

# http://westerndesigncenter.com/wdc/documentation/w65c816s.pdf, page 37
# page 37... I kinda like the symbols
addressing_modes=[
	["#A",		"Immediate (size of accumulator)"],
	["#X",		"Immediate (size of index)"],
	["#8",		"Immediate 8-bit"],
	#["A",		"Accumulator"],
	["r",		"Program Counter Relative"],
	["rl",		"Program Counter Relative long"],
	["I",		"Implied"],
	["s",		"Stack"],
	["d",		"Direct"],
	["d,x",		"Direct indexed,X"],
	["d,y",		"Direct indexed,Y"],
	["(d)",		"Direct indirect"],
	["(d,x)",	"Direct indexed indirect"],
	["(d),y",	"Direct indirect indexed"],
	["[d]",		"Direct indirect long"],
	["[d],y",	"Direct indirect indexed long,Y"],
	["a",		"Absolute"],
	["a,x",		"Absolute indexed,X"],
	["a,y",		"Absolute indexed,Y"],
	["al",		"Absolute long"],
	["al,x",	"Absolute indexed long"],
	["d,s",		"Stack relative"],
	["(d,s),y",	"Stack relative indirect indexed,Y"],
	["(a)",		"Absolute indirect"],
	["(a,x)",	"Absolute indexed indirect"],
	["xyc",		"Block move"]
]

def parse_table():
	amodedict={}
	extradict={}
	for a in range(len(addressing_modes)):
		amodedict[addressing_modes[a][1].lower()]=a
		extradict[addressing_modes[a][0]]=a

	# Fix some errors in the table
	corrections = {}
	corrections["dp indexed indirect,x"] = "d,x"
	corrections["dp indexed indirect, y"] = "(d),y"
	corrections["direct page"] = "d"
	corrections["dp indirect long"] = "[d]"
	corrections["dp indirect indexed, y"] = "(d),y"
	corrections["dp indirect long indexed, y"] = "[d],y"
	corrections["dp indirect"] = "(d)"
	corrections["dp indexed,x"] = "(d,x)"
	corrections["sr indirect indexed,y"] = "(d,s),y"
	corrections["absolute long indexed,x"] = "al,x"
	corrections["accumulator"] = "I"
	corrections["program counter relative"] = "r"
	corrections["program counter relative long"] = "rl"
	corrections["dp indexed,y"] = "d,y"
	corrections["stack (dp indirect)"] = "(d,s),y"
	corrections["stack (pc relative long)"] = "d,s"
	corrections["stack/interrupt"] = "s"
	corrections["stack (absolute)"] = "s"
	corrections["stack (push)"] = "s"
	corrections["stack (pull)"] = "s"
	corrections["stack (rti)"] = "s"
	corrections["stack (rtl)"] = "s"
	corrections["stack (rts)"] = "s"
	corrections["absolute indirect long"] = "(a)"
	corrections[""] = "I"

	for e in corrections:
		short = corrections[e]
		idx = extradict[short]
		amodedict[e] = idx

	ops={}

	IA = 0
	IX = 1
	I8 = 2

	f2 = open("operands.py", "wt")
	f2.write("# Generated from table at http://wiki.superfamicom.org/snes/show/65816+Reference\n")
	f2.write("# Address mode names from http://westerndesigncenter.com/wdc/documentation/w65c816s.pdf, page 37\n")
	f2.write("# https://wiki.superfamicom.org/snes/show/Jay%27s+ASM+Tutorial\n")
	f2.write("\n")

	f2.write("addressing_modes=[\n")
	lineWriter = CommaList(f2, indent = "\t")
	for a in addressing_modes:
		lineWriter.write("(\"" + a[0] + "\", \"" + a[1] + "\")")
	lineWriter.finish()

	f2.write("]\n\n")

	f2.write("# Tuple (hexcode, mnemonic, addressing mode (index into adressing_modes), mnemonic alias)\n")
	f2.write("opcodes=[\n")

	f = open("table.txt", "rt")

	lineWriter = CommaList(f2, indent = "\t")

	for line in f:
		parts = line.split("\t")
		op = parts[0].split(" ")[0]

		address_mode = 0
		amode = parts[4].lower()
		if amode == "immediate":
			if op == "ADC": address_mode=IA
			elif op == "AND": address_mode=IA
			elif op == "BIT": address_mode=IA
			elif op == "CMP": address_mode=IA
			elif op == "CPX": address_mode=IX
			elif op == "CPY": address_mode=IX
			elif op == "EOR": address_mode=IA
			elif op == "LDA": address_mode=IA
			elif op == "LDX": address_mode=IX
			elif op == "LDY": address_mode=IX
			elif op == "ORA": address_mode=IA
			elif op == "REP": address_mode=I8
			elif op == "SEP": address_mode=I8
			elif op == "SBC": address_mode=IA
			else:
				print(parts)
				break

		elif not amode in amodedict:
			print(parts)
			break
		else:
			address_mode = amodedict[amode]

		print(parts)

		hex_code = parts[3]

		lineWriter.write("(\"0x" + hex_code + "\", \"" + op + "\", " + str(address_mode) + ", \"" + parts[1] + "\")")
	lineWriter.finish()
	f2.write("]\n")
	f2.close()

parse_table()