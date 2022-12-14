add_executable(game)


# must match with pio filename and executable name from above
pico_generate_pio_header(game ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(game ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(game ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

# must match with executable name and source file names
target_sources(game PRIVATE game.c vga_graphics.c)

# must match with executable name
target_link_libraries(game PRIVATE 
    pico_stdlib
    pico_divider
    pico_multicore
    pico_bootsel_via_double_reset
    hardware_pio
    hardware_dma
    hardware_adc
    hardware_irq
    hardware_spi
    hardware_clocks
    hardware_pll)

# must match with executable name
pico_add_extra_outputs(game)

add_compile_options(-Ofast)
