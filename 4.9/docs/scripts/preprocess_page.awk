BEGIN {
    PROTOS = ENVIRON["PROTOS"]

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
