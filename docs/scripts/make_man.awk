BEGIN {
    PANDOC = ENVIRON["PANDOC"]
    PROTOS = ENVIRON["PROTOS"]
    SECTION = ENVIRON["SECTION"]
    MANUAL = ENVIRON["MANUAL"]

    # Load prototypes into an array `protos'.
    FS = /: /
    while ((getline line < PROTOS) > 0) {
        split(line, arr, /: /)
        name = arr[1]
        text = arr[2]
        protos[name] = protos[name] "\n    " text
    }
    close(PROTOS)
    FS = " "
}

# At the start of a new file.
FNR == 1 {
    # Close previous pipe if any.
    if (CMD) {
        close(CMD)
    }

    # Get the basename of the file, which is the name of the
    # C symbol being documented.
    name = FILENAME
    sub(/^.*\//, "", name)

    # Build the pandoc command line.
    # For testing, you can set this to "cat".
    CMD = PANDOC " - -t man --standalone -o " name "." SECTION

    printf "%% %s(%s) %s\n", name, SECTION, MANUAL | CMD
    print "# NAME\n" | CMD
    print name | CMD
    print "\n# SYNOPSIS\n" | CMD
    print protos[name] | CMD
    print "\n# DESCRIPTION\n" | CMD
}

/\*?Return [Vv]alue:\*?/ {
    print "# RETURN VALUE\n" | CMD
    next
}

/\*?See [Aa]lso:\*?/ {
    print "# SEE ALSO\n" | CMD
    next
}

{
    # Replace [al_foo] and [ALLEGRO_foo] by al_foo(3) and ALLEGRO_foo(3).
    # This would be easy with gensub but that's gawk-specific :(
    while (match($0, /\[al_[^]]+\]|\[ALLEGRO_[^]]+\]/) > 0) {
        before = substr($0, 0, RSTART - 1)
        name   = substr($0, RSTART + 1, RLENGTH - 2)
        rest   = substr($0, RSTART + RLENGTH)
        printf "%s%s(%s)", before, name, SECTION | CMD
        $0 = rest
    }
    print $0 | CMD
}

# vim: set et:
