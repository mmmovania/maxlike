/**
 * max_loader.js — read Autodesk 3ds Max .max scene files into three.js, in pure JavaScript.
 *
 *   import { MaxLoader } from './max_loader.js';
 *   const loader = new MaxLoader();
 *   const { scene, nodes, log } = await loader.loadAsync('models/box.max');
 *   threeScene.add(scene);
 *
 * Or from a File / ArrayBuffer:
 *   const result = await new MaxLoader().parseAsync(await file.arrayBuffer());
 *
 * Supported (verified against 3ds Max R3.1 sample files):
 *   - Compound File (OLE2) container, zlib-compressed streams, chunk trees, class directory
 *   - Scene graph: nodes, names, parent links, Position/Rotation/Scale controllers, pivot offsets,
 *     wire colours, instances (shared objects)
 *   - Geometry: Editable Mesh (+ smoothing groups), Editable Poly (unverified), standard primitives
 *     (Box, Sphere, GeoSphere, Cylinder, Cone, Torus, Tube, Pyramid, Teapot, Plane)
 *   - Modifier stacks: Derived Objects; Edit Mesh vertex moves (base must be Editable Mesh/Poly or a 1x1x1 Box)
 *   - Materials: diffuse colour of Standard / Multi-Sub / VRay / Architectural (best effort, unverified)
 * Not evaluated: other modifiers (shown unmodified), splines, cameras, lights, textures/UVs, animation.
 *
 * Requires three.js r150+ (uses 'three/addons/'). Reverse-engineered; see max-format.md.
 * Based on community research: Blender io_import_max (S. Sille) and Importer3D (J. M. Plonka).
 */
import * as THREE from 'three';
import { TeapotGeometry } from 'three/addons/geometries/TeapotGeometry.js';

// Wire colours are 8-bit sRGB; tell three.js so colour management keeps them as Max shows them.
function srgbColor(r, g, b) {
  const c = new THREE.Color();
  return THREE.SRGBColorSpace ? c.setRGB(r, g, b, THREE.SRGBColorSpace) : c.setRGB(r, g, b);
}

/* =====================================================================
   1. OLE2 / Compound File Binary reader (a .max file is a CFB container)
   ===================================================================== */
const SPECIAL = 0xFFFFFFFA; // sector ids >= this are markers (end of chain, free, FAT, DIFAT)

function parseCFB(buf) {
  const u8 = new Uint8Array(buf), dv = new DataView(buf);
  const sig = [0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1];
  if (u8.length < 512 || sig.some((b, i) => u8[i] !== b))
    throw new Error('This is not a 3ds Max scene file: the compound-file signature is missing.');

  const secSize = 1 << dv.getUint16(0x1E, true);
  const miniSize = 1 << dv.getUint16(0x20, true);
  const firstDir = dv.getUint32(0x30, true);
  const cutoff = dv.getUint32(0x38, true) || 4096;
  const firstMiniFat = dv.getUint32(0x3C, true);
  const numMiniFat = dv.getUint32(0x40, true);
  let difat = dv.getUint32(0x44, true);
  const numDifat = dv.getUint32(0x48, true);
  const off = s => (s + 1) * secSize;

  // FAT sector list: 109 in the header, the rest in DIFAT sectors
  const fatSecs = [];
  for (let i = 0; i < 109; i++) { const v = dv.getUint32(0x4C + i * 4, true); if (v < SPECIAL) fatSecs.push(v); }
  for (let k = 0; k < numDifat && difat < SPECIAL; k++) {
    const o = off(difat); if (o + secSize > u8.length) break;
    for (let j = 0; j < secSize / 4 - 1; j++) { const v = dv.getUint32(o + j * 4, true); if (v < SPECIAL) fatSecs.push(v); }
    difat = dv.getUint32(o + secSize - 4, true);
  }
  const per = secSize / 4;
  const fat = new Uint32Array(fatSecs.length * per).fill(0xFFFFFFFE);
  fatSecs.forEach((s, i) => {
    const o = off(s);
    for (let j = 0; j < per && o + j * 4 + 4 <= u8.length; j++) fat[i * per + j] = dv.getUint32(o + j * 4, true);
  });

  const chain = (start, table) => {
    const out = []; let s = start; const lim = table.length + 1;
    while (s < SPECIAL && s < table.length && out.length < lim) { out.push(s); s = table[s]; }
    if (s < SPECIAL && s >= table.length && out.length < lim) out.push(s);
    return out;
  };
  const readBig = (start, size) => {
    const secs = chain(start, fat);
    if (size == null) size = secs.length * secSize;
    const out = new Uint8Array(size); let w = 0;
    for (const s of secs) {
      if (w >= size) break;
      const o = off(s), n = Math.min(secSize, size - w, Math.max(0, u8.length - o));
      out.set(u8.subarray(o, o + n), w); w += n;
    }
    return w < size ? out.subarray(0, w) : out;
  };

  // Directory entries (128 bytes each)
  const dir = readBig(firstDir, null);
  const ddv = new DataView(dir.buffer, dir.byteOffset, dir.byteLength);
  const td = new TextDecoder('utf-16le');
  const entries = [];
  for (let o = 0; o + 128 <= dir.length; o += 128) {
    const type = dir[o + 66];
    if (type === 0) { entries.push(null); continue; }
    const nl = Math.min(64, ddv.getUint16(o + 64, true));
    const name = td.decode(dir.subarray(o, o + Math.max(0, nl - 2)));
    const start = ddv.getUint32(o + 116, true);
    let size = ddv.getUint32(o + 120, true);
    if (secSize !== 512) size += ddv.getUint32(o + 124, true) * 4294967296;
    entries.push({ name, type, start, size });
  }
  const root = entries[0];
  let mini = null, minifat = null;
  const readMini = (start, size) => {
    if (!mini) {
      mini = readBig(root.start, root.size);
      const mf = readBig(firstMiniFat, numMiniFat * secSize);
      minifat = new Uint32Array(mf.slice(0, mf.byteLength & ~3).buffer);
    }
    const secs = chain(start, minifat);
    const out = new Uint8Array(size); let w = 0;
    for (const s of secs) {
      if (w >= size) break;
      const o = s * miniSize, n = Math.min(miniSize, size - w, Math.max(0, mini.length - o));
      out.set(mini.subarray(o, o + n), w); w += n;
    }
    return out.subarray(0, w);
  };
  const streams = entries.filter(e => e && e.type === 2);
  return {
    streams,
    read: e => (e.size < cutoff ? readMini(e.start, e.size) : readBig(e.start, e.size)),
    find: name => streams.find(e => e.name.toLowerCase() === name.toLowerCase()) || null,
  };
}

// Newer files may store streams zlib-compressed
async function maybeInflate(u8) {
  if (u8.length > 2 && u8[0] === 0x78 && ((u8[0] << 8) | u8[1]) % 31 === 0 && typeof DecompressionStream !== 'undefined') {
    try {
      const ds = new DecompressionStream('deflate');
      const out = await new Response(new Blob([u8]).stream().pipeThrough(ds)).arrayBuffer();
      return { data: new Uint8Array(out), compressed: true };
    } catch (e) { /* not zlib after all */ }
  }
  return { data: u8, compressed: false };
}

/* =====================================================================
   2. 3ds Max chunk reader
      header: uint16 type, uint32 size (bit 31 = container)
      if size == 0: uint64 size follows (bit 63 = container)
   ===================================================================== */
function parseChunks(u8) {
  const dv = new DataView(u8.buffer, u8.byteOffset, u8.byteLength);
  let count = 0;
  const walk = (start, end, depth) => {
    const list = []; let o = start;
    while (o + 6 <= end) {
      const type = dv.getUint16(o, true);
      let size = dv.getUint32(o + 2, true), hdr = 6, cont;
      if (size === 0) {
        if (o + 14 > end) break;
        const lo = dv.getUint32(o + 6, true), hi = dv.getUint32(o + 10, true);
        cont = (hi & 0x80000000) !== 0; size = (hi & 0x7FFFFFFF) * 4294967296 + lo; hdr = 14;
      } else {
        cont = (size & 0x80000000) !== 0; size = size & 0x7FFFFFFF;
      }
      if (size < hdr || o + size > end) break;
      const ch = { type, dv, ds: o + hdr, de: o + size, cont, children: null };
      count++;
      if (cont && depth < 64) ch.children = walk(o + hdr, o + size, depth + 1);
      list.push(ch); o += size;
    }
    return list;
  };
  const list = walk(0, u8.length, 0);
  return { list, count };
}

const clen = c => c.de - c.ds;
const u16 = (c, o) => c.dv.getUint16(c.ds + o, true);
const u32 = (c, o) => c.dv.getUint32(c.ds + o, true);
const i32 = (c, o) => c.dv.getInt32(c.ds + o, true);
const f32 = (c, o) => c.dv.getFloat32(c.ds + o, true);
const td16 = new TextDecoder('utf-16le');
const str16 = c => c ? td16.decode(new Uint8Array(c.dv.buffer, c.dv.byteOffset + c.ds, clen(c) & ~1)).replace(/\0+$/, '') : '';
const first = (c, t) => { if (!c || !c.children) return null; for (const k of c.children) if (k.type === t) return k; return null; };
const findDeep = (list, pred, out = []) => { for (const c of list) { if (pred(c)) out.push(c); else if (c.children) findDeep(c.children, pred, out); } return out; };

function readClassDirectory(chunks) {
  // each entry: 0x2060 = {int32 dll, uint32 classA, uint32 classB, uint32 superclass}, 0x2042 = name
  return findDeep(chunks, c => !!first(c, 0x2060)).map((c, index) => {
    const h = first(c, 0x2060);
    const ok = clen(h) >= 16;
    const a = ok ? u32(h, 4) : 0, b = ok ? u32(h, 8) : 0;
    return { index, dll: ok ? i32(h, 0) : -1, a, b, superId: ok ? u32(h, 12) : 0, name: str16(first(c, 0x2042)), guid: (BigInt(b) << 32n) | BigInt(a) };
  });
}
function readDllDirectory(chunks) {
  return findDeep(chunks, c => !!(first(c, 0x2037) || first(c, 0x2039))).map(c => ({ desc: str16(first(c, 0x2037)), file: str16(first(c, 0x2039)) }));
}

/* =====================================================================
   3. Scene interpretation (references, nodes, controllers, geometry)
   ===================================================================== */
let S = null; // current parse context: { list, classes, log }

const clsOf = ch => (ch && S.classes[ch.type]) || null;
const guidOf = ch => { const c = clsOf(ch); return c ? c.guid : null; };

// References: 0x2034 = positional index list, 0x2035 = [x, (slot, index) pairs...]
function refsOf(ch) {
  if (!ch) return [];
  if (ch._refs) return ch._refs;
  const out = [];
  const get = i => (i >= 0 && i < S.list.length ? S.list[i] : null);
  const r35 = first(ch, 0x2035), r34 = first(ch, 0x2034);
  if (r35) {
    const v = []; for (let i = 0; i + 4 <= clen(r35); i += 4) v.push(i32(r35, i));
    const rest = v.slice(1);
    let paired = rest.length % 2 === 0;
    for (let k = 2; paired && k < rest.length; k += 2) if (rest[k] <= rest[k - 2]) paired = false;
    if (paired) { for (let k = 0; k < rest.length; k += 2) if (rest[k] >= 0 && rest[k] < 4096) out[rest[k]] = get(rest[k + 1]); }
    else rest.forEach((idx, k) => { out[k] = get(idx); });
  } else if (r34) {
    for (let i = 0; i + 4 <= clen(r34); i += 4) out.push(get(i32(r34, i)));
  }
  ch._refs = out;
  return out;
}

// ---- controllers -> TRS ----
function ctrlFloat(c, def) {
  if (!c) return def;
  const inner = first(c, 0x7127) || c;
  const v = first(inner, 0x2501);
  return v && clen(v) >= 4 ? f32(v, 0) : def;
}
const vec3 = (c, o = 0) => [f32(c, o), f32(c, o + 4), f32(c, o + 8)];

function getPosition(c, depth = 0) {
  if (!c || depth > 3) return [0, 0, 0];
  const g = guidOf(c);
  if (g === 0xFFEE238A118F7E02n) { const r = refsOf(c); return [ctrlFloat(r[0], 0), ctrlFloat(r[1], 0), ctrlFloat(r[2], 0)]; } // Position XYZ
  const p = first(c, 0x2503);
  if (p && clen(p) >= 12) return vec3(p);
  const r = refsOf(c).filter(Boolean); // list / wire controllers: follow first sub-controller
  return r.length ? getPosition(r[0], depth + 1) : [0, 0, 0];
}
function getRotation(c, depth = 0) {
  const q = new THREE.Quaternion();
  if (!c || depth > 3) return q;
  const g = guidOf(c);
  if (g === 0x2012n) { // Euler XYZ: three float sub-controllers, radians
    const r = refsOf(c);
    return q.setFromEuler(new THREE.Euler(ctrlFloat(r[0], 0), ctrlFloat(r[1], 0), ctrlFloat(r[2], 0), 'ZYX'));
  }
  const d = first(c, 0x2504);
  if (d && clen(d) >= 16) {
    // Max quaternions are stored left-handed: conjugate to get the standard rotation
    q.set(-f32(d, 0), -f32(d, 4), -f32(d, 8), f32(d, 12)).normalize();
    return q;
  }
  const r = refsOf(c).filter(Boolean);
  return r.length ? getRotation(r[0], depth + 1) : q;
}
function getScale(c) {
  if (!c) return [1, 1, 1];
  const g = guidOf(c);
  if (g === 0xFEEE238B118F7C01n) { const r = refsOf(c); return [ctrlFloat(r[0], 1), ctrlFloat(r[1], 1), ctrlFloat(r[2], 1)]; } // Scale XYZ
  const s = first(c, 0x2505) || first(c, 0x2501);
  if (s && clen(s) >= 12) { const v = vec3(s); if (v.every(x => isFinite(x) && x !== 0)) return v; }
  return [1, 1, 1];
}
function readTransform(prs) {
  const t = { p: [0, 0, 0], q: new THREE.Quaternion(), s: [1, 1, 1] };
  if (!prs) return t;
  const r = refsOf(prs);
  if (guidOf(prs) === 0x2005n || r.length >= 3) { // Position/Rotation/Scale
    try { t.p = getPosition(r[0]); } catch (e) {}
    try { t.q = getRotation(r[1]); } catch (e) {}
    try { t.s = getScale(r[2]); } catch (e) {}
  }
  if (!t.p.every(isFinite)) t.p = [0, 0, 0];
  return t;
}

// ---- materials (diffuse colour only) ----
function propColor(params, idx) {
  if (!params || !params.children) return null;
  for (const c of params.children) {
    if ((c.type & 0x100E) && clen(c) >= 2 && u16(c, 0) === idx) {
      const o = clen(c) > 23 ? 15 : 11;
      if (clen(c) < o + 12) return null;
      const v = vec3(c, o);
      return v.every(x => isFinite(x) && x >= 0 && x <= 1.5) ? v : null;
    }
  }
  return null;
}
function materialColor(mat, depth = 0) {
  if (!mat || depth > 3) return null;
  try {
    const g = guidOf(mat), r = refsOf(mat);
    if (g === 0x2n) return propColor(refsOf(r[2])[0], 0x01);           // Standard
    if (g === 0x200n) { const k = r.filter(Boolean); return materialColor(k[k.length - 1], depth + 1); } // Multi/Sub
    if (g === 0x7034695C37BF3F2Fn) return propColor(r[1], 0x01);         // VRayMtl
    if (g === 0x4A16365470B05735n) return propColor(r[0], 0x1A);         // Architectural
  } catch (e) {}
  return null;
}

// ---- geometry ----
// Detect by content: 0x08FE holds the mesh data of Editable Mesh / Editable Poly
function findGeometry(ch, depth = 0, seen = new Set()) {
  if (!ch || depth > 5 || seen.has(ch)) return null;
  seen.add(ch);
  const p = first(ch, 0x08FE);
  if (p && p.children) {
    if (first(p, 0x0914) && first(p, 0x0912)) return { kind: 'Editable Mesh', src: ch, p };
    if (first(p, 0x0100) && (first(p, 0x011A) || first(p, 0x0108))) return { kind: 'Editable Poly', src: ch, p };
  }
  for (const r of refsOf(ch)) { const g = findGeometry(r, depth + 1, seen); if (g) return g; } // derived objects, shells…
  return null;
}

function triangulate(poly, P, idx) {
  const n = poly.length;
  if (n < 3) return 0;
  if (n === 3) { idx.push(poly[0], poly[1], poly[2]); return 1; }
  let nx = 0, ny = 0, nz = 0; // Newell normal
  for (let i = 0; i < n; i++) {
    const a = poly[i] * 3, b = poly[(i + 1) % n] * 3;
    nx += (P[a + 1] - P[b + 1]) * (P[a + 2] + P[b + 2]);
    ny += (P[a + 2] - P[b + 2]) * (P[a] + P[b]);
    nz += (P[a] - P[b]) * (P[a + 1] + P[b + 1]);
  }
  const fan = () => { for (let i = 1; i < n - 1; i++) idx.push(poly[0], poly[i], poly[i + 1]); return n - 2; };
  if (n === 4) return fan();
  const ax = Math.abs(nx), ay = Math.abs(ny), az = Math.abs(nz);
  const [u, v] = ax >= ay && ax >= az ? [1, 2] : ay >= az ? [2, 0] : [0, 1];
  let tris = null;
  try { tris = THREE.ShapeUtils.triangulateShape(poly.map(p => new THREE.Vector2(P[p * 3 + u], P[p * 3 + v])), []); } catch (e) {}
  if (!tris || !tris.length) return fan();
  for (const t of tris) {
    let a = poly[t[0]], b = poly[t[1]], c = poly[t[2]];
    const e1x = P[b * 3] - P[a * 3], e1y = P[b * 3 + 1] - P[a * 3 + 1], e1z = P[b * 3 + 2] - P[a * 3 + 2];
    const e2x = P[c * 3] - P[a * 3], e2y = P[c * 3 + 1] - P[a * 3 + 1], e2z = P[c * 3 + 2] - P[a * 3 + 2];
    const dot = (e1y * e2z - e1z * e2y) * nx + (e1z * e2x - e1x * e2z) * ny + (e1x * e2y - e1y * e2x) * nz;
    if (dot < 0) [b, c] = [c, b];
    idx.push(a, b, c);
  }
  return tris.length;
}

function buildEditableMesh(p) {
  const vc = first(p, 0x0914), fc = first(p, 0x0912);
  const nv = u32(vc, 0);
  if (4 + nv * 12 > clen(vc)) throw new Error('vertex block is shorter than its count');
  const pos = new Float32Array(nv * 3);
  for (let i = 0; i < nv * 3; i++) pos[i] = f32(vc, 4 + i * 4);
  const nfc = Math.min(u32(fc, 0), Math.floor((clen(fc) - 4) / 20));
  const idx = [], sm = [];
  for (let f = 0; f < nfc; f++) {
    const o = 4 + f * 20, a = u32(fc, o), b = u32(fc, o + 4), c = u32(fc, o + 8);
    if (a < nv && b < nv && c < nv) { idx.push(a, b, c); sm.push(u32(fc, o + 12)); } // Face: v[3], smGroup, flags
  }
  return { pos, idx, sm, nv, polys: nfc };
}

function readPolys011A(c) { // n, n indices, uint16 flags, optional extras
  const L = clen(c), out = []; let o = 4;
  while (o + 4 <= L) {
    const n = u32(c, o); o += 4;
    if (n > 65536 || o + n * 4 + 2 > L) break;
    const pts = new Array(n);
    for (let k = 0; k < n; k++) pts[k] = u32(c, o + k * 4);
    o += n * 4;
    const flags = u16(c, o); o += 2;
    if (flags & 0x01) o += 4;
    if (flags & 0x08) o += 2;
    if (flags & 0x10) o += 4;
    if (flags & 0x20) o += 8 * Math.max(0, n - 3);
    if (n >= 3) out.push(pts);
  }
  return out;
}
function readPolys0108(c) { // fixed 6 int32 records
  const out = [];
  for (let o = 4; o + 24 <= clen(c); o += 24) {
    const v = []; for (let k = 0; k < 6; k++) v.push(i32(c, o + k * 4));
    let i = 5; while (i > 3 && v[i] < 0) i--;
    if (i > 2) out.push(v.slice(1, i));
  }
  return out;
}
function buildEditablePoly(p) {
  const vc = first(p, 0x0100);
  const nv = Math.floor((clen(vc) - 4) / 16);
  const pos = new Float32Array(nv * 3);
  for (let i = 0; i < nv; i++) { const o = 4 + i * 16 + 4; pos[i * 3] = f32(vc, o); pos[i * 3 + 1] = f32(vc, o + 4); pos[i * 3 + 2] = f32(vc, o + 8); }
  const pc = first(p, 0x011A);
  const polys = pc ? readPolys011A(pc) : readPolys0108(first(p, 0x0108));
  const idx = [];
  for (const poly of polys) if (poly.every(i => i >= 0 && i < nv)) triangulate(poly, pos, idx);
  return { pos, idx, nv, polys: polys.length };
}

const geomCache = new Map();
function geometryFor(g) {
  if (geomCache.has(g.src)) return geomCache.get(g.src);
  let d = null;
  try { d = g.kind === 'Editable Mesh' ? buildEditableMesh(g.p) : buildEditablePoly(g.p); }
  catch (e) { S.log.push(`Could not decode ${g.kind} data: ${e.message}`); }
  let res = null;
  if (d && d.idx.length) {
    const geo = new THREE.BufferGeometry();
    geo.setAttribute('position', new THREE.BufferAttribute(d.pos, 3));
    geo.setIndex(d.nv > 65535 ? new THREE.Uint32BufferAttribute(d.idx, 1) : new THREE.Uint16BufferAttribute(d.idx, 1));
    geo.computeVertexNormals();
    geo.computeBoundingSphere();
    res = { geo, verts: d.nv, tris: d.idx.length / 3, polys: d.polys, kind: g.kind };
  }
  geomCache.set(g.src, res);
  return res;
}

// ---- parametric primitives (old ParamBlock and ParamBlock2) ----
function readParamBlock(pb) {
  const vals = [];
  if (!pb || !pb.children) return vals;
  for (const e of pb.children) {
    if (e.type === 0x0002) {
      // ParamBlock record = sub-chunks: 0x0003 index, 0x0100 float | 0x0101 int | 0x0104 bool
      let o = e.ds, idx = null, val = null;
      while (o + 6 <= e.de) {
        const t = e.dv.getUint16(o, true), s = e.dv.getUint32(o + 2, true) & 0x7FFFFFFF;
        if (s < 6 || o + s > e.de) break;
        const d = o + 6, n = s - 6;
        if (t === 0x0003 && n >= 4) idx = e.dv.getUint32(d, true);
        else if (t === 0x0100 && n >= 4) val = e.dv.getFloat32(d, true);
        else if ((t === 0x0101 || t === 0x0104) && n >= 4) val = e.dv.getInt32(d, true); // int, bool
        o += s;
      }
      if (idx !== null && idx < 256) vals[idx] = val;
    } else if (e.type === 0x000A && clen(e) >= 15) {
      // ParamBlock2 entry: u16 id, u32 type (1 int, 4 bool, else float-like), flags, value @ 11
      const id = u16(e, 0), type = u32(e, 2);
      if (id < 256) vals[id] = (type === 1 || type === 4) ? i32(e, 11) : f32(e, 11);
    }
  }
  return vals;
}
const num = (v, d) => (typeof v === 'number' && isFinite(v) ? v : d);
const segs = (v, d, max = 200) => Math.max(1, Math.min(max, Math.round(num(v, d))));
const nz = v => Math.abs(v) || 1e-4;
const PRIMITIVES = {
  // indices follow the Max SDK parameter order for each object
  Box: p => { // 0 length(Y) 1 width(X) 2 height(Z) 3 wsegs 4 lsegs 5 hsegs
    const l = num(p[0], 25), w = num(p[1], 25), h = num(p[2], 25);
    const g = new THREE.BoxGeometry(nz(w), nz(l), nz(h), segs(p[3], 1), segs(p[4], 1), segs(p[5], 1));
    return g.translate(0, 0, h / 2);
  },
  Sphere: p => { // 0 radius 1 segments 2 smooth 3 hemisphere 4 chop/squash 5 base-to-pivot 6 uvs 7-9 slice
    const r = Math.abs(num(p[0], 25)), s = segs(p[1], 32);
    const hemi = Math.min(Math.max(num(p[3], 0), 0), 0.99);
    const theta = Math.acos(Math.min(1, Math.max(-1, 1 - 2 * (1 - hemi)))); // keep the cap above the cut plane
    const g = new THREE.SphereGeometry(nz(r), Math.max(3, s), Math.max(2, Math.round(s / 2 * (hemi ? theta / Math.PI : 1))), 0, Math.PI * 2, 0, theta).rotateX(Math.PI / 2);
    g.userData.smooth = p[2] !== 0;
    return p[5] ? g.translate(0, 0, r) : g;
  },
  GeoSphere: p => { // ParamBlock2 ids: 0 radius 1 segments 2 base type (0 tetra, 1 octa, 2 icosa) 3 hemisphere 4 smooth 5 base-to-pivot 6 uvs
    const r = Math.abs(num(p[0], 25)), detail = segs(p[1], 4, 64) - 1, type = num(p[2], 2);
    let g;
    if (type === 0) g = new THREE.TetrahedronGeometry(nz(r), detail);
    else if (type === 1) g = new THREE.OctahedronGeometry(nz(r), detail);
    else {
      g = new THREE.IcosahedronGeometry(nz(r), detail);
      const t = (1 + Math.sqrt(5)) / 2; // put a vertex on the pole, as Max does
      g.applyMatrix4(new THREE.Matrix4().makeRotationFromQuaternion(new THREE.Quaternion().setFromUnitVectors(new THREE.Vector3(0, 1, t).normalize(), new THREE.Vector3(0, 0, 1))));
    }
    const pos = g.attributes.position, nrm = new Float32Array(pos.count * 3), v = new THREE.Vector3();
    for (let i = 0; i < pos.count; i++) { v.fromBufferAttribute(pos, i).normalize(); nrm[i * 3] = v.x; nrm[i * 3 + 1] = v.y; nrm[i * 3 + 2] = v.z; }
    g.setAttribute('normal', new THREE.BufferAttribute(nrm, 3)); // true sphere normals for smooth shading
    g.userData.smooth = p[4] !== 0;
    return p[5] ? g.translate(0, 0, r) : g;
  },
  Pyramid: p => { // 0 width(X) 1 depth(Y) 2 height 3 wsegs 4 dsegs 5 hsegs 6 uvs (verified on a Max R3.1 file)
    const w = nz(num(p[0], 20)), d = nz(num(p[1], 20)), h = num(p[2], 20) || 1e-4;
    const ws = segs(p[3], 1), ds = segs(p[4], 1), hs = segs(p[5], 1);
    const P = [], flip = h < 0;
    const tri = (a, b, c) => {
      const ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2], vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
      if (Math.hypot(uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx) < 1e-12) return; // apex-degenerate
      if (flip) P.push(...a, ...c, ...b); else P.push(...a, ...b, ...c);
    };
    const pt = (u, v, j) => { const s = 1 - j / hs; return [u * w / 2 * s, v * d / 2 * s, h * j / hs]; };
    // four sides, walking the base counter-clockwise seen from above; each side is a ws/ds x hs grid shrinking to the apex
    const edges = [[[-1, -1], [1, -1], ws], [[1, -1], [1, 1], ds], [[1, 1], [-1, 1], ws], [[-1, 1], [-1, -1], ds]];
    for (const [[u0, v0], [u1, v1], n] of edges) {
      for (let i = 0; i < n; i++) for (let j = 0; j < hs; j++) {
        const t0 = i / n, t1 = (i + 1) / n, U = t => u0 + (u1 - u0) * t, V = t => v0 + (v1 - v0) * t;
        const A = pt(U(t0), V(t0), j), B = pt(U(t1), V(t1), j), C = pt(U(t1), V(t1), j + 1), D = pt(U(t0), V(t0), j + 1);
        tri(A, B, C); tri(A, C, D);
      }
    }
    // base, facing away from the apex
    for (let i = 0; i < ws; i++) for (let k = 0; k < ds; k++) {
      const x0 = -w / 2 + w * i / ws, x1 = -w / 2 + w * (i + 1) / ws, y0 = -d / 2 + d * k / ds, y1 = -d / 2 + d * (k + 1) / ds;
      tri([x0, y0, 0], [x1, y1, 0], [x1, y0, 0]); tri([x0, y0, 0], [x0, y1, 0], [x1, y1, 0]);
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(new Float32Array(P), 3));
    g.computeVertexNormals(); // non-indexed, so every face stays flat
    g.userData.smooth = false;
    return g;
  },
  Teapot: p => { // 0 radius 1 segments 2 smooth 3 (unused) 4 body 5 handle 6 spout 7 lid 8 uvs (verified on a Max R3.1 file)
    if (!TeapotGeometry) throw new Error('teapot geometry script did not load');
    // Utah teapot (Blinn proportions): body radius 2.0, height 3.15. Max's Radius is the body radius.
    const r = Math.abs(num(p[0], 25)) || 1e-4, size = r / 2 * 1.575;
    const body = p[4] !== 0 || p[5] !== 0 || p[6] !== 0, lid = p[7] !== 0;
    if (!body && !lid) throw new Error('all teapot parts are switched off');
    const g = new TeapotGeometry(size, segs(p[1], 4, 32), body, lid, body, false, true);
    g.rotateX(Math.PI / 2).translate(0, 0, size); // Y-up -> Z-up, base on the pivot
    g.userData.smooth = p[2] !== 0;
    return g;
  },
  Plane: p => { // ParamBlock2 ids: 0 length(Y) 1 width(X) 2 length segs 3 width segs 4 render scale 5 render density 6 uvs
    const g = new THREE.PlaneGeometry(nz(num(p[1], 25)), nz(num(p[0], 25)), segs(p[3], 4), segs(p[2], 4)); // already in XY, facing +Z
    g.userData.smooth = false;
    return g;
  },
  Cylinder: p => { // 0 radius 1 height 2 hsegs 3 capsegs 4 sides 5 smooth
    const r = Math.max(0, num(p[0], 15)), h = num(p[1], 25);
    const g = new THREE.CylinderGeometry(nz(r), nz(r), nz(h), Math.max(3, segs(p[4], 18)), segs(p[2], 1)).rotateX(Math.PI / 2).translate(0, 0, Math.abs(h) / 2);
    if (h < 0) g.scale(1, 1, -1);
    g.userData.smooth = p[5] !== 0;
    return g;
  },
  Cone: p => { // 0 radius1 (base) 1 radius2 (top) 2 height 3 hsegs 4 capsegs 5 sides 6 smooth
    // Max clamps negative radii to 0 (a creation drag can store e.g. -20 while the UI shows 0.0)
    const r1 = Math.max(0, num(p[0], 15)), r2 = Math.max(0, num(p[1], 0)), h = num(p[2], 25);
    const g = new THREE.CylinderGeometry(r2, r1, nz(h), Math.max(3, segs(p[5], 24)), segs(p[3], 1)).rotateX(Math.PI / 2).translate(0, 0, Math.abs(h) / 2);
    if (h < 0) g.scale(1, 1, -1); // negative height builds downward from the pivot
    g.userData.smooth = p[6] !== 0;
    return g;
  },
  Torus: p => { // 0 radius1 1 radius2 2 rotation 3 twist 4 segments 5 sides
    const g = new THREE.TorusGeometry(nz(num(p[0], 25)), nz(num(p[1], 5)), Math.max(3, segs(p[5], 12)), Math.max(3, segs(p[4], 24)));
    g.userData.smooth = true;
    return g;
  },
  Tube: p => { // 0 radius1 1 radius2 2 height 3 sides 4 capsegs 5 hsegs 6 smooth (verified on a Max R3.1 file)
    const a = Math.max(0, num(p[0], 15)), b = Math.max(0, num(p[1], 10)), h = num(p[2], 25);
    const ro = Math.max(a, b), ri = Math.min(a, b), H = nz(h);
    const sides = Math.max(3, segs(p[3], 18)), cs = segs(p[4], 1), hs = segs(p[5], 1);
    const wall = r => new THREE.CylinderGeometry(nz(r), nz(r), H, sides, hs, true).rotateX(Math.PI / 2).translate(0, 0, H / 2);
    const top = new THREE.RingGeometry(ri, nz(ro), sides, cs, -Math.PI / 2).translate(0, 0, H);
    const bottom = flipGeo(new THREE.RingGeometry(ri, nz(ro), sides, cs, -Math.PI / 2));
    const parts = [wall(ro), top, bottom];
    if (ri > 0) parts.push(flipGeo(wall(ri)));
    const g = mergeGeos(parts);
    if (h < 0) g.scale(1, 1, -1);
    g.userData.smooth = p[6] !== 0;
    return g;
  },
};
// helpers: turn a geometry inside out, and concatenate non-indexed geometries
function flipGeo(g) {
  g = g.index ? g.toNonIndexed() : g;
  const P = g.attributes.position.array, N = g.attributes.normal.array;
  for (let i = 0; i < P.length; i += 9) for (const A of [P, N]) for (let k = 0; k < 3; k++) { const t = A[i + 3 + k]; A[i + 3 + k] = A[i + 6 + k]; A[i + 6 + k] = t; }
  for (let i = 0; i < N.length; i++) N[i] = -N[i];
  return g;
}
function mergeGeos(list) {
  list = list.map(g => (g.index ? g.toNonIndexed() : g));
  const n = list.reduce((s, g) => s + g.attributes.position.array.length, 0);
  const P = new Float32Array(n), N = new Float32Array(n);
  let o = 0;
  for (const g of list) { P.set(g.attributes.position.array, o); N.set(g.attributes.normal.array, o); o += g.attributes.position.array.length; }
  const out = new THREE.BufferGeometry();
  out.setAttribute('position', new THREE.BufferAttribute(P, 3));
  out.setAttribute('normal', new THREE.BufferAttribute(N, 3));
  return out;
}
function primitiveFor(obj) {
  if (!obj) return null;
  if (geomCache.has(obj)) return geomCache.get(obj);
  const name = (clsOf(obj) || {}).name, build = PRIMITIVES[name];
  let res = null;
  if (build) {
    const pb = refsOf(obj).find(r => r && r.children && r.children.some(c => c.type === 0x0002 || c.type === 0x000A));
    try {
      const geo = build(readParamBlock(pb));
      geo.computeBoundingSphere();
      const tris = geo.index ? geo.index.count / 3 : geo.attributes.position.count / 3;
      res = { geo, verts: geo.attributes.position.count, tris, polys: tris, kind: `${name} (parametric)`, smooth: !!geo.userData.smooth };
    } catch (e) { S.log.push(`Could not rebuild ${name}: ${e.message}`); }
  }
  geomCache.set(obj, res);
  return res;
}
// Node pivot offset: 0x096A position, 0x096B rotation, 0x096C scale (+ scale axis)
function applyObjectOffset(m, ch) {
  const p = first(ch, 0x096A), q = first(ch, 0x096B), s = first(ch, 0x096C);
  if (p && clen(p) >= 12) { const v = vec3(p); if (v.every(isFinite)) m.position.fromArray(v); }
  if (q && clen(q) >= 16) m.quaternion.set(-f32(q, 0), -f32(q, 4), -f32(q, 8), f32(q, 12)).normalize();
  if (s && clen(s) >= 12) { const v = vec3(s); if (v.every(x => isFinite(x) && x !== 0)) m.scale.fromArray(v); }
}
// Node wire colour: 0x0974 = R, G, B bytes
function wireColor(ch) {
  const c = first(ch, 0x0974);
  if (!c || clen(c) < 3) return null;
  const d = c.dv, o = c.ds;
  return srgbColor(d.getUint8(o) / 255, d.getUint8(o + 1) / 255, d.getUint8(o + 2) / 255);
}

// ---- modifier stacks: Derived Objects (chunk type 0x2032/0x2033, not a class index) ----
// refs = [top modifier, ..., bottom modifier, base object]; local mod data lives in 0x2500 as one 0x2510..0x2513 group per modifier
const DERIVED_TYPES = new Set([0x2032, 0x2033]);
const isDerived = ch => !!ch && !clsOf(ch) && DERIVED_TYPES.has(ch.type);
const bytesOf = c => new Uint8Array(c.dv.buffer, c.dv.byteOffset + c.ds, clen(c));
const evalCache = new Map();

function baseOf(obj) {
  let o = obj, d = 0;
  while (isDerived(o) && d++ < 8) { const it = refsOf(o).filter(Boolean); o = it[it.length - 1] || null; }
  return o;
}
function modifiersOf(obj) {
  const out = []; let o = obj, d = 0;
  while (isDerived(o) && d++ < 8) { const it = refsOf(o).filter(Boolean); out.push(...it.slice(0, -1)); o = it[it.length - 1]; }
  return out;
}
function describeObject(obj) {
  if (!isDerived(obj)) return (clsOf(obj) || {}).name || 'Unknown';
  const base = baseOf(obj), mods = modifiersOf(obj).map(m => (clsOf(m) || {}).name || 'modifier');
  return ((clsOf(base) || {}).name || 'Unknown') + (mods.length ? ' + ' + mods.join(', ') : '');
}

// Box with Max's own vertex order (needed so Edit Mesh vertex indices line up). 1x1x1 segments only.
function maxBoxRaw(obj) {
  const pb = refsOf(obj).find(r => r && r.children && r.children.some(c => c.type === 0x0002 || c.type === 0x000A));
  const p = readParamBlock(pb);
  if (segs(p[3], 1) !== 1 || segs(p[4], 1) !== 1 || segs(p[5], 1) !== 1) return null;
  const l = num(p[0], 25), w = num(p[1], 25), h = num(p[2], 25), x = w / 2, y = l / 2;
  const pos = new Float32Array([-x, -y, 0, x, -y, 0, -x, y, 0, x, y, 0, -x, -y, h, x, -y, h, -x, y, h, x, y, h]);
  const idx = [0, 2, 3, 3, 1, 0, 4, 5, 7, 7, 6, 4, 0, 1, 5, 5, 4, 0, 1, 3, 7, 7, 5, 1, 3, 2, 6, 6, 7, 3, 2, 0, 4, 4, 6, 2];
  const sm = [2, 2, 4, 4, 8, 8, 16, 16, 32, 32, 64, 64]; // one smoothing group per side, like Max's box
  return { pos, idx, sm, nv: 8, polys: 12, kind: 'Box (parametric)', smooth: false };
}

function applyModifier(mod, local, raw) {
  const name = (clsOf(mod) || {}).name || 'Unknown modifier';
  if (name === 'Edit Mesh') {
    const ld = local && first(local, 0x4000);
    if (!ld) return true; // modifier present but nothing edited
    const nvc = first(ld, 0x0100);
    if (nvc && clen(nvc) >= 4 && u32(nvc, 0) !== raw.nv) { S.log.push(`Edit Mesh expects ${u32(nvc, 0)} vertices but the base mesh has ${raw.nv}; edits skipped.`); return false; }
    const mv = first(ld, 0x0140); // vertex moves: count, then {u32 index, float dx, dy, dz}
    if (mv) {
      const n = u32(mv, 0);
      for (let k = 0; k < n; k++) {
        const o = 4 + k * 16; if (o + 16 > clen(mv)) break;
        const i = u32(mv, o);
        if (i < raw.nv) { raw.pos[i * 3] += f32(mv, o + 4); raw.pos[i * 3 + 1] += f32(mv, o + 8); raw.pos[i * 3 + 2] += f32(mv, o + 12); }
      }
    }
    raw.kind += ' + Edit Mesh';
    return true;
  }
  S.log.push(`${name} modifier is not evaluated by this reader; the object is shown without its modifiers.`);
  return false;
}

// Exact, index-preserving mesh for objects whose vertex order matches Max (meshes, polys, simple boxes), with modifiers applied
function evalRaw(obj, depth = 0) {
  if (!obj || depth > 8) return null;
  if (isDerived(obj)) {
    const it = refsOf(obj).filter(Boolean);
    if (!it.length) return null;
    const base = it[it.length - 1], mods = it.slice(0, -1);
    const raw = evalRaw(base, depth + 1);
    if (!raw || !mods.length) return raw;
    const md = first(obj, 0x2500);
    // one ModContext group per modifier: 0x2510 tm, 0x2511 bounds, 0x2512 local data (container), 0x2513 flags
    const locals = [];
    if (md) for (const c of parseChunks(bytesOf(md)).list) {
      if (c.type === 0x2510) locals.push(null);
      else if (c.type === 0x2512) { if (!locals.length) locals.push(null); locals[locals.length - 1] = c; }
    }
    const out = { ...raw, pos: raw.pos.slice(), idx: raw.idx.slice() };
    for (let i = mods.length - 1; i >= 0; i--) if (!applyModifier(mods[i], locals[i], out)) return null; // bottom of stack first
    return out;
  }
  const g = findGeometry(obj);
  if (g) {
    try {
      const d = g.kind === 'Editable Mesh' ? buildEditableMesh(g.p) : buildEditablePoly(g.p);
      return { pos: d.pos, idx: d.idx, sm: d.sm, nv: d.nv, polys: d.polys, kind: g.kind, smooth: false };
    } catch (e) { S.log.push(`Could not decode ${g.kind} data: ${e.message}`); return null; }
  }
  if ((clsOf(obj) || {}).name === 'Box') return maxBoxRaw(obj);
  return null;
}

// Per-corner normals from Max smoothing groups: a corner averages the faces around its vertex
// that share a smoothing bit with its own face; group 0 faces stay flat. Returns non-indexed arrays.
function smoothingGroupGeometry(pos, idx, sm) {
  const nt = idx.length / 3, fn = new Float32Array(nt * 3), byVert = new Map();
  for (let t = 0; t < nt; t++) {
    const a = idx[t * 3] * 3, b = idx[t * 3 + 1] * 3, c = idx[t * 3 + 2] * 3;
    const ux = pos[b] - pos[a], uy = pos[b + 1] - pos[a + 1], uz = pos[b + 2] - pos[a + 2];
    const vx = pos[c] - pos[a], vy = pos[c + 1] - pos[a + 1], vz = pos[c + 2] - pos[a + 2];
    fn[t * 3] = uy * vz - uz * vy; fn[t * 3 + 1] = uz * vx - ux * vz; fn[t * 3 + 2] = ux * vy - uy * vx; // area-weighted
    for (let k = 0; k < 3; k++) { const v = idx[t * 3 + k]; let l = byVert.get(v); if (!l) byVert.set(v, l = []); l.push(t); }
  }
  const P = new Float32Array(nt * 9), N = new Float32Array(nt * 9);
  for (let t = 0; t < nt; t++) for (let k = 0; k < 3; k++) {
    const v = idx[t * 3 + k], o = t * 9 + k * 3, g = sm[t] >>> 0;
    P[o] = pos[v * 3]; P[o + 1] = pos[v * 3 + 1]; P[o + 2] = pos[v * 3 + 2];
    let x = 0, y = 0, z = 0;
    for (const f of (g ? byVert.get(v) : [t])) if (f === t || ((sm[f] >>> 0) & g)) { x += fn[f * 3]; y += fn[f * 3 + 1]; z += fn[f * 3 + 2]; }
    const l = Math.hypot(x, y, z) || 1;
    N[o] = x / l; N[o + 1] = y / l; N[o + 2] = z / l;
  }
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(P, 3));
  geo.setAttribute('normal', new THREE.BufferAttribute(N, 3));
  return geo;
}

function evalObject(obj) {
  if (!obj) return null;
  if (evalCache.has(obj)) return evalCache.get(obj);
  let res = null;
  const raw = evalRaw(obj);
  if (raw && raw.idx.length && raw.sm && raw.sm.length === raw.idx.length / 3) {
    const geo = smoothingGroupGeometry(raw.pos, raw.idx, raw.sm);
    geo.computeBoundingSphere();
    res = { geo, verts: raw.nv, tris: raw.idx.length / 3, polys: raw.polys, kind: raw.kind, smooth: true, smGroups: true };
  } else if (raw && raw.idx.length) {
    const geo = new THREE.BufferGeometry();
    geo.setAttribute('position', new THREE.BufferAttribute(raw.pos, 3));
    geo.setIndex(raw.nv > 65535 ? new THREE.Uint32BufferAttribute(raw.idx, 1) : new THREE.Uint16BufferAttribute(raw.idx, 1));
    geo.computeVertexNormals(); geo.computeBoundingSphere();
    res = { geo, verts: raw.nv, tris: raw.idx.length / 3, polys: raw.polys, kind: raw.kind, smooth: raw.smooth };
  } else {
    // fall back to the base object without its modifiers
    const base = baseOf(obj), g = findGeometry(base);
    res = g ? geometryFor(g) : primitiveFor(base);
  }
  evalCache.set(obj, res);
  return res;
}

function readScene(sceneChunks) {
  const top = sceneChunks[0];
  S.list = top && top.children ? top.children : [];
  const nodes = [];
  S.list.forEach((ch, i) => {
    const c = clsOf(ch);
    if (!c || c.superId !== 1) return;
    if (c.a === 2 && c.b === 0) { S.rootIndex = i; return; } // scene root node
    if (!(c.a === 1 && c.b === 0)) return;                  // INode
    const pc = first(ch, 0x0960);
    const refs = refsOf(ch);
    const obj = refs[1] || null;
    const node = {
      index: i, chunk: ch,
      name: str16(first(ch, 0x0962)) || `Node ${i}`,
      parent: pc && clen(pc) >= 4 ? i32(pc, 0) : -1,
      prs: refs[0] || null, obj, mat: refs[3] || null,
      objClass: obj ? describeObject(obj) : null,
      children: [], three: null, mesh: null, stats: null,
    };
    nodes.push(node);
  });
  return nodes;
}


/* =====================================================================
   4. three.js scene building and the Loader API
   ===================================================================== */
function resetState() {
  geomCache.clear();
  evalCache.clear();
}

function defaultMaterial(color, smooth) {
  return new THREE.MeshStandardMaterial({ color, roughness: 0.7, metalness: 0, side: THREE.DoubleSide, flatShading: !smooth });
}

function hashColor(name) {
  let h = 2166136261; for (const ch of name) { h ^= ch.charCodeAt(0); h = Math.imul(h, 16777619); }
  return new THREE.Color().setHSL(((h >>> 0) % 360) / 360, 0.38, 0.62);
}

function buildScene(nodes, opts) {
  const root = new THREE.Group();
  root.name = 'MaxScene';
  if (!opts.zUp) root.rotation.x = -Math.PI / 2; // 3ds Max is Z-up, three.js is Y-up
  const byIndex = new Map(nodes.map(n => [n.index, n]));
  for (const n of nodes) {
    const grp = new THREE.Group();
    grp.name = n.name;
    const t = readTransform(n.prs);
    grp.position.fromArray(t.p); grp.quaternion.copy(t.q); grp.scale.fromArray(t.s);
    grp.userData.max = { index: n.index, objectClass: n.objClass };
    n.three = grp;
    const geo = evalObject(n.obj);
    if (geo) {
      const mc = materialColor(n.mat);
      const color = mc ? srgbColor(mc[0], mc[1], mc[2]) : wireColor(n.chunk) || hashColor(n.name);
      const material = opts.materialFactory ? opts.materialFactory({ name: n.name, color, smooth: !!geo.smooth, kind: geo.kind })
                                            : defaultMaterial(color, !!geo.smooth);
      const m = new THREE.Mesh(geo.geo, material);
      m.name = n.name + '_mesh';
      applyObjectOffset(m, n.chunk);
      grp.add(m);
      n.mesh = m; n.kind = geo.kind; n.stats = geo;
      grp.userData.max.kind = geo.kind;
    }
  }
  for (const n of nodes) { // parent-relative transforms
    const p = byIndex.get(n.parent);
    let cyc = false;
    for (let a = p, guard = 0; a && guard < 1000; a = byIndex.get(a.parent), guard++) if (a === n) { cyc = true; break; }
    if (p && !cyc) { p.children.push(n); p.three.add(n.three); } else root.add(n.three);
  }
  return root;
}

export class MaxLoader extends THREE.Loader {
  /**
   * @param {THREE.LoadingManager} [manager]
   * Options (set as properties):
   *   zUp             keep Max's Z-up axes instead of rotating the root to Y-up (default false)
   *   materialFactory ({name, color, smooth, kind}) => THREE.Material, to supply your own materials
   */
  constructor(manager) {
    super(manager);
    this.zUp = false;
    this.materialFactory = null;
  }

  load(url, onLoad, onProgress, onError) {
    const loader = new THREE.FileLoader(this.manager);
    loader.setPath(this.path);
    loader.setResponseType('arraybuffer');
    loader.setRequestHeader(this.requestHeader);
    loader.setWithCredentials(this.withCredentials);
    loader.load(url, buffer => {
      this.parseAsync(buffer).then(onLoad).catch(e => {
        if (onError) onError(e); else console.error(e);
        this.manager.itemError(url);
      });
    }, onProgress, onError);
  }

  /**
   * Parse a .max file.
   * @param {ArrayBuffer|Uint8Array} data
   * @returns {Promise<{scene: THREE.Group, nodes: object[], classes: object[], dlls: object[], streams: object[], log: string[], chunkCount: number}>}
   */
  async parseAsync(data) {
    const buffer = data instanceof ArrayBuffer ? data : data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
    const cfb = parseCFB(buffer);
    const streams = cfb.streams.map(s => ({ name: s.name.replace(/[\x00-\x1f]/g, ''), size: s.size, compressed: false }));
    const grab = async name => {
      const e = cfb.find(name); if (!e) return null;
      const { data: bytes, compressed } = await maybeInflate(cfb.read(e));
      const si = streams.find(s => s.name === e.name.replace(/[\x00-\x1f]/g, '')); if (si) si.compressed = compressed;
      return bytes;
    };
    // all async work first, so the synchronous build below cannot interleave with another parse
    const cd = (await grab('ClassDirectory3')) || (await grab('ClassDirectory2')) || (await grab('ClassDirectory'));
    const dd = await grab('DllDirectory');
    const sc = await grab('Scene');

    resetState();
    S = { list: [], classes: [], log: [], rootIndex: -1 };
    try {
      if (!cd) S.log.push('No ClassDirectory stream: objects cannot be identified.');
      S.classes = cd ? readClassDirectory(parseChunks(cd).list) : [];
      const dlls = dd ? readDllDirectory(parseChunks(dd).list) : [];
      if (!sc) throw new Error('This file has no Scene stream, so there is no geometry to read.');
      const parsed = parseChunks(sc);
      if (!parsed.list.length) throw new Error('The Scene stream could not be read. The file may be password-protected or from an unsupported Max version.');
      const nodes = readScene(parsed.list);
      const scene = buildScene(nodes, { zUp: this.zUp, materialFactory: this.materialFactory });
      for (const n of nodes) if (n.obj && !n.mesh) S.log.push(`${n.name}: ${n.objClass} is not decoded and was skipped.`);
      const classes = S.classes.map(({ index, name, a, b, superId, dll }) => ({ index, name, classId: [a, b], superClassId: superId, dll }));
      return {
        scene,
        nodes: nodes.map(n => ({
          name: n.name, index: n.index, parentIndex: n.parent, objectClass: n.objClass, kind: n.kind || null,
          vertices: n.stats ? n.stats.verts : 0, triangles: n.stats ? n.stats.tris : 0, object3D: n.three, mesh: n.mesh,
        })),
        classes, dlls, streams, log: [...new Set(S.log)], chunkCount: parsed.count,
      };
    } finally {
      S = null; resetState(); // geometries now belong to the returned meshes
    }
  }

  /** THREE.Loader compatibility: synchronous parse is impossible (zlib streams), so this throws. */
  parse() { throw new Error('MaxLoader: use parseAsync(arrayBuffer) or loadAsync(url).'); }
}

export default MaxLoader;
