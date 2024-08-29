project('rudyscung', 'c',
        default_options: [
                'c_std=gnu2x',
                'default_library=static'
        ])

cc = meson.get_compiler('c')

m_dep = cc.find_library('m', required: false)
gl_dep = dependency('gl')
glew_dep = dependency('glew')
sdl2main_dep = dependency('sdl2main')
sdl2_dep = dependency('sdl2')

sources = []

subdir('lib')
subdir('src')

executable('rudyscung', sources,
        # win_subsystem: 'windows',
        dependencies: [
                m_dep,
                gl_dep,
                glew_dep,
                sdl2main_dep,
                sdl2_dep
        ])