# This script generates things related to command line syntax.
# The purpose for a script are many:
#   Make sure the snestistics command line parser is of high quality. This means consistent naming and behavior
#   Make sure snestistics can give good command line syntax printing that is accurate
#   Make sure that the snestistics command line parser and the documentation of the same match (by generating documentation)

import collections
import re

Option = collections.namedtuple('Option', 'group name short_name type default description')
EnumOption = collections.namedtuple('EnumOption', 'name description')

option_reference_re = re.compile("\${([^}]*)}") # Match ${word} with word ending up in first group

options=[
	Option("rom",        "Rom",              "r",  "input",   "",      "ROM file. Currently only LoROM ROMs are allowed"),
	Option("rom",        "RomHeader",        "rh", "enum",    "auto",      "Specify header type of ROM"),
	Option("rom",        "RomSize",          "rs", "uint",    "0",     "Size of ROM cartridge (without header). When a 0 if specified this is determined as ROM file size minus ROM header size"),
	Option("rom",        "RomMode",          "rm", "enum",    "lorom",      "Type of ROM"),
	Option("trace",      "Trace",            "t",  "input*",  "",      "Trace file from an emulation session. Multiple allowed for assembly source listing (but not trace log or rewind)"),
	Option("trace",      "Regenerate",       "rg", "bool",    "false", "Regenerate emulation caches. Needs to be run if trace files has been updated"),
	Option("trace",      "Predict",          "p",  "enum",    "functions",      "Predict can add instructions that was not part of the trace by guessing. This setting specify where snestistics is allowed to guess"),
	Option("tracelog",   "NmiFirst",         "n0", "uint",    "0",     "First NMI to consider for things that are nmi range based. Currently only affects the trace log"),
	Option("tracelog",   "NmiLast",          "n1", "uint",    "0",     "Last NMI to consider for things that are nmi range based. Currently only affects the trace log"),
	Option("tracelog",   "TraceLog",         "tl", "output",  "",      "Generated trace log. Nmi range can be controlled using ${NmiFirst} and ${NmiLast}. Custom printing can be done using scripting"),
	Option("tracelog",   "Script",           "s",  "input",   "",      "A squirrel script. See scripting reference in the user guide for entry point functions as well as API specification"),
	Option("annotation", "Labels",           "l",  "input*",  "",      "A file containing annotations. Custom file format"),
	Option("annotation", "AutoLabels",       "al", "inout",   "",      "A file containing annotations. These are special as it will be regenerated if deleted or if ${AutoAnnotate} is specified"),
	Option("annotation", "AutoAnnotate"    , "aa", "bool",    "false", "Auto annotate labels. Automatically generate labels in free space (not used by symbols from regular ${Labels}-files) space and save to ${AutoLabels}. This will also happen if the file specified by ${AutoLabels} is missing"),
	Option("annotation", "SymbolFma",        "sf", "output",  "",      "Generated symbols file in FMA format compatible with bsnes-plus"),
	Option("rewind",     "Rewind",           "rw", "output",  "",      "Generated rewind report in .DOT file format. Use graphviz to generate PDF/PNG report"),
	Option("report",     "Report",           "rp", "output",  "",      "Generated assembly report. Companion file to ${Asm}"),
	Option("asm",        "Asm",              "a",  "output",  "",      "Generated assembly listing"),
	Option("asm",        "AsmHeader",        "ah", "input",   "",      "Content of this file will be pasted in the Header section of the generated assembly source listing"),
	Option("asm",        "AsmPc",            "ap", "bool",    "true",  "Print program counter in assembly source listing"),
	Option("asm",        "AsmBytes",         "ab", "bool",    "true",  "Print opcode bytes in assembly source listing"),
	Option("asm",        "AsmLowerCaseOp",   "",   "bool",    "true",  "Print lower-case opcode in assembly source listing"),

	# Future
	# We could add an option to specify the format of the symbol file, but being able to export multiple in one go might be nice too
]

enums={
	"RomHeader" : [
		EnumOption("none", "No header"),
		EnumOption("copier", "Copiers usually add a 512 byte header before the ROM"),
		EnumOption("auto", "Tries to guess header. Assumes that files are composed of a header and then a multiple of 32KBs (32*1024 bytes)"),
	],
	"RomMode" : [
		EnumOption("lorom", "LoROM"),
		EnumOption("hirom", "HiROM"),
	],
	"Predict": [
		EnumOption("never", "No prediction"),
		EnumOption("functions", "Only predict within annotated functions"),
		EnumOption("everywhere", "Predict as much as possible")
	],
}
enums_prefix = {"RomHeader":"RH", "RomMode":"RM", "Predict":"PRD"}

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
		n = o.name.lower()
		sn = o.short_name.lower()

		if n in long_names:
			print("Error, name " + n + " already used!")
		else:
			long_names.add(n)
		if sn != "":
			if sn in short_names:
				print("Error, short name " + sn + " already used by " + short_names[sn])
			else:
				short_names[sn] = n

		if o.type == "enum":
			found = False
			for v in enums[o.name]:
				if v.name == o.default:
					found = True
			if not found:
				print("Error, option " + o.name + " used unknown default value " + o.default)

validate_names()

def stripped_name_type(option):
	type = option.type
	multiple = False
	if type[-1:] == '*':
		type = type[:-1]
		multiple = True

	name = option.name
	if type == "output":
		name = name + "Out"
	if type == "input" or type == "output" or type == "inout":
		name = name + "File"

	return (name, type, multiple)

def string_to_snake(s):
	r = s[0].lower()
	for a in s[1:]:
		if a.isupper():
			r = r + "_" + a.lower()
		else:
			r = r + a
	return r

def write_documentation(file, section_prefix = "##"):

	nice_types = {
		"input"  : "input file name",
		"output" : "output file name",
		"inout"  : "input/output file name",
		"uint"   : "integer",
		"bool"   : "boolean",
		"enum"   : "enumeration"
	}

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
			name = name.lower()

			nice_type = type
			if type in nice_types:
				nice_type = nice_types[type]

			def option_reference_patcher(m):
				if not m.group(1) in options_by_name:
					print("Oops, no such option '" + m.group(1) + "'!")
				option = options_by_name[m.group(1)]
				(name, type, multiple) = stripped_name_type(option)
				return "*" +name.lower()+"*"

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

def generate_parser(file):

	parser_boilerplate = [
		"#pragma once",
		"",
		"// This file is generated by utilities/command-line-parsing.py",
		"",
		"#include <string>",
		"#include <vector>",
		"#include <stdint.h>",
		"",
		"struct Options {",
		"<<CONTENT>>",
		"};",
		"",
		"void parse(const int argc, const char* const argv, Options &options);"
	]

	def enum_value_name(enum, value):
		prefix = enums_prefix[enum]
		return prefix + "_" + value.upper()

	for line in parser_boilerplate:
		if line != "<<CONTENT>>":
			file.write(line)
			file.write("\n")
			continue

		# First generate all enums
		for enum in enums:
			file.write("\tenum " + enum + "Enum {\n")
			for v in enums[enum]:
				file.write("\t\t{:<16} // {}\n".format(enum_value_name(enum, v.name)+",", v.description))
				#file.write("\t\t" + enum_value_name(enum, v.name) + ", // " + v.description + "\n")
			file.write("\t};\n")
			file.write("\n")

		type_translation={
			"input":  "std::string",
			"input*":  "std::vector<std::string>",
			"output": "std::string",
			"inout":  "std::string",
			"uint":   "uint32_t",
			"bool":   "bool",
			"enum":   "enum",
		}

		first_value = True

		# Now all actual value
		for option in options:
			tt = type_translation[option.type]
			if tt == "enum":
				tt = option.name + "Enum"

			# Expand references in description
			def option_reference_patcher(m):
				if not m.group(1) in options_by_name:
					print("Oops, no such option '" + m.group(1) + "'!")
				option = options_by_name[m.group(1)]
				(name, type, multiple) = stripped_name_type(option)
				return string_to_snake(name)

			# Replace "${ref}"" with "ref" after validating that there still is an option "ref"
			description = option_reference_re.sub(option_reference_patcher, option.description)

			# TODO: We need to do a pretty multi line printer using /**/ here

			if not first_value:
				file.write("\n")
			first_value = False

			file.write("\t// " + description + ".\n")

			name = option.name
			if option.type in ["output"]:
				name = name + "Out"
			if option.type in ["input", "output", "inout", "input*"]:
				name = name + "File"

			file.write("\t{:<28} {}".format(tt, string_to_snake(name)))
			if option.default:
				if option.type == "enum":
					file.write(" = " + enum_value_name(option.name, option.default))
				else:
					file.write(" = " + option.default)
			file.write(";\n")

file = open("../docs/_includes/command-line.html", "wt")
#file.write("<meta charset=\"utf-8\">\n\n")
#file.write("Command Line Reference:\n")
#file.write("=======================\n")
write_documentation(file, "##")
#file.write("\n<!-- Markdeep: --><style class=\"fallback\">body{visibility:hidden;white-space:pre;font-family:monospace}</style><script src=\"markdeep.min.js\"></script><script src=\"https://casual-effects.com/markdeep/latest/markdeep.min.js?\"></script><script>window.alreadyProcessedMarkdeep||(document.body.style.visibility=\"visible\")</script>\n")
file.close()

file2 = open("../source/options.h", "wt")
generate_parser(file2)
file2.close()
