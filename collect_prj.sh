#!/bin/bash

# Имя выходного файла
OUTPUT_FILE="project_context.txt"

# Очищаем файл, если он существует
echo "" > $OUTPUT_FILE

# Список исключений (добавьте сюда папки, которые не нужно сканировать)
EXCLUDE_DIRS="-not -path '*/.*' -not -path '*/build/*' -not -path '*/bin/*' -not -path '*/obj/*'"

echo "--- START OF PROJECT STRUCTURE ---" >> $OUTPUT_FILE
tree -I "build|bin|obj|.git" >> $OUTPUT_FILE 2>/dev/null || find . -maxdepth 2 -not -path '*/.*' >> $OUTPUT_FILE
echo "--- END OF PROJECT STRUCTURE ---" >> $OUTPUT_FILE
echo "" >> $OUTPUT_FILE

# Находим все файлы (cpp, h, hpp, md, cmake, Makefile)
find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.md" -o -name "CMakeLists.txt" -o -name "Makefile" \) \
$EXCLUDE_DIRS | while read -r file; do
    echo "========================================" >> $OUTPUT_FILE
    echo "FILE: $file" >> $OUTPUT_FILE
    echo "========================================" >> $OUTPUT_FILE
    cat "$file" >> $OUTPUT_FILE
    echo -e "\n" >> $OUTPUT_FILE
done

echo "Готово! Весь проект собран в файл: $OUTPUT_FILE"

