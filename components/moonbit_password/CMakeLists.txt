set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(MOONBIT_SOURCE_DIR "${PROJECT_ROOT}/moonbit")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_MOONBIT_C "${GENERATED_DIR}/password_core.c")
set(GENERATED_WORDLIST_C "${GENERATED_DIR}/eff_short_wordlist.c")

set(MOONBIT_SOURCES
    "${MOONBIT_SOURCE_DIR}/entropy.mbt"
    "${MOONBIT_SOURCE_DIR}/ffi.mbt"
    "${MOONBIT_SOURCE_DIR}/passphrase.mbt"
    "${MOONBIT_SOURCE_DIR}/password.mbt"
    "${MOONBIT_SOURCE_DIR}/policy.mbt"
    "${MOONBIT_SOURCE_DIR}/random_source.mbt"
    "${MOONBIT_SOURCE_DIR}/state.mbt"
)

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
add_custom_command(
    OUTPUT "${GENERATED_MOONBIT_C}"
    COMMAND "${PYTHON}" "${PROJECT_ROOT}/tools/generate_moonbit.py"
            --source-dir "${MOONBIT_SOURCE_DIR}"
            --output "${GENERATED_MOONBIT_C}"
    DEPENDS ${MOONBIT_SOURCES}
            "${MOONBIT_SOURCE_DIR}/moon.pkg"
            "${PROJECT_ROOT}/tools/generate_moonbit.py"
    VERBATIM
    COMMENT "Generating C from MoonBit password core"
)

add_custom_command(
    OUTPUT "${GENERATED_WORDLIST_C}"
    COMMAND "${PYTHON}" "${PROJECT_ROOT}/tools/generate_wordlist.py"
            --input "${PROJECT_ROOT}/assets/wordlists/eff-short-wordlist-1.txt"
            --output "${GENERATED_WORDLIST_C}"
    DEPENDS "${PROJECT_ROOT}/assets/wordlists/eff-short-wordlist-1.txt"
            "${PROJECT_ROOT}/tools/generate_wordlist.py"
    VERBATIM
    COMMENT "Generating flash-resident EFF word list"
)

set_source_files_properties(
    "${GENERATED_MOONBIT_C}"
    "${GENERATED_WORDLIST_C}"
    PROPERTIES GENERATED TRUE
)
endif()

idf_component_register(
    SRCS "runtime/runtime.c"
         "runtime_support.c"
         "${GENERATED_MOONBIT_C}"
         "${GENERATED_WORDLIST_C}"
    INCLUDE_DIRS "include" "runtime"
)

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MOONBIT_ALLOCATOR=MOONBIT_ALLOCATOR_SYSTEM
    )
    # MoonBit 0.1.20260904 emits valid C that trips two ESP-IDF style
    # diagnostics promoted by -Werror. Keep every other warning fatal.
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -Wno-error=missing-braces
        -Wno-error=parentheses
    )
endif()
