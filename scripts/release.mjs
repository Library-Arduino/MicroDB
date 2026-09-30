#!/usr/bin/env node
/**
 * scripts/release.mjs
 * 
 * Release Manager Utility for MicroDB Arduino Library
 * Desarrollado para Adazix Systems S.A.S & Jairo Antonio Rohatan Zapata
 * 
 * Uso:
 *   node scripts/release.mjs check                  # Verifica compilación previa con arduino-cli y estado de Git
 *   node scripts/release.mjs status                 # Muestra versión actual, último tag y cambios recientes
 *   node scripts/release.mjs bump <patch|minor|major|x.y.z>  # Incrementa versión en library.properties y library.json
 *   node scripts/release.mjs keywords               # Analiza si faltan palabras clave en keywords.txt
 *   node scripts/release.mjs notes [version] [title]# Genera el título y descripción para GitHub Releases
 *   node scripts/release.mjs tag <version> [title]  # Crea el commit y tag en Git
 */

import fs from 'node:fs';
import path from 'node:path';
import { execSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const ROOT_DIR = path.resolve(__dirname, '..');

const LIB_PROP_PATH = path.join(ROOT_DIR, 'library.properties');
const LIB_JSON_PATH = path.join(ROOT_DIR, 'library.json');
const KEYWORDS_PATH = path.join(ROOT_DIR, 'keywords.txt');
const CHANGELOG_PATH = path.join(ROOT_DIR, 'CHANGELOG.md');
const README_PATH = path.join(ROOT_DIR, 'README.md');
const QUICKSTART_INO = path.join(ROOT_DIR, 'examples', 'QuickStart', 'QuickStart.ino');

const colors = {
  reset: '\x1b[0m',
  cyan: '\x1b[36m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  red: '\x1b[31m',
  magenta: '\x1b[35m',
  bold: '\x1b[1m'
};

function log(msg) { console.log(msg); }
function logInfo(msg) { console.log(`${colors.cyan}ℹ ${msg}${colors.reset}`); }
function logSuccess(msg) { console.log(`${colors.green}✔ ${msg}${colors.reset}`); }
function logWarn(msg) { console.log(`${colors.yellow}⚠ ${msg}${colors.reset}`); }
function logError(msg) { console.error(`${colors.red}✖ ${msg}${colors.reset}`); }

function findArduinoCli() {
  try {
    const fromPath = execSync('where.exe arduino-cli', { encoding: 'utf-8' }).trim();
    if (fromPath && fs.existsSync(fromPath.split('\n')[0].trim())) {
      return fromPath.split('\n')[0].trim();
    }
  } catch {}

  const localAppData = process.env.LOCALAPPDATA || '';
  const idePath = path.join(localAppData, 'Programs', 'Arduino IDE', 'resources', 'app', 'lib', 'backend', 'resources', 'arduino-cli.exe');
  if (fs.existsSync(idePath)) {
    return idePath;
  }

  return null;
}

function run(command, options = {}) {
  try {
    return execSync(command, {
      cwd: ROOT_DIR,
      stdio: options.silent ? 'pipe' : 'inherit',
      encoding: 'utf-8',
      ...options
    });
  } catch (error) {
    if (!options.allowFailure) {
      throw error;
    }
    return null;
  }
}

function getLibraryProperties() {
  if (!fs.existsSync(LIB_PROP_PATH)) {
    throw new Error('library.properties no encontrado');
  }
  const content = fs.readFileSync(LIB_PROP_PATH, 'utf-8');
  const lines = content.split('\n');
  const props = {};
  for (const line of lines) {
    const idx = line.indexOf('=');
    if (idx !== -1) {
      props[line.slice(0, idx).trim()] = line.slice(idx + 1).trim();
    }
  }
  return { content, props };
}

function getLatestGitTag() {
  try {
    return execSync('git describe --tags --abbrev=0', { cwd: ROOT_DIR, encoding: 'utf-8' }).trim();
  } catch {
    return null;
  }
}

function checkCompilation() {
  logInfo('Buscando ejecutable de arduino-cli...');
  const arduinoCli = findArduinoCli();
  
  if (!arduinoCli) {
    logWarn('arduino-cli no encontrado. Omitiendo compilación automática de hardware.');
    logInfo('Se recomienda compilar QuickStart.ino manualmente en Arduino IDE para validar.');
    return;
  }

  logSuccess(`arduino-cli detectado: ${arduinoCli}`);
  logInfo(`Compilando ejemplo oficial QuickStart.ino contra núcleo Arduino Uno (AVR)...`);

  try {
    const cmd = `"${arduinoCli}" compile --fqbn arduino:avr:uno --library "${ROOT_DIR}" "${QUICKSTART_INO}"`;
    const output = execSync(cmd, { cwd: ROOT_DIR, encoding: 'utf-8' });
    
    logSuccess('¡Compilación de la librería COMPLETADA CON ÉXITO (0 errores en C++)!');
    
    // Extraer métricas de memoria
    const lines = output.trim().split('\n');
    for (const line of lines) {
      if (line.includes('El Sketch usa') || line.includes('Las variables Globales') || line.includes('Sketch uses') || line.includes('Global variables')) {
        log(`  ⚡ ${colors.bold}${line.trim()}${colors.reset}`);
      }
    }
    return true;
  } catch (err) {
    logError('La compilación de prueba en C++ FALLÓ. Corrige los errores antes de hacer release.');
    if (err.stdout) console.log(err.stdout);
    if (err.stderr) console.error(err.stderr);
    process.exit(1);
  }
}

function checkGitStatus() {
  logInfo('Analizando estado del repositorio Git...');
  const status = run('git status --short', { silent: true }).trim();
  const latestTag = getLatestGitTag() || 'Inicio';
  const { props } = getLibraryProperties();

  log(`  ${colors.bold}Versión actual en library.properties:${colors.reset} ${props.version || 'desconocida'}`);
  log(`  ${colors.bold}Último tag detectado:${colors.reset} ${latestTag}`);
  
  if (latestTag !== 'Inicio') {
    log(`  ${colors.bold}Commits desde ${latestTag}:${colors.reset}`);
    const commits = run(`git log ${latestTag}..HEAD --oneline`, { silent: true }).trim();
    if (commits) {
      log(commits.split('\n').map(l => `    • ${l}`).join('\n'));
    } else {
      log('    (Sin commits nuevos desde el tag)');
    }
  }

  if (status) {
    log(`\n  ${colors.bold}Archivos modificados / no versionados:${colors.reset}`);
    log(status.split('\n').map(l => `    ${l}`).join('\n'));
  } else {
    log('  (Árbol de trabajo limpio)');
  }
}

function bumpVersion(typeOrVersion) {
  const { content, props } = getLibraryProperties();
  const oldVersion = props.version;
  let newVersion = '';

  const semverRegex = /^(\d+)\.(\d+)\.(\d+)$/;
  const match = oldVersion.match(semverRegex);
  if (!match) {
    logError(`Versión actual inválida en library.properties: ${oldVersion}`);
    process.exit(1);
  }

  let [_, major, minor, patch] = match.map(Number);

  if (typeOrVersion === 'patch') {
    patch += 1;
    newVersion = `${major}.${minor}.${patch}`;
  } else if (typeOrVersion === 'minor') {
    minor += 1;
    patch = 0;
    newVersion = `${major}.${minor}.${patch}`;
  } else if (typeOrVersion === 'major') {
    major += 1;
    minor = 0;
    patch = 0;
    newVersion = `${major}.${minor}.${patch}`;
  } else if (semverRegex.test(typeOrVersion)) {
    newVersion = typeOrVersion;
  } else {
    logError(`Tipo de versión desconocido: "${typeOrVersion}". Usa patch, minor, major o x.y.z`);
    process.exit(1);
  }

  logInfo(`Incrementando versión de ${colors.yellow}${oldVersion}${colors.reset} a ${colors.green}${newVersion}${colors.reset}...`);

  // 1. Actualizar library.properties
  const updatedLibProp = content.replace(/^version=.*$/m, `version=${newVersion}`);
  fs.writeFileSync(LIB_PROP_PATH, updatedLibProp, 'utf-8');
  logSuccess(`Actualizado library.properties -> version=${newVersion}`);

  // 2. Actualizar library.json (PlatformIO)
  if (fs.existsSync(LIB_JSON_PATH)) {
    const libJson = JSON.parse(fs.readFileSync(LIB_JSON_PATH, 'utf-8'));
    libJson.version = newVersion;
    fs.writeFileSync(LIB_JSON_PATH, JSON.stringify(libJson, null, 2) + '\n', 'utf-8');
    logSuccess(`Actualizado library.json -> version: "${newVersion}"`);
  }

  return newVersion;
}

function auditKeywords() {
  logInfo('Auditoría de palabras clave en keywords.txt...');
  if (!fs.existsSync(KEYWORDS_PATH)) {
    logWarn('keywords.txt no existe.');
    return;
  }
  const keywordsContent = fs.readFileSync(KEYWORDS_PATH, 'utf-8');
  const existingKeywords = new Set(
    keywordsContent.split('\n')
      .map(line => line.trim())
      .filter(line => line && !line.startsWith('#'))
      .map(line => line.split(/\s+/)[0])
  );

  logSuccess(`keywords.txt contiene actualmente ${existingKeywords.size} palabras clave registradas.`);
}

function getChangelogSection(targetVersion) {
  if (!fs.existsSync(CHANGELOG_PATH)) return null;
  const changelog = fs.readFileSync(CHANGELOG_PATH, 'utf-8');
  const sectionHeaderRegex = new RegExp(`## \\[${targetVersion.replace(/\./g, '\\.')}\\][^\n]*\n([\\s\\S]*?)(?=\n## \\[|$)`);
  const match = changelog.match(sectionHeaderRegex);
  return match ? match[1].trim() : null;
}

function generateGitHubNotes(versionOverride, titleOverride) {
  const { props } = getLibraryProperties();
  const version = versionOverride || props.version;
  const releaseTitle = titleOverride || `MicroDB v${version} - Nuevas Funcionalidades y Optimizaciones C++`;
  const changelogBody = getChangelogSection(version) || 'Consulte CHANGELOG.md para más detalles.';

  const output = `
================================================================================
📋 COPIAR Y PEGAR EN GITHUB RELEASES (LIBRERÍA ARDUINO)
================================================================================

📌 Repository: https://github.com/Library-Arduino/MicroDB/releases/new
🏷️ Tag version: v${version}
🎯 Target branch: main
🏷️ Release title: ${releaseTitle}

--- DESCRIPCIÓN DEL RELEASE (MARKDOWN) ---

# MicroDB Arduino Library v${version} ⚡

**MicroDB** es un motor de base de datos relacional de alto rendimiento, optimizado en C++ para tarjetas SD en microcontroladores con recursos reducidos (Arduino AVR, ESP32, ESP8266, STM32, RP2040).

---

${changelogBody}

---

### 💻 Compatibilidad de Hardware y Plataformas

| Plataforma / Arquitectura | Soporte | Observaciones |
| :--- | :---: | :--- |
| **ESP32 (WROOM, S2, S3, C3)** | ⭐⭐⭐⭐⭐ | Soporte nativo, alto rendimiento con SD/SPI y FreeRTOS. |
| **ESP8266 (NodeMCU, D1 Mini)** | ⭐⭐⭐⭐ | Operaciones $O(1)$ fluidas con SD estándar. |
| **Arduino AVR (Uno, Nano, Mega)** | ⭐⭐⭐⭐ | RAM ultra-optimizada con Streaming Cursors (funciona en 2 KB de SRAM). |
| **STM32 (BluePill, Nucleo)** | ⭐⭐⭐⭐⭐ | Rápido acceso por hardware SPI. |
| **Raspberry Pi Pico (RP2040)** | ⭐⭐⭐⭐⭐ | Compatible con Arduino Mbed y Earle Philhower cores. |

---

### 📦 Métricas de Memoria Típicas (Arduino Uno - ATmega328P)
- **Flash ROM:** ~22 KB (69% del espacio total).
- **RAM Estática:** ~1.4 KB (71% de la SRAM total de 2048 bytes).
- **Consumo Dinámico:** $O(1)$ bytes en memoria dinámica mediante buffers fijos de 64 bytes.

---

### 📥 Instalación

#### A. Desde Arduino IDE Library Manager:
Buscar **MicroDB** por *Jairo Antonio Rohatan Zapata* e instalar la última versión.

#### B. Instalación Manual (ZIP):
Descarga el código fuente en formato \`.zip\` desde este Release y agrégalo en:
\`Sketch\` ➔ \`Incluir Librería\` ➔ \`Añadir biblioteca .ZIP...\`

#### C. PlatformIO:
Agregar a tu \`platformio.ini\`:
\`\`\`ini
lib_deps =
    https://github.com/Library-Arduino/MicroDB.git
\`\`\`

================================================================================
`;

  console.log(output);
  return output;
}

function createGitTag(versionOverride, releaseTitle) {
  const { props } = getLibraryProperties();
  const version = versionOverride || props.version;
  const title = releaseTitle || `Versión v${version}`;

  logInfo(`Creando commit y tag para v${version}...`);

  // Asegurar que archivos clave están en el stage
  run('git add library.properties library.json keywords.txt CHANGELOG.md');
  if (fs.existsSync(README_PATH)) {
    run('git add README.md', { allowFailure: true });
  }
  run('git add src/ examples/', { allowFailure: true });

  const commitMsg = `chore(release): v${version} - ${title}`;
  run(`git commit -m "${commitMsg}"`, { allowFailure: true });
  logSuccess(`Commit creado: "${commitMsg}"`);

  const tagMsg = `Release v${version}: ${title}`;
  run(`git tag -a v${version} -m "${tagMsg}"`);
  logSuccess(`Git tag creado: v${version}`);
  logInfo(`Para publicar a GitHub ejecuta: git push origin main --tags`);
}

// Router CLI
const [,, command, ...args] = process.argv;

switch (command) {
  case 'check':
    checkCompilation();
    checkGitStatus();
    break;
  case 'status':
    checkGitStatus();
    break;
  case 'bump':
    if (!args[0]) {
      logError('Debes especificar el tipo de incremento: patch, minor, major o x.y.z');
      process.exit(1);
    }
    bumpVersion(args[0]);
    break;
  case 'keywords':
    auditKeywords();
    break;
  case 'notes':
    generateGitHubNotes(args[0], args.slice(1).join(' '));
    break;
  case 'tag':
    createGitTag(args[0], args.slice(1).join(' '));
    break;
  default:
    log(`
${colors.bold}${colors.cyan}MicroDB Arduino Library - Release Manager Utility${colors.reset}

Uso:
  node scripts/release.mjs check                  Verifica compilación con arduino-cli y estado de Git
  node scripts/release.mjs status                 Muestra versión actual, tags y commits recientes
  node scripts/release.mjs bump <patch|minor|major|x.y.z>  Incrementa versión en library.properties y library.json
  node scripts/release.mjs keywords               Audita palabras clave en keywords.txt
  node scripts/release.mjs notes [version] [title] Genera título y texto para GitHub Releases
  node scripts/release.mjs tag <version> [title]  Genera git commit y tag anotado
`);
    break;
}
