# Muuntaa PNG-kehykset 240x160 JPEG-kuviksi ja C-headereiksi
import sys, glob, os
from PIL import Image
src, dst = sys.argv[1], sys.argv[2]
os.makedirs(dst+'/jpg', exist_ok=True)
for view in ('pitch','roll'):
    files = sorted(glob.glob(f'{src}/{view}_*.png'))
    out = [f'// Automaattisesti generoitu ({view}), {len(files)} kehystä 240x160 JPEG\n',
           '// Kehys k vastaa kuvan kallistusta (k - 30) astetta.\n#pragma once\n#include <Arduino.h>\n\n']
    names, total = [], 0
    for i, f in enumerate(files):
        im = Image.open(f).convert('RGB').resize((240,160), Image.LANCZOS)
        jp = f'{dst}/jpg/{view}_{i:02d}.jpg'
        im.save(jp, 'JPEG', quality=85, optimize=True, subsampling=2)
        data = open(jp,'rb').read(); total += len(data)
        n = f'{view}_{i:02d}'; names.append((n, len(data)))
        out.append(f'static const uint8_t {n}[] PROGMEM = {{\n')
        for j in range(0, len(data), 24):
            out.append('  ' + ','.join(f'0x{b:02x}' for b in data[j:j+24]) + ',\n')
        out.append('};\n')
    V = view.upper()
    out.append(f'\nstatic const uint8_t* const {V}_FRAMES[] PROGMEM = {{ ' + ', '.join(n for n,_ in names) + ' };\n')
    out.append(f'static const uint32_t {V}_SIZES[] PROGMEM = {{ ' + ', '.join(str(s) for _,s in names) + ' };\n')
    out.append(f'static const int {V}_FRAME_COUNT = {len(names)};\n')
    open(f'{dst}/frames_{view}.h','w').write(''.join(out))
    print(view, len(names), 'kehystä', total//1024, 'kt')
