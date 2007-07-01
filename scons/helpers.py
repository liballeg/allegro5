def read_cmake_list(name):
    """
    Read a cmake files list and return a dictionary with each cmake variable
    matched to a list of filenames.

    This makes it easy to add/remove files, as only the cmake list needs to be
    modified and scons will automatically pick up the changes.
    """
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
