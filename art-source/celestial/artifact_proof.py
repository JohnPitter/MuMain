"""Tie measured geometry checks to the exact editable and exported artifacts."""
import hashlib


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_proof(proof, model, blend):
    expected = dict(model_sha256=digest(model), blend_sha256=digest(blend))
    for field, value in expected.items():
        if proof.get(field) != value:
            raise ValueError(f'Stale or missing geometry proof: {model.name}/{field}')
