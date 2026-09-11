"""Minimal path protocol for the existing BMD inspector, without extracting archives."""


class MemoryAsset:
    def __init__(self, name, raw):
        self.name = name
        self.raw = raw

    def read_bytes(self):
        return self.raw

    def resolve(self):
        return self.name
