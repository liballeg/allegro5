# mawk does not like [/] for matching slash.

/\/\* (Function|Type|Enum): / {
    found_tag = 1
    do_print = 0
    prefix = $3
    next
}

/^ ?\*\/ *$/ && found_tag {
    found_tag = 0
    do_print = 1
    next
}

/^{|^$/ {
    do_print = 0
    next
}

do_print {
    print prefix ": " $0
}

# Stop if we see opening brace or semicolon at end of line.
/{ *$|; *$/ {
    do_print = 0
}

# vim: set sts=4 sw=4 et:
