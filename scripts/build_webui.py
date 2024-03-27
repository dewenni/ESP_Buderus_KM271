import os
import re
import gzip

#================================================================
# HTML
#================================================================
# Definieren Sie die Pfade der Quelldateien
source_files = [
    'web/html/00_header.html',
    'web/html/01_dash.html',
    'web/html/02_ctrl.html',
    'web/html/03_hc1.html',
    'web/html/04_hc2.html',
    'web/html/05_hw.html',   
    'web/html/06_burner.html',
    'web/html/07_general.html',
    'web/html/08_alarm.html',
    'web/html/09_system.html',
    'web/html/10_logger.html',
    'web/html/11_ota.html',
    'web/html/12_settings.html',
    'web/html/99_footer.html'
]

# Zielpfad der generierten Datei
output_file_html = 'web/temp/index.html'
output_file_gzip_html = 'include/gzip_html.h'

combined_html = ''

# Regex-Muster für den Abschnittsstart und -ende
section_start = re.compile(r'<!--SECTION-START-->')
section_end = re.compile(r'<!--SECTION-END-->')

# Durchlaufen Sie jede Datei und extrahieren Sie den gewünschten Inhalt
for file_path in source_files:
    with open(file_path, 'r') as file:
        file_content = file.read()
        # Suche nach dem Abschnitt zwischen den Markierungen
        section_contents = re.findall(f'(?s){section_start.pattern}(.*?){section_end.pattern}', file_content)
        # Füge nur den Inhalt zwischen den Markierungen hinzu, falls vorhanden
        for content in section_contents:
            combined_html += content.strip() + '\n'

# Schreiben Sie die kombinierte Definition in die Ausgabedatei
with open(output_file_html, 'w') as file:
    file.write(combined_html)

def compress_html_to_gzip_c_array(input_file_path, output_file_path):
    # HTML-Datei einlesen
    with open(input_file_path, 'rb') as file:
        content = file.read()
    
    # Inhalt mit GZIP komprimieren
    compressed_content = gzip.compress(content)
    
    # Komprimierten Inhalt in ein C-Array umwandeln
    c_array_content = ', '.join(['0x{:02x}'.format(byte) for byte in compressed_content])
    
    # C-Array in eine neue Datei schreiben
    with open(output_file_path, 'w') as file:
        file.write('const uint8_t PROGMEM gzip_html[] = {' + c_array_content + '};\n')
        file.write(f'const unsigned int gzip_html_size = {len(compressed_content)};')

# Pfad zur HTML-Datei und zum Ausgabe-C-Datei
input_html_file = output_file_html
output_c_file = output_file_gzip_html

compress_html_to_gzip_c_array(input_html_file, output_c_file)


#================================================================
# CSS - Merge and gzip
#================================================================
# Definieren Sie die Pfade der Quelldateien
source_files = [
    'web/css/custom.css',
    'web/css/icons.css',
]

# Zielpfad der generierten Datei
output_file_css = 'web/temp/custom.css'
output_file_gzip_css = 'include/gzip_c_css.h'

combined_content = ''

# Durchlaufen Sie jede Datei und fügen Sie ihren Inhalt hinzu
for file_path in source_files:
    with open(file_path, 'r') as file:
        combined_content += file.read() + '\n'

# Schreiben Sie die kombinierte Definition in die Ausgabedatei
with open(output_file_css, 'w') as file:
    file.write(combined_content)


def compress_css_to_gzip_c_array(input_file_path, output_file_path):
    # HTML-Datei einlesen
    with open(input_file_path, 'rb') as file:
        content = file.read()
    
    # Inhalt mit GZIP komprimieren
    compressed_content = gzip.compress(content)
    
    # Komprimierten Inhalt in ein C-Array umwandeln
    c_array_content = ', '.join(['0x{:02x}'.format(byte) for byte in compressed_content])
    
    # C-Array in eine neue Datei schreiben
    with open(output_file_path, 'w') as file:
        file.write('const uint8_t PROGMEM c_gzip_css[] = {' + c_array_content + '};\n')
        file.write(f'const unsigned int c_gzip_css_size = {len(compressed_content)};')

# Pfad zur HTML-Datei und zum Ausgabe-C-Datei
input_html_file = output_file_css
output_c_file = output_file_gzip_css

compress_css_to_gzip_c_array(input_html_file, output_c_file)

#================================================================
# CSS - only gzip
#================================================================

# Definieren Sie die Pfade der Quelldateien
source_files = [
    'web/css/pico.css',
]

# Pfad zur HTML-Datei und zum Ausgabe-C-Datei
input_html_file = 'web/css/pico.css'
output_c_file = 'include/gzip_m_css.h'

def compress_css_to_gzip_c_array(input_file_path, output_file_path):
    # HTML-Datei einlesen
    with open(input_file_path, 'rb') as file:
        content = file.read()
    
    # Inhalt mit GZIP komprimieren
    compressed_content = gzip.compress(content)
    
    # Komprimierten Inhalt in ein C-Array umwandeln
    c_array_content = ', '.join(['0x{:02x}'.format(byte) for byte in compressed_content])
    
    # C-Array in eine neue Datei schreiben
    with open(output_file_path, 'w') as file:
        file.write('const uint8_t PROGMEM m_gzip_css[] = {' + c_array_content + '};\n')
        file.write(f'const unsigned int m_gzip_css_size = {len(compressed_content)};')

compress_css_to_gzip_c_array(input_html_file, output_c_file)


#================================================================
# JS
#================================================================
# Definieren Sie die Pfade der Quelldateien
source_files = [
    'web/js/main.js',
    'web/js/lang.js',
    'web/js/tools.js',   
]

# Zielpfad der generierten Datei
output_file_js = 'web/temp/main.js'
output_file_gzip_js = 'include/gzip_js.h'

combined_content = ''

# Durchlaufen Sie jede Datei und fügen Sie ihren Inhalt hinzu
for file_path in source_files:
    with open(file_path, 'r') as file:
        combined_content += file.read() + '\n'

# Schreiben Sie die kombinierte Definition in die Ausgabedatei
with open(output_file_js, 'w') as file:
    file.write(combined_content)

def compress_css_to_gzip_c_array(input_file_path, output_file_path):
    # HTML-Datei einlesen
    with open(input_file_path, 'rb') as file:
        content = file.read()
    
    # Inhalt mit GZIP komprimieren
    compressed_content = gzip.compress(content)
    
    # Komprimierten Inhalt in ein C-Array umwandeln
    c_array_content = ', '.join(['0x{:02x}'.format(byte) for byte in compressed_content])
    
    # C-Array in eine neue Datei schreiben
    with open(output_file_path, 'w') as file:
        file.write('const uint8_t PROGMEM gzip_js[] = {' + c_array_content + '};\n')
        file.write(f'const unsigned int gzip_js_size = {len(compressed_content)};')

# Pfad zur HTML-Datei und zum Ausgabe-C-Datei
input_html_file = output_file_js
output_c_file = output_file_gzip_js

compress_css_to_gzip_c_array(input_html_file, output_c_file)
