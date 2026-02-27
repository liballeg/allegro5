ABI Compatibility Guidelines
============================

# ABI description

Allegro is split into the core library (`liballegro`) and addons (e.g.
`liballegro_image`). The public API starts with `al_` for functions and
`ALLEGRO_` for types. The private API that is used by the addons starts with
`_al_` for functions and `_AL` for types. There are some exceptions where there
are some `ALLEGRO_` types defined in `aintern_*.h` headers. Despite their names
starting with `ALLEGRO_` those are private types.

Within a minor version (5.0, 5.2 etc) we maintain ABI backwards compatibility
only for public API. This means that we can't remove functions (even if we turn
them into compatibility functions), can't change their arguments. For types, we
can't change their size. If the type is not opaque, this means we can't add or
remove fields, change their types. These considerations do not apply to private
APIs: Allegro addons and Allegro core are intended to match in their precise
version when used together. Therefore private APIs have no backwards compatibility.

Similarly, the unstable public API (marked as such in the documentation) also
does not come with backwards guarantees.

# Tooling

To help maintain ABI compatibility we provide two scripts. To use them you must
be on Linux and have built the non-monolith version of the library with debug
symbols (e.g. RelWithDebInfo). First, we have `misc/generate_abi.sh`:

```bash
./misc/generate_abi.sh $BUILD_DIR
```

This is used to generate "golden" ABI XML files which are checked in and
updated every release. Then, there's `misc/compare_abi.sh`:

```bash
./misc/compare_abi.sh $BUILD_DIR
```

This is run before making a release to check for ABI compatibility regressions.
Check over the outputs of this script to make sure the ABI compatibility is
maintained. This process is manual due to the somewhat complex ABI guarantees,
but we'll strive to make this more automatic in the future.
