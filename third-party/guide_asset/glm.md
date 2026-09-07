

git submodule add https://github.com/g-truc/glm.git

cp -r glm "$LOCAL_MINOR_ROOT/include/"

mkdir -p "$LOCAL_MINOR_ROOT/lib/cmake/glm"
cat << 'EOF' > "$LOCAL_MINOR_ROOT/lib/cmake/glm/glmConfig.cmake"
add_library(glm::glm INTERFACE IMPORTED)
set_target_properties(glm::glm PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "$ENV{LOCAL_MINOR_ROOT}/include"
)
EOF

