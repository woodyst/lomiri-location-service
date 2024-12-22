include(FindGettext)
find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)

set(DOMAIN ${PROJECT_NAME})
set(POT_FILE ${DOMAIN}.pot)

file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/LINGUAS LINGUAS
     REGEX "^[^#].*")
string(REGEX MATCHALL "[^ \t]+" LANGS "${LINGUAS}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in
               ${CMAKE_CURRENT_BINARY_DIR}/POTFILES COPYONLY
)

add_custom_target(${POT_FILE} ALL
    COMMENT "Generating translation template..."
    COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -o ${CMAKE_CURRENT_SOURCE_DIR}/${POT_FILE}
        -D ${CMAKE_SOURCE_DIR}
        --from-code=UTF-8
        --c++ --qt --add-comments=TRANSLATORS
        --keyword=tr --keyword=tr:1,2 --keyword=N_ --keyword=_ --keyword=tag --keyword=tag:1c,2
        --package-name='${PROJECT_NAME}'
        --copyright-holder='Canonical Ltd.'
        --join-existing
        --files-from=${CMAKE_CURRENT_BINARY_DIR}/POTFILES
)

foreach(LANG ${LANGS})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${LANG}.po
                   ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.po
                   COPYONLY
    )
endforeach(LANG)
gettext_process_pot_file(${POT_FILE} ALL
                         INSTALL_DESTINATION ${CMAKE_INSTALL_LOCALEDIR}
                         LANGUAGES ${LANGS}
)
