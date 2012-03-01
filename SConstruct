CFLAGS="-Wall"

if ARGUMENTS.get('debug', 0):
    env = Environment(CCFLAGS=CFLAGS + " -g -DDEBUG")
else:
    env = Environment(CCFLAGS=CFLAGS)

env.Program([
    "n64rd.c", "gspro.c"
])
