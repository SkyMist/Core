# Copyright (C) 2008-2010 Trinity <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

find_program(_HG_EXEC
  NAMES
    hg
  HINTS
    ENV PATH
  DOC "hg installation path"
)

if(_HG_EXEC)
  execute_process(
    COMMAND "${_HG_EXEC}" log -l 1
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE rev_info
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
else()
  message("")
  message(STATUS "WARNING - Missing or outdated git - did you forget to install a recent version?")
  message(STATUS "WARNING - Observe that for revision ID/hash to work you need at least version ${_REQUIRED_GIT_VERSION}")
  message(STATUS "WARNING - Continuing anyway, but setting the revision-ID and hash to Rev:0 Hash: Archive")
  message("")
endif()
if(_HG_EXEC)
  execute_process(
    COMMAND "${_HG_EXEC}" log -l 1 --template {date|date}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE rev_date
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    )
else()
  message("")
  message(STATUS "WARNING - Missing or outdated git - did you forget to install a recent version?")
  message(STATUS "WARNING - Observe that for revision ID/hash to work you need at least version ${_REQUIRED_GIT_VERSION}")
  message(STATUS "WARNING - Continuing anyway, but setting the revision-ID and hash to Rev:0 Hash: Archive")
  message("")
endif()
# Last minute check - ensure that we have a proper revision
# If everything above fails (means the user has erased the git revision control directory or removed the origin/HEAD tag)
if(NOT rev_info)
  # No valid ways available to find/set the revision/hash, so let's force some defaults
  set(rev_id_str "0")
  set(rev_id "0")
else()
  # Extract revision and hash from git
  string(REGEX REPLACE changeset:\t[^0-9]+: "" rev_id_str ${rev_info})
  set(rev_hash  ${rev_id_str})
  string(REGEX MATCH [0-9]+:[0-9a-z]*  rev_hash ${rev_hash})  
  string(REGEX MATCH [0-9]+  rev_id_str ${rev_id_str})
  string(REGEX REPLACE [+]+ "" rev_id ${rev_id_str})
endif()

# Its not set during initial run
if(NOT BUILDDIR)
  set(BUILDDIR ${CMAKE_BINARY_DIR})
endif()

# Create the actual revision.h file from the above params
if(NOT "${rev_id_cached}" MATCHES "${rev_id_str}")
  configure_file(
    "${CMAKE_SOURCE_DIR}/revision.h.in.cmake"
    "${BUILDDIR}/revision.h"
    @ONLY
  )
  set(rev_id_cached "${rev_id_str}" CACHE INTERNAL "Cached revision ID")
endif()
