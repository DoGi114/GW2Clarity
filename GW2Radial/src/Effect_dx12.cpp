#include <Effect_dx12.h>
#include <UnitQuad.h>

namespace GW2Radial {

Effect_dx12::Effect_dx12(IDirect3DDevice9 * iDev) : Effect(iDev)
{

}

Effect_dx12::~Effect_dx12()
{

}

int Effect_dx12::Load()
{
	if (!Effect::Load())
		return 0;

	//megai2: prepare pso for drawing
	IDirect3DVertexDeclaration9* vdcl = NULL;
	dev->CreateVertexDeclaration(UnitQuad::def(), &vdcl);

	dev->SetPixelShader(ps);
	dev->SetVertexShader(vs);
	dev->SetVertexDeclaration(vdcl);

	dev->SetRenderState(D3DRS_ZENABLE, 0);
	dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	dev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
	dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

	dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);

	if (dev->GetRenderState(D3DRS_D912PXY_ENQUEUE_PSO_COMPILE, (DWORD*)&PSO_bgImg) < 0)
		return 0;

	dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);

	if (dev->GetRenderState(D3DRS_D912PXY_ENQUEUE_PSO_COMPILE, (DWORD*)&PSO_mountAlpha) < 0)
		return 0;
			
	dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	if (dev->GetRenderState(D3DRS_D912PXY_ENQUEUE_PSO_COMPILE, (DWORD*)&PSO_cursor) < 0)
		return 0;

	vdcl->Release();

	//megai2: set and save desired sampler
	dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	dev->GetRenderState(D3DRS_D912PXY_SAMPLER_ID, &tsSamplers[0]);

	dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
	dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
	dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	dev->GetRenderState(D3DRS_D912PXY_SAMPLER_ID, &tsSamplers[1]);

	dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	dev->GetRenderState(D3DRS_D912PXY_SAMPLER_ID, &tsSamplers[2]);

	perTechPSO[EFF_TC_BGIMAGE] = (DWORD**)&PSO_bgImg;
	perTechPSO[EFF_TC_MOUNTIMAGE] = (DWORD**)&PSO_bgImg;
	perTechPSO[EFF_TC_CURSOR] = (DWORD**)&PSO_cursor;
	perTechPSO[EFF_TC_MOUNTIMAGE_ALPHABLEND] = (DWORD**)&PSO_mountAlpha;
	   
	return true;
}

void Effect_dx12::SetTechnique(EffectTechnique val)
{
	SetFloat(EFF_VS_TECH_ID, val * 1.0f);

	DWORD* pso = *perTechPSO[val];

	if (pso)
		dev->GetRenderState(D3DRS_D912PXY_SETUP_PSO, pso);
}

void Effect_dx12::SetTexture(EffectTextureSlot slot, IDirect3DTexture9 * val)
{
	//megai2: this will get us texture id 
	tsTexId[slot] = val->GetPriority();

	dev->GetRenderState(D3DRS_D912PXY_GPU_WRITE, &tsTexId[0]);	
}

void Effect_dx12::SceneBegin(void * drawBuf)
{
	//megai2: mark draw start so we can see that app is issuing some not default dx9 api approach
	dev->SetRenderState(D3DRS_D912PXY_DRAW, 0);

	//setup saved sampler by writing directly into gpu buffer
	dev->SetRenderState(D3DRS_D912PXY_GPU_WRITE, D912PXY_ENCODE_GPU_WRITE_DSC(1, D912PXY_GPU_WRITE_OFFSET_SAMPLER));
	dev->GetRenderState(D3DRS_D912PXY_GPU_WRITE, &tsSamplers[0]);

	//prepare to write texture id for draws
	dev->SetRenderState(D3DRS_D912PXY_GPU_WRITE, D912PXY_ENCODE_GPU_WRITE_DSC(1, D912PXY_GPU_WRITE_OFFSET_TEXBIND));

	//megai2: FIXME set UnitQuad* type to method parameter and make it compile
	((UnitQuad*)drawBuf)->Bind();
}

void Effect_dx12::SceneEnd()
{
	//megai2: update dirty flags so we transfer to dx9 mode safely
	dev->SetRenderState(D3DRS_D912PXY_DRAW, 0x701);
	dev->SetRenderState(D3DRS_D912PXY_SETUP_PSO, 0);
}

void Effect_dx12::BeginPass(int whatever)
{
}

void Effect_dx12::Begin(uint * pass, int whatever)
{
}

void Effect_dx12::EndPass()
{
}

void Effect_dx12::End()
{
}

}