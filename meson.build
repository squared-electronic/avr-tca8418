project(
    'avr-tca8418',
    'cpp',
    'c',
    version: '0.5',
    default_options: ['c_std=gnu11', 'warning_level=1'],
)

add_project_arguments(
    ['-D__AVR_ATmega644P__', '-mmcu=atmega644p', '-DF_CPU=7372800', '-Wimplicit-fallthrough'],
    language: ['cpp', 'c'],
)

src = files('src/TCA8418.cpp', 'src/twi/twi_master.c')

tca_library_inc = [
    'src',
]

avr_tca8418_lib = static_library(
    'avr_tca8418_lib',
    src,
    pic: false,
    include_directories: include_directories(tca_library_inc),
)

avr_tca8418_dep = declare_dependency(
    link_with: avr_tca8418_lib,
    include_directories: include_directories(tca_library_inc),
)

subdir('test')