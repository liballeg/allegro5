#!BPY
# Version 9: Modified "top", so upper edges means all on the upper side of the
#            mesh, not any edges facing up. Took as long as all the rest of this
#            script.
#
#            Friction is now sum of R, G, B components of material color.
# Version 8: added the "top" property, and an error message
# Version 7: "foreground" meshes and objects now possible
#            friction can now be set for materials (using color components)
# Version 6: merge object types, round numbers to integer
# Version 5: objects
# Version 4: read edge_height property
# Version 3: process geometry into clockwise triangles
# Version 2: materials
# Version 1: only mesh geometry

"""
Name: 'Allegro Demo Game Level (.txt)...'
Blender: 237
Group: 'Export'
Tooltip: 'Export Allegro Demo Game Level (.txt)'
"""

__bpydoc__ = """\
All mesh objects are exported as triangles. Coordinates are scaled by a factor
of 10, so something at Blender coordinate x=10 will be x=100 in the game. And the
y axis is flipped, since in the game, positive y is downwards.

Every mesh must have a material assigned for each of its faces. You can do this
by just giving a material to the mesh object, or also by applying per-face
materials.

A material must have texture slot 0 and 1 filled in and set to Image textures,
where the first texture's image is the base texture, and the second the edge
texture.
The sum of the R,G,B components of the material color is used as friction.
All other material settings are ignored.

Mesh object understand the following properties:

"edge_height" - The edge height for all faces in the mesh.
"foreground" - All edges are marked foreground.
"background" - No edge is marked collidable.
"top" - Only upper edges of the mesh are marked as edges. Selected are all those
edges part of the outer contour of the mesh, which are lying between the leftmost
and rightmost vertex of that contour, in clockwise order.

Objects are inserted as Blender "Empties". Each object has the following
properties:

"bitmap" - Name of the bitmap to use.
"sound" - Name of the sound to play.
"bonus" - If 1, this object does not count towards the number of needed fruits.
          (So if a level has 30 fruits, and 10 are marked "bonus", only 20 need
          to be collected. It doesn't matter at all which out of the 30 though.)
"foreground" - This is a foreground object.
"deco" - This object is not collidable.

There are some reserved names. Empties whose name starts with "player" or
"Player" mark the player starting position. Empties whose name starts with
"exit" or "Exit" mark the level exit position.

All other objects (cameras, lamps, ...) are ignored.

Currently, there is no sanity checking done, so be sure to:
- Assign materials to meshes/faces.
- Make only 2D meshes. Blender will happily twist your meshes around, but the
  exported level simply will crash. Only a planar mesh is allowed.
- Don't forget the Empties for the player and exit positions, and set at least
  one other collectible.
"""

import Blender, meshtools, sys, time

# scaling factor when exporting vertex coordinates from blender
SCALE_X = 10
SCALE_Y = -10

COMMENTS = False # Set to True to enable additional comments in the output

# can be modified on a per-mesh basis via the "edge_height" property
DEFAULT_EDGE_HEIGHT = 2

message_string = "?"

def message_draw():
	# clearing screen
	Blender.BGL.glClearColor(0.5, 0.5, 0.5, 1)
	Blender.BGL.glColor3f(1.,1.,1.)
	Blender.BGL.glClear(Blender.BGL.GL_COLOR_BUFFER_BIT)
	
	# Buttons
	Blender.Draw.Button("Oh oh", 1, 10, 40, 100, 25)

	#Text
	Blender.BGL.glColor3f(1, 1, 1)
	Blender.BGL.glRasterPos2d(10, 310)
	Blender.Draw.Text(message_string)

def message_event(evt, val):
	if evt == Blender.Draw.ESCKEY and not val: Blender.Draw.Exit()

def message_bevent(evt):
	
	if evt == 1: Blender.Draw.Exit()

class MessageException(Exception): pass

def message(str):
    global message_string
    message_string = str
    Blender.Draw.Register(message_draw, message_event, message_bevent)
    raise MessageException

def transform(matrix, vector):
    """
    Given a matrix and a vector, return the result of transforming the vector by
    the matrix.
    matrix = [[0, 0, 0, 0], [0, 0, 0, 0]]
    vector = [0, 0, 0, 0]
    return = [0, 0]
    """
    vout = []
    for j in range(2):
        x = 0
        for i in range(4):
            x += matrix[i][j] * vector[i]
        vout += [x]
    return vout


def direction(tri):
    """
    Given a triangle, determine if it is counterclockwise or not.
    tri = [i, i, i],
    where i is a global vertex index.
    """
    v1 = (V[tri[1]][0] - V[tri[0]][0], V[tri[1]][1] - V[tri[0]][1])
    v2 = (V[tri[2]][0] - V[tri[1]][0], V[tri[2]][1] - V[tri[1]][1])
    v = v1[0] * v2[1] - v1[1] * v2[0]
    return v

def upper(i1, i2):
    """
    Given two vertex ids, determine if the edge is an upper face.
    """
    v = V[i2][0] - V[i1][0]
    return v

def get_prop(ob, name, val):
    try:
        prop = ob.getProperty(name)
        v = prop.getData()
    except AttributeError:
        v = val
    return v

def make_contour(ob, base_i):
    """
    ob # the Blender mesh object

    Returns a pair (edges, upper_contour).
    edges = {v: n} # maps vertex to sucessor
    upper_contour = {v: 1} # set of vertices in the upper mesh outline
    """
    mesh = ob.getData()
    # count how often each edge is present in the mesh
    boundaries = {}
    for f in mesh.faces:
        prev = f.v[-1]
        for v in f.v:
            vs = [prev.index, v.index]
            vs.sort()
            vs = tuple(vs)
            if vs in boundaries:
                boundaries[vs] += 1
            else:
                boundaries[vs] = 1
            prev = v

    # map each vertex to its connected vertex
    topmost = None
    edges = {}
    for f in mesh.faces:
        prev = f.v[-1]
        for v in f.v:
            vs = [prev.index, v.index]
            vs.sort()
            vs = tuple(vs)
            if vs in boundaries and boundaries[vs] == 1:
                if not base_i + prev.index in edges: edges[base_i + prev.index] = {}
                if not base_i + v.index in edges: edges[base_i + v.index] = {}
                edges[base_i + prev.index][base_i + v.index] = 1
                edges[base_i + v.index][base_i + prev.index] = 1
                if not topmost or V[base_i + prev.index][1] > V[topmost][1]:
                    topmost = base_i + prev.index
            prev = v

    # Build the contour list. We start by picking the topmost vertex, then
    # finding its contour neighbour so we are facing a clockwise direction.
    cv = topmost # pick topmost contour vertex
    next = list(edges[cv])[0] # next one
    ok = False
    # find the face this edge is part of
    edge = (cv, next)
    for f in mesh.faces:
        for i in range(len(f.v)):
            if (base_i + f.v[i - 1].index, base_i + f.v[i].index) == edge or\
                (base_i + f.v[i].index, base_i + f.v[i - 1].index) == edge:
                j = base_i + f.v[(i + 1) % len(f.v)].index
                if direction([cv, next, j]) < 0:
                    ok = True
                    break
        if ok: break
    if not ok:
        next = list(edges[cv])[1]

    full_contour = [cv] # a list of vertex indices, representing the mesh outline
    prev = cv
    while next != cv:
        full_contour += [next]
        v = list(edges[next])[0]
        if v == prev: v = list(edges[next])[1]
        prev = next
        next = v

    leftmost = None
    rightmost = None
    if COMMENTS: print "\t# contour:",
    for vi in range(len(full_contour)):
        v = full_contour[vi]
        if COMMENTS: print "%d" % v,
        if leftmost == None or V[v][0] < V[full_contour[leftmost]][0]:
            leftmost = vi
        if rightmost == None or V[v][0] > V[full_contour[rightmost]][0]:
            rightmost = vi
    if COMMENTS: print

    if COMMENTS: print "\t# left: %d right: %d" % (full_contour[leftmost],
        full_contour[rightmost])

    upper_contour = {}
    for v in range(len(full_contour)):
        if rightmost > leftmost:
            if v >= leftmost and v < rightmost:
                upper_contour[full_contour[v]] = 1
        else:
            if v >= leftmost or v < rightmost:
                upper_contour[full_contour[v]] = 1
    return (edges, upper_contour)

def write(filename):
    global V
    file = open(filename, "wb")
    stdout = sys.stdout
    sys.stdout=file

    print "# Exported from Blender %s" % Blender.Get("version")
    print "# on %s (%s)" % (time.ctime(), time.tzname[0])
    print "# do not edit, edit %s instead" % Blender.Get("filename")
    print

    # Retrieve a list of all mesh objects in Blender.
    meshobs = [ob for ob in Blender.Object.Get() if ob.getType() == "Mesh"]

    # Export all current Blender materials.
    mnum = 0
    materials = {}
    print "# Materials"
    print "{"
    for m in Blender.Material.Get():
        t = []
        for tex in m.getTextures()[:2]:
            if tex and tex.tex.getImage():
                t += [tex.tex.getImage().name.split(".")[0]]
            else:
                t += [""]
        print """\t{"%s", "%s", %f} # %d \"%s\"""" % (t[0], t[1],
            m.R + m.G + m.B, mnum, m.getName())
        materials[m.name] = mnum
        mnum += 1
    print "}"
    print

    # Export all vertices of all mesh objects.
    V = {}
    print "# Vertices"
    print "{"
    i = 0
    for ob in meshobs:
        print "\t# Mesh \"%s\"" % ob.name
        matrix = ob.matrix
        mesh = ob.getData()
        for v in mesh.verts:
            pos = transform(matrix, [v.co.x, v.co.y, v.co.z, 1])
            V[i] = pos
            print "\t{%d, %d} # %d" % (round(pos[0] * SCALE_X),
                round(pos[1] * SCALE_Y), i)
            i += 1
    print "}"
    print

    # Export all faces of all mesh objects (split into triangles).
    print "# Triangles"
    print "{"

    base_i = 0
    for ob in meshobs:
        print "\t# Mesh \"%s\"" % ob.name

        mesh = ob.getData()

        meshmaterials = mesh.getMaterials(1)

        edge_height = get_prop(ob, "edge_height", DEFAULT_EDGE_HEIGHT)
        foreground = get_prop(ob, "foreground", 0)
        background = get_prop(ob, "background", 0)
        top = get_prop(ob, "top", 0)

        edges, upper_contour = make_contour(ob, base_i)

        for f in mesh.faces:
            # First triangle.
            tri1 = [base_i + f.v[2].index, base_i + f.v[1].index, base_i +
                f.v[0].index]
            if direction(tri1) > 0:
                tri1.reverse()
            tris = [tri1]

            # If the face has 4 vertices, add another triangle.
            if len(f.v) > 3:
                tri2 = [tri1[2], base_i + f.v[3].index, tri1[0]]
                if direction(tri2) > 0:
                    tri2.reverse()
                tris += [tri2]

            for tri in tris:
                # Handle one triangle.
                print "\t{"
                for i in range(3):
                    v = tri[i]
                    if i == 2:
                        next = tri[0]
                    else:
                        next = tri[i + 1]
                    flags = ""
                    edge_ok = False
                    if v in edges and next in edges[v]: edge_ok = True
                    if top and not v in upper_contour: edge_ok = False
                    if edge_ok:
                        flags += " \"edge\""
                        if background == 0 and foreground == 0:
                             flags += " \"collidable\""
                    if foreground != 0:
                        flags += " \"foreground\""
                    if flags != "":
                        flags = ", " + flags
                    print "\t\t{%d, %d%s}," % (v, edge_height, flags)

                try:
                    print "\t\t%d" % materials[meshmaterials[f.mat].name]
                except IndexError:
                    message("You forgot to assign a material to mesh %s (or \
one of its faces)" % ob.name)
                print "\t}"
        base_i += len(mesh.verts)
    print "}"
    print

    # Retrieve a list of all empty objects in Blender.
    gameobs = [ob for ob in Blender.Object.Get() if ob.getType() == "Empty"]

    obtypes = {}
    n = 0

    print "# Object types"
    print "{"
    for ob in gameobs:
        if ob.name.lower().startswith("player"):
            continue
        if ob.name.lower().startswith("exit"):
            continue
        bitmap = get_prop(ob, "bitmap", "")
        sound = get_prop(ob, "sound", "")

        if not (bitmap, sound) in obtypes:
            print "\t{ \"%s\", \"%s\" } # %d" % (bitmap, sound, n)
            obtypes[(bitmap, sound)] = n
            n += 1
    print "}"
    print

    collectibles_count = 0
    print "# Objects"
    print "{"
    for ob in gameobs:
        if ob.name.lower().startswith("player"):
            continue
        x, y, z = ob.getLocation()
        anglex, angley, anglez = ob.getEuler()
        deco = get_prop(ob, "deco", 0)
        if ob.name.lower().startswith("exit"):
            type = -1
        else:
            bitmap = get_prop(ob, "bitmap", "")
            sound = get_prop(ob, "sound", "")
            type = obtypes[(bitmap, sound)]
            bonus = get_prop(ob, "bonus", 0)
            if bonus == 0 and deco == 0: collectibles_count += 1
        foreground = get_prop(ob, "foreground", 0)
        flags = ""
        if deco == 0: flags += "\"collidable\""
        if foreground != 0: flags += "\"foreground\""
        if flags != "":
            flags = ", " + flags

        print "\t{ %d, %d, %d, %d%s } # \"%s\"" % (round(x * SCALE_X), round(y * SCALE_Y),
            round(anglez), type, flags, ob.name)
    print "}"
    print

    print "# Level"
    print "{"
    for ob in gameobs:
        if ob.name.lower().startswith("player"):
            x, y, z = ob.getLocation()
            print "\t%d, %d, %d" % (round(x * SCALE_X), round(y * SCALE_Y),
                collectibles_count)
            break
    print "}"

    sys.stdout = stdout

def fs_callback(filename):
    # Mesh access is better done outside edit mode I believe.
    editmode = Blender.Window.EditMode()
    if editmode: Blender.Window.EditMode(0)
    try:
        write(filename)
    except MessageException:
        pass
    if editmode: Blender.Window.EditMode(1)

Blender.Window.FileSelector(fs_callback, "Export Allegro Demo Game Level",
    "mylevel.txt")
