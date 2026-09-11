"""Read-only asset sources; every directory/archive keeps its own namespace."""
import hashlib
import subprocess
from pathlib import Path, PurePosixPath
from zipfile import BadZipFile, ZipFile

from mappings import is_candidate


EXCLUDED_DIRS = ('node_modules', '.git', 'ThirdParty', '.nuget')


def in_scope(relative):
    return relative.lower().endswith('.bmd') and relative.lower().startswith(('data/player/', 'data/item/'))


class AssetSource:
    def __init__(self, location, prefix='', canonical=False):
        self.location = Path(location)
        self.prefix = prefix
        self.canonical = canonical
        self.archive = ZipFile(self.location) if self.location.is_file() else None
        self.entries = {}
        self.texture_names = None
        self.id = hashlib.sha256((str(self.location) + '!' + prefix).encode()).hexdigest()[:12]

    def read(self, relative):
        actual = self.entries[relative.lower()]
        return self.archive.read(actual) if self.archive else Path(actual).read_bytes()

    def display(self, relative):
        actual = self.entries[relative.lower()]
        return str(self.location) + '!' + actual if self.archive else str(actual)

    def close(self):
        if self.archive:
            self.archive.close()

    def info(self):
        return dict(id=self.id, location=str(self.location), prefix=self.prefix,
                    kind='zip' if self.archive else 'directory', canonical=self.canonical,
                    indexed_files=len(self.entries), candidates=sum(in_scope(n) for n in self.entries))


def normalized_entry(name):
    parts = PurePosixPath(name.replace('\\', '/')).parts
    lower = [part.lower() for part in parts]
    if 'data' in lower:
        index = lower.index('data')
        return '/'.join(parts[:index]), '/'.join(parts[index:])
    if lower and lower[0] in ('player', 'local', 'item'):
        return '', 'Data/' + '/'.join(parts)
    return None, None


def archive_sources(path, canonical):
    result, prefixes = [], set()
    with ZipFile(path) as archive:
        for name in archive.namelist():
            prefix, relative = normalized_entry(name)
            if relative and in_scope(relative):
                prefixes.add(prefix)
    for prefix in sorted(prefixes):
        source = AssetSource(path, prefix, path.resolve() == canonical.resolve())
        for name in source.archive.namelist():
            current, relative = normalized_entry(name)
            if current == prefix and relative and not name.endswith('/'):
                source.entries[relative.lower()] = name
        result.append(source)
    return result


def scan_paths(roots):
    command = ['rg', '--files', '--hidden', '--no-ignore']
    for extension in ('*.bmd', '*.BMD', '*.zip', '*.7z', '*.rar'):
        command.extend(['-g', extension])
    for directory in EXCLUDED_DIRS:
        command.extend(['-g', f'!**/{directory}/**'])
    result = subprocess.run([*command, *map(str, roots)], check=True, capture_output=True, text=True, encoding='utf-8')
    return sorted(set(Path(line) for line in result.stdout.splitlines()))


def directory_source(root):
    source = AssetSource(root)
    for path in (root / 'Data').rglob('*'):
        if path.is_file():
            source.entries[path.relative_to(root).as_posix().lower()] = str(path)
    return source


def discover(roots, canonical):
    sources, audit, directories = [], [], set()
    for path in scan_paths(roots):
        if path.suffix.lower() == '.bmd':
            prefix, relative = normalized_entry(path.as_posix())
            if relative and in_scope(relative):
                directories.add(Path(prefix))
            elif is_candidate(path.name):
                audit.append(dict(path=str(path), status='loose_model_outside_Data_Player'))
            continue
        if path.suffix.lower() != '.zip':
            audit.append(dict(path=str(path), status='unsupported_archive_format'))
            continue
        try:
            found = archive_sources(path, canonical)
            sources.extend(found)
            audit.append(dict(path=str(path), status='indexed' if found else 'no_player_equipment', datasets=len(found)))
        except (BadZipFile, OSError, ValueError) as error:
            audit.append(dict(path=str(path), status='archive_error', error=str(error)))
    sources.extend(directory_source(directory) for directory in sorted(directories))
    if sum(source.canonical for source in sources) != 1:
        raise ValueError('Expected exactly one canonical Data/Player dataset')
    return sorted(sources, key=lambda source: (not source.canonical, str(source.location), source.prefix)), audit
