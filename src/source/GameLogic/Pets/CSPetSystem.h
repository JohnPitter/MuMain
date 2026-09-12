//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <memory>

#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"

class CSPetSystem
{
protected:
    CHARACTER* m_PetOwner;
    CHARACTER* m_PetTarget;
    CHARACTER   m_PetCharacter;
    PET_TYPE    m_PetType;
    PET_INFO* m_pPetInfo;
    std::uint8_t m_byCommand;
    std::unique_ptr<vec34_t[]> m_BoneTransforms;

public:
    CSPetSystem();
    virtual ~CSPetSystem();

    PET_TYPE    GetPetType(void) { return m_PetType; }
    void		SetPetInfo(PET_INFO* pPetInfo) { m_pPetInfo = pPetInfo; };

    virtual void    MovePet(void) = 0;
    virtual void	CalcPetInformation(const PET_INFO& Petinfo) = 0;
    virtual void    RenderPetInventory(void) = 0;
    virtual void    RenderPet(int PetState = 0) = 0;

    virtual void    Eff_LevelUp(void) = 0;
    virtual void    Eff_LevelDown(void) = 0;

    void    CreatePetPointer(int Type, unsigned char PositionX, unsigned char PositionY, float Rotation);
    bool    PlayAnimation(OBJECT* o);

    void    MoveInventory(void);
    void    RenderInventory(void);

    void    SetAI(int AI);
    void    SetCommand(int Key, std::uint8_t cmd);
    void    SetAttack(int Key, int attackType);

    int		GetObjectType()
    {
        return m_PetCharacter.Object.Type;
    }
};

class CSPetDarkSpirit : public CSPetSystem
{
private:

public:
    CSPetDarkSpirit(CHARACTER* c);
    virtual ~CSPetDarkSpirit(void);

    virtual void MovePet(void);
    virtual void CalcPetInformation(const PET_INFO& Petinfo);
    virtual void RenderPetInventory(void);
    virtual void RenderPet(int PetState = 0);

    virtual void Eff_LevelUp(void);
    virtual void Eff_LevelDown(void);

    void    AttackEffect(CHARACTER* c, OBJECT* o);
    void    RenderCmdType(void);
};

// Authored Poseidon eagle (item 13/206). Follows the CSPetSystem flight
// contract of the darkspirit (fly / flying / stand / escape) with the authored
// model, but it is a visual companion: no server pet commands, no duel state
// and no pet info packets. Dives at the owner's target while the owner swings,
// otherwise glides along and perches on the owner's shoulder in safe zones.
class CSPetPoseidonEagle : public CSPetSystem
{
private:
    void MoveFlight(OBJECT* o, OBJECT* Owner, const vec3_t& TargetPosition, float FlyRange);
    void MoveDive(OBJECT* o, const vec3_t& TargetPosition);
    void MovePerch(OBJECT* o, const CHARACTER* owner);
    void MoveReturnToPerch(OBJECT* o, const CHARACTER* owner);
    void MoveDiveEffect(OBJECT* o);

public:
    CSPetPoseidonEagle(CHARACTER* c);
    virtual ~CSPetPoseidonEagle(void);

    virtual void MovePet(void);
    virtual void CalcPetInformation(const PET_INFO& Petinfo);
    virtual void RenderPetInventory(void);
    virtual void RenderPet(int PetState = 0);

    virtual void Eff_LevelUp(void);
    virtual void Eff_LevelDown(void);
};
