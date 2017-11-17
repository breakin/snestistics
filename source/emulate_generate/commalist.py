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