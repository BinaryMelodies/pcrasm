#! /usr/bin/python3

import os
import sys

ADDLINEDIR = True

DEBUG = False
def include_file(filename, parts):
	global ADDLINEDIR, DEBUG

	pathname = os.path.split(filename)[0]
	include_names = []
	include_parts = {}
	already_included = set()
	with open(filename, 'r') as file:
		if ADDLINEDIR or True:
			parts[0].append(f"#line 1 \"{filename}\"\n")
		section = 0
		for lineno, line in enumerate(file):
			if line.strip().startswith('%include'):
				include_name = line[line.find('"') + 1:line.rfind('"')]
				if include_name in include_names:
					if DEBUG:
						print(f"Include {include_name} in {section} by directive") # TODO
					parts[section] += include_parts[include_name][section]
					if ADDLINEDIR or section != 1:
						parts[section].append(f"#line {lineno+2} \"{filename}\"\n")
					already_included.add(include_name)
				else:
					i_parts = [[], [], []]
					include_file(os.path.join(pathname, include_name), i_parts)
					for i in range(section + 1):
						if DEBUG:
							print(f"Include {include_name} in {i} by directive") # TODO
						parts[i] += i_parts[i]
						if ADDLINEDIR or i != 1:
							parts[i].append(f"#line {lineno+2} \"{filename}\"\n")
					include_names.append(include_name)
					include_parts[include_name] = i_parts
					already_included.add(include_name)
			elif line.strip() == '%%':
				for include_name in include_names:
					if include_name in already_included:
						continue
					if DEBUG:
						print(f"Include {include_name} in {section} by section termination") # TODO
					parts[section] += include_parts[include_name][section]
				section += 1
				if ADDLINEDIR or section != 1:
					parts[section].append(f"#line {lineno+2} \"{filename}\"\n")
				already_included.clear()
			else:
				parts[section].append(line)

	for include_name in include_names:
		if include_name in already_included:
			continue
		if DEBUG:
			print(f"Include {include_name} in {section} by section termination") # TODO
		parts[section] += include_parts[include_name][section]

def main():
	global ADDLINEDIR
	if sys.argv[1] == '--noline':
		ADDLINEDIR = False
		sys.argv.pop(1)
	parts = [[], [], []]
	include_file(sys.argv[1], parts)
	with open(sys.argv[2], 'w') as file:
		for line in parts[0] + ['%%\n'] + parts[1] + ['%%\n'] + parts[2]:
			print(line, end = '', file = file)

if __name__ == '__main__':
	main()

