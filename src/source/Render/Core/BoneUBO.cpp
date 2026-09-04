#include "stdafx.h"
#include "Render/Core/BoneUBO.h"
#include <cstring>
#include <cmath>

BoneUBO& BoneUBO::Instance()
{
    static BoneUBO instance;
    return instance;
}

BoneUBO::~BoneUBO()
{
    Destroy();
}

void BoneUBO::Create()
{
    if (m_UBOHandle.IsValid()) return;

    memset(m_BoneRows, 0, sizeof(m_BoneRows));

    // Initialize with identity transforms -- row form: row0=(1,0,0,0), row1=(0,1,0,0),
    // row2=(0,0,1,0) (the implicit 4th row (0,0,0,1) that made these identity as a mat4 is now
    // the shader's job to supply, not this buffer's).
    for (int i = 0; i < GPU_MAX_BONES; i++) {
        float* m = m_BoneRows + i * 12;
        m[0] = 1.0f; m[5] = 1.0f; m[10] = 1.0f;
    }

    // Binding Slot 2 (Slot 1 reserved for SceneUBO fog) -- unchanged from the pre-RHI binding.
    m_UBOHandle = RHI::CreateUniformBlock(sizeof(m_BoneRows), 2);
    RHI::UpdateUniformBlock(m_UBOHandle, m_BoneRows, sizeof(m_BoneRows));
}

void BoneUBO::Destroy()
{
    if (m_UBOHandle.IsValid()) {
        RHI::DestroyUniformBlock(m_UBOHandle);
        m_UBOHandle = {};
    }
}

void BoneUBO::Bind()
{
    if (!m_UBOHandle.IsValid()) Create();
    // RHI::CreateUniformBlock already bound this handle to slot 2 at creation, and
    // RHI_GL's uniform-block binding point (unlike a texture unit) isn't clobbered by
    // anything else that could bind in between -- nothing left to (re)do here.
}

void BoneUBO::UploadBones(const void* boneTransforms, int numBones, unsigned int version)
{
    if (!m_UBOHandle.IsValid()) Create();
    if (!m_UBOHandle.IsValid() || !boneTransforms || numBones <= 0) return;

    // Same object's palette as last call (e.g. the next equipped armor piece on the same
    // character) — already uploaded, skip the repack + GPU upload.
    if (boneTransforms == m_LastUploadedPtr && version == m_LastUploadedVersion) return;
    m_LastUploadedPtr = boneTransforms;
    m_LastUploadedVersion = version;

    if (numBones > GPU_MAX_BONES) numBones = GPU_MAX_BONES;

    // GLP-11: BoneTransform is float[MAX_BONES][3][4] -- row-major 3x4 affine, already exactly
    // the layout the vec4[3*N] palette wants (row i of bone b is 4 contiguous floats). No
    // transpose, no per-element repack.
    //
    // Non-finite sanitize (ITEMDROP fix): a single bone matrix carrying a NaN/Inf component skins
    // every vertex weighted to it to an undefined coordinate, which the GPU rasterizes as a
    // grotesque elongated spike (world-drop armor/pants intermittently exploded into a long
    // spike). This is the same failure class the DXP-24 identity-fill and Dummy-bone fill already
    // guard against in BMD::Animation()'s CPU buffer -- extended here to the single UBO upload
    // chokepoint so ANY degenerate matrix reaching the GPU (from any model, any code path) is
    // neutralised to identity (rest pose at the model's placement) instead of spiking. Finite,
    // valid matrices are copied bit-for-bit -- normal item drops and equipped rendering are
    // unchanged; only genuinely corrupt (non-finite) bone slots are affected, and they still
    // render (as the un-skinned rest pose) rather than exploding. Per-bone (12 floats) so one bad
    // bone never poisons the rest of the skeleton.
    const float* src = static_cast<const float*>(boneTransforms);
    for (int b = 0; b < numBones; ++b)
    {
        const float* s = src + (size_t)b * 12;
        float* d = m_BoneRows + (size_t)b * 12;
        bool finite = true;
        for (int e = 0; e < 12; ++e)
        {
            if (!std::isfinite(s[e])) { finite = false; break; }
        }
        if (finite)
        {
            memcpy(d, s, 12 * sizeof(float));
        }
        else
        {
            // Row-major identity 3x4: row0=(1,0,0,0) row1=(0,1,0,0) row2=(0,0,1,0).
            d[0] = 1.f; d[1] = 0.f; d[2]  = 0.f; d[3]  = 0.f;
            d[4] = 0.f; d[5] = 1.f; d[6]  = 0.f; d[7]  = 0.f;
            d[8] = 0.f; d[9] = 0.f; d[10] = 1.f; d[11] = 0.f;
        }
    }

    // Always upload the FULL m_BoneRows, not just the touched numBones*12 floats: RHI's
    // uniform-block update is a Map(WRITE_DISCARD) under D3D11 (CreateUniformBlock's doc
    // comment on RHI_D3D11's side), which invalidates the ENTIRE previous GPU allocation, not
    // just the written sub-range like GL's glBufferSubData did pre-RHI. A partial upload would
    // leave every bone past numBones as undefined garbage instead of the Create()-time identity
    // rows once a second real UploadBones() call landed under D3D11 -- m_BoneRows already
    // holds correct identity padding out to GPU_MAX_BONES from Create(), so this costs nothing
    // extra on GL (same bytes, just more of them) and removes the D3D11 footgun outright.
    RHI::UpdateUniformBlock(m_UBOHandle, m_BoneRows, sizeof(m_BoneRows));
}
