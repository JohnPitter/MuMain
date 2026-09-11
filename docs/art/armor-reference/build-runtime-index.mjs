import { createHash } from 'node:crypto';
import { existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, relative, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const outputDirectory = dirname(fileURLToPath(import.meta.url));
const clientRoot = resolve(outputDirectory, '../../..');
const sourceRoot = join(clientRoot, 'src/source');
const unique = values => [...new Set(values)];
const hash = bytes => createHash('sha256').update(bytes).digest('hex');
const normalize = path => path.replaceAll('\\', '/');

function sourceFiles(directory) {
  return readdirSync(directory, { withFileTypes: true }).flatMap(entry => {
    const path = join(directory, entry.name);
    if (entry.isDirectory()) return sourceFiles(path);
    return /\.(cpp|h|hpp|inl)$/i.test(entry.name) ? [path] : [];
  }).sort();
}

function matchedNames(line, pattern) {
  return unique([...line.matchAll(pattern)].map(match => match[1]));
}

function classify(line) {
  const categories = [];
  if (/\bMODEL_[A-Z0-9_]+\b/.test(line)) categories.push('model-reference');
  if (/\b(?:BITMAP|SOUND)_[A-Z0-9_]+\b/.test(line)) categories.push('resource-reference');
  if (/\bCreate(?:Effect|Particle|Sprite|Joint|Blur|ObjectBlur|Spark|Pointer|Point)\w*\s*\(/.test(line)) categories.push('effect-call');
  if (/\b(?:Delete|Search|Move|Render|Check)(?:Effect|Particle|Sprite|Joint|Blur|ObjectBlur|Spark|Pointer|Point)\w*\s*\(/.test(line)) categories.push('effect-lifecycle');
  if (/\b(RenderMesh|RenderBody|RenderPartObject\w*|RenderLinkObject|RenderParts)\s*\(|\bRENDER_(?:CHROME\d*|METAL|BRIGHT|TEXTURE|COLOR|LIGHTMAP|WAVE)\b/.test(line)) categories.push('material-render');
  if (/\b(?:BlendMesh\w*|HiddenMesh|BodyLight|LightEnable|m_csTScript|m_bBright|m_bHiddenMesh|m_bStreamMesh|m_bNoneBlendMesh|parsingTScriptA)\b/.test(line)) categories.push('material-state');
  if (/\b(?:Vector|VectorCopy|VectorScale)\s*\(/.test(line)) categories.push('color-or-position-vector');
  if (/\b(?:BoneTransform|LinkBone|TransformPosition|GetBonePosition|Attach\w*|ParentMatrix)\b/.test(line)) categories.push('bone-attachment');
  if (/\b(?:AnimationFrame|NumAnimationKeys|PlaySpeed|FPS_ANIMATION_FACTOR|LockPositions|Loop)\b|\b(?:PlayAnimation|Animation)\s*\(/.test(line)) categories.push('motion-timing');
  if (/\b(?:WorldTime|rand_fps_check|GetRenderLevel|Distance|eBuff_Cloaking)\b/.test(line)) categories.push('timing-visibility');
  if (/\b(?:PlayBuffer|LoadWaveFile|LoadSound|StopBuffer)\s*\(|\bSOUND_[A-Z0-9_]+\b/.test(line)) categories.push('audio');
  if (/\b(?:AccessModel|OpenTexture|LoadBitmap|LoadTexture)\s*\(/.test(line)) categories.push('asset-loader');
  if (/\b(?:ExcellentFlags|AncientDiscriminator|ancientDiscriminator|SocketCount|SocketSeedID|SocketSeedSphere)\b/.test(line)) categories.push('item-option');
  return categories;
}

function readSource(path) {
  const bytes = readFileSync(path);
  const text = bytes.toString('utf8');
  const lines = text.split(/\r?\n/);
  const events = [];
  for (let index = 0; index < lines.length; index++) {
    const code = lines[index].trim();
    if (/^(\/\/|\*|\/\*)/.test(code)) continue;
    const categories = classify(code);
    if (categories.length === 0) continue;
    events.push({
      line: index + 1,
      categories,
      models: matchedNames(code, /\b(MODEL_[A-Z0-9_]+)\b/g),
      resources: matchedNames(code, /\b((?:BITMAP|SOUND)_[A-Z0-9_]+)\b/g),
      code,
    });
  }
  return { path: normalize(relative(clientRoot, path)), sha256: hash(bytes), lineCount: lines.length, events };
}

function extractPlayerActions() {
  const path = 'src/source/Core/Globals/_enum.h';
  const lines = readFileSync(join(clientRoot, path), 'utf8').split(/\r?\n/);
  const begin = lines.findIndex(line => /^\s*PLAYER_SET,\s*$/.test(line));
  const end = lines.findIndex((line, index) => index > begin && /\bMAX_PLAYER_ACTION\b/.test(line));
  const actions = lines.slice(begin, end).flatMap((line, index) => {
    const match = line.match(/^\s*(PLAYER_[A-Z0-9_]+)(?:\s*=\s*([^,]+))?,/);
    return match ? [{ name: match[1], explicitExpression: match[2] ?? null, line: begin + index + 1 }] : [];
  });
  if (actions.length < 100) throw new Error('Player action enum extraction failed.');
  return { path, total: actions.length, numericIndexResolved: false, note: 'Declaration order only; preprocessor guards and aliases require the actual build configuration.', actions };
}

function extractArmorTint() {
  const path = 'src/source/Engine/Object/ZzzObject.cpp';
  const text = readFileSync(join(clientRoot, path), 'utf8');
  const begin = text.indexOf('void PartObjectColor(');
  const end = text.indexOf('void PartObjectColor2(', begin);
  const body = text.slice(begin, end);
  const itemSwitch = body.slice(body.indexOf('switch (ItemType % MAX_ITEM_INDEX)'));
  const mappings = [...itemSwitch.matchAll(/case\s+(\d+):\s*Color\s*=\s*(\d+);/g)]
    .map(match => ({ itemNumber: Number(match[1]), colorIndex: Number(match[2]) }));
  const colors = [...body.matchAll(/case\s+(\d+):Vector\(Bright\s*\*\s*([\d.]+)f,\s*Bright\s*\*\s*([\d.]+)f,\s*Bright\s*\*\s*([\d.]+)f,\s*Light\)/g)]
    .map(match => ({ colorIndex: Number(match[1]), rgbMultiplier: match.slice(2, 5).map(Number) }));
  if (!mappings.length || !colors.length) throw new Error('Armor tint extraction found no mappings.');
  return { path, function: 'PartObjectColor', groups: [7, 8, 9, 10, 11], defaultColorIndex: 0, mappings, colors };
}

function referenceIndex(files, property) {
  const result = {};
  files.forEach((file, fileIndex) => file.events.forEach((event, eventIndex) => {
    for (const symbol of event[property]) {
      (result[symbol] ??= []).push([fileIndex, eventIndex]);
    }
  }));
  return Object.fromEntries(Object.entries(result).sort(([a], [b]) => a.localeCompare(b)));
}

if (!existsSync(sourceRoot)) throw new Error(`Client source directory not found: ${sourceRoot}`);
const scanned = sourceFiles(sourceRoot);
const files = scanned.map(readSource).filter(file => file.events.length > 0);
const byModelSymbol = referenceIndex(files, 'models');
const byResourceSymbol = referenceIndex(files, 'resources');
const categories = {};
files.forEach(file => file.events.forEach(event => event.categories.forEach(category => {
  categories[category] = (categories[category] ?? 0) + 1;
})));
const result = {
  schemaVersion: 1,
  generatedAt: new Date().toISOString(),
  sourceRoot: normalize(clientRoot),
  scope: 'All .cpp/.h/.hpp/.inl below src/source; lexical index, including non-equipment models.',
  limitations: [
    'Not a C++ AST, preprocessor evaluator or transitive call graph. Inline/block comments may remain.',
    'A model occurrence does not imply that effects nearby apply to that model. Read enclosing control flow.',
    'Runtime numeric model arithmetic, external scripts and installed database state are not resolved.',
    'Source calls do not prove texture/model availability, successful rendering, sound playback or gameplay availability.',
  ],
  counts: { scannedFiles: scanned.length, indexedFiles: files.length, events: files.reduce((sum, file) => sum + file.events.length, 0), modelSymbols: Object.keys(byModelSymbol).length, categories },
  playerActions: extractPlayerActions(),
  armorMaterialTint: extractArmorTint(),
  byModelSymbol,
  byResourceSymbol,
  files,
};
mkdirSync(outputDirectory, { recursive: true });
const output = join(outputDirectory, 'equipment-effects-index.json');
writeFileSync(output, JSON.stringify(result));
console.log(JSON.stringify({ output, bytes: readFileSync(output).length, counts: result.counts, actions: result.playerActions.total, armorTints: result.armorMaterialTint.mappings.length }, null, 2));
