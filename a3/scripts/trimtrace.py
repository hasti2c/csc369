#!/usr/bin/env python3

import re
import sys


markerfile = sys.argv[1]
tracefile = sys.argv[2]

with open(markerfile) as f:
	start_marker, end_marker = [int(s, 16) for s in f.read().strip().split()]

found_start = False
found_end = False

p = re.compile(r'([ILMS])\s+([0-9a-fA-F]+),')

with open(tracefile) as f:
	for line in f:
		line = line.strip()
		m = p.match(line)
		if not m:
			continue

		reftype = m.group(1)
		addr = int(m.group(2), 16)

		if (addr == start_marker) and (reftype == 'S'):
			if found_start:
				raise Exception("Start marker appears more than once")
			found_start = True
		elif (addr == end_marker) and (reftype == 'S'):
			if found_end:
				raise Exception("End marker appears more than once")
			found_end = True

		if found_start and not found_end:
			print(line)
