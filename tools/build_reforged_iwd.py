"""Rebuild the reviewed static connection art IWD without using local runtime files."""
from pathlib import Path
import argparse
import hashlib
import io
import subprocess
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
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.open('xb') as out,zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,content in sorted(entries.items()):
            info=zipfile.ZipInfo(name,(2000,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;info.create_system=3
            z.writestr(info,content,compresslevel=9)
    print('Reviewed static IWD:',len(entries),'members;',hashlib.sha256(a.output.read_bytes()).hexdigest())
if __name__=='__main__':main()
