"""Validate the packaged Edelweiss bootstrap, EDL and optional sidecar."""
from pathlib import Path
import re
import struct
import sys

root = Path(sys.argv[1])
cache=(root/'CMakeCache.txt').read_text()
def option(key):
    m=re.search(rf'^{re.escape(key)}:STRING=(.*)$', cache, re.MULTILINE)
    assert m,key
    return m.group(1)
appid=option('EXWORD_APP_ID')
module=option('EXWORD_MODULE')
bootstrap=(root/'edelweiss.d01').read_bytes()
payload=(root/'main.edl').read_bytes()
assert bootstrap[:16]==b'CASIODICPLUGIN10'
assert 128<len(bootstrap)<=131072
assert (root/f'{module}.d01').read_bytes()==bootstrap
assert payload[:7]==b'\x7fELF\x01\x02\x01'
assert struct.unpack_from('>H',payload,18)[0]==42
assert re.fullmatch(r'[0-9a-f]{40}\n',(root/'edelweiss-version.txt').read_text())
for locale in ('ja','cn'):
    package=root/'packages'/locale/appid
    assert (package/f'{module}.d01').read_bytes()==bootstrap
    assert (package/'main.edl').read_bytes()==payload
    assert (package/'fileinfo.cji').is_file()
    assert list(package.glob('*.htm'))
    for page in package.glob('*.htm'):
        content=page.read_bytes()
        assert b'@APP' not in content and b'\r\n' in content
    sidecar=package/'tower.gmpak'
    if sidecar.exists():
        asset=sidecar.read_bytes()
        assert asset[:4]==b'GMPK' and struct.unpack_from('<H',asset,4)[0]==1
        toc=struct.unpack_from('<I',asset,6)[0]
        count=struct.unpack_from('<I',asset,toc)[0]
        assert toc+4+count*25==len(asset)
        print(locale,appid,'EDL + asset pack: PASS')
    else:
        print(locale,appid,'EDL PASS (game asset must be installed separately)')

# The host must resolve resident hardware symbols, not link another kernel.
u32=lambda off:struct.unpack_from('>I',payload,off)[0]
u16=lambda off:struct.unpack_from('>H',payload,off)[0]
base=u32(32); count=u16(48)
imports=set(); defined=set()
for i in range(count):
    sec=base+i*40
    if u32(sec+4)!=2:continue
    strings=base+u32(sec+24)*40
    for off in range(u32(sec+16),u32(sec+16)+u32(sec+20),16):
        nameoff=u32(strings+16)+u32(off)
        symbol=payload[nameoff:payload.index(b'\0',nameoff)].decode()
        if u16(off+14)==0xfff1 and u32(off+4)==0:imports.add(symbol)
        elif u16(off+14):defined.add(symbol)
assert {'_pollevent','_timer_configure','_r61523_display'} <= imports
assert not {'_start','_exword_entry','_gint_kernel_install','_mmu_exword_prepare'} & defined
print('No second kernel / EX-word hardware imports: PASS')

# EBrew reads the .edl.meta section, not the package title or a PNG. Check the
# actual ELF payload's name and 32x32 RGB565 pixels so CI catches missing or
# accidentally stale launcher artwork.
shent_size=u16(46)
shstr_index=u16(50)
assert shent_size>=40 and shstr_index<count
names=base+shstr_index*shent_size
names_off=u32(names+16)
names_size=u32(names+20)
def section_name(index):
    pos=names_off+u32(base+index*shent_size)
    assert names_off<=pos<names_off+names_size
    return payload[pos:payload.index(b'\x00',pos)].decode()
meta_sections=[base+i*shent_size for i in range(count) if section_name(i)=='.edl.meta']
assert len(meta_sections)==1,'missing or duplicated .edl.meta'
meta_off=u32(meta_sections[0]+16)
meta_size=u32(meta_sections[0]+20)
assert meta_size>=8+32+64+32*32*2
assert u32(meta_off)==0x45444c31 and u32(meta_off+4)==1
assert payload[meta_off+8:meta_off+40].split(b'\x00',1)[0]==b'TKING'
icon_literals=re.findall(r'0x([0-9a-fA-F]{4})', (Path(__file__).resolve().parents[1]/'src/icon.h').read_text())
assert len(icon_literals)==1024
wanted_icon=struct.pack('>1024H',*(int(h,16) for h in icon_literals))
assert payload[meta_off+104:meta_off+104+2048]==wanted_icon
print('EBrew title TKING and 32x32 crown RGB565 metadata: PASS')
