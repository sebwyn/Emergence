file(GLOB src_files src/*.cpp src/rendering/*.cpp)

add_executable(glslang::validator IMPORTED)
find_program(GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
set_property(TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")

function(add_shaders_target TARGET)
  cmake_parse_arguments("SHADER" "" "CHAPTER_NAME" "SOURCES" ${ARGN})
  set(SHADERS_DIR shaders)
  message(${SHADERS_DIR})
  add_custom_command(
    OUTPUT ${SHADERS_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )
  add_custom_command(
    OUTPUT ${SHADERS_DIR}/frag.spv ${SHADERS_DIR}/vert.spv
    COMMAND glslang::validator
    ARGS --target-env vulkan1.0 ${SHADER_SOURCES} --quiet
    WORKING_DIRECTORY ${SHADERS_DIR}
    DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
    COMMENT "Compiling Shaders"
    VERBATIM
  )
  add_custom_target(${TARGET} DEPENDS ${SHADERS_DIR}/frag.spv ${SHADERS_DIR}/vert.spv)
endfunction()

function(add_chapter CHAPTER_NAME)
  cmake_parse_arguments(CHAPTER "" "SHADER" "LIBS;TEXTURES;MODELS" ${ARGN})

  add_executable(${CHAPTER_NAME} ${api_sources} ${src_files})
  set_target_properties(
    ${CHAPTER_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CHAPTER_NAME}
  )
  set_target_properties(${CHAPTER_NAME} PROPERTIES CXX_STANDARD 17)
  target_link_libraries(${CHAPTER_NAME} vulkan glfw)
  target_include_directories(${CHAPTER_NAME} PRIVATE ${STB_INCLUDEDIR})

  if(DEFINED CHAPTER_SHADER)
    set(CHAPTER_SHADER_TARGET ${CHAPTER_NAME}_shader)
    file(GLOB SHADER_SOURCES ${CHAPTER_SHADER}.frag ${CHAPTER_SHADER}.vert)
    add_shaders_target(${CHAPTER_SHADER_TARGET} CHAPTER_NAME ${CHAPTER_NAME} SOURCES ${SHADER_SOURCES})
    add_dependencies(${CHAPTER_NAME} ${CHAPTER_SHADER_TARGET})
  endif()
  if(DEFINED CHAPTER_LIBS)
    target_link_libraries(${CHAPTER_NAME} ${CHAPTER_LIBS})
  endif()
  if(DEFINED CHAPTER_MODELS)
    file(COPY ${CHAPTER_MODELS} DESTINATION ${CMAKE_BINARY_DIR}/${CHAPTER_NAME}/models)
  endif()
  if(DEFINED CHAPTER_TEXTURES)
    file(COPY ${CHAPTER_TEXTURES} DESTINATION ${CMAKE_BINARY_DIR}/${CHAPTER_NAME}/textures)
  endif()
  target_precompile_headers(${CHAPTER_NAME} PUBLIC pch.hpp)
endfunction()

add_chapter(
  client
  SHADER resources/shaders/27_shader_depth
  MODELS resources/viking_room.obj
  TEXTURES resources/viking_room.png
  LIBS glm::glm ncurses
)

#add_executable(EmergenceClient ${src_files} ${api_sources})
#target_link_libraries(EmergenceClient ncurses glfw vulkan)
#target_precompile_headers(EmergenceClient PUBLIC pch.hpp)
