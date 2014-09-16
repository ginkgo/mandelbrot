Import('env')
Import('python3')
Import('config')

env.Command(['#/%s/generated/flextGL.c' % config, '#/%s/generated/flextGL.h' % config],
            ['#/src/GL/extensions.glprofile', '#/tools/flextGL/flextGLgen.py', '#/tools/flextGL/flext.py',
             Glob('#/tools/flextGL/templates/glfw3/*template')],
            python3+' tools/flextGL/flextGLgen.py src/GL/extensions.glprofile -D%s/generated -Tglfw3' % config)
             
env.Command(['#/%s/generated/Config.cpp' % config, '#/%s/generated/Config.h' % config],
            ['#/src/mandelbrot/config.xml', Glob('#/tools/configGen/*')],
            python3+' tools/configGen/configGen.py src/mandelbrot/config.xml -D%s/generated -Hh -Ccpp' % config)
             
env.Command(['#/%s/generated/GLConfig.cpp' % config, '#/%s/generated/GLConfig.h' % config],
            ['#/src/GL/config.xml', Glob('#/tools/configGen/*')],
            python3+' tools/configGen/configGen.py src/GL/config.xml -D%s/generated -Hh -Ccpp' % config)
             
             
env.Command(['#/%s/generated/format_map.cpp' % config, '#/%s/generated/format_map.h' % config],
            Glob('tools/format*'),
            python3+' tools/formatMapGen.py -D%s/generated -Hh -Ccpp' % config)


base = env.Object(Glob('src/base/*.cpp') + ['#/%s/generated/Config.cpp' % config])
GL = env.Object(Glob('src/GL/*.cpp') + ['#/%s/generated/flextGL.c' % config, '#/%s/generated/format_map.cpp' % config, '#/%s/generated/GLConfig.cpp' % config])

env.Program('#/mandelbrot_%s' % config,
            Glob('src/mandelbrot/*.cpp') + base + GL)
