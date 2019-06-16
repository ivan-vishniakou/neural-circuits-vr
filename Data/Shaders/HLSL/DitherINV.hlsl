#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"

const static float PaletteRGBSize = 4.;// number of values possible for each R, G, B.
const static float ResolutionDivisor = 2.;

const static int dither[8][8] = {
	{ 0, 32, 8, 40, 2, 34, 10, 42 }, /* 8x8 Bayer ordered dithering */
	{ 48, 16, 56, 24, 50, 18, 58, 26 }, /* pattern. Each input pixel */
	{ 12, 44, 4, 36, 14, 46, 6, 38 }, /* is scaled to the 0..63 range */
	{ 60, 28, 52, 20, 62, 30, 54, 22 }, /* before looking in this table */
	{ 3, 35, 11, 43, 1, 33, 9, 41 }, /* to determine the action. */
	{ 51, 19, 59, 27, 49, 17, 57, 25 },
	{ 15, 47, 7, 39, 13, 45, 5, 37 },
	{ 63, 31, 55, 23, 61, 29, 53, 21 } };

void VS(float4 iPos : POSITION,
	out float2 oScreenPos : TEXCOORD0,
	out float4 oPos : OUTPOSITION)
{
	float4x3 modelMatrix = iModelMatrix;
	float3 worldPos = GetWorldPos(modelMatrix);
	oPos = GetClipPos(worldPos);
	oScreenPos = GetScreenPosPreDiv(oPos);
}

const static float scale = 200.0;

void PS(float2 iScreenPos : TEXCOORD0,
	out float4 oColor : OUTCOLOR0)
{
	float2 xy = (float2(iScreenPos * scale) % 1.0);
	float3 sampl = Sample2D(DiffMap, iScreenPos - xy/scale).rgb;

	float limit = 0.0;
	int2 ixy = int2(xy.x*8.0, xy.y*8.0);
	limit = (float(dither[ixy.x][ixy.y] + 1.0) / 64.0);// +1.0) / 32.0;

	if (sampl.r < limit) {
		limit = 0.0;
	}
	else {
		limit = 1.0;
	}
	oColor = float4(1-limit, 1-limit, 1-limit,1);
}