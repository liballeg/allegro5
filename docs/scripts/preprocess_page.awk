BEGIN {
    PROTOS = ENVIRON["PROTOS"]

    # Load prototypes into an array `protos'.
    while ((getline line < PROTOS) > 0) {
        # Strip CR terminators
        sub(/\r$/, "", line)

        split(line, arr, /: /)
        name = arr[1]
        text = arr[2]
        protos[name] = protos[name] "\n    " text
    }
    close(PROTOS)
}

# Strip CR terminators in case files have DOS terminators.
{ sub(/\r$/, "") }

/^#+/ {
    # Raise sections by one level. Top-most becomes the document title.
    if (raise_sections) {
        if ($1 == "#") {
            $1 = "%"
        }
        else {
            sub(/^#/, "", $1)
        }
    }
}

/^#+ API: / {
    hashes = $1
    name = $3
    print hashes, name
    print protos[name]
    next
}

{
    print
}

# vim: set et:
