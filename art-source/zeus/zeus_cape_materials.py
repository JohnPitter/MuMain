"""Definitive cape materials: the same authored Zeus atlases as the set.

Reuses the Zeus family materials (zeus_materials.create_materials) verbatim so
the cape reads as one set with the armor and wings: Azul Celeste principal,
Branco Platinado nos detalhes (branco platina aprovado) and the emissive storm
channels. The material names stay the exact atlas files the BMD encoder embeds
(``Zeus_{role}.jpg``), and the cape's rigid ferragem/emblem samples the same
authored atlases; the runtime cloth grid will stretch the full fields across
UV 0..1 per ``cape-cloth-contract.md``.
"""
from zeus_materials import create_materials


def create_cape_materials():
    return create_materials()
