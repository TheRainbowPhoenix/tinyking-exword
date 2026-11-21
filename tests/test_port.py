#!/usr/bin/env python3
"""Host proof: Python and C Q8 step parity + GMPK streaming smoke."""
from __future__ import annotations
import ctypes as ct
import importlib.util
import pathlib
import random
import subprocess
import tempfile
import struct
import sys
import types

ROOT=pathlib.Path(__file__).resolve().parents[1]
SRC=ROOT/'src'
PACK=ROOT/'tower.gmpak'
spec=importlib.util.spec_from_file_location('reference_core',ROOT/'tests/reference_core.py')
ref=importlib.util.module_from_spec(spec)
spec.loader.exec_module(ref)

class Player(ct.Structure):
    _fields_=[(n,ct.c_int) for n in ('x','y','vx','vy','charge','direction','previous','facing','crash','falls')]+[(n,ct.c_bool) for n in ('grounded','landed','bounced','flying','ice')]
class Box(ct.Structure):
    _fields_=[(n,ct.c_int) for n in ('x','y','w','h','kind')]

class Entry(ct.Structure):
    _fields_=[('name',ct.c_char*16),('type',ct.c_uint8),('offset',ct.c_uint32),('length',ct.c_uint32)]
class Pak(ct.Structure):
    _fields_=[('fd',ct.c_int),('count',ct.c_int),('size',ct.c_uint32),('end',ct.c_uint32),('toc',Entry*512)]
class World(ct.Structure):
    _fields_=[('pak',ct.POINTER(Pak))]+[(n,ct.c_int) for n in ('width','rooms','spawn_x','spawn_y','height','room','band','all_count','near_count')]+[('all',Box*600),('near',Box*600)]
class Weather(ct.Structure):
    _fields_=[('pak',ct.POINTER(Pak)),('rooms',ct.c_int),('screen',ct.c_int),('phase',ct.c_int),('enabled',ct.c_int),('settings',ct.c_ubyte*400),('gusts',ct.c_ubyte*784),('mask',ct.c_ubyte*676)]

class ReferencePack:
    def __init__(self,blob):
        toc=struct.unpack_from('<I',blob,6)[0]
        count=struct.unpack_from('<I',blob,toc)[0]
        self.data={}
        for i in range(count):
            name,kind,off,size=struct.unpack_from('<16sBII',blob,toc+4+i*25)
            self.data[name.rstrip(b'\x00').decode()]=(kind,blob[off:off+size])
        self.kind=-1
    def read(self,name):
        self.kind,data=self.data[name]
        return data

def compare_world_weather(tmp):
    if not PACK.is_file():
        print('No asset sidecar: world/weather parity skipped')
        return
    # The original modules are independent of a real device when draw() is unused.
    sys.modules.setdefault('gint',types.ModuleType('gint'))
    sys.modules.setdefault('core',ref)
    def module(name,path):
        z=importlib.util.spec_from_file_location(name,path)
        obj=importlib.util.module_from_spec(z)
        z.loader.exec_module(obj)
        return obj
    rw=module('reference_world',ROOT/'tests/reference_world.py')
    rweather=module('reference_weather',ROOT/'tests/reference_weather.py')
    native=pathlib.Path(tmp)/'libtinyking_world.so'
    subprocess.run(['cc','-std=c11','-O2','-shared','-fPIC',
                    '-I'+str(ROOT/'tests/stub'),'-I'+str(SRC),
                    *(str(SRC/p) for p in ('core.c','pak.c','world.c','weather.c')),
                    '-o',str(native)],check=True)
    dll=ct.CDLL(str(native))
    dll.tk_pak_open.argtypes=[ct.POINTER(Pak),ct.c_char_p]
    dll.tk_pak_close.argtypes=[ct.POINTER(Pak)]
    dll.tk_world_init.argtypes=[ct.POINTER(World),ct.POINTER(Pak)]
    dll.tk_world_update.argtypes=[ct.POINTER(World),ct.c_int]
    dll.tk_weather_init.argtypes=[ct.POINTER(Weather),ct.POINTER(Pak),ct.c_int]
    dll.tk_weather_tick.argtypes=[ct.POINTER(Weather),ct.POINTER(Player)]
    dll.tk_weather_force.argtypes=[ct.POINTER(Weather),ct.POINTER(Player)]
    cpak=Pak();assert dll.tk_pak_open(ct.byref(cpak),str(PACK).encode())==0
    cw=World();assert dll.tk_world_init(ct.byref(cw),ct.byref(cpak))==0
    cp=ReferencePack(PACK.read_bytes());pyworld=rw.World(cp)
    count=0
    # 32px samples cover all 22 rooms; extra 479->480 samples catch the
    # same-band / new-room cache invalidation boundary.
    ys=sorted(set(range(0,cw.height,32))|set(r*480+d for r in range(cw.rooms) for d in (0,1,479)))
    for y in ys:
        pyworld.update(y*256)
        assert dll.tk_world_update(ct.byref(cw),y*256)==0
        values=[]
        for i in range(cw.near_count):
            b=cw.near[i]
            values.extend((b.x,b.y,b.w,b.h,b.kind))
        assert values==pyworld.boxes,('world',y,len(values),len(pyworld.boxes))
        count+=1
    # C must handle room changes *within* the same 64px band (room 480
    # changes at y=480, whereas band 7 remains active through y=511).
    for y in (479,480,959,960,1439,1440):
        pyworld.update(y*256)
        assert dll.tk_world_update(ct.byref(cw),y*256)==0
        assert sum(([b.x,b.y,b.w,b.h,b.kind] for b in cw.near[:cw.near_count]),[])==pyworld.boxes
    print('Collision broad-phase parity PASS:',count,'tower positions')
    cweather=Weather();assert dll.tk_weather_init(ct.byref(cweather),ct.byref(cpak),cw.rooms)==0
    pyweather=rweather.Weather(cp,pyworld.rooms)
    rng=random.Random(98)
    for t in range(3500):
        p=ref.Player(rng.randint(0,308),rng.randint(0,cw.height-1))
        p.grounded=bool(rng.randint(0,1));p.ice=bool(rng.randint(0,1))
        p.crash=rng.choice([-1,-1,0,4]);p.flying=bool(rng.randint(0,8)==0)
        q=Player()
        dll.tk_player_init(ct.byref(q),p.x//256,p.y//256)
        for attr in ATTRS:setattr(q,attr,getattr(p,attr))
        pyweather.tick(p)
        assert dll.tk_weather_tick(ct.byref(cweather),ct.byref(q))==0
        a=pyweather.force(p)
        b=dll.tk_weather_force(ct.byref(cweather),ct.byref(q))
        assert a==b,('wind',t,p.x,p.y,pyweather.screen,a,b)
    print('Weather force parity PASS: 3500 random samples')
    dll.tk_pak_close(ct.byref(cpak))

ATTRS=[n for n,_ in Player._fields_]

def compare(lib,seed,boxes,start=(140,170),steps=2000):
    rng=random.Random(seed)
    p=ref.Player(*start)
    q=Player()
    lib.tk_player_init(ct.byref(q),*start)
    raw=sum(([x,y,w,h,k] for x,y,w,h,k in boxes),[])
    arr=(Box*len(boxes))(*(Box(*b) for b in boxes))
    for tick in range(steps):
        # Stick to plausible game inputs, with occasional fresh-release pulses.
        mask=rng.choice([0,0,0,1,2,4,4,4,5,6,7,16])
        wind=rng.randint(-60,60)
        ref.step(p,mask,raw,320,wind)
        lib.tk_step(ct.byref(q),mask,arr,len(boxes),320,wind)
        for attr in ATTRS:
            a,b=getattr(p,attr),getattr(q,attr)
            if a!=b:
                raise AssertionError(f'case {seed} tick {tick} {attr}: Python={a} C={b}; keys={mask}, wind={wind}')
    return steps

def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'libtinyking.so'
        common=['cc','-std=c11','-Wall','-Wextra','-Werror','-O2']
        subprocess.run(common+['-shared','-fPIC','-I'+str(SRC),str(SRC/'core.c'),'-o',str(so)],check=True)
        lib=ct.CDLL(str(so))
        lib.tk_player_init.argtypes=[ct.POINTER(Player),ct.c_int,ct.c_int]
        lib.tk_step.argtypes=[ct.POINTER(Player),ct.c_int,ct.POINTER(Box),ct.c_int,ct.c_int,ct.c_int]
        cases=[
            ([],(160,100)),
            ([(0,220,320,10,0)],(160,196)),
            ([(0,220,320,10,1),(0,300,320,20,0)],(160,196)),
            ([(0,240,320,16,6),(45,190,30,12,0)],(160,220)),
            ([(12,200,60,60,2),(100,180,60,60,3),(180,220,45,50,4),(250,200,55,60,5),(0,300,320,10,0)],(170,120)),
            ([(0,170,15,120,0),(300,170,20,120,0),(0,300,320,30,0)],(160,200)),
        ]
        total=0
        for i,(boxes,start) in enumerate(cases):
            total+=compare(lib,42+i,boxes,start,steps=2500)
        print('Q8 physics parity PASS:',total,'ticks ×',len(ATTRS),'state fields')
        smoke=pathlib.Path(tmp)/'pack_smoke'
        subprocess.run(common+['-I'+str(ROOT/'tests/stub'),'-I'+str(SRC),
            str(SRC/'core.c'),str(SRC/'pak.c'),str(SRC/'world.c'),str(SRC/'weather.c'),str(SRC/'scene.c'),
            str(ROOT/'tests/pack_smoke.c'),'-o',str(smoke)],check=True)
        if not PACK.is_file():
            print('No tower.gmpak: skipped stream smoke')
        else:
            subprocess.run([str(smoke),str(PACK)],check=True)
        compare_world_weather(tmp)

if __name__=='__main__':main()
