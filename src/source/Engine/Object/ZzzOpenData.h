#pragma once

void DeleteNpcs();
void OpenNpc(int Type);
void DeleteMonsters();
// loadSounds=false: geometry/textures only (UI previews). Sounds/BoneHead still
// load on a later full OpenMonsterModel when the monster appears in the world.
void OpenMonsterModel(EMonsterModelType Type);
void OpenMonsterModel(EMonsterModelType Type, bool loadSounds);

void OpenModel(int Type, wchar_t* Dir, wchar_t* ModelFileName, ...);
void OpenBasicData(HDC hDC);
void ReleaseMainData();
bool OpenFont();
void OpenLogoSceneData();
void ReleaseLogoSceneData();
void OpenCharacterSceneData();
void ReleaseCharacterSceneData();

void OpenPlayerTextures();
void OpenItemTextures();

void SaveOptions();
void MarkSkillBarDirty();
void OnSkillBarRestoredFromServer();
void OnHelperConfigRestoredFromServer();
void AllowHelperConfigSave();
void ResetSkillConfigPersistGate();
void TickSkillConfigPersist();
void PersistSkillConfigNow();
void SaveMacro(const wchar_t* FileName);
void OpenMacro(const wchar_t* FileName);

extern wchar_t AbuseFilter[][20];
extern wchar_t AbuseNameFilter[][20];
extern int  AbuseFilterNumber;
extern int  AbuseNameFilterNumber;