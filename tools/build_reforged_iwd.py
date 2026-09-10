"""Build reviewed connection and main-menu UI assets without reading played trees."""
from pathlib import Path
import argparse
import hashlib
import io
import subprocess
import struct
from PIL import Image
import zipfile
ROOT=Path(__file__).resolve().parents[1]
BASE_BLOB='fcb20f972cea3ed57eb2479976626fbe34bc1a58cea2d626135e379b51c2fa16'
BASE_REVISION='ebd04e7'

def main():
    p=argparse.ArgumentParser();p.add_argument('--git',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    raw=subprocess.check_output([a.git,'-C',str(ROOT),'show',BASE_REVISION+':src/embedded/iw_CoD2x_01.iwd'])
    if hashlib.sha256(raw).hexdigest()!=BASE_BLOB: raise ValueError('Untrusted baseline IWD')
    entries={}
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        for name in z.namelist():
            if not name.endswith('/'):entries[name]=z.read(name)
    for name in ['images/rfg_conn.iwi','materials/rfg_conn']:
        if name in entries:raise ValueError('Unexpected baseline overlap')
        entries[name]=(ROOT/'src/reforged-assets'/name).read_bytes()
    # Explicit source overrides only: no wildcard/runtime directory packaging.
    art=ROOT/'src/reforged-assets/mainmenu'
    for menu in ['main.menu','background.menu','rfg_options.menu','options_multi.menu','menus.txt']:
        entries['ui_mp/'+menu]=(ROOT/'src/reforged-assets/ui_mp'/menu).read_bytes()
    template=(art/'material.template').read_bytes()
    if template.count(b'rfg_rank')!=2: raise ValueError('Unexpected UI material template')
    for name,source,size in [('rfg_mnbg','background.png',(2048,1024)),('rfg_mnlg','logo.png',(512,256))]:
        with Image.open(art/source) as image:
            pixels=image.convert('RGBA').resize(size,Image.Resampling.LANCZOS).tobytes('raw','BGRA')
        total=28+len(pixels)
        entries['images/'+name+'.iwi']=struct.pack('<3sBBBHHH4I',b'IWi',5,1,2,*size,1,*([total]*4))+pixels
        entries['materials/'+name]=template.replace(b'rfg_rank',name.encode('ascii'))
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.open('xb') as out,zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,content in sorted(entries.items()):
            info=zipfile.ZipInfo(name,(2000,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;info.create_system=3
            z.writestr(info,content,compresslevel=9)
    print('Reviewed client UI IWD:',len(entries),'members;',hashlib.sha256(a.output.read_bytes()).hexdigest())
if __name__=='__main__':main()
