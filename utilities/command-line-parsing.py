# This script generates things related to command line syntax.
# The purpose for a script are many:
#   Make sure the snestistics command line parser is of high quality. This means consistent naming and behavior
#   Make sure snestistics can give good command line syntax printing that is accurate
#   Make sure that the snestistics command line parser and the documentation of the same match (by generating documentation)

# IDEA: Have both short and long description. For doc we use short + long. For syntax we use short (unless maybe -h or something).

import collections
import re

Option = collections.namedtuple('Option', 'group name short_name type default description')
EnumOption = collections.namedtuple('EnumOption', 'name description')

option_reference_re = re.compile("\${([^}]*)}") # Match ${word} with word ending up in first group

options=[
	Option("rom",        "Rom",              "r",  "input",   "",      "ROM file. Currently only LoROM ROMs are supported"),
	Option("rom",        "RomSize",          "rs", "uint",    "0",     "Size of ROM cartridge (without header). 0 means auto-detect"),
	#Option("rom",        "RomMode",          "rm", "enum",    "trace", "Type of ROM"),
	Option("trace",      "Trace",            "t",  "input*",  "",      "Trace file from an emulation session. Multiple allowed for assembly source listing"),
	#Option("trace",      "Regenerate",       "rg", "bool",    "false", "Regenerate emulation caches. Should happen automatically"),
	Option("tracelog",   "NmiFirst",         "n0", "uint",    "0",     "First NMI to consider for trace log"),
	Option("tracelog",   "NmiLast",          "n1", "uint",    "0",     "Last NMI to consider for trace log"),
	Option("tracelog",   "TraceLog",         "tl", "output",  "",      "Generate trace log. Nmi range can be controlled using ${NmiFirst} and ${NmiLast}. Custom printing can be done using scripting"),
	Option("tracelog",   "Script",           "s",  "input",   "",      "A squirrel script. See user guide for scripting reference"),
	Option("annotation", "Labels",           "l",  "input*",  "",      "A file containing annotations. Custom file format"),
	Option("annotation", "AutoLabels",       "al", "inout",   "",      "A file containing annotations. It will be regenerated if missing or if ${AutoAnnotate} is specified"),
	Option("annotation", "AutoAnnotate"    , "aa", "bool",    "false", "A file where automatically generated annotations are stored"),
	Option("annotation", "SymbolFma",        "sf", "output",  "",      "Generate symbols file in FMA format compatible with bsnes-plus"),
	Option("rewind",     "Rewind",           "rw", "output",  "",      "Generate rewind report in dot file format. Use graphviz to generate PDF/PNG report"),
	Option("asm",        "Report",           "rp", "output",  "",      "Generate assembly report. Companion file to ${Asm}"),
	Option("asm",        "Asm",              "a",  "output",  "",      "Generate assembly listing"),
	Option("asm",        "Predict",          "p",  "enum",    "functions",      "This setting specify where snestistics is allowed to predict code. This is currently only used for assembly listing"),
	Option("asm",        "AsmHeader",        "ah", "input",   "",      "File content will be included in assembly listing"),
	Option("asm",        "AsmPrintPc",            "apc", "bool",    "true",  "Print program counter in assembly source listing"),
	Option("asm",        "AsmPrintBytes",         "ab", "bool",    "true",  "Print opcode bytes in assembly source listing"),
	#Option("asm",        "AsmPrintTraceComments",   "atc",   "bool", "true",  "Print data and jump targets based on trace in comments"),
	Option("asm",        "AsmPrintRegisterSizes",     "ars",   "bool", "true",  "Print registers sizes in assembly source listing"),
	Option("asm",        "AsmPrintDb",                "adb",   "bool", "true",  "Print data bank in assembly source listing"),
	Option("asm",        "AsmPrintDp",                "adp",   "bool", "true",  "Print direct page in assembly source listing"),
	Option("asm",        "AsmLowerCaseOp",       "",   "bool", "true",  "Print lower-case opcode in assembly source listing"),
	Option("asm",        "AsmCorrectWla",        "",   "bool", "false", "Make sure generated source compiled in WLA DX"),
	# Future
	# We could add an option to specify the format of the symbol file, but being able to export multiple in one go might be nice too
]

# These are only used for the user guide
long_descriptions = {
	"AutoAnnotate" : "Automatically generate labels in free space (not used by symbols from regular ${Labels}-files) space and save to ${AutoLabels}. This will also happen if the file specified by ${AutoLabels} is missing",
}

enums={
	#"RomMode" : [
	#	EnumOption("trace", "Read correct mode from .trace-file"),
	#	EnumOption("lorom", "LoROM"),
	#	EnumOption("hirom", "HiROM"),
	#],
	"Predict": [
		EnumOption("never", "No prediction"),
		EnumOption("functions", "Only predict within annotated functions"),
		EnumOption("everywhere", "Predict as much as possible")
	],
}
enums_prefix = {"RomHeader":"RH", "RomMode":"RM", "Predict":"PRD"}

# Do not add meaningless but ok dependencies (such as asm-pc needing asm)
needs={
	"trace"        : set(["Asm", "TraceLog", "Rewind", "Regenerate", "Predict"]),
	"single_trace" : set(["TraceLog", "Rewind"]),
	"rom"          : set(["Trace"]),
}

options_by_name = {}

for option in options:
	options_by_name[option.name] = option

needs_by_option={}

for need in needs:
	for d in needs[need]:
		if not d in needs_by_option:
			needs_by_option[d] = set([])
		needs_by_option[d].add(need)

def validate_names():
	# Validate unique names
	long_names = set([])
	short_names= {}

	for o in options:
		n = o.name.lower()
		sn = o.short_name.lower()

		if n in long_names:
			print("Error, name '" + n + "'' already used!")
		else:
			long_names.add(n)
		if sn != "":
			if sn in short_names:
				print("Error, short name '" + sn + "'' already used by " + short_names[sn])
			else:
				short_names[sn] = n

		if o.type == "enum":
			found = False
			for v in enums[o.name]:
				if v.name == o.default:
					found = True
			if not found:
				print("Error, option '" + o.name + "'' used unknown default value " + o.default)

	for need in needs:
		for d in needs[need]:
			if not d.lower() in long_names:
				print("Option '" + d + "'' in needs is not an option")

	for ld in long_descriptions:
		if not ld.lower() in long_names:
			print("Long description for '" + ld + "'' does not have corresponding option")

validate_names()

def full_name(option, plural_s = False):
	name = option.name
	t = option.type
	multiple = False
	if t[-1]=='*':
		t=t[:-1]
		multiple = True
	if t == "output":
		name = name + "Out"
	if t in ["input", "output", "inout"]:
		name = name + "File"
	if plural_s and multiple:
		name = name + "s"
	return name

def variable_name(option):
	return string_to_snake(full_name(option, True))

def switch_name(option):
	return full_name(option).lower()

def short_switch_name(option):
	return option.short_name.lower()

def enum_switch_value_name(option, value):
	return value.name.lower()

def enum_variable_value_name(option, value):
	return enums_prefix[option.name] + "_" + string_to_snake(value.name).upper()

# TODO: Migrate users of this functions to variable_name, swtich_name and full_name
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

			desc = option.description

			if option.name in long_descriptions:
				desc = desc + ". " + long_descriptions[option.name]

			# Replace "${ref}"" with "ref" after validating that there still is an option "ref"
			desc = option_reference_re.sub(option_reference_patcher, desc) + "."

			if option.default != "" and type != "enum":
				desc = desc + "<br>default: " + option.default;

			file.write("*" + name + "* | " + option.short_name + " | " + nice_type + " | " + desc)

			if type == "enum":
				file.write("<br>")
				e = enums[option.name]
				for m in e:
					if option.default == m.name:
						# TODO: Can we make sure m.name won't break lines if it has hyphens?
						file.write("<br>**" + m.name +"**: " + m.description + " (default)")
					else:
						file.write("<br>**" + m.name +"**: " + m.description)
			file.write("\n")
		file.write("\n")

def generate_parser_header(file):

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
		"void parse_options(const int argc, const char * const argv[], Options &options);"
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

		# Now all actual value
		for option in options:
			tt = type_translation[option.type]
			if tt == "enum":
				tt = option.name + "Enum"

			name = variable_name(option)

			file.write("\t{:<28} {}".format(tt, name))
			if option.default:
				if option.type == "enum":
					file.write(" = " + enum_value_name(option.name, option.default))
				else:
					file.write(" = " + option.default)
			file.write(";\n")

def generate_parser_source(file):

	parser_boilerplate = [
		"#include \"options.h\"",
		"",
		"// This file is generated by utilities/command-line-parsing.py",
		"",
		"#include \"utils.h\" // CUSTOM_ASSERT",
		"#include <cstdlib>",
		"#include <cstring> // strcmp",
		"#include <stdexcept> // std::runtime_error",
		"",
		"namespace {",
		"	bool parse_bool(const char * const s, bool &error) {",
		"		if ((strcmp(s, \"true\" )==0) || (strcmp(s, \"on\" )==0) || (strcmp(s, \"1\" )==0)) return true;",
		"		if ((strcmp(s, \"false\")==0) || (strcmp(s, \"off\")==0) || (strcmp(s, \"0\" )==0)) return false;",
		"		error = true;",
		"		return true;",
		"	}",
		"	uint32_t parse_uint(const char * const s, bool &error) {",
		"		return atoi(s);",
		"	}",
		"",
		"	void syntax() {",
		"		printf(\"Snestistics Syntax:\\n\");",
		"		printf(\"  snestistics --option1 value --option2 value\\n\");",
		"		printf(\"\\n\");",
		"		printf(\"Options:\\n\");",
		"		printf(\"\\n\");",
		"		<<SYNTAX>>",
		"	}",
		"}",
		"",
		"void parse_options(const int argc, const char * const argv[], Options &options) {",
		"	bool error = false;",
		"	bool had_option = false;",
		"	<<NEEDS>>",
		"",
		"	for (int k=1; k<argc; k++) {",
		"		if (!had_option && argv[k][0] != '-')",
		"			continue; // When launched from .bat files sometimes the name of the binary is in both argv[0] and argv[1]",
		"",
		"		had_option = true;",
		"",
		"		const char *const cmd = &argv[k][1];",
		"		const char *const opt = k+1 != argc ? argv[k+1] : \"\";", # TODO: Check so this test makes sense
		"",
		"		<<TESTS>>",
		"		} else {",
		"			printf(\"Switch '%s' not recognized\\n\", cmd);",
		"			error = true;",
		"			break;",
		"		}",
		"	}",
		"	CUSTOM_ASSERT(!need_trace        || !options.trace_files.empty());",
		"	CUSTOM_ASSERT(!need_rom          || !options.rom_file.empty());",
		"	CUSTOM_ASSERT(!need_single_trace || options.trace_files.size() == 1);",
		"",
		"	if (error) {",
		"		printf(\"There was an error in the command line.\\n\");",
		"		syntax();",
		"		exit(1);",
		"	}",
		"}"
	]

	for line in parser_boilerplate:
		if line != "\t<<NEEDS>>" and line != "\t\t<<TESTS>>" and line != "\t\t<<SYNTAX>>":
			file.write(line)
			file.write("\n")
			continue

		if line == "\t<<NEEDS>>":
			for need in needs:
				file.write("\tbool need_" + need + " = false;\n")
			continue

		if line == "\t\t<<SYNTAX>>":
			file.write("")

			for option in options:
				def option_reference_patcher(m):
					if not m.group(1) in options_by_name:
						print("Oops, no such option '" + m.group(1) + "'!")
					option = options_by_name[m.group(1)]
					return "-" + switch_name(option)

				# Replace "${ref}"" with "ref" after validating that there still is an option "ref"
				description = option_reference_re.sub(option_reference_patcher, option.description)

				if option.type == "uint" and option.default != "":
					description = description + ". Default: " + option.default

				description_array = description.split(".") # A bit crude, wants to detect new line



				name_part = " -" + switch_name(option)
				ssn = short_switch_name(option)

				if len(ssn)!=0:
					 name_part = name_part + " (--" + ssn + ")"

				type_part = ""
				if option.type == "enum":
					for v in enums[option.name]:
						add = v.name
						if add == option.default:
							add = "*" + add + "*"
						if len(type_part)!=0:
							type_part = type_part + "|" + add
						else:
							type_part = add
				else:
					nice_types = {
						"input"  : "filename",
						"input*" : "filename",
						"output" : "filename",
						"inout"  : "filename",
						"uint"   : "number",
						"bool"   : "true|false",
					}
					type_part = nice_types[option.type]

				name_part = name_part + " <" + type_part + ">"

				file.write("\t\tprintf(\"" + "{:<48}".format(name_part) + description_array[0] + ".\\n\");\n")

				for d_line in description_array[1:]:
					file.write("\t\tprintf(\"                                               " + d_line + ".\\n\");\n")

			continue

		# The tests
		first = True
		for option in options:
			file.write("\t\t")
			if not first:
				file.write("} else ")
			first = False

			file.write("if (strcmp(cmd, \"" + switch_name(option) + "\")==0 || strcmp(cmd, \"-" + short_switch_name(option) + "\")==0) {\n")
			if option.type == "bool":
				file.write("\t\t\toptions." + variable_name(option) + " = parse_bool(opt, error);\n")
			elif option.type == "uint":
				file.write("\t\t\toptions." + variable_name(option) + " = parse_uint(opt, error);\n")
			elif option.type == "enum":
				values = enums[option.name]
				for v in values:
					file.write("\t\t\tif (strcmp(opt, \"" + enum_switch_value_name(option, v) + "\")==0) options." + variable_name(option) + " = Options::" + enum_variable_value_name(option, v) + ";\n")
			elif option.type[-1]=='*':
				file.write("\t\t\toptions." + variable_name(option) + ".push_back(opt);\n")
			else:
				file.write("\t\t\toptions." + variable_name(option) + " = opt;\n")

			if option.name in needs_by_option:
				for need in sorted(needs_by_option[option.name]):
					file.write("\t\t\tneed_" + need + " = true;\n")
			file.write("\t\t\tk++;\n")

file = open("../docs/_includes/generated-command-line-reference.html", "wt")
write_documentation(file, "##")
file.close()

file2 = open("../source/options.h", "wt")
generate_parser_header(file2)
file2.close()
file3 = open("../source/options.cpp", "wt")
generate_parser_source(file3)
file3.close()
