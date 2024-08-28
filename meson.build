project('rudyscung', 'c',
        default_options: [
                'c_std=gnu2x',
                'default_library=static'
        ])

cc = meson.get_compiler('c')

m_dep = cc.find_library('m', required: false)
glew_dep = dependency('glew')
sdl2main_dep = dependency('sdl2main')
sdl2_dep = dependency('sdl2')
sdl2_image_dep = dependency('sdl2_image')

sources = []

subdir('src')

executable('rudyscung', sources,
        # win_subsystem: 'windows',
        dependencies: [
                m_dep,
                glew_dep,
                sdl2main_dep,
                sdl2_dep,
                sdl2_image_dep
        ])