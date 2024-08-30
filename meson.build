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

common_lib = library('common', common_sources,
        dependencies: [
                m_dep
        ],
        link_args: [
                '-static'
        ]
)

if host_system == 'darwin'
        executable('client', client_sources,
                link_with: common_lib,
                dependencies: [
                        m_dep,
                        cglm_dep,
                        gl_dep,
                        glew_dep,
                        sdl2_dep
                ]
        )
        executable('server', server_sources,
                link_with: common_lib,
                dependencies: [
                        m_dep
                ]
        )
else
        executable('client', client_sources,
                c_args: [
                        '-DGLEW_STATIC'
                ],
                link_with: common_lib,
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
        executable('server', server_sources,
                link_with: common_lib,
                dependencies: [
                        m_dep
                ],
                link_args: [
                        '-static'
                ]
        )
endif
