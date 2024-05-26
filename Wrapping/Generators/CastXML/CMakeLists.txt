###############################################################################
option(ITK_USE_SYSTEM_CASTXML "Use system castxml. If OFF, castxml is built as external project." OFF)
mark_as_advanced(ITK_USE_SYSTEM_CASTXML)
if(WIN32)
  set(exe .exe)
endif()
set(castxml_ep ${CMAKE_CURRENT_BINARY_DIR}/castxml/bin/castxml${exe})

if(ITK_USE_SYSTEM_CASTXML)
  # the path set for the EP build prevents find_program to do its job
  if("${CASTXML_EXECUTABLE}" STREQUAL "${castxml_ep}")
    unset(CASTXML_EXECUTABLE CACHE)
  endif()
  find_program(CASTXML_EXECUTABLE castxml)
  if(NOT CASTXML_EXECUTABLE)
    message(FATAL_ERROR "Could NOT find castxml. Turn ITK_USE_SYSTEM_CASTXML to OFF to build it with ITK.")
  endif()
else()
  include(ExternalProject)

  set(_download_castxml_binaries 0)
  set(_castxml_url)
  set(_castxml_hash)
  set(_castxml_version 0.6.5)

  # If 64 bit Linux build host, use the CastXML binary
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux" AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")

    set(_castxml_hash
      592fcb6c7f85b6a1670cef7e0692ec6d1c9ba2e250825032ed6dcf9581aa169540eded608510aa1208ea1174df48c16390ee7daf7a17c7114d93a83a8a8e109b
    )
    set(_castxml_url
        "https://github.com/CastXML/CastXMLSuperbuild/releases/download/v${_castxml_version}/castxml-linux.tar.gz")
    set(_download_castxml_binaries 1)
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux" AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")

    set(_castxml_hash
      229d5339e217660f09dd87e2e639d666921a8c4e6c328a754dcae4290bba6bcac9d3b8e953814314ecdbf908d5d8e0d7dacbf1fdf6040a2e20d7acb98fb32f7d
    )
    set(_castxml_url
        "https://github.com/CastXML/CastXMLSuperbuild/releases/download/v${_castxml_version}/castxml-linux-aarch64.tar.gz")
    set(_download_castxml_binaries 1)

    # If 64 bit Windows build host, use the CastXML binary
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows" AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "AMD64")

    set(_castxml_hash
      7c1970ad6f2e5f06a8704606db92df3400c4cd9716f88cac604924430c7e6865f8256a67282d28005714f0ed0a42f7f6e386f24ce80fb075371902d35674c6cc
    )
    set(_castxml_url
      "https://github.com/CastXML/CastXMLSuperbuild/releases/download/v${_castxml_version}/castxml-windows.zip"
    )
    set(_download_castxml_binaries 1)

    # If 64 bit Mac OS X build host ( >= 10.9, Mavericks), use the CastXML binary
  elseif(
    CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin"
    AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64"
    AND (NOT
         CMAKE_HOST_SYSTEM_VERSION
         VERSION_LESS
         "11.0.0"
        ))

    set(_castxml_hash
      c6986a796ab9a4f4deaf569534d628cc584088aa8b0e56026ea5ba19550b8ceeb41c34f46a85566a21205d6bb529717ee8944cfa9a9c7c27edb0504eece5544a
    )
    set(_castxml_url
        "https://github.com/CastXML/CastXMLSuperbuild/releases/download/v${_castxml_version}/castxml-macosx.tar.gz")
    set(_download_castxml_binaries 1)
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin" AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")

    set(_castxml_hash
        4c8c969f7e53cd758b516bada449b322d37ad19d6d46602660d83ece20ce07f3d55462493382a1c422048928962fd33f9704638e2e41637d1147473562a55f94
    )
    set(_castxml_url
        "https://github.com/CastXML/CastXMLSuperbuild/releases/download/v${_castxml_version}/castxml-macos-arm.tar.gz")
    set(_download_castxml_binaries 1)

  endif()

  # Download binaries
  if(_download_castxml_binaries AND NOT TARGET castxml)
    if(ITK_BINARY_DIR)
      itk_download_attempt_check(CastXML)
    endif()
    # Set the CMP0135 policy to NEW: set the timestamps of the
    # extracted files to their extraction timestamps. Used for
    # CMake-based projects such as CastXML.
    cmake_policy(PUSH)
    if(POLICY CMP0135)
      cmake_policy(SET CMP0135 NEW)
    endif()
    ExternalProject_Add(
      castxml
      URL ${_castxml_url}
      URL_HASH SHA512=${_castxml_hash}
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/castxml-prefix/src/castxml"
                      "${CMAKE_CURRENT_BINARY_DIR}/castxml")
    cmake_policy(POP)
    set(CASTXML_EXECUTABLE ${castxml_ep})
    set(CASTXML_EXECUTABLE
        ${castxml_ep}
        CACHE FILEPATH "castxml executable." FORCE)
    # Build from source
  elseif(NOT TARGET castxml)
    set(compiler_information)
    if(NOT CMAKE_CROSSCOMPILING)
      set(CMAKE_CXX_COMPILER_LAUNCHER_FLAG -DCMAKE_CXX_COMPILER_LAUNCHER:FILEPATH=${CMAKE_CXX_COMPILER_LAUNCHER})
      set(CMAKE_C_COMPILER_LAUNCHER_FLAG -DCMAKE_C_COMPILER_LAUNCHER:FILEPATH=${CMAKE_C_COMPILER_LAUNCHER})
      set(compiler_information
          -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
          ${CMAKE_CXX_COMPILER_LAUNCHER_FLAG}
          -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
          ${CMAKE_C_COMPILER_LAUNCHER_FLAG}
          )
    endif()
    # might be set to on by default when llvm/clang 3.6 are released
    # option(ITK_USE_SYSTEM_LLVM "Use system llvm and clang. If OFF, llvm and clang are built as external projects." ON)
    set(ITK_USE_SYSTEM_LLVM OFF)
    if(ITK_USE_SYSTEM_LLVM)
      find_package(LLVM REQUIRED)
      set(castxml_deps)
    else()
      # check the version of clang compiler
      if(APPLE
         AND (((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.2")
              OR ((CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang") AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0")))
        message(FATAL_ERROR "Mac OS X 10.9 (Mavericks) or newer is required to wrap ITK.")
      endif()
      # If we are building ITK
      if(ITK_BINARY_DIR)
        itk_download_attempt_check(LLVM)
        itk_download_attempt_check(Clang)
      endif()
      set(llvm_version 18.1.3)
      set(llvm_folder 18.1.3)
      set(LLVM_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm/lib/cmake/llvm/)

      if(${CMAKE_VERSION} VERSION_LESS 3.24)
        set(download_extract_timestamp_flag)
      else()
        set(download_extract_timestamp_flag DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
      endif()

      set(llvm_cmake_sha256 acfecb615d41c5b1a0a31e15324994ca06f7a3f37d8958d719b20de0d217b71b)
      ExternalProject_Add(llvm-cmake
        URL "https://github.com/llvm/llvm-project/releases/download/llvmorg-${llvm_folder}/cmake-${llvm_version}.src.tar.xz"
        URL_HASH SHA256=${llvm_cmake_sha256}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        ${download_extract_timestamp_flag}
        LOG_BUILD 0
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm-prefix/src/cmake
        )

      set(llvm_sha256 fa6db8951f5ef576ac6bad43d5e1ed83962754538c998fbfa0397cd4521abc00)
      ExternalProject_Add(llvm
        URL "https://github.com/llvm/llvm-project/releases/download/llvmorg-${llvm_folder}/llvm-${llvm_version}.src.tar.xz"
        URL_HASH SHA256=${llvm_sha256}
        DEPENDS llvm-cmake
        CMAKE_ARGS -Wno-dev
        CMAKE_GENERATOR "${CMAKE_GENERATOR}"
        CMAKE_CACHE_ARGS
          -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
          -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
          -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
          -DLLVM_STATIC_LINK_CXX_STDLIB:BOOL=ON
          -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
          -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
          -DLLVM_INSTALL_PREFIX:PATH=<INSTALL_DIR>
          -DLLVM_ENABLE_TERMINFO:BOOL=OFF
          -DLLVM_INCLUDE_TESTS:BOOL=OFF
          -DLLVM_INCLUDE_BENCHMARKS:BOOL=OFF
          -DLLVM_INCLUDE_DOCS:BOOL=OFF
          -DLLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN:BOOL=ON
        ${download_extract_timestamp_flag}
        LOG_BUILD 0
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm
        )


      set(clang_sha256 e43e1729713ac0241aa026fa2f98bb54e74a196a6fed60ab4819134a428eb6d8)
      ExternalProject_Add(clang
        URL "https://github.com/llvm/llvm-project/releases/download/llvmorg-${llvm_folder}/clang-${llvm_version}.src.tar.xz"
        URL_HASH SHA256=${clang_sha256}
        DEPENDS llvm
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm-prefix/src/cfe-${llvm_version}
        CMAKE_ARGS -Wno-dev
        CMAKE_GENERATOR "${CMAKE_GENERATOR}"
        CMAKE_CACHE_ARGS
          -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
          -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
          -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
          -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
          -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
          -DCLANG_INCLUDE_DOCS:BOOL=OFF
          -DCLANG_INCLUDE_TESTS:BOOL=OFF
          -DLLVM_INCLUDE_TESTS:BOOL=OFF
          -DLLVM_CONFIG:PATH=${CMAKE_CURRENT_BINARY_DIR}/llvm/bin/llvm-config
          -DLLVM_DIR:PATH=${LLVM_DIR}
          -DLLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN:BOOL=ON
        ${download_extract_timestamp_flag}
        LOG_BUILD 0
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm
        )

      set(castxml_deps llvm clang)
    endif()

    # If we are building ITK
    if(ITK_BINARY_DIR)
      itk_download_attempt_check(CastXML)
    endif()
    ExternalProject_Add(
      castxml
      GIT_REPOSITORY https://github.com/CastXML/CastXML.git
      GIT_TAG v${_castxml_version}
      UPDATE_COMMAND ""
      DEPENDS ${castxml_deps}
      CMAKE_ARGS -Wno-dev
      CMAKE_GENERATOR "${CMAKE_GENERATOR}"
      CMAKE_CACHE_ARGS
        ${compiler_information}
        -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DLLVM_DIR:PATH=${LLVM_DIR}
      INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/castxml)
    set(CASTXML_EXECUTABLE ${castxml_ep})
    set(CASTXML_EXECUTABLE
        ${castxml_ep}
        CACHE FILEPATH "castxml executable." FORCE)
  endif()
endif()
mark_as_advanced(CASTXML_EXECUTABLE)

###############################################################################
# store the current dir, so it can be reused later
set(ITK_WRAP_CASTXML_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "castxml source dir")


macro(itk_wrap_include_castxml include_file)
  if("${include_file}" MATCHES "<.*>")
    set(CASTXML_INCLUDES "${CASTXML_INCLUDES}#include ${include_file}\n")
  else()
    set(CASTXML_INCLUDES "${CASTXML_INCLUDES}#include \"${include_file}\"\n")
  endif()
endmacro()


macro(itk_wrap_simple_type_castxml wrap_class swig_name)
  set(CASTXML_TYPEDEFS "${CASTXML_TYPEDEFS}    typedef ${wrap_class} ${swig_name};\n")
  set(CASTXML_FORCE_INSTANTIATE "${CASTXML_FORCE_INSTANTIATE}    (void)sizeof(${swig_name});\n")
endmacro()


macro(itk_wrap_submodule_castxml module)
  # clear the typedefs and the includes
  set(CASTXML_TYPEDEFS )
  set(CASTXML_INCLUDES )
  set(CASTXML_FORCE_INSTANTIATE )
endmacro()

macro(itk_end_wrap_submodule_castxml module)
  # write the wrap_*.cxx file
  #
  # Global vars used: WRAPPER_INCLUDE_FILES WRAPPER_MODULE_NAME and WRAPPER_TYPEDEFS
  # Global vars modified: none

  # Create the cxx file which will be given to castxml.
  set(cxx_file "${WRAPPER_LIBRARY_OUTPUT_DIR}/${module}.cxx")
  configure_file("${ITK_WRAP_CASTXML_SOURCE_DIR}/wrap_.cxx.in" "${cxx_file}" @ONLY)

  # generate the xml file
  set(xml_file "${WRAPPER_LIBRARY_OUTPUT_DIR}/${module}.xml")

  set(_castxml_depends)
  if(NOT ITK_USE_SYSTEM_CASTXML)
    # ExternalProject target for CastXML.
    set(_castxml_depends castxml)
  endif()
  set(ccache_cmd)
  if(ITK_USE_CCACHE)
    set(_ccache_cmd ${CCACHE_EXECUTABLE})
  endif()
  set(_castxml_cc_flags ${CMAKE_CXX_FLAGS})
  separate_arguments(_castxml_cc_flags)
  if(MSVC)
    set(_castxml_cc --castxml-cc-msvc ( "${CMAKE_CXX_COMPILER}" ${_castxml_cc_flags} ) -fexceptions)
    if(MSVC90)
      # needed for VS2008 64 bit
      set(_castxml_cc ${_castxml_cc} "-D_HAS_TR1=0")
    endif()
  else()
    set(_castxml_cc --castxml-cc-gnu ( "${CMAKE_CXX_COMPILER}" ${_castxml_cc_flags} ))
  endif()
  set(_target)
  if(CMAKE_CROSSCOMPILING)
    if(NOT CMAKE_CXX_COMPILER_TARGET)
      message(FATAL_ERROR "Set the target triple in CMAKE_CXX_COMPILER_TARGET "
      " as described in http://clang.llvm.org/docs/CrossCompilation.html")
    endif()
    set(_target "--target=${CMAKE_CXX_COMPILER_TARGET}")
  endif()
  set(_build_env)
  if(APPLE)
    # If building on OS X, make sure that CastXML's calls to the compiler have the
    # settings that the output files will be compiled with.  This prevents headers
    # from one version of OS X from being used when building for another version.
    list(APPEND _build_env
      env
        "SDKROOT=${CMAKE_OSX_SYSROOT}"
        "MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
    )
  endif()

  set(_include ${${WRAPPER_LIBRARY_NAME}_SOURCE_DIR}/include)
  set(_hdrs )
  set(glob_hdrs )
  if(EXISTS ${_include})
    file(GLOB_RECURSE glob_hdrs ${_include}/*.h)
  endif()
  foreach(header IN LISTS glob_hdrs)
    get_filename_component(header_name ${header} NAME)
    # This should be updated when cmake_minimum_required is 3.3
    # cmake_policy(SET CMP0057 NEW)
    # https://cmake.org/cmake/help/v3.3/policy/CMP0057.html
    # Next block should be replaced by:
    ###################
    # if(${header_name} IN_LIST WRAPPER_INCLUDE_FILES)
    list(FIND WRAPPER_INCLUDE_FILES ${header_name} pos)
    if( pos GREATER -1 )
    # end of block of code to be replaced
      list(APPEND _hdrs ${header})
    endif()
  endforeach()

  add_custom_command(
    OUTPUT ${xml_file}
    COMMAND ${_build_env} ${_ccache_cmd} ${CASTXML_EXECUTABLE}
          -o ${xml_file}
          --castxml-gccxml
          ${_target}
          --castxml-start _wrapping_
          ${_castxml_cc}
          -w
          -c # needed for ccache to think we are not calling for link
          @${castxml_inc_file}
          ${cxx_file}
          VERBATIM
    DEPENDS ${_castxml_depends} ${cxx_file} ${castxml_inc_file} ${_hdrs}
  )

  list(APPEND CastXML_OUTPUT_FILES ${xml_file})

endmacro()


macro(itk_wrap_one_type_castxml  wrap_method wrap_class swig_name template_params)
  # insert a blank line to separate the classes
  set(CASTXML_TYPEDEFS "${CASTXML_TYPEDEFS}\n")
  # add a piece of code for type instantiation
  set(CASTXML_FORCE_INSTANTIATE "${CASTXML_FORCE_INSTANTIATE}\n")
endmacro()

macro(itk_wrap_module_castxml library_name)
  # create the files used to pass the file to include to castxml
  set(castxml_inc_file "${WRAPPER_LIBRARY_OUTPUT_DIR}/${library_name}.castxml.inc")
  get_directory_property(include_dir_list INCLUDE_DIRECTORIES)
  list(REMOVE_DUPLICATES include_dir_list)

  set(c)
  foreach(dir ${include_dir_list})
    set(c "${c}\"-I${dir}\"\n")
  endforeach()
  set(c "${c}-Qunused-arguments\n")
  set(c "${c}-DITK_WRAPPING_PARSER\n")
  set(c "${c}-DITK_MANUAL_INSTANTIATION\n")

  set(CONFIG_CASTXML_INC_CONTENTS "${c}")
  configure_file("${ITK_WRAP_CASTXML_SOURCE_DIR}/cast_xml.inc.in" "${castxml_inc_file}" @ONLY)

  set(CastXML_OUTPUT_FILES )
endmacro()

macro(itk_end_wrap_module_castxml)
  if(NOT TARGET ${WRAPPER_LIBRARY_NAME}CastXML)
    add_custom_target(${WRAPPER_LIBRARY_NAME}CastXML DEPENDS ${CastXML_OUTPUT_FILES})
    set(${WRAPPER_LIBRARY_NAME}XmlFiles ${CastXML_OUTPUT_FILES} CACHE INTERNAL "Internal ${WRAPPER_LIBRARY_NAME}Xml file list.")
  endif()
endmacro()
