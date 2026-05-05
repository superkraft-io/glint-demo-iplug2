#!/usr/bin/env node
// Auto-increment the plugin patch version in GP/config.h.
//
// Usage:
//   node scripts/bump_build_number.mjs
//
// Reads/writes: GP/config.h
//   - Increments the patch component of PLUG_VERSION_STR ("M.m.p").
//   - patch wraps 0..99 -> minor +=1; minor wraps 0..99 -> major +=1.
//   - Keeps PLUG_VERSION_HEX (0x00MMmmpp) in sync.
//
// Cross-platform (Windows + macOS). Invoked from prebuild-win.bat and safe
// to call from macOS/Xcode pre-action scripts as well.

import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname  = dirname(__filename);
const repoRoot   = resolve(__dirname, '..');
const configPath = resolve(repoRoot, 'GP', 'config.h');

if (!existsSync(configPath)) {
  console.error(`[bump_build_number] ERROR: ${configPath} not found`);
  process.exit(1);
}

const original = readFileSync(configPath, 'utf8');

const strRe = /(#define\s+PLUG_VERSION_STR\s+")(\d+)\.(\d+)\.(\d+)(")/;
const hexRe = /(#define\s+PLUG_VERSION_HEX\s+)0x[0-9A-Fa-f]+/;

const strMatch = original.match(strRe);
if (!strMatch) {
  console.error('[bump_build_number] ERROR: could not find PLUG_VERSION_STR "M.m.p" in config.h');
  process.exit(1);
}

let major = parseInt(strMatch[2], 10);
let minor = parseInt(strMatch[3], 10);
let patch = parseInt(strMatch[4], 10);

// Cascade: patch 0..99, minor 0..99, major unbounded.
patch += 1;
if (patch > 99) { patch = 0; minor += 1; }
if (minor > 99) { minor = 0; major += 1; }

const versionStr = `${major}.${minor}.${patch}`;
const packed     = ((major & 0xFF) << 16) | ((minor & 0xFF) << 8) | (patch & 0xFF);
const versionHex = `0x${packed.toString(16).padStart(8, '0').toUpperCase()}`;

let updated = original.replace(strRe, `$1${versionStr}$5`);
updated = updated.replace(hexRe, `$1${versionHex}`);

if (updated !== original) {
  writeFileSync(configPath, updated, 'utf8');
}

console.log(`[bump_build_number] ${versionStr}  (${versionHex})`);
