
env = Environment()
env['CPPPATH'] = ['#/external', '#/generated', '#src']
env['LIBS'] = ['GL', 'glfw', 'boost_regex', 'IL', 'OpenCL', 'Xrandr']
env['CCFLAGS'] = ['-ggdb']

python = 'python2.7'

env.Command(['#/generated/flextGL.c', '#/generated/flextGL.h'],
            ['#/src/extensions.txt'],
            python+' tools/flextGL/flextGLgen.py src/extensions.txt -Dgenerated -Tglfw')
             
env.Command(['#/generated/Config.cpp', '#/generated/Config.h'],
            ['#/src/config.xml'],
            python+' tools/configGen/configGen.py src/config.xml -Dgenerated -Hh -Ccpp')
             
env.Command(['#/generated/format_map.cpp', '#/generated/format_map.h'],
            Glob('tools/format*'),
            python+' tools/formatMapGen.py -Dgenerated -Hh -Ccpp')

env.Program('micropolis', Glob('src/*cpp') + 
            ['generated/flextGL.c', 
             'generated/Config.cpp', 
             'generated/format_map.cpp'])
