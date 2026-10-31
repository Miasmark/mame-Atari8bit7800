#!/bin/sh
# appimage.sh STAGED_DIR OUTPUT LINUXDEPLOY
#
# Turns a staged Linux release (stage.sh) into one AppImage that carries SDL
# and the other libraries MAME needs, so it runs on distributions newer than
# the one it was built on. The software lists and the documents go inside; the
# AppRun adds the lists to the hash path after the usual ./hash.
set -e
staged=$(cd "$1" && pwd); out=$2; linuxdeploy=$3
here=$(cd "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d)
app=$work/AppDir

mkdir -p "$app/usr/bin" "$app/usr/share/mame" "$app/usr/share/doc/mame"
cp "$staged/mame" "$app/usr/bin/mame"
cp -r "$staged/hash" "$app/usr/share/mame/"
cp "$staged/README.txt" "$staged/README-atari.md" "$staged/README-intellivision.md" "$staged/COPYING.txt" "$app/usr/share/doc/mame/"

cat > "$work/mame.desktop" <<TEXT
[Desktop Entry]
Type=Application
Name=MAME (Atari)
Comment=MAME 0.289 with the Atari 2600/7800/8-bit patch set
Exec=mame
Icon=mame
Terminal=false
Categories=Game;Emulator;
TEXT

# the icon from MAME's Windows resources, largest image
python3 - "$here/scripts/resources/windows/mame/mame.ico" "$work/mame.png" <<'PY'
import struct, sys, zlib
data = open(sys.argv[1], 'rb').read()
count = struct.unpack_from('<H', data, 4)[0]
best = None
for i in range(count):
    w, h, _, _, _, bpp, size, offset = struct.unpack_from('<BBBBHHII', data, 6 + i * 16)
    w = w or 256; h = h or 256
    if best is None or (w, bpp) > (best[0], best[1]):
        best = (w, bpp, size, offset, h)
w, bpp, size, offset, h = best
image = data[offset:offset + size]
if image[:8] == b'\x89PNG\r\n\x1a\n':
    open(sys.argv[2], 'wb').write(image)
    sys.exit()
# BMP entry: 32 bit BGRA, bottom-up, height doubled for the mask
header = struct.unpack_from('<IiiHH', image, 0)
assert header[4] == 32, 'only 32 bit icon images are handled'
pixels = image[header[0]:]
rows = []
for y in range(h - 1, -1, -1):
    row = pixels[y * w * 4:(y + 1) * w * 4]
    rows.append(b'\0' + b''.join(row[x + 2:x + 3] + row[x + 1:x + 2] + row[x:x + 1] + row[x + 3:x + 4] for x in range(0, w * 4, 4)))
def chunk(kind, body):
    return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(b''.join(rows))) + chunk(b'IEND', b'')
open(sys.argv[2], 'wb').write(png)
PY

cat > "$work/AppRun" <<'TEXT'
#!/bin/sh
here=$(dirname "$(readlink -f "$0")")
for arg in "$@"; do
	case "$arg" in
	-hashpath|-hash_directory) exec "$here/usr/bin/mame" "$@" ;;
	esac
done
exec "$here/usr/bin/mame" -hashpath "hash;$here/usr/share/mame/hash" "$@"
TEXT
chmod +x "$work/AppRun"

(cd "$work" && ARCH=x86_64 OUTPUT=out.AppImage "$linuxdeploy" --appdir "$app" -e "$app/usr/bin/mame" \
	-d "$work/mame.desktop" -i "$work/mame.png" --custom-apprun "$work/AppRun" --output appimage)
mv "$work/out.AppImage" "$out"
rm -rf "$work"
