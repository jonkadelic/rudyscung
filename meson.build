project('rudyscung', 'c',
        default_options: [
                'c_std=c2x',
                'default_library=static'
        ]
)

host_system = host_machine.system()

cc = meson.get_compiler('c')

m_dep = cc.find_library('m', required: false)
cglm_dep = dependency('cglm')
gl_dep = dependency('gl')
glew_dep = dependency('glew', static: true)
if host_system != 'darwin'
        sdl2main_dep = dependency('sdl2main')
endif
sdl2_dep = dependency('sdl2')

common_sources = []
client_sources = []
server_sources = []

subdir('lib')
subdir('src')

if host_system == 'darwin'
        executable('rudyscung', [common_sources, client_sources],
                dependencies: [
                        m_dep,
                        cglm_dep,
                        gl_dep,
                        glew_dep,
                        sdl2_dep
                ]
        )
else
        executable('rudyscung', [common_sources, client_sources],
                c_args: [
                        '-DGLEW_STATIC'
                ],
                dependencies: [
                        m_dep,
                        cglm_dep,
                        gl_dep,
                        glew_dep,
                        sdl2main_dep,
                        sdl2_dep
                ],
                link_args: [
                        '-static'
                ]
        )
endif
