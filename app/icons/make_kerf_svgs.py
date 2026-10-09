# Kerf 전용 아이콘 SVG 를 만든다(Lucide 규격: 24 격자 · stroke 2 · round). 화판 7(docs/design/boards/kerf-design-v5.html)의 그림과 같다.
# 실행: python make_kerf_svgs.py  → app/icons/kerf/*.svg 를 다시 쓴다. 목록을 바꾸면 icons.cmake 의 KERF_ICON_FILES 도 고친다.
import os, sys
sys.stdout.reconfigure(encoding='utf-8')
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'kerf')
ICONS = {
    'clock': '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',
    'flip': '<path d="M8 3 4 7l4 4M4 7h16M16 21l4-4-4-4M20 17H4"/>',
    'list': '<path d="M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01"/>',
    'contrast': '<circle cx="12" cy="12" r="9"/><path d="M12 3v18a9 9 0 0 0 0-18z" fill="currentColor"/>',
    'keyboard': '<rect x="2" y="6" width="20" height="12" rx="2"/><path d="M6 10h.01M10 10h.01M14 10h.01M18 10h.01M8 14h8"/>',
    'info': '<circle cx="12" cy="12" r="9"/><path d="M12 16v-4M12 8h.01"/>',
    'zoom-in': '<circle cx="11" cy="11" r="7"/><path d="m21 21-4.3-4.3M8 11h6M11 8v6"/>',
    'zoom-out': '<circle cx="11" cy="11" r="7"/><path d="m21 21-4.3-4.3M8 11h6"/>',
    'chevron-down': '<path d="m6 9 6 6 6-6"/>',
    'star': '<path d="m12 3 2.7 5.6 6.1.9-4.4 4.3 1 6.1-5.4-2.9-5.4 2.9 1-6.1L3.2 9.5l6.1-.9z"/>',
    'section-line': '<circle cx="5" cy="19" r="2"/><circle cx="19" cy="5" r="2"/><path d="M6.5 17.5 17.5 6.5M9 11l2 2M13 7l2 2"/>',
    'hatch': '<rect x="4" y="4" width="16" height="16" rx="1"/><path d="M4 12l8-8M4 20 20 4M12 20l8-8"/>',
    'outline': '<path d="M5 9l4-5 9 2 2 8-6 6-8-2z"/><circle cx="5" cy="9" r="1.5" fill="currentColor"/><circle cx="20" cy="14" r="1.5" fill="currentColor"/>',
    'profile': '<path d="M3 9h5l2 8h4l2-8h5"/>',
    'points-xyz': '<circle cx="7" cy="16" r="1.5"/><circle cx="11" cy="9" r="1.5"/><circle cx="16" cy="13" r="1.5"/><circle cx="19" cy="6" r="1.5"/><path d="M3 21V3M3 21h18"/>',
    'cloud-las': '<path d="M21 8 12 3 3 8v8l9 5 9-5z"/><path d="M3 8l9 5 9-5M12 13v8"/>',
    'geotiff': '<rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="9" cy="9" r="2"/><path d="m21 15-5-5L5 21"/>',
    'csv': '<path d="M14 3H6a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"/><path d="M14 3v6h6M8 13h8M8 17h8"/>',
    'sections-list': '<rect x="5" y="4" width="14" height="17" rx="2"/><path d="M9 2h6v4H9zM9 12h6M9 16h6"/>',
    'vex': '<path d="M4 20h16M12 16V4M8 8l4-4 4 4"/>',
    'top-view': '<path d="M12 3 4 7l8 4 8-4z"/><path d="M4 7v10l8 4 8-4V7M12 11v10"/>',
    'height': '<path d="M12 3v18M8 7l4-4 4 4M8 17l4 4 4-4"/>',
    'crosshair': '<circle cx="12" cy="12" r="7"/><path d="M12 2v4M12 18v4M2 12h4M18 12h4"/>',
    'levels': '<path d="M3 7h18M3 12h18M3 17h18"/>',
    'fade': '<circle cx="12" cy="12" r="9" stroke-dasharray="3 3"/>',
    'image-layer': '<rect x="3" y="5" width="18" height="14" rx="2"/><path d="m3 15 5-5 4 4 3-3 6 6"/>',
    'north': '<path d="m12 2 5 18-5-4-5 4z"/>',
    'legend': '<rect x="3" y="4" width="6" height="5"/><path d="M12 6h9M12 12h9M12 18h9"/><rect x="3" y="15" width="6" height="5"/>',
    'title-block': '<rect x="3" y="4" width="18" height="16" rx="1"/><path d="M3 14h18M11 14v6"/>',
    'scalebar': '<path d="M3 12h18M3 9v6M9 9v6M15 9v6M21 9v6"/><rect x="3" y="10" width="6" height="4" fill="currentColor"/>',
    'paper': '<rect x="4" y="6" width="16" height="12" rx="1"/><path d="M14 2l3 3-3 3"/>',
    'edit-move': '<path d="M5 9l-3 3 3 3M9 5l3-3 3 3M15 19l-3 3-3-3M19 9l3 3-3 3M2 12h20M12 2v20"/>',
    'copy': '<rect x="9" y="9" width="12" height="12" rx="2"/><path d="M5 15V5a2 2 0 0 1 2-2h10"/>',
    'enter': '<path d="M20 5v6a2 2 0 0 1-2 2H5M9 9l-4 4 4 4"/>',
    'esc': '<rect x="3" y="5" width="18" height="14" rx="2"/><path d="M8 12h8"/>',
    'gap': '<path d="M3 12h5M16 12h5"/><path d="M10 9l-1 6M15 9l-1 6"/>',
    'menu': '<path d="M4 7h16M4 12h16M4 17h16"/>',
    'sheet': '<rect x="4" y="3" width="16" height="18" rx="1"/><rect x="7" y="6" width="10" height="8"/><path d="M7 17h10"/>',
}
HEAD = ('<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" '
        'stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">\n')
os.makedirs(OUT, exist_ok=True)
for name, body in ICONS.items():
    with open(os.path.join(OUT, name + '.svg'), 'w', encoding='utf-8', newline='\n') as f:
        f.write(HEAD + '  ' + body + '\n</svg>\n')
print('kerf svgs', len(ICONS))
print(' '.join(f'kerf/{n}.svg' for n in ICONS))
