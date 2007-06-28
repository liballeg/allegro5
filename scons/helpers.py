def read_cmake_list(name):
	lists = {}
	for line in open(name):
		if line.startswith("set"):
			current = []
			name = line[4:].strip()
			lists[name] = current
		else:
			w = line.strip("( )\t\r\n")
			if w: current.append(w)
	return lists
