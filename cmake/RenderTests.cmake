option(SOLAR_RENDER_TESTS "Enable real OpenGL launch/asset smoke tests (needs a video driver)" OFF)
set(SOLAR_TEST_VIDEO_DRIVER "" CACHE STRING "SDL test driver; offscreen for headless Mesa Linux")
if(SOLAR_RENDER_TESTS)
  find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
  add_test(NAME render_smoke COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tools/render_smoke.py" "$<TARGET_FILE:solar_system_sim>"
    --output "${PROJECT_BINARY_DIR}/render-smoke" --suite quick
    --video-driver "${SOLAR_TEST_VIDEO_DRIVER}")
  set_tests_properties(render_smoke PROPERTIES TIMEOUT 300 LABELS "render" RUN_SERIAL TRUE)
endif()
