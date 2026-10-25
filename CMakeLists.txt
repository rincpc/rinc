cmake_minimum_required(VERSION 3.13)

# Rinc OS. One project, two builds:
#   RINC_PLATFORM=rp2350   the Adafruit Fruit Jam, with the Pico SDK (the default when PICO_SDK_PATH is set)
#   RINC_PLATFORM=desktop  a native build, for working on the shared code and apps
# RIO_DIR picks the rio checkout: the lib/rio submodule by default, or a working copy while developing both.
set(RIO_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/rio CACHE PATH "rio checkout to build against")
if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
  set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()
if(NOT RINC_PLATFORM)
  if(PICO_SDK_PATH)
    set(RINC_PLATFORM rp2350)
  else()
    set(RINC_PLATFORM desktop)
  endif()
endif()
set(RINC_PLATFORM ${RINC_PLATFORM} CACHE STRING "rp2350 or desktop")

if(RINC_PLATFORM STREQUAL rp2350)
  set(PICO_BOARD adafruit_fruit_jam CACHE STRING "Board type")
  set(PICO_PIO_USB_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/Pico-PIO-USB) # USB host on the USB-A ports
  include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
endif()

# with MSVC, the static C runtime: one self-contained exe (other compilers ignore this; it has to come
# before project(), which is also before MSVC is known)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(rinc C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(RINC_PLATFORM STREQUAL rp2350)
  pico_sdk_init()
endif()

include(${RIO_DIR}/library.cmake)
# the most host procs a vm holds, on every Rinc (rio_MAX_FFI, below; tools/web.py's too):
# the pc test fails if rinc registers more
set(RINC_MAX_FFI 96)
# rio's fixed caps, the same on every Rinc (the Fruit Jam's, MCU-sized, but room for Rinc's host procs),
# so an app behaves the same everywhere: its log lines and error texts as long, say
target_compile_definitions(riocore PUBLIC rio_SMALL rio_MAX_FFI=${RINC_MAX_FFI})
# the build's id (rinc_buildId): made again with every build, so another build's cached app images go
set(RINC_BUILD_ID ${CMAKE_BINARY_DIR}/build_id.c)
execute_process(COMMAND ${CMAKE_COMMAND} -DOUT=${RINC_BUILD_ID} -P ${CMAKE_CURRENT_LIST_DIR}/tools/buildid.cmake)
add_custom_target(rinc_build_id COMMAND ${CMAKE_COMMAND} -DOUT=${RINC_BUILD_ID} -P ${CMAKE_CURRENT_LIST_DIR}/tools/buildid.cmake
                  BYPRODUCTS ${RINC_BUILD_ID})
# pc.rio, built in: the pc module every app sees (its host procs, and its types)
set(RINC_PC_SOURCE ${CMAKE_BINARY_DIR}/pc_source.c)
add_custom_command(OUTPUT ${RINC_PC_SOURCE}
  COMMAND ${CMAKE_COMMAND} -DIN=${CMAKE_CURRENT_LIST_DIR}/src/core/pc.rio -DOUT=${RINC_PC_SOURCE} -DNAME=rinc_pcSource -P ${CMAKE_CURRENT_LIST_DIR}/tools/embed_text.cmake
  DEPENDS src/core/pc.rio tools/embed_text.cmake)
add_library(rinccore STATIC src/core/rinc.c src/core/console.c src/core/font.c src/core/audio.c src/core/png.c ${RINC_BUILD_ID} ${RINC_PC_SOURCE})
add_dependencies(rinccore rinc_build_id)
target_include_directories(rinccore PUBLIC src/core)
target_link_libraries(rinccore PUBLIC riocore)
if(NOT MSVC)
  target_compile_options(rinccore PRIVATE -Wall -Wextra)
endif()

if(RINC_PLATFORM STREQUAL rp2350)
  option(RINC_RIO_IN_RAM "run rio's interpreter loop from SRAM instead of flash" ON)
  if(RINC_RIO_IN_RAM)
    target_compile_definitions(riocore PRIVATE rio_HOT_SECTION=".time_critical.rio")
  endif()
  target_compile_definitions(rinccore PRIVATE rinc_HOT_SECTION=".time_critical.rinc") # the sound mixer, in RAM
  # video.c: HDMI out through the HSTX, on core 1. keyboard.c: TinyUSB's host stack over PIO-USB, its
  # sources compiled here (tinyusb_host would turn the native port into a host too: it stays a device,
  # for the serial port); tusb_config.h in src/rp2350 configures both
  set(TUSB ${PICO_SDK_PATH}/lib/tinyusb/src)
  # the firmware's bundle (docs/spec/apps.md#bundles): none by default, as on the desktop and the web: the
  # apps and help texts go on the SD card (tools/install.py). A firmware can still carry apps, such as a
  # game of its own: RINC_BUNDLE's .rio files (the card's apps of the same names take their place), and
  # RINC_HOME the one home means
  set(RINC_BUNDLE "" CACHE STRING "the firmware's apps, their .rio files, separated by | (none by default)")
  set(RINC_HOME "" CACHE STRING "the firmware's home app, by name (by default the only app, else one named home)")
  # and its help texts, for the help app: each name=folder's Markdown, as help/<name>/... (on the card by
  # default, in /rinc/help: rinc=docs|rio=lib/rio/docs, say)
  set(RINC_HELP "" CACHE STRING
    "the firmware's help texts: name=folder pairs, separated by |, each folder's .md files (none by default)")
  string(REPLACE "|" ";" bundleApps "${RINC_BUNDLE}")
  set(helpDocs "") # (so the bundle is made again when a help text changes: the apps' own, and the rest)
  foreach(app ${bundleApps})
    get_filename_component(dir "${app}" DIRECTORY)
    file(GLOB_RECURSE more "${dir}/help/*.md" "${dir}/parts/*.rio") # (and its other files: system apps' parts)
    list(APPEND helpDocs ${more})
  endforeach()
  string(REPLACE "|" ";" helpSets "${RINC_HELP}")
  foreach(set ${helpSets})
    string(REGEX REPLACE "^[^=]*=" "" dir "${set}")
    file(GLOB_RECURSE more "${dir}/*.md")
    list(APPEND helpDocs ${more})
  endforeach()
  set(bundle ${CMAKE_CURRENT_BINARY_DIR}/bundle.c)
  add_custom_command(OUTPUT ${bundle}
    COMMAND ${CMAKE_COMMAND} "-DAPPS=${RINC_BUNDLE}" "-DHELP=${RINC_HELP}" -DHOME=${RINC_HOME} -DOUT=${bundle} -P ${CMAKE_CURRENT_LIST_DIR}/tools/bundle.cmake
    DEPENDS ${bundleApps} ${helpDocs} tools/bundle.cmake VERBATIM)
  add_executable(rinc ${bundle} src/rp2350/main.c src/rp2350/video.c src/rp2350/keyboard.c src/rp2350/audio.c src/rp2350/hdmi_packet.c src/rp2350/gamepad.c src/rp2350/sd.c
    lib/fatfs/ff.c lib/fatfs/ffunicode.c
    ${TUSB}/host/usbh.c ${TUSB}/host/hub.c ${TUSB}/class/hid/hid_host.c ${TUSB}/portable/raspberrypi/pio_usb/hcd_pio_usb.c)
  target_include_directories(rinc BEFORE PRIVATE src/rp2350 lib/fatfs) # (sd.c: the SD card, through FatFs)
  target_compile_definitions(rinc PRIVATE PICO_STACK_SIZE=0x1000 PICO_CORE1_STACK_SIZE=0x800 PICO_HEAP_SIZE=0x100) # (no malloc: see main.c)
  pico_generate_pio_header(rinc ${CMAKE_CURRENT_LIST_DIR}/src/rp2350/i2s.pio) # audio.c: sound out of the 3.5 mm jack
  target_link_libraries(rinc PRIVATE rinccore pico_stdlib pico_multicore hardware_dma hardware_irq hardware_clocks hardware_pio hardware_i2c hardware_spi
    tinyusb_pico_pio_usb)
  pico_enable_stdio_usb(rinc 1)  # the REPL talks over USB serial
  pico_enable_stdio_uart(rinc 0)
  pico_add_extra_outputs(rinc)   # rinc.uf2, to drop on the board in BOOT mode
  # cmake --build build --target flash: picotool reboots a running rinc (or anything else built with
  # USB stdio) into BOOT mode over USB, writes rinc, and starts it. A board that isn't answering on USB
  # needs BOOT + RESET by hand first.
  if(TARGET picotool)
    add_custom_target(flash COMMAND picotool load -f -x $<TARGET_FILE:rinc> DEPENDS rinc USES_TERMINAL)
  endif()
else()
  add_executable(rinc src/desktop/main.c src/desktop/files.c src/desktop/about.c)
  target_link_libraries(rinc PRIVATE rinccore)
  # the window: SDL3, built from source here and linked in statically, for a single binary
  option(RINC_SDL "a window, through SDL3 (off: headless only)" ON)
  if(RINC_SDL)
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    foreach(off CAMERA HAPTIC SENSOR POWER DIALOG TRAY GPU)
      set(SDL_${off} OFF CACHE BOOL "" FORCE)
    endforeach()
    FetchContent_Declare(SDL3
      URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
      URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68)
    FetchContent_MakeAvailable(SDL3)
    target_link_libraries(rinc PRIVATE SDL3::SDL3-static)
    target_compile_definitions(rinc PRIVATE rinc_SDL=1)
  endif()
  # rinc -web: the web players built in, gzipped (python tools/web.py --wasm=rinc.wasm
  # --wasm-full=rinc-full.wasm; CI builds them), so making a page needs neither zig nor Python. A page gets
  # the VM-only player, unless an app runs Rio itself (pc.rio*): then the one with the compiler
  set(RINC_WEB_WASM "" CACHE FILEPATH "VM-only rinc.wasm to build in, for rinc -web")
  set(RINC_WEB_WASM_FULL "" CACHE FILEPATH "rinc-full.wasm (with the compiler) to build in, for rinc -web")
  if(RINC_WEB_WASM)
    get_filename_component(wasm ${RINC_WEB_WASM} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR}) # (relative: to the source)
    set(full "")
    if(RINC_WEB_WASM_FULL)
      get_filename_component(full ${RINC_WEB_WASM_FULL} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
    endif()
    set(player ${CMAKE_CURRENT_BINARY_DIR}/web_player.c)
    add_custom_command(OUTPUT ${player}
      COMMAND ${CMAKE_COMMAND} -DWASM=${wasm} -DWASM_FULL=${full} -DPAGE=${CMAKE_CURRENT_LIST_DIR}/src/web/page.html -DOUT=${player}
        -P ${CMAKE_CURRENT_LIST_DIR}/tools/embed.cmake
      DEPENDS ${wasm} ${full} src/web/page.html tools/embed.cmake)
    target_sources(rinc PRIVATE ${player})
    target_compile_definitions(rinc PRIVATE rinc_WEB=1)
  endif()

  # tests: every app runs headless for a few seconds; an app exported runs the same; with the web
  # player, a page gets made
  enable_testing()
  file(GLOB apps ${CMAKE_CURRENT_LIST_DIR}/apps/*.rio ${CMAKE_CURRENT_LIST_DIR}/apps/*/*.rio) # (an app of one file, or a folder's main one)
  foreach(app ${apps})
    get_filename_component(dir ${app} DIRECTORY)
    get_filename_component(folder ${dir} NAME)
    get_filename_component(name ${app} NAME_WE)
    if(NOT folder STREQUAL "apps" AND NOT folder STREQUAL name)
      continue() # (a folder app's other files)
    endif()
    get_filename_component(name ${app} NAME_WE)
    add_test(NAME app_${name} COMMAND rinc ${app} -frames 180 -o ${CMAKE_CURRENT_BINARY_DIR}/${name}.ppm)
  endforeach()
  # the built-in apps: home runs, and the console, typed at, runs Rio and its commands
  # storage in a folder of the tests' own (never the user's): apps installed there, then run by name
  set(data ${CMAKE_CURRENT_BINARY_DIR}/test-data)
  add_test(NAME install_clear COMMAND ${CMAKE_COMMAND} -E rm -rf ${data}) # (each run from nothing)
  add_test(NAME install COMMAND rinc -data ${data} -install ${CMAKE_CURRENT_LIST_DIR}/system/home/home.rio
    ${CMAKE_CURRENT_LIST_DIR}/system/console/console.rio ${CMAKE_CURRENT_LIST_DIR}/system/edit/edit.rio ${CMAKE_CURRENT_LIST_DIR}/apps/hello.rio)
  set_tests_properties(install PROPERTIES DEPENDS install_clear)
  add_test(NAME app_by_name COMMAND rinc -data ${data} -app hello -frames 2)
  set_tests_properties(app_by_name PROPERTIES DEPENDS install PASS_REGULAR_EXPRESSION "drawn at")
  add_test(NAME app_home COMMAND rinc -data ${data} -frames 30)
  set_tests_properties(app_home PROPERTIES DEPENDS install)
  add_test(NAME app_console COMMAND rinc -data ${data} -app console -frames 60
    -type "x := 6 * 7\\nsq :: proc(n: Int) -> Int\\nreturn n * n\\n\\x08\\x08end\\nsq(x)\\nnope\\nls\\n") # (end after its indent's backspaced: still end)
  set_tests_properties(app_console PROPERTIES DEPENDS install PASS_REGULAR_EXPRESSION "1764[\r\n]+> nope[\r\n]+1:1: undefined name")
  # the editor: a new app (Ctrl+N), typed, an undo (Ctrl+Z), then saved as t1 and run (Ctrl+R)
  add_test(NAME app_edit COMMAND rinc -data ${data} -app edit -frames 200
    -type "\\e\\x0elog(1)\\x1alog(6 * 7 + 1000)\\x12t1\\n")
  set_tests_properties(app_edit PROPERTIES DEPENDS install PASS_REGULAR_EXPRESSION "(^|[\r\n])1042[\r\n]")
  # help texts: bundled with -docs, listed and read by an app (pc.helps, helpName, helpRead); and the help app runs
  set(helps ${CMAKE_CURRENT_BINARY_DIR}/helps-game${CMAKE_EXECUTABLE_SUFFIX})
  add_test(NAME export_help COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/tests/helps.rio -docs rinc=${CMAKE_CURRENT_LIST_DIR}/docs -export ${helps})
  add_test(NAME export_help_runs COMMAND ${helps} -frames 1 -data ${CMAKE_CURRENT_BINARY_DIR}/helps-data)
  set_tests_properties(export_help_runs PROPERTIES DEPENDS export_help PASS_REGULAR_EXPRESSION "rinc/SUMMARY true # Summary[\r\n]+true -1 -1")
  # colors: moved over every part (down through PICO-8's and the grays into the cube, deeper, turned),
  # then a color picked with nothing waiting for it
  add_test(NAME app_colors COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/system/colors/colors.rio -frames 60
    -data ${CMAKE_CURRENT_BINARY_DIR}/colors-data -type "\\x1b[A\\x1b[B\\x1b[B\\x1b[B\\x1b[C\\t\\t \\x1b[Z\\n")
  # -hold: buttons held in the frames it names
  add_test(NAME hold COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/tests/hold.rio -frames 6 -hold "b 2-3, x 4-4")
  set_tests_properties(hold PROPERTIES PASS_REGULAR_EXPRESSION "frame 1 b 0 x 0[^f]*frame 2 b 1 x 0[^f]*frame 3 b 1 x 0[^f]*frame 4 b 0 x 1[^f]*frame 5 b 0 x 0")
  add_test(NAME app_help COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/system/help/help.rio -frames 30 -type "\\t\\n")
  # an app's own help, in its folder (apps/demo/help/intro.md): demo/intro
  set(apphelp ${CMAKE_CURRENT_BINARY_DIR}/apphelp-data)
  add_test(NAME app_help_setup COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/tests/apphelp ${apphelp}/apps)
  add_test(NAME app_help_own COMMAND rinc -data ${apphelp} -app demo -frames 1)
  set_tests_properties(app_help_own PROPERTIES DEPENDS app_help_setup PASS_REGULAR_EXPRESSION "found # Intro")
  # which app home is: the home setting's (homer, which goes home on its third frame, so starts again),
  # and rinc app/ for the session, a folder's app run from its files
  set(homedata ${CMAKE_CURRENT_BINARY_DIR}/home-data)
  add_test(NAME home_setup COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/tests/home ${homedata})
  add_test(NAME home_setting COMMAND rinc -data ${homedata} -frames 10)
  add_test(NAME home_session COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/tests/home/apps/homer -data ${CMAKE_CURRENT_BINARY_DIR}/home-session-data -frames 10)
  set_tests_properties(home_setting PROPERTIES DEPENDS home_setup PASS_REGULAR_EXPRESSION "homer started[\r\n]+homer started")
  set_tests_properties(home_session PROPERTIES PASS_REGULAR_EXPRESSION "homer started[\r\n]+homer started")
  # the theme setting's theme, read by pc.theme (roles it leaves out: Rinc's own); and Themes making one
  set(themedata ${CMAKE_CURRENT_BINARY_DIR}/theme-data)
  add_test(NAME theme_setup COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/tests/theme ${themedata})
  add_test(NAME theme_read COMMAND rinc -data ${themedata} -app show -frames 1)
  set_tests_properties(theme_read PROPERTIES DEPENDS theme_setup PASS_REGULAR_EXPRESSION "theme 3 200 7")
  add_test(NAME themes_setup COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/system/rinc/themes ${themedata}/apps/rinc/themes)
  add_test(NAME app_themes COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/system/themes/themes.rio -data ${themedata} -frames 30 -type "nfresh\\n")
  add_test(NAME app_themes_made COMMAND ${CMAKE_COMMAND} -E cat ${themedata}/save/rinc/themes/fresh.theme.rio)
  set_tests_properties(themes_setup PROPERTIES DEPENDS theme_setup)
  set_tests_properties(app_themes PROPERTIES DEPENDS themes_setup)
  set_tests_properties(app_themes_made PROPERTIES DEPENDS app_themes PASS_REGULAR_EXPRESSION "# fresh: from")
  # editing a locked theme (show/odd, the one in use) makes its copy, show/odd+, in show's save space
  set(themecopy ${CMAKE_CURRENT_BINARY_DIR}/theme-copy-data)
  add_test(NAME theme_copy_setup COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/tests/theme ${themecopy})
  add_test(NAME app_themes_copy COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/system/themes/themes.rio -data ${themecopy} -frames 30 -type "e")
  add_test(NAME app_themes_copied COMMAND ${CMAKE_COMMAND} -E cat ${themecopy}/save/show/themes/odd.theme.rio)
  set_tests_properties(app_themes_copy PROPERTIES DEPENDS theme_copy_setup)
  set_tests_properties(app_themes_copied PROPERTIES DEPENDS app_themes_copy PASS_REGULAR_EXPRESSION "# Odd: two roles")
  # an app's stash: none the first time, then what it wrote
  set(stash ${CMAKE_CURRENT_BINARY_DIR}/stash-data)
  add_test(NAME stash_clear COMMAND ${CMAKE_COMMAND} -E rm -rf ${stash})
  add_test(NAME stash_first COMMAND rinc -data ${stash} ${CMAKE_CURRENT_LIST_DIR}/tests/stash.rio -frames 1)
  add_test(NAME stash_again COMMAND rinc -data ${stash} ${CMAKE_CURRENT_LIST_DIR}/tests/stash.rio -frames 1)
  set_tests_properties(stash_first PROPERTIES DEPENDS stash_clear PASS_REGULAR_EXPRESSION "^none[\r\n]+0 0 0 -1[\r\n]+-1 true")
  set_tests_properties(stash_again PROPERTIES DEPENDS stash_first PASS_REGULAR_EXPRESSION "^here[\r\n]+0 0 0 -1[\r\n]+-1 true")
  # an app's memory: the -m report
  add_test(NAME memory_report COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/apps/chord -m)
  set_tests_properties(memory_report PROPERTIES PASS_REGULAR_EXPRESSION "fits in the 256 KB app space")
  set(game ${CMAKE_CURRENT_BINARY_DIR}/breakout-game${CMAKE_EXECUTABLE_SUFFIX})
  add_test(NAME export_native COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/apps/breakout.rio -export ${game})
  add_test(NAME export_native_runs COMMAND ${game} -frames 180 -o ${CMAKE_CURRENT_BINARY_DIR}/breakout-game.ppm)
  set_tests_properties(export_native_runs PROPERTIES DEPENDS export_native)
  # (the exported game draws exactly what rinc does running the app)
  add_test(NAME export_native_same COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_BINARY_DIR}/breakout.ppm ${CMAKE_CURRENT_BINARY_DIR}/breakout-game.ppm)
  set_tests_properties(export_native_same PROPERTIES DEPENDS "export_native_runs;app_breakout")
  # every rio example in the docs, run by a headless rinc (with Rio's runner, once lib/rio has it)
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND AND EXISTS ${RIO_DIR}/tests/docs.py)
    add_test(NAME docs COMMAND Python3::Interpreter ${RIO_DIR}/tests/docs.py $<TARGET_FILE:rinc>
      ${CMAKE_CURRENT_LIST_DIR}/docs ${CMAKE_CURRENT_LIST_DIR}/README.md -- -frames 2 -data ${CMAKE_CURRENT_BINARY_DIR}/docs-data)
    set_tests_properties(docs PROPERTIES PASS_REGULAR_EXPRESSION "all [0-9]+ doc examples passed")
  endif()
  if(Python3_FOUND) # apps' compiled images, cached
    add_test(NAME app_cache COMMAND Python3::Interpreter ${CMAKE_CURRENT_LIST_DIR}/tests/cache.py $<TARGET_FILE:rinc> ${CMAKE_CURRENT_BINARY_DIR}/cache-data)
    set_tests_properties(app_cache PROPERTIES PASS_REGULAR_EXPRESSION "the cache came through")
    add_test(NAME projects COMMAND Python3::Interpreter ${CMAKE_CURRENT_LIST_DIR}/tests/projects.py $<TARGET_FILE:rinc> ${CMAKE_CURRENT_BINARY_DIR}/projects-data)
    set_tests_properties(projects PROPERTIES PASS_REGULAR_EXPRESSION "projects came through")
    add_test(NAME files COMMAND Python3::Interpreter ${CMAKE_CURRENT_LIST_DIR}/tests/files.py $<TARGET_FILE:rinc> ${CMAKE_CURRENT_LIST_DIR}/system/files/files.rio ${CMAKE_CURRENT_BINARY_DIR}/files-data)
    set_tests_properties(files PROPERTIES PASS_REGULAR_EXPRESSION "files came through")
    add_test(NAME manifests COMMAND Python3::Interpreter ${CMAKE_CURRENT_LIST_DIR}/tests/manifests.py ${CMAKE_CURRENT_LIST_DIR}/system ${CMAKE_CURRENT_LIST_DIR}/apps)
    set_tests_properties(manifests PROPERTIES PASS_REGULAR_EXPRESSION "every files.txt lists its app")
    add_test(NAME multifile COMMAND Python3::Interpreter ${CMAKE_CURRENT_LIST_DIR}/tests/multifile.py $<TARGET_FILE:rinc> ${CMAKE_CURRENT_BINARY_DIR}/multifile-data)
    set_tests_properties(multifile PROPERTIES PASS_REGULAR_EXPRESSION "more files came through")
    add_test(NAME editor COMMAND Python3::Interpreter ${CMAKE_CURRENT_LIST_DIR}/tests/editor.py $<TARGET_FILE:rinc> ${CMAKE_CURRENT_LIST_DIR}/system ${CMAKE_CURRENT_BINARY_DIR}/editor-data)
    set_tests_properties(editor PROPERTIES PASS_REGULAR_EXPRESSION "the editor came through")
    # installing apps from a bundle (a zip, as a release's) and from an app's folder
    add_test(NAME install_bundle COMMAND Python3::Interpreter ${CMAKE_CURRENT_LIST_DIR}/tests/bundle.py $<TARGET_FILE:rinc> ${CMAKE_CURRENT_BINARY_DIR}/bundle-data)
    set_tests_properties(install_bundle PROPERTIES PASS_REGULAR_EXPRESSION "bundles came through")
    add_test(NAME colors_views COMMAND Python3::Interpreter ${CMAKE_CURRENT_LIST_DIR}/tests/colors.py $<TARGET_FILE:rinc> ${CMAKE_CURRENT_LIST_DIR}/system/colors ${CMAKE_CURRENT_BINARY_DIR}/colors-views-data)
    set_tests_properties(colors_views PROPERTIES PASS_REGULAR_EXPRESSION "colors came through")
  endif()
  # pc.rio, the host procs written out, matches what rinc registers
  add_executable(rinc_pc_test tests/pc.c)
  target_link_libraries(rinc_pc_test PRIVATE rinccore)
  target_compile_definitions(rinc_pc_test PRIVATE rinc_MAX_FFI=${RINC_MAX_FFI})
  add_test(NAME pc_declared COMMAND rinc_pc_test ${CMAKE_CURRENT_LIST_DIR}/src/core/pc.rio)
  set_tests_properties(pc_declared PROPERTIES PASS_REGULAR_EXPRESSION "pc.rio declares all [0-9]+ host procs")
  # typed keys, as a terminal sends them, with their modifiers
  add_executable(rinc_keys_test tests/keys.c)
  target_link_libraries(rinc_keys_test PRIVATE rinccore)
  add_test(NAME keys COMMAND rinc_keys_test)
  set_tests_properties(keys PROPERTIES PASS_REGULAR_EXPRESSION "all keys came through")
  # a bundle of two apps, exported: it starts at the one -home names
  set(pair ${CMAKE_CURRENT_BINARY_DIR}/pair-game${CMAKE_EXECUTABLE_SUFFIX})
  add_test(NAME export_bundle COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/apps/bounce.rio ${CMAKE_CURRENT_LIST_DIR}/apps/hello.rio -export ${pair} -home hello)
  add_test(NAME export_bundle_runs COMMAND ${pair} -frames 2 -data ${CMAKE_CURRENT_BINARY_DIR}/pair-data)
  set_tests_properties(export_bundle_runs PROPERTIES DEPENDS export_bundle PASS_REGULAR_EXPRESSION "drawn at")
  if(RINC_WEB_WASM)
    add_test(NAME export_web COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/apps/breakout.rio -web ${CMAKE_CURRENT_BINARY_DIR}/breakout.html)
    if(RINC_WEB_WASM_FULL) # (the console runs Rio itself, so its page gets the player with the compiler)
      add_test(NAME export_web_full COMMAND rinc ${CMAKE_CURRENT_LIST_DIR}/system/home/home.rio ${CMAKE_CURRENT_LIST_DIR}/system/console/console.rio
        -web ${CMAKE_CURRENT_BINARY_DIR}/rinc.html)
    endif()
  endif()
endif()
