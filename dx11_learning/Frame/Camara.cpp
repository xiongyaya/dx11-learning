#include<algorithm>
#include"Camara.h"

#undef min
#undef max

using namespace ul;

void  BaseCamara::LookTo(XMFLOAT4& eye, XMFLOAT4& direction)
{
	position_ = eye;
	lookDirection_ = direction;
	
	XMVECTOR pos = XMLoadFloat4(&eye);
	XMVECTOR dir = XMLoadFloat4(&direction);
	dir = XMVector4Normalize(dir);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMMATRIX view = XMMatrixLookToLH(pos, dir, up);
	XMStoreFloat4x4(&view_, view);

	XMVECTOR det;
	XMMATRIX invView = XMMatrixInverse(&det, view);
	XMVECTOR angleVector = invView.r[2];
}

void  BaseCamara::LookAt(XMFLOAT4& eye, XMFLOAT4& at)
{
	position_ = eye;
	lookDirection_ = at;

	XMVECTOR vpos = XMLoadFloat4(&eye);
	XMVECTOR vat = XMLoadFloat4(&at);
	XMVECTOR vup = XMVectorSet(0, 1, 0, 0);

	XMMATRIX view = XMMatrixLookAtLH(vpos, vat, vup);
	XMStoreFloat4x4(&view_, view);

	XMVECTOR det;
	XMMATRIX invView = XMMatrixInverse(&det, view);
	XMVECTOR angleVector = invView.r[2];

	XMFLOAT4 angles;
	XMStoreFloat4(&angles, angleVector);
	yaw_   = atan2f(angles.x, angles.z);
	pitch_ = -atan2f(angles.y, angles.x*angles.x + angles.z*angles.z);
	

}

void  BaseCamara::SetProject(eCamaraProjectType type,
	float p, float q, float nearclip =0.1, float farclip =1000 )
{
	near_ = nearclip;
	far_ = farclip;
	projectType_ = type;
	projParam_[0] = p;
	projParam_[1] = q;
	XMMATRIX project;

	switch (type)
	{
	case eCamara_Perspective:
		project = XMMatrixPerspectiveFovLH(p, q, nearclip, farclip);
		XMStoreFloat4x4(&project_, project);
		break;
	case eCamara_Ortho:
		project = XMMatrixOrthographicLH(p, q, nearclip, farclip);
		XMStoreFloat4x4(&project_, project);
		break;
	default:
		Log_Err("project type not support!");
		assert(0);
		break;
	}
}

FirstPersonCamara::eCamaraKey FirstPersonCamara::keyMap(ulUint keycode)
{
	switch (keycode)
	{
	case 'A':
		return eCamaraMoveLeft;
	case 'D':
		return eCamaraMoveRight;
	case 'W':
		return eCamaraMoveForward;
	case 'S':
		return eCamaraMoveBackward;
	case 'Q':
		return eCamaraMoveUp;
	case 'E':
		return eCamaraMoveDown;
	case VK_SPACE:
		return eCamaraMoveReset;
	default:
		return eCamaraMoveMax;
		break;
	}
}

void FirstPersonCamara::ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
		eCamaraKey mappedKey;
		if (eCamaraMoveMax != (mappedKey = keyMap(wParam)))
		{
			if (uMsg == WM_KEYDOWN)
				keyDown_[mappedKey] = true;
			else
				keyDown_[mappedKey] = false;
		}
		break;
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_LBUTTONDOWN:
		uMsg == WM_RBUTTONDOWN?(mouseDown_[eCamaraMouseRight] = true):1;
		uMsg == WM_MBUTTONDOWN?(mouseDown_[eCamaraMouseMiddle] = true):1;
		uMsg == WM_LBUTTONDOWN?(mouseDown_[eCamaraMouseLeft] = true):1;
		if (mouseDown_[eCamaraMouseLeft] || mouseDown_[eCamaraMouseMiddle] || mouseDown_[eCamaraMouseRight])
		{
			//Log_Info("capture");
			GetCursorPos(&lastMousePosition_);
			SetCapture(hWnd);
		}
		break;

	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_LBUTTONUP:
		mouseDown_[eCamaraMouseRight] = false;
		mouseDown_[eCamaraMouseMiddle] = false;
		mouseDown_[eCamaraMouseLeft] = false;
		if (!mouseDown_[eCamaraMouseLeft] && !mouseDown_[eCamaraMouseMiddle] && !mouseDown_[eCamaraMouseRight])
		{
			//Log_Info("release");
			ReleaseCapture();
		}
		break;

	case WM_CAPTURECHANGED:
		Log_Info("window capture the mouse.");
		break;
	}


}

void FirstPersonCamara::Update(float elapsedTime)
{
	//recover the position of camara
	if (keyDown_[eCamaraMoveReset])
		Reset();

	//move
	if (keyDown_[eCamaraMoveForward])
		moveDirection_.z += 1;
	if (keyDown_[eCamaraMoveBackward])
		moveDirection_.z -= 1;
	if (keyDown_[eCamaraMoveRight])
		moveDirection_.x += 1;
	if (keyDown_[eCamaraMoveLeft])
		moveDirection_.x -= 1;
	if (keyDown_[eCamaraMoveUp])
		moveDirection_.y += 1;
	if (keyDown_[eCamaraMoveDown])
		moveDirection_.y -= 1;
	moveDirection_.w = 0;

	float yawDelta = 0;
	float pitchDelta = 0;
	POINT ptCurMouseDelta = { 0, 0 };
	if (mouseDown_[eCamaraMouseLeft])
	{
		POINT ptCurMousePos;
		GetCursorPos(&ptCurMousePos);

		// Calc how far it's moved since last frame
		ptCurMouseDelta.x = ptCurMousePos.x - lastMousePosition_.x;
		ptCurMouseDelta.y = ptCurMousePos.y - lastMousePosition_.y;

		lastMousePosition_ = ptCurMousePos;
	}

	XMVECTOR moveDelta = XMLoadFloat4(&moveDirection_);
	moveDelta = XMVector3Normalize(moveDelta) * moveScaler_*elapsedTime;

	ptCurMouseDelta = ptCurMouseDelta ;
	yawDelta = ptCurMouseDelta.x * rotateScaler_*elapsedTime;
	pitchDelta = ptCurMouseDelta.y * rotateScaler_*elapsedTime;

	yaw_ += yawDelta;
	pitch_ += pitchDelta;

	//Log_Info("yaw:%f pitch:%f delta(%f %f)", yaw_, pitch_, yawDelta, pitchDelta);

	// Limit pitch to straight up or straight down
	pitch_ = std::max(-XM_PI / 2.0f, pitch_);
	pitch_ = std::min( XM_PI / 2.0f, pitch_);

	XMMATRIX camaraRot = XMMatrixRotationRollPitchYaw(pitch_, yaw_, 0);
	XMVECTOR localUp = XMVectorSet(0, 1, 0, 0);
	XMVECTOR localForward = XMVectorSet(0, 0, 1, 0);
	XMVECTOR worldUp = XMVector4Transform(localUp, camaraRot);
	XMVECTOR worldForward = XMVector4Transform(localForward, camaraRot);
	moveDelta = XMVector4Transform(moveDelta, camaraRot);

	XMVECTOR eyePos = XMLoadFloat4(&position_);
	eyePos += moveDelta;

	XMVECTOR lookAt = eyePos + worldForward;
	XMMATRIX view = XMMatrixLookAtLH(eyePos, lookAt, worldUp);
	XMStoreFloat4x4(&view_, view);
	XMStoreFloat4(&position_, eyePos);
	
	moveDirection_ = XMFLOAT4(0, 0, 0, 0);
}
