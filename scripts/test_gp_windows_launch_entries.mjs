import fs from 'fs';
import path from 'path';
import process from 'process';
import { fileURLToPath } from 'url';

// Static validator only: this checks launch/task/bundle wiring on disk.
// It does not compile targets or launch hosts.

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..');

const MODE_ICONS = {
  noBundle: '\u{1F4BD}',
  shallow: '\u{1F4E6}',
  deep: '\u{1F9F1}',
};

const WINDOWS_SECTIONS = [
  'Standalone',
  'VST3 (Reaper)',
  'VST3 (Studio One)',
];

function readText(filePath) {
  return fs.readFileSync(filePath, 'utf8').replace(/^\uFEFF/, '');
}

function stripJsonComments(text) {
  let output = '';
  let inString = false;
  let isEscaped = false;

  for (let index = 0; index < text.length; index += 1) {
    const current = text[index];
    const next = text[index + 1];

    if (inString) {
      output += current;
      if (isEscaped) {
        isEscaped = false;
      } else if (current === '\\') {
        isEscaped = true;
      } else if (current === '"') {
        inString = false;
      }
      continue;
    }

    if (current === '"') {
      inString = true;
      output += current;
      continue;
    }

    if (current === '/' && next === '/') {
      while (index < text.length && text[index] !== '\n') {
        index += 1;
      }
      if (index < text.length) {
        output += text[index];
      }
      continue;
    }

    if (current === '/' && next === '*') {
      index += 2;
      while (index < text.length) {
        if (text[index] === '*' && text[index + 1] === '/') {
          index += 1;
          break;
        }
        if (text[index] === '\n') {
          output += '\n';
        }
        index += 1;
      }
      continue;
    }

    output += current;
  }

  return output;
}

function stripTrailingCommas(text) {
  let previous;
  let current = text;
  do {
    previous = current;
    current = current.replace(/,(\s*[}\]])/g, '$1');
  } while (current !== previous);
  return current;
}

function parseJsonc(filePath) {
  const source = readText(filePath);
  const clean = stripTrailingCommas(stripJsonComments(source));
  return JSON.parse(clean);
}

function normalizeName(name) {
  return String(name || '')
    .replace(/\u2003/g, ' ')
    .replace(/[\u2502\u251C\u2514\u2500]/g, ' ')
    .replace(/\u7530/g, ' ')
    .replace(/\s+/g, ' ')
    .trim();
}

function parseLeafLabel(label) {
  for (const [mode, icon] of Object.entries(MODE_ICONS)) {
    const prefix = `${icon} `;
    if (!label.startsWith(prefix)) {
      continue;
    }

    const buildType = label.slice(prefix.length).trim();
    if (buildType !== 'Debug' && buildType !== 'Release') {
      return null;
    }

    return { icon, buildType, mode };
  }

  return null;
}

function getDependsOn(task) {
  if (!task || task.dependsOn == null) return [];
  return Array.isArray(task.dependsOn) ? task.dependsOn : [task.dependsOn];
}

function hasArg(task, expected) {
  return Array.isArray(task.args) && task.args.some((arg) => String(arg) === expected);
}

function hasArgFragment(task, fragment) {
  return Array.isArray(task.args) && task.args.some((arg) => String(arg).includes(fragment));
}

function ensure(condition, message, errors) {
  if (!condition) {
    errors.push(message);
  }
}

function expectedTaskLabel(section, buildType, mode) {
  const prefix = section === 'Standalone' ? 'GP Standalone Build' : 'GP VST3 Rebuild';
  if (mode === 'noBundle') return `${prefix} ${buildType} VS2022`;
  if (mode === 'shallow') return `${MODE_ICONS.shallow} ${prefix} ${buildType} Shallow VS2022`;
  if (mode === 'deep') return `${MODE_ICONS.deep} ${prefix} ${buildType} Deep VS2022`;
  throw new Error(`Unknown mode: ${mode}`);
}

function bundleModeForTask(task) {
  const dependsOn = getDependsOn(task);
  if (dependsOn.includes('Glint Bundle Shallow')) return 'shallow';
  if (dependsOn.includes('Glint Bundle Deep')) return 'deep';
  return 'noBundle';
}

function validateBundlerTask(task, mode, errors) {
  ensure(Boolean(task), `Missing ${mode} bundler task.`, errors);
  if (!task) return;

  ensure(task.command === 'node', `${task.label} should invoke node.`, errors);
  ensure(hasArgFragment(task, path.join('third_party', 'glint', 'scripts', 'bundler', 'glint_bundler.mjs')), `${task.label} should call glint_bundler.mjs.`, errors);
  ensure(hasArg(task, '--input'), `${task.label} should provide --input.`, errors);
  ensure(hasArgFragment(task, path.join('glint_user_code', 'web')), `${task.label} should bundle glint_user_code/web.`, errors);
  ensure(hasArg(task, '--output'), `${task.label} should provide --output.`, errors);
  ensure(hasArgFragment(task, path.join('GP', 'glint_bundle')), `${task.label} should write to GP/glint_bundle.`, errors);
  ensure(hasArg(task, '--mode'), `${task.label} should provide --mode.`, errors);
  ensure(hasArg(task, mode), `${task.label} should set --mode ${mode}.`, errors);
}

function validatePropsFile(filePath, defineName, errors) {
  const text = readText(filePath);
  ensure(text.includes(`<PreprocessorDefinitions>${defineName};%(PreprocessorDefinitions)</PreprocessorDefinitions>`), `${path.basename(filePath)} should define ${defineName}.`, errors);
}

function validateDocumentBundleDispatch(filePath, errors) {
  const text = readText(filePath);
  ensure(text.includes('#if defined(GLINT_BUNDLE_DEEP) || defined(GLINT_BUNDLE_SHALLOW)'), 'GPGlintDocument.hpp should guard bundle dispatch with both bundle defines.', errors);
  ensure(text.includes('static glint_bundle::glint_bundle_library sBundle;'), 'GPGlintDocument.hpp should create the bundle library before file fallback.', errors);
  ensure(text.includes('if (sBundle.dispatch(request))'), 'GPGlintDocument.hpp should dispatch bundled resources before file fallback.', errors);
}

function collectWindowsLaunchEntries(configurations, errors) {
  const entries = [];
  let inWindows = false;
  let currentSection = null;

  for (const config of configurations) {
    const label = normalizeName(config.name);
    if (!label) continue;

    if (!inWindows) {
      if (label === 'Windows') {
        inWindows = true;
      }
      continue;
    }

    if (label === 'MacOS') {
      break;
    }

    if (WINDOWS_SECTIONS.includes(label)) {
      currentSection = label;
      continue;
    }

    const parsedLeaf = parseLeafLabel(label);
    if (!parsedLeaf) {
      continue;
    }

    ensure(Boolean(currentSection), `Launch entry ${config.name} is missing a Windows subsection header.`, errors);
    if (!currentSection) continue;

    entries.push({
      config,
      section: currentSection,
      ...parsedLeaf,
    });
  }

  return entries;
}

function main() {
  const errors = [];
  const launchPath = path.join(repoRoot, '.vscode', 'launch.json');
  const tasksPath = path.join(repoRoot, '.vscode', 'tasks.json');
  const deepPropsPath = path.join(repoRoot, 'GP', 'config', 'glint_bundle_deep.props');
  const shallowPropsPath = path.join(repoRoot, 'GP', 'config', 'glint_bundle_shallow.props');
  const documentPath = path.join(repoRoot, 'GP', 'GPGlintDocument.hpp');

  const launchJson = parseJsonc(launchPath);
  const tasksJson = parseJsonc(tasksPath);
  const tasks = Array.isArray(tasksJson.tasks) ? tasksJson.tasks : [];
  const taskByLabel = new Map(tasks.map((task) => [task.label, task]));

  validateBundlerTask(taskByLabel.get('Glint Bundle Shallow'), 'shallow', errors);
  validateBundlerTask(taskByLabel.get('Glint Bundle Deep'), 'deep', errors);
  validatePropsFile(shallowPropsPath, 'GLINT_BUNDLE_SHALLOW', errors);
  validatePropsFile(deepPropsPath, 'GLINT_BUNDLE_DEEP', errors);
  validateDocumentBundleDispatch(documentPath, errors);

  const windowsLaunchEntries = collectWindowsLaunchEntries(launchJson.configurations || [], errors);
  const expectedCount = WINDOWS_SECTIONS.length * 2 * 3;
  ensure(windowsLaunchEntries.length === expectedCount, `Expected ${expectedCount} GP Windows launch entries, found ${windowsLaunchEntries.length}.`, errors);

  const seenCombos = new Set();
  for (const entry of windowsLaunchEntries) {
    const comboKey = `${entry.section}|${entry.buildType}|${entry.mode}`;
    ensure(!seenCombos.has(comboKey), `Duplicate launch entry for ${comboKey}.`, errors);
    seenCombos.add(comboKey);

    const expectedLabel = expectedTaskLabel(entry.section, entry.buildType, entry.mode);
    ensure(entry.config.preLaunchTask === expectedLabel, `${entry.config.name} should use preLaunchTask ${expectedLabel}, found ${entry.config.preLaunchTask || '<missing>'}.`, errors);

    const task = taskByLabel.get(expectedLabel);
    ensure(Boolean(task), `Missing task for launch entry ${entry.config.name}: ${expectedLabel}.`, errors);
    if (!task) continue;

    const expectedTarget = entry.section === 'Standalone' ? '/t:GP-app' : '/t:GP-vst3:Rebuild';
    ensure(hasArg(task, expectedTarget), `${expectedLabel} should target ${expectedTarget}.`, errors);
    ensure(hasArg(task, `/p:Configuration=${entry.buildType}`), `${expectedLabel} should build ${entry.buildType}.`, errors);
    ensure(hasArg(task, '/p:Platform=x64'), `${expectedLabel} should build x64.`, errors);

    const actualBundleMode = bundleModeForTask(task);
    ensure(actualBundleMode === entry.mode, `${expectedLabel} bundle mode mismatch: launch expects ${entry.mode}, task is wired as ${actualBundleMode}.`, errors);

    if (entry.mode === 'noBundle') {
      ensure(getDependsOn(task).length === 0, `${expectedLabel} should not depend on a bundle task.`, errors);
      ensure(!hasArgFragment(task, 'ForceImportBeforeCppTargets='), `${expectedLabel} should not inject bundle props.`, errors);
    }

    if (entry.mode === 'shallow') {
      ensure(getDependsOn(task).includes('Glint Bundle Shallow'), `${expectedLabel} should depend on Glint Bundle Shallow.`, errors);
      ensure(task.dependsOrder === 'sequence', `${expectedLabel} should run bundle generation before MSBuild.`, errors);
      ensure(hasArgFragment(task, path.join('GP', 'config', 'glint_bundle_shallow.props')), `${expectedLabel} should inject glint_bundle_shallow.props.`, errors);
      ensure(!hasArgFragment(task, path.join('GP', 'config', 'glint_bundle_deep.props')), `${expectedLabel} should not inject glint_bundle_deep.props.`, errors);
    }

    if (entry.mode === 'deep') {
      ensure(getDependsOn(task).includes('Glint Bundle Deep'), `${expectedLabel} should depend on Glint Bundle Deep.`, errors);
      ensure(task.dependsOrder === 'sequence', `${expectedLabel} should run bundle generation before MSBuild.`, errors);
      ensure(hasArgFragment(task, path.join('GP', 'config', 'glint_bundle_deep.props')), `${expectedLabel} should inject glint_bundle_deep.props.`, errors);
      ensure(!hasArgFragment(task, path.join('GP', 'config', 'glint_bundle_shallow.props')), `${expectedLabel} should not inject glint_bundle_shallow.props.`, errors);
    }
  }

  for (const section of WINDOWS_SECTIONS) {
    for (const buildType of ['Debug', 'Release']) {
      for (const mode of ['noBundle', 'shallow', 'deep']) {
        const comboKey = `${section}|${buildType}|${mode}`;
        ensure(seenCombos.has(comboKey), `Missing launch entry for ${comboKey}.`, errors);
      }
    }
  }

  if (errors.length > 0) {
    console.error(`[FAIL] GP Windows launch validation found ${errors.length} issue(s):`);
    for (const [index, error] of errors.entries()) {
      console.error(`${index + 1}. ${error}`);
    }
    process.exit(1);
  }

  console.log('[PASS] Verified static wiring for all 18 GP Windows launch entries.');
  console.log(`[PASS] ${MODE_ICONS.noBundle} entries use the expected plain MSBuild tasks.`);
  console.log(`[PASS] ${MODE_ICONS.shallow} entries use Glint Bundle Shallow + glint_bundle_shallow.props.`);
  console.log(`[PASS] ${MODE_ICONS.deep} entries use Glint Bundle Deep + glint_bundle_deep.props.`);
  console.log('[PASS] GPGlintDocument bundle dispatch is present before file fallback.');
}

main();