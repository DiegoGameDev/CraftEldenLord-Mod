# Dependencias locais

`nlohmann/json` 3.11.3, licenca MIT, usado somente em selection_reader.cpp.
O header e a licenca originais ficam em `nlohmann/`.

Origem: https://github.com/nlohmann/json/releases/tag/v3.11.3
Header: https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp
Licenca: https://raw.githubusercontent.com/nlohmann/json/v3.11.3/LICENSE.MIT
SHA-256 publicado do header:
`9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6`

O CMake verifica esse hash e nao baixa dependencias durante a compilacao.
MinHook nao e vinculado nem carregado neste milestone.
