
test_src = files('main.cpp')

executable(
    'test-tca',
    test_src,
    # link_args: ['-fsanitize=address,undefined'],
    dependencies: [avr_tca8418_dep],
)
