# Validación de Fmaill Utilities v0.2.0

## Pruebas ejecutadas antes de empaquetar

- Parseo correcto de `mod.json` como JSON.
- Parseo correcto del workflow multiplataforma como YAML.
- Compilación de las utilidades con C++23 usando `-Wall -Wextra -Werror`.
- Ejecución de pruebas unitarias normales y con AddressSanitizer/UndefinedBehaviorSanitizer.
- Casos probados: IDs individuales, duplicados, espacios, rangos normales, rangos invertidos, límites, entradas inválidas, serialización y paginación.
- Comprobación de sintaxis de `VisualIconSelector.cpp` con un contrato de API basado en Geode SDK 5.7.1.
- Configuración y construcción completa del proyecto con CMake, generando una biblioteca compartida de prueba.
- Revisión de las firmas oficiales utilizadas: `Popup`, `SimplePlayer`, `ButtonSprite`, `CCMenuItemSpriteExtra`, `GameManager`, `Mod::getSavedValue` y `Mod::setSavedValue`.
- Verificación de la estructura y del contenido final del ZIP.

## Alcance de la prueba

La sesión no contiene los toolchains completos de Windows, Android, macOS e iOS ni todos los binarios del SDK. Por eso, la comprobación binaria definitiva de las cinco plataformas debe realizarla el workflow de GitHub Actions después del push. El código y la lógica sí fueron compilados y probados localmente antes de empaquetarse.
