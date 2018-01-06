import collections
import re

Option = collections.namedtuple('Option', 'group name short_name type default description')
EnumOption = collections.namedtuple('EnumOption', 'name description')

option_reference_re = re.compile("\${([^}]*)}") # Match ${word} with word ending up in first group

options=[
	Option("rom",        "rom",              "r",  "input",   "",      "ROM file. Currently only LoROM ROMs are allowed"),
	Option("rom",        "romheader",        "rh", "enum",    "auto",      "Specify header type of ROM"),
	Option("rom",        "romsize",          "rs", "int",     "0",     "Size of ROM cartridge (without header). When a 0 if specified this is determined as ROM file size minus ROM header size"),
	Option("rom",        "rommode",          "rm", "enum",    "lorom",      "Type of ROM"),
	Option("trace",      "trace",            "t",  "input*",  "",      "Trace file from an emulation session. Multiple allowed for assembly source listing (but not trace log or rewind)"),
	Option("trace",      "regenerate",       "rg", "bool",    "false", "Regenerate emulation caches. Needs to be run if trace files has been updated"),
	Option("trace",      "predict",          "p",  "enum",    "functions",      "Predict can add instructions that was not part of the trace by guessing"),
	Option("tracelog",   "nmifirst",         "n0", "int",     "0",     "First NMI to consider for things that are nmi range based. Currently only affects the trace log"),
	Option("tracelog",   "nmilast",          "n1", "int",     "0",     "Last NMI to consider for things that are nmi range based. Currently only affects the trace log"),
	Option("tracelog",   "tracelog",         "tl", "output",  "",      "Generated trace log. Nmi range can be controlled using ${nmifirst} and ${nmilast}. Custom printing can be done using scripting"),
	Option("tracelog",   "script",           "s",  "input",   "",      "A squirrel script. See scripting reference in the user guide for entry point functions as well as API specification"),
	Option("annotation", "labels",           "l",  "input*",  "",      "A file containing annotations. Custom file format"),
	Option("annotation", "autolabels",       "al", "inout",   "",      "A file containing annotations. These are special as it will be regenerated if deleted or if ${autoannotate} is specified"),
	Option("annotation", "autoannotate"    , "aa", "bool",    "false", "Auto annotate labels. Automatically generate labels in free space (not used by symbols from regular ${labels}-files) space and save to ${autolabels}. This will also happen if the file specified by ${autolabels} is missing"),
	Option("annotation", "symbolfma",        "sf", "output",  "",      "Generated symbols file in FMA format compatible with bsnes-plus"),
	Option("rewind",     "rewind",           "rw", "output",  "",      "Generated rewind report in .DOT file format. Use graphviz to generate PDF/PNG report"),
	Option("report",     "report",           "rp", "output",  "",      "Generated assembly report. Companion file to ${asm}"),
	Option("asm",        "asm",              "a",  "output",  "",      "Generated assembly listing"),
	Option("asm",        "asmheader",        "ah", "input",   "",      "Content of this file will be pasted in the Header section of the generated assembly source listing"),
	Option("asm",        "asmpc",            "ap", "bool",    "true",  "Print program counter in assembly source listing"),
	Option("asm",        "asmbytes",         "ab", "bool",    "true",  "Print opcode bytes in assembly source listing"),
	Option("asm",        "asmloweropcode",   "",   "bool",    "true",  "Print lower-case opcode in assembly source listing"),

	# Future
	# We could add an option to specify the format of the symbol file, but being able to export multiple in one go might be nice too
]

enums={
	"romheader" : [
		EnumOption("auto", "Size of ROM file % (32*1024)"),
		EnumOption("none", "0"),
		EnumOption("smc", "512"),
	],
	"rommode" : [
		EnumOption("lorom", "LoROM"),
		EnumOption("hirom", "HiROM"),
	],
	"predict": [
		EnumOption("nothing", "No prediction"),
		EnumOption("functions", "Only predict within annotated functions"),
		EnumOption("everything", "Predict as much as possible")
	],
}

options_by_name = {}

for option in options:
	options_by_name[option.name] = option

# Do not add meaningless but ok dependencies (such as asm-pc needing asm)
needs={
	"trace"        : set(["asm", "tracelog", "rewind", "regenerate", "predict"]),
	"single_trace" : set(["tracelog", "rewind"]),
	"rom"          : set(["trace"]),
}

def validate_names():
	# Validate unique names
	long_names = set([])
	short_names= {}

	for o in options:
		n = o.name
		sn = o.short_name

		if n in long_names:
			print("Error, name " + n + " already used!")
		else:
			long_names.add(n)
		if sn != "":
			if sn in short_names:
				print("Error, short name " + sn + " already used by " + short_names[sn])
			else:
				short_names[sn] = n

validate_names()

def stripped_name_type(option):
	type = option.type
	multiple = False
	if type[-1:] == '*':
		type = type[:-1]
		multiple = True

	name = option.name
	if type == "output":
		name = name + "out"
	if type == "input" or type == "output" or type == "inout":
		name = name + "file"

	return (name, type, multiple)

nice_types = {"input":"input file name", "output":"output file name", "inout":"input/output file name", "int":"integer", "bool":"boolean", "enum":"enumeration"}

def write_documentation(file, section_prefix = "##"):

	sections=[
		("rom", "Rom"),
		("trace", "Trace"),
		("asm", "Assembly Listing"),
		("tracelog", "Trace Log"),
		("annotation", "Annotations"),
		("report", "Reports"),
		("rewind", "Rewind"),
	]

	for section in sections:
		file.write(section_prefix + " " + section[1] + "\n")
		file.write("Name | Short | Type | Description\n")
		file.write(":----|-------|:-----|:-----------\n")

		for option in options:
			if option.group != section[0]:
				continue

			(name, type, multiple) = stripped_name_type(option)

			nice_type = type
			if type in nice_types:
				nice_type = nice_types[type]

			def option_reference_patcher(m):
				if not m.group(1) in options_by_name:
					print("Oops, no such option '" + m.group(1) + "'!")
				option = options_by_name[m.group(1)]
				(name, type, multiple) = stripped_name_type(option)
				return "*" +name+"*"

			# Replace "${ref}"" with "ref" after validating that there still is an option "ref"
			description = option_reference_re.sub(option_reference_patcher, option.description)

			file.write("*" + name + "* | " + option.short_name + " | " + nice_type + " | " + description + ".")

			if type == "enum":
				file.write("<br>")
				e = enums[option.name]
				for m in e:
					if option.default == m.name:
						# TODO: Can we make sure m.name won't break if it has hyphens?
						file.write("<br>**" + m.name +"**: " + m.description + " (default)")
					else:
						file.write("<br>**" + m.name +"**: " + m.description)
			file.write("\n")
		file.write("\n")

file = open("command-line-doc.html", "wt")
file.write("<meta charset=\"utf-8\">\n\n")

write_documentation(file)

file.write("\n<!-- Markdeep: --><style class=\"fallback\">body{visibility:hidden;white-space:pre;font-family:monospace}</style><script src=\"markdeep.min.js\"></script><script src=\"https://casual-effects.com/markdeep/latest/markdeep.min.js?\"></script><script>window.alreadyProcessedMarkdeep||(document.body.style.visibility=\"visible\")</script>\n")

file.close()
