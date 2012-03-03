CCFLAGS="-Wall"

## Enable debug mode
if ARGUMENTS.get('debug', 0):
    CCFLAGS += " -g -DDEBUG"

## Acquire a configure environment
env = Environment(CCFLAGS=CCFLAGS)
conf = Configure(env)

## Run configuration
if conf.CheckHeader('sys/io.h'):
    conf.env.Append(CCFLAGS=' -DHAS_SYSIO_H')

env = conf.Finish()

## Build
env.Program([
    "n64rd.c", "gspro.c"
])
