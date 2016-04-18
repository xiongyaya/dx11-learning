#include<assert.h>

#include"MeshRender.h"
#include"../resource/ResourceMgr.h"
#include"../scene/SceneManager.h"

using namespace ul;

SubBatch::SubBatch() :refParameter_(nullptr), texCount_(0), indexOffset_(0), indexCount_(0), refSceneMgr_(nullptr), constBuffer_(nullptr)
{
	refSceneMgr_ = SceneMgr::GetSingletonPtr();
	assert(refSceneMgr_ != nullptr);
}



void SubBatch::Render(ID3D11DeviceContext* context)
{
	context->VSSetShader(refParameter_->vsEnterPoint_, nullptr, 0);
	context->PSSetShader(refParameter_->psEnterPoint_, nullptr, 0);
	context->VSSetConstantBuffers(0, 1, &constBuffer_);
	context->PSSetConstantBuffers(0, 1, &constBuffer_);
	context->PSSetShaderResources(0, CONST_MAX_SHADER_RESOURCE_NUM, refParameter_->srvs_);
	context->DrawIndexed(indexCount_, indexOffset_, 0);
}



bool BaseModel::Create(
	ID3D11Device* dev,
	SModelData& data)
{
	D3D11_BUFFER_DESC vbd;
	D3D11_BUFFER_DESC ibd;
	D3D11_SUBRESOURCE_DATA vdata, idata;
	ResourceMgr* mgr = ResourceMgr::GetSingletonPtr();

	//vb
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = data.primtives_.verticeNum_*data.primtives_.stride_;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	ZeroMemory(&vdata, sizeof(vdata));
	vdata.pSysMem = &data.primtives_.verticeBuffer_[0];

	Null_Return_False((vb_ = mgr->CreateBuffer(vbd, &vdata)));

	//ib
	ibd = vbd;
	ibd.ByteWidth = sizeof(short)*data.primtives_.indices_.size();;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ZeroMemory(&idata, sizeof(idata));
	idata.pSysMem = &data.primtives_.indices_[0];

	Null_Return_False((ib_ = mgr->CreateBuffer(ibd, &idata)));

	//material 
	renderParameters_.reserve(data.materials_.size());

	//vertex layout
	D3D11_INPUT_ELEMENT_DESC desc[CONST_MAX_INPUT_ELEMENT_COUNT];
	ulUint count;
	Vertex_GetInputDescByType(data.primtives_.type_, desc, &count);
	if (data.materials_.size() > 0 && data.materials_[0]->shaderFile!="" 
		&& data.materials_[0]->vsEnterPoint != ""
		&& data.materials_[0]->psEnterPoint != "")
	{
		renderParameters_.at(0)->vsEnterPoint_ = mgr->CreateVertexShaderAndInputLayout(data.materials_[0]->shaderFile.c_str(),
			data.materials_[0]->vsEnterPoint.c_str() , "vs_5_0", desc, count, &this->vertexLayout_);
		renderParameters_.at(0)->psEnterPoint_ = mgr->CreatePixelShader(data.materials_[0]->shaderFile.c_str(),
			data.materials_[0]->psEnterPoint.c_str(), "ps_5_0");
	}

	
	for (ulUint i = 0; i < data.materials_.size(); ++i)
	{
		SMaterialData *pMaterial = data.materials_[i];
		SRenderParameter* pParameter = new SRenderParameter();

		//texture
		for (int i = 0; i < CONST_MAX_SHADER_RESOURCE_NUM; ++i)
		{
			if (pMaterial->texturePath[i] != "")
			{
				pParameter->srvs_[i] = mgr->CreateTextureFromFile(pMaterial->texturePath[i]);
			}
			else{
				pParameter->srvs_[i] = nullptr;
			}
		}
		
		//shader
		if (i != 0)
		{
			pParameter->vsEnterPoint_ = mgr->CreateVertexShader(pMaterial->shaderFile.c_str(),
				pMaterial->vsEnterPoint.c_str(), "ps_5_0");
			pParameter->psEnterPoint_ = mgr->CreatePixelShader(pMaterial->shaderFile.c_str(),
				pMaterial->psEnterPoint.c_str(), "ps_5_0");
			if (Null(pParameter->vsEnterPoint_) || Null(pParameter->psEnterPoint_))
			{
				Log_Err("load material from file %s, batch %d error.", data.sourceFile_.c_str(), i);
				return false;
			}
		}
		renderParameters_.push_back(pParameter);
	}

	//render batch
	children_.reserve(data.groups_.size());
	for (ulUint i = 0; i < data.groups_.size(); ++i)
	{
		SRenderGroupInfo* pGI = data.groups_[i];
		SubBatch batch;

		batch.SetRenderBatch(pGI->indexOffset_, pGI->indexCount_);

		if (data.materials_.size()>0 )
		{
			batch.SetParameter(renderParameters_[ pGI->materialID] );
		}
			
		children_.push_back(batch);
	}

	childCount_ = children_.size();
	stride_ = data.primtives_.stride_;
	offset_ = 0;
	return true;
}



void BaseModel::Render(ID3D11DeviceContext* context)
{
	assert(context != 0);
	context->IASetVertexBuffers(0, 1, &vb_, &stride_, &offset_);
	context->IASetIndexBuffer(ib_, DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(vertexLayout_);

	for (ulUint i = 0; i < childCount_; ++i)
	{
		this->SetShaderParameter(context);
		children_[i].ApplyMaterial(context);
		children_[i].Render(context);
	}
	int a = 0;
}
